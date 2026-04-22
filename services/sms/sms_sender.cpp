/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "sms_sender.h"

#include <cinttypes>

#include "core_manager_inner.h"
#include "ims_sms_client.h"
#include "radio_event.h"
#include "securec.h"
#include "sms_mms_common.h"
#include "sms_hisysevent.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"

namespace OHOS {
namespace Telephony {
using namespace std;
using namespace std::chrono;
int64_t SmsSender::msgRef64bit_ = 0;
std::unordered_map<int64_t, std::shared_ptr<SmsSendIndexer>> SmsSender::sendCacheMap_;

SmsSender::SmsSender(int32_t slotId, function<void(shared_ptr<SmsSendIndexer>)> &sendRetryFun)
    : TelEventHandler("SmsSender"), slotId_(slotId), sendRetryFun_(sendRetryFun)
{}

SmsSender::~SmsSender() {}

void SmsSender::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is nullptr");
        return;
    }
    shared_ptr<SmsSendIndexer> smsIndexer = nullptr;
    uint32_t eventId = event->GetInnerEventId();
    TELEPHONY_LOGI("SmsSender::ProcessEvent eventId %{public}d", eventId);
    switch (eventId) {
        case RadioEvent::RADIO_SEND_SMS:
        case RadioEvent::RADIO_SEND_CDMA_SMS:
        case RadioEvent::RADIO_SEND_IMS_GSM_SMS:
        case RadioEvent::RADIO_SEND_SMS_EXPECT_MORE: {
            smsIndexer = FindCacheMapAndTransform(event);
            HandleMessageResponse(smsIndexer);
            break;
        }
        case MSG_SMS_RETRY_DELIVERY: {
            smsIndexer = event->GetSharedObject<SmsSendIndexer>();
            if (sendRetryFun_ != nullptr) {
                sendRetryFun_(smsIndexer);
            }
            break;
        }
        case RadioEvent::RADIO_SMS_STATUS: {
            StatusReportAnalysis(event);
            break;
        }
        case RadioEvent::RADIO_SET_IMS_SMS: {
            StatusReportSetImsSms(event);
            SyncSwitchISmsResponse();
            break;
        }
        case RadioEvent::RADIO_GET_IMS_SMS: {
            StatusReportGetImsSms(event);
            SyncSwitchISmsResponse();
            break;
        }
        default:
            TELEPHONY_LOGE("SmsSender::ProcessEvent Unknown %{public}d", eventId);
            break;
    }
}

void SmsSender::SyncSwitchISmsResponse()
{
    std::unique_lock<std::mutex> lck(ctx_);
    resIsSmsReady_ = true;
    TELEPHONY_LOGI("resIsSmsReady_ = %{public}d", resIsSmsReady_);
    cv_.notify_one();
}

void SmsSender::HandleMessageResponse(const shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("smsIndexer is nullptr");
        return;
    }
    if (!SendCacheMapEraseItem(smsIndexer->GetMsgRefId64Bit())) {
        TELEPHONY_LOGE("SendCacheMapEraseItem fail !!!!!");
    }
    SendCacheMapTimeoutCheck();
    if (!smsIndexer->GetIsFailure()) {
        if (smsIndexer->GetDeliveryCallback() != nullptr) {
            // Expecting a status report.  Add it to the list.
            if (reportList_.size() > MAX_REPORT_LIST_LIMIT) {
                reportList_.pop_front();
            }
            reportList_.push_back(smsIndexer);
        }
        SendMessageSucceed(smsIndexer);
    } else {
        HandleResend(smsIndexer);
    }
}

void SmsSender::SendMessageSucceed(const shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("SendMessageSucceed but smsIndexer drop!");
        return;
    }

    bool isLastPart = false;
    uint8_t unSentCellCount = smsIndexer->GetUnSentCellCount();
    if (unSentCellCount == 0) {
        isLastPart = true;
    }
    TELEPHONY_LOGI("isLastPart:%{public}d", isLastPart);
    if (isLastPart) {
        smsIndexer->SetPsResendCount(INITIAL_COUNT);
        smsIndexer->SetCsResendCount(INITIAL_COUNT);
        ISendShortMessageCallback::SmsSendResult messageType = ISendShortMessageCallback::SEND_SMS_SUCCESS;
        if (smsIndexer->GetHasCellFailed() != nullptr) {
            if (*smsIndexer->GetHasCellFailed()) {
                messageType = ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN;
            }
        }
        SendResultCallBack(smsIndexer->GetSendCallback(), messageType);
        if (messageType == ISendShortMessageCallback::SEND_SMS_SUCCESS) {
            SmsHiSysEvent::WriteSmsSendBehaviorEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE);
        }
        if (smsIndexer->GetDataBaseId() == 0) {
            return;
        }
        DataShare::DataShareValuesBucket sessionBucket;
        sessionBucket.Put(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_SUCCEED);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(SmsMmsInfo::MSG_ID, smsIndexer->GetDataBaseId());
        if (!DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateSms(predicates, sessionBucket)) {
            TELEPHONY_LOGE("modify db fail while SendMessageSucceed. id:%{public}d;", smsIndexer->GetDataBaseId());
        }
        std::string  notifyType = SmsMmsCommonData::MESSAGE_STATUS_CHANGE_NOTIFY;
        if (smsIndexer->GetIsMmsApp()) {
            notifyType = SmsMmsCommonData::SMS_MMS_SENT_RESULT_NOTIFY;
        }
        TELEPHONY_LOGI("before send boradcast. SendMessageSucceed %{public}d", smsIndexer->GetDataBaseId());
        DelayedSingleton<SmsMmsCommon>::GetInstance()->SendBroadcast(smsIndexer->GetDataBaseId(),
            notifyType, SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_SUCCEED,
            SmsMmsCommonData::SMS_MMS_INFO_SMS_TYPE);
    }
}

void SmsSender::SendMessageFailed(const shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("smsIndexer is nullptr");
        return;
    }
    shared_ptr<bool> hasCellFailed = smsIndexer->GetHasCellFailed();
    if (hasCellFailed != nullptr) {
        *hasCellFailed = true;
    }

    bool isLastPart = false;
    uint8_t unSentCellCount = smsIndexer->GetUnSentCellCount();
    if (unSentCellCount == 0) {
        isLastPart = true;
    }
    TELEPHONY_LOGI("isLastPart:%{public}d", isLastPart);
    if (isLastPart) {
        smsIndexer->SetPsResendCount(INITIAL_COUNT);
        smsIndexer->SetCsResendCount(INITIAL_COUNT);
        // save to db and update state
        sptr<ISendShortMessageCallback> sendCallback = smsIndexer->GetSendCallback();
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGI("send sms result fail from ril response. IsMmsApp:%{public}d;dataBaseId:%{public}d;",
            smsIndexer->GetIsMmsApp(), smsIndexer->GetDataBaseId());
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_SEND_RESULT_FAIL, "send sms result fail from ril response");
        if (smsIndexer->GetDataBaseId() == 0) {
            return;
        }
        DataShare::DataShareValuesBucket sessionBucket;
        sessionBucket.Put(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_FAILED);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(SmsMmsInfo::MSG_ID, smsIndexer->GetDataBaseId());
        if (!DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateSms(predicates, sessionBucket)) {
            TELEPHONY_LOGE("modify fail while SendMessageFailed. id:%{public}d;", smsIndexer->GetDataBaseId());
        }
        std::string  notifyType = SmsMmsCommonData::MESSAGE_STATUS_CHANGE_NOTIFY;
        if (smsIndexer->GetIsMmsApp()) {
            notifyType = SmsMmsCommonData::SMS_MMS_SENT_RESULT_NOTIFY;
        }
        TELEPHONY_LOGI("before send boradcast. SendMessageSucceed %{public}d", smsIndexer->GetDataBaseId());
        DelayedSingleton<SmsMmsCommon>::GetInstance()->SendBroadcast(smsIndexer->GetDataBaseId(),
            notifyType, SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_FAILED,
            SmsMmsCommonData::SMS_MMS_INFO_SMS_TYPE);
        TELEPHONY_LOGI("after send boradcast. SendMessageSucceed %{public}d", smsIndexer->GetDataBaseId());
    }
}

void SmsSender::SendResultCallBack(
    const std::shared_ptr<SmsSendIndexer> &indexer, ISendShortMessageCallback::SmsSendResult result)
{
    if (indexer != nullptr && indexer->GetSendCallback() != nullptr) {
        indexer->GetSendCallback()->OnSmsSendResult(result);
    }
}

void SmsSender::SendResultCallBack(
    const sptr<ISendShortMessageCallback> &sendCallback, ISendShortMessageCallback::SmsSendResult result)
{
    if (sendCallback != nullptr) {
        sendCallback->OnSmsSendResult(result);
    }
}

void SmsSender::SendCacheMapTimeoutCheck()
{
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    system_clock::duration timePoint = system_clock::now().time_since_epoch();
    seconds sec = duration_cast<seconds>(timePoint);
    int64_t timeStamp = sec.count();
    auto item = sendCacheMap_.begin();
    while (item != sendCacheMap_.end()) {
        auto iter = item++;
        shared_ptr<SmsSendIndexer> &indexer = iter->second;
        if (indexer == nullptr || (timeStamp - indexer->GetTimeStamp()) > EXPIRED_TIME) {
            sendCacheMap_.erase(iter);
        }
    }
}

bool SmsSender::SendCacheMapLimitCheck(const sptr<ISendShortMessageCallback> &sendCallback)
{
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    if (sendCacheMap_.size() > MSG_QUEUE_LIMIT) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return true;
    }
    return false;
}

bool SmsSender::SendCacheMapAddItem(int64_t id, const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    if (smsIndexer != nullptr) {
        auto result = sendCacheMap_.emplace(id, smsIndexer);
        return result.second;
    }
    return false;
}

bool SmsSender::SendCacheMapEraseItem(int64_t id)
{
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    return (sendCacheMap_.erase(id) != 0);
}

std::shared_ptr<SmsSendIndexer> SmsSender::FindCacheMapAndTransform(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("event is nullptr");
        return nullptr;
    }
    std::shared_ptr<SmsSendIndexer> smsIndexer = nullptr;
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    for (auto const &pair : sendCacheMap_) {
        TELEPHONY_LOGI("Key = %{public}" PRId64 "", pair.first);
    }
    std::shared_ptr<RadioResponseInfo> res = event->GetSharedObject<RadioResponseInfo>();
    if (res != nullptr) {
        TELEPHONY_LOGI("flag = %{public}d", res->flag);
        auto iter = sendCacheMap_.find(res->flag);
        if (iter != sendCacheMap_.end()) {
            smsIndexer = iter->second;
            if (smsIndexer == nullptr) {
                TELEPHONY_LOGE("smsIndexer is nullptr");
                return nullptr;
            }
            smsIndexer->SetErrorCode(ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            smsIndexer->SetMsgRefId64Bit(res->flag);
            smsIndexer->SetIsFailure(true);
            UpdateUnSentCellCount(smsIndexer->GetMsgRefId());
        }
        return smsIndexer;
    }
    std::shared_ptr<SendSmsResultInfo> info = event->GetSharedObject<SendSmsResultInfo>();
    if (info != nullptr) {
        TELEPHONY_LOGI("flag = %{public}" PRId64 "", info->flag);
        auto iter = sendCacheMap_.find(info->flag);
        if (iter != sendCacheMap_.end()) {
            TELEPHONY_LOGI("msgRef = %{public}d", info->msgRef);
            smsIndexer = iter->second;
            if (smsIndexer == nullptr) {
                TELEPHONY_LOGE("smsIndexer is nullptr");
                return nullptr;
            }
            smsIndexer->SetAckPdu(std::move(StringUtils::HexToByteVector(info->pdu)));
            info->errCode != 0? smsIndexer->SetIsFailure(true) : smsIndexer->SetIsFailure(false);
            smsIndexer->SetErrorCode(info->errCode);
            smsIndexer->SetMsgRefId64Bit(info->flag);
            UpdateUnSentCellCount(smsIndexer->GetMsgRefId());
        }
    }
    return smsIndexer;
}

void SmsSender::UpdateUnSentCellCount(uint8_t refId)
{
    std::shared_ptr<SmsSendIndexer> smsIndexer = nullptr;
    for (auto it = sendCacheMap_.begin(); it != sendCacheMap_.end(); ++it) {
        smsIndexer = it->second;
        if (smsIndexer == nullptr) {
            continue;
        }
        uint8_t unSentCount = smsIndexer->GetUnSentCellCount();
        if (smsIndexer->GetMsgRefId() == refId && unSentCount > 0) {
            smsIndexer->SetUnSentCellCount(unSentCount - 1);
        }
    }
}

bool SmsSender::SetImsSmsConfig(int32_t slotId, int32_t enable)
{
    auto smsClient = DelayedSingleton<ImsSmsClient>::GetInstance();
    if (smsClient == nullptr) {
        TELEPHONY_LOGE("SetImsSmsConfig return, ImsSmsClient is nullptr.");
        return false;
    }
    imsSmsCfg_ = enable;
    std::unique_lock<std::mutex> lck(ctx_);
    resIsSmsReady_ = false;

    int32_t reply = smsClient->ImsSetSmsConfig(slotId, enable);
    TELEPHONY_LOGI("SetImsSmsConfig reply = %{public}d", reply);
    while (resIsSmsReady_) {
        TELEPHONY_LOGI("SetImsSmsConfig::wait(), resIsSmsReady_ = false");
        if (cv_.wait_for(lck, std::chrono::seconds(WAIT_TIME_SECOND)) == std::cv_status::timeout) {
            break;
        }
    }
    TELEPHONY_LOGI("SmsSender::SetImsSmsConfig(), %{public}d:", imsSmsCfg_);
    return true;
}

void SmsSender::HandleResend(const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("smsIndexer is nullptr");
        return;
    }
    // resending mechanism
    bool errorCode = false;
    if (smsIndexer->GetErrorCode() == static_cast<int32_t>(ErrType::ERR_CMD_SEND_FAILURE)) {
        errorCode = true;
    }
    bool csResend = false;
    if (!lastSmsDomain_ && smsIndexer->GetCsResendCount() < MAX_SEND_RETRIES) {
        csResend = true;
    }
    bool psResend = false;
    if (lastSmsDomain_ && smsIndexer->GetPsResendCount() <= MAX_SEND_RETRIES) {
        psResend = true;
    }
    if (errorCode && (csResend || psResend)) {
        if (lastSmsDomain_ && psResend) {
            smsIndexer->SetPsResendCount(smsIndexer->GetPsResendCount() + 1);
            SendEvent(MSG_SMS_RETRY_DELIVERY, smsIndexer, DELAY_MAX_TIME_MSCE);
        } else if (csResend) {
            smsIndexer->SetCsResendCount(smsIndexer->GetCsResendCount() + 1);
            SendEvent(MSG_SMS_RETRY_DELIVERY, smsIndexer, DELAY_MAX_TIME_MSCE);
        }
    } else {
        SendMessageFailed(smsIndexer);
    }
}

uint8_t SmsSender::GetMsgRef8Bit()
{
    return ++msgRef8bit_;
}

int64_t SmsSender::GetMsgRef64Bit()
{
    return ++msgRef64bit_;
}

void SmsSender::SetNetworkState(bool isImsNetDomain, int32_t voiceServiceState)
{
    isImsNetDomain_ = isImsNetDomain;
    voiceServiceState_ = voiceServiceState;
    TELEPHONY_LOGD("isImsNetDomain = %{public}s voiceServiceState = %{public}d",
        isImsNetDomain_ ? "true" : "false", voiceServiceState_);
}

bool SmsSender::CheckForce7BitEncodeType()
{
    auto helperPtr = DelayedSingleton<SmsPersistHelper>::GetInstance();
    if (helperPtr == nullptr) {
        TELEPHONY_LOGE("Check User Force 7Bit Encode Type helperPtr nullptr error.");
        return false;
    }
    return helperPtr->QueryParamBoolean(SmsPersistHelper::SMS_ENCODING_PARAM_KEY, false);
}

SmsCodingNationalType SmsSender::GetSmsCodingNationalType(int slotId)
{
    SmsCodingNationalType smsCodingNational = SMS_CODING_NATIONAL_TYPE_DEFAULT;
    OperatorConfig operatorConfig;
    CoreManagerInner::GetInstance().GetOperatorConfigs(slotId, operatorConfig);
    if (operatorConfig.intValue.find(KEY_SMS_CODING_NATIONAL_INT) != operatorConfig.intValue.end()) {
        smsCodingNational = (SmsCodingNationalType)operatorConfig.intValue[KEY_SMS_CODING_NATIONAL_INT];
    }
    return smsCodingNational;
}

std::optional<int32_t> SmsSender::GetNetworkId()
{
    return networkId_;
}

void SmsSender::SetNetworkId(std::optional<int32_t> &id)
{
    networkId_ = id;
}

void SmsSender::OnRilAdapterHostDied()
{
    std::shared_ptr<SmsSendIndexer> smsIndexer = nullptr;
    std::lock_guard<std::mutex> guard(sendCacheMapMutex_);
    for (auto it = sendCacheMap_.begin(); it != sendCacheMap_.end(); ++it) {
        smsIndexer = it->second;
        if (smsIndexer == nullptr || smsIndexer->GetIsFailure()) {
            TELEPHONY_LOGE("smsIndexer is nullptr");
            continue;
        }
        if (!SendCacheMapEraseItem(smsIndexer->GetMsgRefId64Bit())) {
            TELEPHONY_LOGE("SendCacheMapEraseItem fail !!!!!");
        }
        smsIndexer->SetIsFailure(true);
        smsIndexer->SetPsResendCount(INITIAL_COUNT);
        smsIndexer->SetCsResendCount(INITIAL_COUNT);
        sptr<ISendShortMessageCallback> sendCallback = smsIndexer->GetSendCallback();
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGI("Message(s) failed to send due to RIL died.");
    }
}

void SmsSender::CharArrayToString(const uint8_t *data, uint32_t dataLen, std::string &dataStr)
{
    uint32_t indexData = 0;
    while (indexData < dataLen) {
        dataStr += data[indexData];
        indexData++;
    }
}

void SmsSender::DataBasedSmsDeliverySplitPage(GsmSmsMessage &gsmSmsMessage, std::vector<struct SplitInfo> cellsInfos,
    std::shared_ptr<struct SmsTpdu> tpdu, uint8_t msgRef8bit, const std::string &desAddr, const std::string &scAddr,
    int32_t port, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    uint32_t cellsInfosSize = static_cast<uint32_t>(cellsInfos.size());
    for (uint32_t indexData = 0; indexData < cellsInfosSize; indexData++) {
        const uint8_t *dataItem = reinterpret_cast<uint8_t *>(cellsInfos[indexData].text.data());
        uint32_t dataItemLen = static_cast<uint32_t>(cellsInfos[indexData].text.size());

        std::shared_ptr<SmsSendIndexer> indexer =
            make_shared<SmsSendIndexer>(desAddr, scAddr, port, dataItem, dataItemLen, sendCallback, deliveryCallback);
        if (indexer == nullptr) {
            SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            TELEPHONY_LOGE("create SmsSendIndexer nullptr");
            return;
        }

        if (tpdu == nullptr) {
            return;
        }
        (void)memset_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, 0x00, MAX_USER_DATA_LEN + 1);
        if (cellsInfos[indexData].encodeData.size() > MAX_USER_DATA_LEN + 1) {
            TELEPHONY_LOGE("data length invalid.");
            return;
        }
        int ret = memcpy_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, &cellsInfos[indexData].encodeData[0],
            cellsInfos[indexData].encodeData.size());
        if (ret != EOK) {
            SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            TELEPHONY_LOGE("ret:%{public}d", ret);
            return;
        }
        DataBasedSmsDeliveryPacketSplitPage(gsmSmsMessage, tpdu, msgRef8bit, indexData, port, scAddr, sendCallback,
            deliveryCallback, indexer, cellsInfos);
    }
}

void SmsSender::DataBasedSmsDeliveryPacketSplitPage(GsmSmsMessage &gsmSmsMessage, std::shared_ptr<struct SmsTpdu> tpdu,
    uint8_t msgRef8bit, uint32_t indexData, int32_t port, const std::string &scAddr,
    const sptr<ISendShortMessageCallback> &sendCallback, const sptr<IDeliveryShortMessageCallback> &deliveryCallback,
    std::shared_ptr<SmsSendIndexer> indexer, std::vector<struct SplitInfo> cellsInfos)
{
    tpdu->data.submit.userData.length = static_cast<int>(cellsInfos[indexData].encodeData.size());
    tpdu->data.submit.userData.data[cellsInfos[indexData].encodeData.size()] = 0;
    tpdu->data.submit.msgRef = msgRef8bit;
    int headerCnt = 0;
    uint32_t cellsInfosSize = static_cast<uint32_t>(cellsInfos.size());
    if (cellsInfosSize > 1) {
        indexer->SetIsConcat(true);
        SmsConcat concat;
        concat.is8Bits = true;
        concat.msgRef = msgRef8bit;
        concat.totalSeg = static_cast<uint16_t>(cellsInfosSize);
        concat.seqNum = static_cast<uint16_t>(indexData + 1);
        indexer->SetSmsConcat(concat);
        headerCnt += gsmSmsMessage.SetHeaderConcat(headerCnt, concat);
    }
    if (headerCnt >= MAX_UD_HEADER_NUM) {
        TELEPHONY_LOGE("PDU header Array out of bounds");
        return;
    }

    tpdu->data.submit.userData.header[headerCnt].udhType = UDH_APP_PORT_16BIT;
    tpdu->data.submit.userData.header[headerCnt].udh.appPort16bit.destPort = ((unsigned short)port & 0xFFFF);
    tpdu->data.submit.userData.header[headerCnt].udh.appPort16bit.originPort = 0;
    headerCnt++;
    tpdu->data.submit.bHeaderInd = (headerCnt > 0) ? true : false;
    /* Set User Data Header for Alternate Reply Address */
    headerCnt += gsmSmsMessage.SetHeaderReply(headerCnt);
    /* Set User Data Header for National Language Single Shift */
    DataCodingScheme pCodingType = DATA_CODING_7BIT;
    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    headerCnt += gsmSmsMessage.SetHeaderLang(headerCnt, pCodingType, langId);
    tpdu->data.submit.userData.headerCnt = headerCnt;

    std::shared_ptr<struct EncodeInfo> encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(scAddr, false);
    if (encodeInfo == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("encodeInfo nullptr error.");
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_PDU_ENCODEING_FAIL, "data sms gsm encodeInfo nullptr error");
        return;
    }

    if (cellsInfosSize > 1 && indexData < (cellsInfosSize - 1)) {
        tpdu->data.submit.bStatusReport = false;
    } else {
        tpdu->data.submit.bStatusReport = (deliveryCallback == nullptr) ? false : true;
    }
    encodeInfo->isMore_ = (cellsInfosSize > 1) ? true : false;
    DataBasedSmsDeliverySendSplitPage(encodeInfo, sendCallback, indexer, msgRef8bit, cellsInfosSize);
}

void SmsSender::SendCallbackExceptionCase(const sptr<ISendShortMessageCallback> &sendCallback, std::string str)
{
    SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
    TELEPHONY_LOGE("%{public}s tpdu nullptr error.", str.c_str());
    SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
        SmsMmsErrorCode::SMS_ERROR_PDU_ENCODEING_FAIL, "data sms gsm encodeInfo nullptr error");
}

void SmsSender::DataBasedSmsDeliverySendSplitPage(std::shared_ptr<struct EncodeInfo> encodeInfo,
    const sptr<ISendShortMessageCallback> &sendCallback, shared_ptr<SmsSendIndexer> indexer, uint8_t msgRef8bit,
    uint32_t cellsInfosSize)
{
    std::vector<uint8_t> smca(encodeInfo->smcaData_, encodeInfo->smcaData_ + encodeInfo->smcaLen);
    std::vector<uint8_t> pdu(encodeInfo->tpduData_, encodeInfo->tpduData_ + encodeInfo->tpduLen);
    TELEPHONY_LOGE("cellsInfosSize:%{public}d", cellsInfosSize);
    std::shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(cellsInfosSize);
    std::shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    if (unSentCellCount == nullptr || hasCellFailed == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }

    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    int64_t timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    indexer->SetUnSentCellCount(*unSentCellCount);
    indexer->SetHasCellFailed(hasCellFailed);
    indexer->SetEncodeSmca(std::move(smca));
    indexer->SetEncodePdu(std::move(pdu));
    indexer->SetHasMore(encodeInfo->isMore_);
    indexer->SetMsgRefId(msgRef8bit);
    indexer->SetNetWorkType(NET_TYPE_GSM);
    indexer->SetTimeStamp(timeStamp);
    SendSmsToRil(indexer);
}
} // namespace Telephony
} // namespace OHOS
