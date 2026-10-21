/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sms_receive_reliability_handler.h"

#include "common_event.h"
#include "common_event_support.h"
#include "gsm_sms_message.h"
#include "parameter.h"
#include "radio_event.h"
#include "sms_broadcast_subscriber_receiver.h"
#include "sms_hisysevent.h"
#include "sms_persist_helper.h"
#include "telephony_common_utils.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"

namespace OHOS {
namespace Telephony {
using namespace std;
using namespace EventFwk;
static constexpr uint16_t PDU_POS_OFFSET = 1;
static constexpr uint16_t PDU_START_POS = 0;
static constexpr uint16_t SMS_INVALID_PAGE_COUNT = 0;
static constexpr uint16_t SMS_SINGLE_PAGE_COUNT = 1;
static constexpr uint16_t SMS_PAGE_INITIAL = 1;
static constexpr uint16_t SMS_PAGE_INCREMENT = 1;
static constexpr int16_t WAP_PUSH_PORT = 2948;
static constexpr int16_t SMS_TEXT_PORT = -1;
static constexpr int32_t TEXT_MSG_RECEIVE_CODE = 0;
static constexpr int32_t DATA_MSG_RECEIVE_CODE = 1;
static constexpr int64_t ONE_DAY_TOTAL_SECONDS = 86400;
static constexpr uint16_t MAX_TPDU_DATA_LEN = 255;
static constexpr int32_t EXPIRE_DAYS_PARA_SIZE = 128;
static constexpr const char *SMS_EXPIRE_DAYS = "const.telephony.sms.expire.days";
static constexpr const char *DEFAULT_EXPIRE_DAYS = "3";
static constexpr const char *SMS_PAGE_COUNT_INVALID = "0";
static constexpr const char *SMS_BROADCAST_SLOTID_KEY = "slotId";
static constexpr const char *SMS_BROADCAST_PDU_KEY = "pdus";
static constexpr const char *SMS_BROADCAST_SMS_TYPE_KEY = "isCdma";
static constexpr const char *SMS_BROADCAST_SMS_TEXT_TYPE_KEY = "TEXT_SMS_RECEIVE";
static constexpr const char *SMS_BROADCAST_SMS_DATA_TYPE_KEY = "DATA_SMS_RECEIVE";
static constexpr const char *SMS_BROADCAST_SMS_PORT_KEY = "port";
const std::string CT_SMSC = "10659401";
const std::string CT_SMSC_86 = "8610659401";
const std::string CT_SMSC_INTERNATION_86 = "+8610659401";
const std::string CT_AUTO_REG_SMS_ACTION = "ct_auto_reg_sms_receive_completed";

std::shared_ptr<SmsBroadcastSubscriberReceiver> SmsReceiveReliabilityHandler::g_receiver = []() {
    MatchingSkills smsSkills;
    smsSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    CommonEventSubscribeInfo smsSubscriberInfo(smsSkills);
    smsSubscriberInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    return std::make_shared<SmsBroadcastSubscriberReceiver>(smsSubscriberInfo);
}();

SmsReceiveReliabilityHandler::SmsReceiveReliabilityHandler(int32_t slotId) : slotId_(slotId)
{
    smsWapPushHandler_ = std::make_unique<SmsWapPushHandler>(slotId);
    if (smsWapPushHandler_ == nullptr) {
        TELEPHONY_LOGE("make sms wapPush Hander error.");
    }
}

SmsReceiveReliabilityHandler::~SmsReceiveReliabilityHandler() {}

bool SmsReceiveReliabilityHandler::DeleteExpireSmsFromDB()
{
    DataShare::DataSharePredicates predicates;
    std::time_t timep;
    int64_t currentTime = time(&timep);

    std::string smsExpire = GetSmsExpire();
    if (!IsValidDecValue(smsExpire)) {
        TELEPHONY_LOGE("system property telephony.sms.expire.days not decimal, raw value: %{public}s",
            smsExpire.c_str());
        smsExpire = DEFAULT_EXPIRE_DAYS;
    }
    int64_t validityDuration = std::stoi(smsExpire) * ONE_DAY_TOTAL_SECONDS;
    int64_t deadlineTime = currentTime - validityDuration;
    if (deadlineTime <= 0) {
        TELEPHONY_LOGE("deadlineTime is negative, currentTime=%{public}lld, check device time sync", currentTime);
        return false;
    }

    predicates.EqualTo(SmsSubsection::SLOT_ID, std::to_string(slotId_))
        ->BeginWrap()
        ->LessThan(SmsSubsection::START_TIME, std::to_string(deadlineTime))
        ->Or()
        ->EqualTo(SmsSubsection::REW_PUD, "")
        ->Or()
        ->LessThan(SmsSubsection::SIZE, SMS_PAGE_COUNT_INVALID)
        ->EndWrap();
    return DelayedSingleton<SmsPersistHelper>::GetInstance()->Delete(predicates);
}

std::string SmsReceiveReliabilityHandler::GetSmsExpire()
{
    char smsExpireDays[EXPIRE_DAYS_PARA_SIZE] = { 0 };
    GetParameter(SMS_EXPIRE_DAYS, DEFAULT_EXPIRE_DAYS, smsExpireDays, EXPIRE_DAYS_PARA_SIZE);
    return smsExpireDays;
}

void SmsReceiveReliabilityHandler::RemoveBlockedSms(std::vector<SmsReceiveIndexer> &dbIndexers)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SmsSubsection::SLOT_ID, std::to_string(slotId_));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->Query(predicates, dbIndexers);

    for (auto smsPage = dbIndexers.begin(); smsPage != dbIndexers.end();) {
        if (CheckBlockedPhoneNumber(smsPage->GetOriginatingAddress())) {
            TELEPHONY_LOGI("indexer display address is block");
            smsPage = dbIndexers.erase(smsPage);
        } else if (smsPage->GetPdu().size() == 0 || smsPage->GetPdu().size() > MAX_TPDU_DATA_LEN) {
            smsPage = dbIndexers.erase(smsPage);
        } else {
            smsPage++;
        }
    }
}

void SmsReceiveReliabilityHandler::CheckUnReceiveWapPush(std::vector<SmsReceiveIndexer> &dbIndexers)
{
    for (auto place = dbIndexers.begin(); place != dbIndexers.end();) {
        std::shared_ptr<vector<string>> userDataRaws = make_shared<vector<string>>();
        userDataRaws->assign(MAX_SEGMENT_NUM, "");
        if (place->GetDestPort() != WAP_PUSH_PORT) {
            place++;
            continue;
        }
        if (place->GetMsgCount() == SMS_SINGLE_PAGE_COUNT) {
            GetWapPushUserDataSinglePage(*place, userDataRaws);
        } else {
            int32_t smsPagesCount = SMS_PAGE_INITIAL;
            int32_t pos = static_cast<int32_t>(std::distance(dbIndexers.begin(), place));
            GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, pos, userDataRaws);
            if (place->GetMsgCount() != smsPagesCount) {
                place = dbIndexers.erase(place);
                continue;
            }
        }

        if (!userDataRaws->at(PDU_START_POS).empty()) {
            ReadyDecodeWapPushUserData(*place, userDataRaws);
        }
        place = dbIndexers.erase(place);
    }
}

void SmsReceiveReliabilityHandler::GetWapPushUserDataSinglePage(
    SmsReceiveIndexer &indexer, std::shared_ptr<vector<string>> userDataRaws)
{
    string pdu = StringUtils::StringToHex(indexer.GetPdu());
    std::shared_ptr<SmsBaseMessage> baseMessage = GsmSmsMessage::CreateMessage(pdu);
    if (baseMessage == nullptr) {
        TELEPHONY_LOGE("baseMessage nullptr");
        return;
    }
    userDataRaws->at(PDU_START_POS) = baseMessage->GetRawWapPushUserData();
}

void SmsReceiveReliabilityHandler::GetWapPushUserDataMultipage(int32_t &smsPagesCount,
    std::vector<SmsReceiveIndexer> &dbIndexers, int32_t place, std::shared_ptr<vector<string>> userDataRaws)
{
    if (place < 0 || place >= static_cast<int32_t>(dbIndexers.size())) {
        TELEPHONY_LOGE("place invalid");
        return;
    }
    string pdu = StringUtils::StringToHex(dbIndexers[place].GetPdu());
    std::shared_ptr<SmsBaseMessage> baseMessage = GsmSmsMessage::CreateMessage(pdu);
    if (baseMessage == nullptr) {
        TELEPHONY_LOGE("baseMessage nullptr");
        return;
    }
    if (dbIndexers[place].GetMsgSeqId() < PDU_POS_OFFSET || dbIndexers[place].GetMsgSeqId() > MAX_SEGMENT_NUM) {
        TELEPHONY_LOGE("seqId invalid");
        return;
    }
    userDataRaws->at(dbIndexers[place].GetMsgSeqId() - PDU_POS_OFFSET) = baseMessage->GetRawUserData();

    for (auto locate = dbIndexers.begin() + place + SMS_PAGE_INCREMENT; locate != dbIndexers.end();) {
        if (dbIndexers[place].GetMsgRefId() != locate->GetMsgRefId()) {
            locate++;
            continue;
        }
        if (locate->GetPdu().size() > 0) {
            smsPagesCount++;
        }
        pdu = StringUtils::StringToHex(locate->GetPdu());
        baseMessage = GsmSmsMessage::CreateMessage(pdu);
        if (baseMessage == nullptr) {
            TELEPHONY_LOGE("baseMessage nullptr");
            locate = dbIndexers.erase(locate);
            return;
        }
        if (locate->GetMsgSeqId() < PDU_POS_OFFSET || locate->GetMsgSeqId() > MAX_SEGMENT_NUM) {
            TELEPHONY_LOGE("seqId invalid");
            locate = dbIndexers.erase(locate);
            return;
        }
        userDataRaws->at(locate->GetMsgSeqId() - PDU_POS_OFFSET) = baseMessage->GetRawUserData();
        locate = dbIndexers.erase(locate);
    }
}

void SmsReceiveReliabilityHandler::ReadyDecodeWapPushUserData(
    SmsReceiveIndexer &indexerObj, std::shared_ptr<vector<string>> userDataRaws)
{
    string userDataWapPush;
    for (auto userDataRaw : *userDataRaws) {
        userDataWapPush.append(userDataRaw);
    }
    shared_ptr<SmsReceiveIndexer> indexer = std::make_shared<SmsReceiveIndexer>(indexerObj.GetPdu(),
        indexerObj.GetTimestamp(), indexerObj.GetDestPort(), indexerObj.GetIsCdma(), indexerObj.GetOriginatingAddress(),
        indexerObj.GetVisibleAddress(), indexerObj.GetMsgRefId(), indexerObj.GetMsgSeqId(), indexerObj.GetMsgCount(),
        false, StringUtils::StringToHex(indexerObj.GetPdu()));
    indexer->SetDataBaseId(indexerObj.GetDataBaseId());

    if (smsWapPushHandler_ == nullptr) {
        TELEPHONY_LOGI("smsWapPushHandler_ nullptr");
        return;
    }
    if (!smsWapPushHandler_->DecodeWapPushPdu(indexer, userDataWapPush)) {
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::WAP_PUSH,
            SmsMmsErrorCode::SMS_ERROR_PDU_DECODE_FAIL, "Wap push decode wap push fail");
    }
}

void SmsReceiveReliabilityHandler::SmsReceiveReliabilityProcessing()
{
    std::vector<SmsReceiveIndexer> dbIndexers;
    RemoveBlockedSms(dbIndexers);
    CheckUnReceiveWapPush(dbIndexers);

    for (auto position = dbIndexers.begin(); position != dbIndexers.end();) {
        auto msgCount = position->GetMsgCount();
        std::shared_ptr<vector<string>> pdus = make_shared<vector<string>>();
        if (msgCount == SMS_INVALID_PAGE_COUNT) {
            position++;
            continue;
        } else if (msgCount == SMS_SINGLE_PAGE_COUNT) {
            pdus->push_back(StringUtils::StringToHex(position->GetPdu()));
        } else {
            int32_t smsPagesCount = SMS_PAGE_INITIAL;
            int32_t pos = static_cast<int32_t>(std::distance(dbIndexers.begin(), position));
            GetSmsUserDataMultipage(smsPagesCount, msgCount, dbIndexers, pos, pdus);
            if (msgCount != smsPagesCount) {
                position = dbIndexers.erase(position);
                continue;
            }
        }
        if (!pdus->at(PDU_START_POS).empty()) {
            ReadySendSmsBroadcast(*position, pdus);
        }
        position = dbIndexers.erase(position);
    }
}

void SmsReceiveReliabilityHandler::GetSmsUserDataMultipage(int32_t &smsPagesCount, uint16_t msgCount,
    std::vector<SmsReceiveIndexer> &dbIndexers, int32_t position, std::shared_ptr<std::vector<std::string>> pdus)
{
    if (position < 0 || position >= static_cast<int32_t>(dbIndexers.size())) {
        TELEPHONY_LOGE("position over max");
        return;
    }
    pdus->assign(msgCount, "");
    if (dbIndexers[position].GetMsgSeqId() < PDU_POS_OFFSET || dbIndexers[position].GetMsgSeqId() > msgCount) {
        TELEPHONY_LOGE("seqId invalid");
        return;
    }
    pdus->at(dbIndexers[position].GetMsgSeqId() - PDU_POS_OFFSET) =
        StringUtils::StringToHex(dbIndexers[position].GetPdu());
    for (auto locate = dbIndexers.begin() + position + SMS_PAGE_INCREMENT; locate != dbIndexers.end();) {
        if (dbIndexers[position].GetMsgRefId() != locate->GetMsgRefId()) {
            locate++;
            continue;
        }
        if (locate->GetMsgSeqId() < PDU_POS_OFFSET || locate->GetMsgSeqId() > msgCount) {
            TELEPHONY_LOGE("seqId invalid");
            locate = dbIndexers.erase(locate);
            return;
        }
        pdus->at(locate->GetMsgSeqId() - PDU_POS_OFFSET) = StringUtils::StringToHex(locate->GetPdu());
        locate = dbIndexers.erase(locate);
        smsPagesCount++;
    }
}

void SmsReceiveReliabilityHandler::ReadySendSmsBroadcast(
    SmsReceiveIndexer &indexerObj, std::shared_ptr<vector<string>> pdus)
{
    shared_ptr<SmsReceiveIndexer> indexer = std::make_shared<SmsReceiveIndexer>(indexerObj.GetPdu(),
        indexerObj.GetTimestamp(), indexerObj.GetDestPort(), indexerObj.GetIsCdma(), indexerObj.GetOriginatingAddress(),
        indexerObj.GetVisibleAddress(), indexerObj.GetMsgRefId(), indexerObj.GetMsgSeqId(), indexerObj.GetMsgCount(),
        false, StringUtils::StringToHex(indexerObj.GetPdu()));
    indexer->SetDataBaseId(indexerObj.GetDataBaseId());
    TELEPHONY_LOGI("send sms from db for reliability");
    SendBroadcast(indexer, pdus);
}

void SmsReceiveReliabilityHandler::DeleteMessageFormDb(const uint16_t refId, const int32_t dataBaseId,
    const int32_t msgCount, const std::string address)
{
    if (refId == 0 && dataBaseId == 0) {
        TELEPHONY_LOGE("DeleteMessageFormDb fail by refId error, msgCount=%{public}d", msgCount);
        return;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SmsSubsection::SENDER_NUMBER, address)
        ->And()
        ->EqualTo(SmsSubsection::SMS_SUBSECTION_ID, std::to_string(refId))
        ->And()
        ->EqualTo(SmsSubsection::SIZE, std::to_string(msgCount));
    bool ret = DelayedSingleton<SmsPersistHelper>::GetInstance()->Delete(predicates);
    TELEPHONY_LOGI("DeleteMessageFormDb refId=%{public}d,dataBaseId=%{public}d,msgCount=%{public}d,result: %{public}d",
        refId, dataBaseId, msgCount, ret);
}

void SmsReceiveReliabilityHandler::SendBroadcast(
    const std::shared_ptr<SmsReceiveIndexer> indexer, const shared_ptr<vector<string>> pdus)
{
    if (indexer == nullptr || pdus == nullptr) {
        TELEPHONY_LOGE("indexer or pdus is nullptr");
        return;
    }
    std::vector<std::string> newPdus;
    for (const auto &it : *pdus) {
        if (!it.empty()) {
            newPdus.emplace_back(it);
        }
    }
    Want want;
    CommonEventData data;
    CommonEventPublishInfo publishInfo;
    PacketSmsData(want, indexer, data, publishInfo);
    want.SetParam(SMS_BROADCAST_PDU_KEY, newPdus);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_DATABASE_ID_KEY, indexer->GetDataBaseId());
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_MSG_REF_ID_KEY, indexer->GetMsgRefId());
    std::string addr = indexer->GetOriginatingAddress();
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_ADDRESS_KEY, addr);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_MSG_COUNT_KEY, indexer->GetMsgCount());
    data.SetWant(want);

    bool cbResult = false;
    if (CT_SMSC.compare(addr) != 0 && CT_SMSC_86.compare(addr) != 0 && CT_SMSC_INTERNATION_86.compare(addr) != 0) {
        HILOG_COMM_INFO("Sms Broadcast");
        cbResult = CommonEventManager::PublishCommonEvent(data, publishInfo, g_receiver);
    } else {
        TELEPHONY_LOGI("CT AutoReg Broadcast");
        cbResult = CommonEventManager::PublishCommonEvent(data, publishInfo, nullptr);
    }
    HiSysEventCBResult(cbResult);
    if (CT_SMSC.compare(addr) == 0 || CT_SMSC_86.compare(addr) == 0 || CT_SMSC_INTERNATION_86.compare(addr) == 0) {
        TELEPHONY_LOGI("del ct auto sms from db");
        DeleteAutoSmsFromDB(shared_from_this(), indexer);
    }
}

void SmsReceiveReliabilityHandler::PacketSmsData(EventFwk::Want &want, const std::shared_ptr<SmsReceiveIndexer> indexer,
    EventFwk::CommonEventData &data, EventFwk::CommonEventPublishInfo &publishInfo)
{
    if (CT_SMSC.compare(indexer->GetOriginatingAddress()) != 0 &&
        CT_SMSC_86.compare(indexer->GetOriginatingAddress()) != 0 &&
        CT_SMSC_INTERNATION_86.compare(indexer->GetOriginatingAddress()) != 0) {
        want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    } else {
        want.SetAction(CT_AUTO_REG_SMS_ACTION);
    }
    TELEPHONY_LOGI("Sms slotId_:%{public}d", slotId_);
    want.SetParam(SMS_BROADCAST_SLOTID_KEY, static_cast<int>(slotId_));
    want.SetParam(SMS_BROADCAST_SMS_TYPE_KEY, indexer->GetIsCdma());
    if (indexer->GetIsText() || indexer->GetDestPort() == SMS_TEXT_PORT) {
        data.SetData(SMS_BROADCAST_SMS_TEXT_TYPE_KEY);
        data.SetCode(TEXT_MSG_RECEIVE_CODE);
    } else {
        data.SetData(SMS_BROADCAST_SMS_DATA_TYPE_KEY);
        data.SetCode(DATA_MSG_RECEIVE_CODE);
        want.SetParam(SMS_BROADCAST_SMS_PORT_KEY, static_cast<short>(indexer->GetDestPort()));
    }

    publishInfo.SetOrdered(true);
    std::vector<std::string> smsPermissions;
    smsPermissions.emplace_back(Permission::RECEIVE_MESSAGES);
    publishInfo.SetSubscriberPermissions(smsPermissions);
}

void SmsReceiveReliabilityHandler::HiSysEventCBResult(bool publishResult)
{
    if (!publishResult) {
        TELEPHONY_LOGE("SendBroadcast PublishBroadcastEvent result fail");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_PUBLISH_COMMON_EVENT_FAIL, "publish short message broadcast event fail");
        return;
    }
    DelayedSingleton<SmsHiSysEvent>::GetInstance()->SetSmsBroadcastStartTime();
}

void SmsReceiveReliabilityHandler::DeleteAutoSmsFromDB(
    std::shared_ptr<SmsReceiveReliabilityHandler> handler, const std::shared_ptr<SmsReceiveIndexer> indexer)
{
    handler->DeleteMessageFormDb(indexer->GetMsgRefId(), indexer->GetDataBaseId(),
        indexer->GetMsgCount(), indexer->GetOriginatingAddress());
}

bool SmsReceiveReliabilityHandler::CheckBlockedPhoneNumber(std::string originatingAddress)
{
    return DelayedSingleton<SmsPersistHelper>::GetInstance()->QueryBlockPhoneNumber(originatingAddress);
}

bool SmsReceiveReliabilityHandler::CheckSmsCapable()
{
    auto helperPtr = DelayedSingleton<SmsPersistHelper>::GetInstance();
    if (helperPtr == nullptr) {
        return true;
    }
    return helperPtr->QueryParamBoolean(SmsPersistHelper::SMS_CAPABLE_PARAM_KEY, true);
}
} // namespace Telephony
} // namespace OHOS