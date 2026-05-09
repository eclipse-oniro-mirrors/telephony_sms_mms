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

#include "sms_receive_handler.h"
#include "gsm_sms_message.h"
#include "radio_event.h"
#include "sms_common.h"
#include "sms_hisysevent.h"
#include "sms_persist_helper.h"
#include "sms_policy_utils.h"
#include "telephony_log_wrapper.h"
#include "iservice_registry.h"

namespace OHOS {
namespace Telephony {
using namespace std;
constexpr static uint16_t PDU_POS_OFFSET = 1;
constexpr static uint8_t SMS_TYPE_GSM = 1;
constexpr static uint8_t SMS_TYPE_CDMA = 2;
static const std::string WAP_SEQ_NUMBER_TAG = "0003";
constexpr static size_t WAP_SEQ_NUMBER_LEN = 10;
const std::string STR_URI = "datashare:///com.ohos.smsmmsability/sms_mms/sms_subsection";
const std::string EXT_URI = "datashare:///com.ohos.smsmmsability";

SmsReceiveHandler::SmsReceiveHandler(int32_t slotId) : TelEventHandler("SmsReceiveHandler"), slotId_(slotId)
{
    smsWapPushHandler_ = std::make_unique<SmsWapPushHandler>(slotId);
    if (smsWapPushHandler_ == nullptr) {
        TELEPHONY_LOGE("make sms wapPush Hander error.");
    }
    CreateRunningLockInner();
}

SmsReceiveHandler::~SmsReceiveHandler() {}
// Function is used to judge if datashare interface is ready
bool SmsReceiveHandler::IsDataShareReady()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        TELEPHONY_LOGE("Get system ability mgr failed.");
        return false;
    }
    auto remoteObj = saManager->GetSystemAbility(TELEPHONY_SMS_MMS_SYS_ABILITY_ID);
    if (remoteObj == nullptr) {
        TELEPHONY_LOGE("Getsystemability service failed.");
        return false;
    }
    std::lock_guard<std::mutex> lock(datashareMutex_);
    std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> ret =
        DataShare::DataShareHelper::Create(remoteObj, STR_URI, EXT_URI);
    TELEPHONY_LOGI("create data_share helper, ret=%{public}d", ret.first);
    if (ret.first == E_OK) {
        ret.second->Release();
        return true;
    }
    return false;
}

void SmsReceiveHandler::HandleMessageQueue()
{
    size_t queueLength = smsBaseMessageQueue_.size();
    // send un-operate message to the remain receive procedure.
    for (size_t i = 0; i < queueLength; i++) {
        HandleRemainDataShare(smsBaseMessageQueue_.front());
        smsBaseMessageQueue_.pop();
        TELEPHONY_LOGI("operated a sms and there are %{public}zu sms wait to be operated",
            smsBaseMessageQueue_.size());
    }
    // give app 20s power lock time to handle receive.
    this->SendEvent(DELAY_RELEASE_RUNNING_LOCK_EVENT_ID, DELAY_REDUCE_RUNNING_LOCK_SMS_QUEUE_TIMEOUT_MS);
    reconnectDataShareCount_ = 0;
    // set the flag of datashare is not ready to false.
    alreadySendEvent_ = false;
}

void SmsReceiveHandler::HandleReconnectEvent()
{
    if (IsDataShareReady()) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        HandleMessageQueue();
    } else {
        reconnectDataShareCount_++;
        // if retry times over 20(the datashare is not ready in 100s), stop try.
        if (reconnectDataShareCount_ >= RECONNECT_MAX_COUNT) {
            TELEPHONY_LOGI("Over the max reconnect count:20 and there are %{public}zu sms have not been operated",
                smsBaseMessageQueue_.size());
            reconnectDataShareCount_ = 0;
            alreadySendEvent_ = false;
            std::lock_guard<std::mutex> lockRequest(mutexRunningLock_);
            ReleaseRunningLock();
        } else {
            TELEPHONY_LOGI("Send event %{public}u times", reconnectDataShareCount_);
            /*
             *applylock can only hold power lock 60s, so reapply lock every 5s to ensure have entire 60s to
             *operate sms when datashare is ready('reduce' to release and 'apply' to rehold)
             */
            ReduceRunningLock();
            ApplyRunningLock();
            this->SendEvent(RETRY_CONNECT_DATASHARE_EVENT_ID, DELAY_RETRY_CONNECT_DATASHARE_MS);
        }
    }
}

void SmsReceiveHandler::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("SmsReceiveHandler::ProcessEvent event == nullptr");
        return;
    }

    uint32_t eventId = 0;
    eventId = event->GetInnerEventId();
    TELEPHONY_LOGI("SmsReceiveHandler::ProcessEvent eventId = %{public}d", eventId);
    switch (eventId) {
        case RadioEvent::RADIO_GSM_SMS:
        case RadioEvent::RADIO_CDMA_SMS: {
            HandleSmsEvent(event);
            break;
        }
        case RUNNING_LOCK_TIMEOUT_EVENT_ID:
            HandleRunningLockTimeoutEvent(event);
            break;
        case DELAY_RELEASE_RUNNING_LOCK_EVENT_ID:
            ReduceRunningLock();
            break;
        case RETRY_CONNECT_DATASHARE_EVENT_ID: {
            HandleReconnectEvent();
            break;
        }
#ifdef BASE_POWER_IMPROVEMENT_FEATURE
        case SMS_EVENT_NEW_SMS_REPLY:
            HandleSmsReply();
            break;
#endif
        default:
            TELEPHONY_LOGE("SmsReceiveHandler::ProcessEvent Unknown eventId %{public}d", eventId);
            break;
    }
}

void SmsReceiveHandler::HandleSmsEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (SmsPolicyUtils::IsSmsPolicyDisable()) {
        TELEPHONY_LOGE("SmsReceiveHandler::ProcessEvent sms policy is disabled");
        ReplySmsToSmsc(AckIncomeCause::SMS_ACK_RESULT_OK);
        return;
    }
    ApplyRunningLock();
    std::shared_ptr<SmsBaseMessage> message = TransformMessageInfo(event->GetSharedObject<SmsMessageInfo>());
    if (!alreadySendEvent_) {
        if (IsDataShareReady()) {
            HandleReceivedSms(message);
            this->SendEvent(DELAY_RELEASE_RUNNING_LOCK_EVENT_ID, DELAY_REDUCE_RUNNING_LOCK_TIMEOUT_MS);
        } else {
            HandleReceivedSmsWithoutDataShare(message);
            this->SendEvent(RETRY_CONNECT_DATASHARE_EVENT_ID, DELAY_RETRY_CONNECT_DATASHARE_MS);
            alreadySendEvent_ = true;
        }
    } else {
        HandleReceivedSmsWithoutDataShare(message);
    }
}

#ifdef BASE_POWER_IMPROVEMENT_FEATURE
void SmsReceiveHandler::HandleSmsReply()
{
    TELEPHONY_LOGI("HandleSmsReply enter");
    auto smsStateHandler = DelayedSingleton<SmsStateHandler>::GetInstance();
    smsStateHandler->ProcessStrExitFinishEvent();
}
#endif

void SmsReceiveHandler::ApplyRunningLock()
{
#ifdef ABILITY_POWER_SUPPORT
    if (smsRunningLock_ == nullptr) {
        CreateRunningLockInner();
    }
    std::lock_guard<std::mutex> lockGuard(mutexRunningLock_);
    if (smsRunningLock_ != nullptr) {
        smsRunningLockCount_++;
        smsLockSerialNum_++;
        TELEPHONY_LOGI("ApplyRunningLock, try to lock. count: %{public}d, serial: %{public}d",
            static_cast<int>(smsRunningLockCount_), static_cast<int>(smsLockSerialNum_));
        smsRunningLock_->Lock(RUNNING_LOCK_DEFAULT_TIMEOUT_MS); // Automatic release after the 60s.
        this->SendEvent(RUNNING_LOCK_TIMEOUT_EVENT_ID, smsLockSerialNum_, RUNNING_LOCK_DEFAULT_TIMEOUT_MS);
    }
#endif
}

void SmsReceiveHandler::ReduceRunningLock()
{
#ifdef ABILITY_POWER_SUPPORT
    std::lock_guard<std::mutex> lockRequest(mutexRunningLock_);
    TELEPHONY_LOGI("ReduceRunningLock, count:%{public}d", static_cast<int>(smsRunningLockCount_));
    if (smsRunningLock_ != nullptr) {
        if (smsRunningLockCount_ > 1) {
            smsRunningLockCount_--;
        } else {
            smsRunningLockCount_ = 0;
            ReleaseRunningLock();
        }
    }
#endif
}

void SmsReceiveHandler::ReleaseRunningLock()
{
#ifdef ABILITY_POWER_SUPPORT
    if (smsRunningLock_ == nullptr) {
        TELEPHONY_LOGE("ReleaseRunningLock, smsRunningLock_ is nullptr");
        return;
    }
    TELEPHONY_LOGI("ReleaseRunningLock, try to unlock.");
    smsRunningLockCount_ = 0;
    int ret = smsRunningLock_->UnLock();
    if (ret != PowerMgr::E_GET_POWER_SERVICE_FAILED) {
        // Call UnLock success, remove event.
        this->RemoveEvent(RUNNING_LOCK_TIMEOUT_EVENT_ID);
        return;
    }
    TELEPHONY_LOGI("ReleaseRunningLock, no found power service, retry.");
    this->SendEvent(RUNNING_LOCK_TIMEOUT_EVENT_ID, smsLockSerialNum_, DELAY_RELEASE_RUNNING_LOCK_TIMEOUT_MS);
#endif
}

void SmsReceiveHandler::CreateRunningLockInner()
{
#ifdef ABILITY_POWER_SUPPORT
    auto &powerMgrClient = PowerMgr::PowerMgrClient::GetInstance();
    std::lock_guard<std::mutex> lockGuard(mutexRunningLock_);
    smsRunningLock_ = powerMgrClient.CreateRunningLock("telSmsRunningLock",
        PowerMgr::RunningLockType::RUNNINGLOCK_BACKGROUND_PHONE);
    smsRunningLockCount_ = 0;
    smsLockSerialNum_ = 0;
#endif
}

void SmsReceiveHandler::HandleRunningLockTimeoutEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
#ifdef ABILITY_POWER_SUPPORT
    auto serial = event->GetParam();
    std::lock_guard<std::mutex> lockRequest(mutexRunningLock_);
    if (serial == smsLockSerialNum_) {
        TELEPHONY_LOGE("HandleRunningLockTimeoutEvent, serial:%{public}d, smsLockSerialNum_:%{public}d",
            static_cast<int>(serial), static_cast<int>(smsLockSerialNum_));
        ReleaseRunningLock();
    }
#endif
}

void SmsReceiveHandler::HandleReceivedSms(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("smsBaseMessage is nullptr");
        return;
    }
    ReplySmsToSmsc(HandleSmsByType(smsBaseMessage));
}

void SmsReceiveHandler::HandleReceivedSmsWithoutDataShare(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("smsBaseMessage is nullptr");
        return;
    }
    int32_t result = ReplySmsToSmsc(HandleAck(smsBaseMessage));
    if (result) {
        std::lock_guard lock(queueMutex_);
        smsBaseMessageQueue_.push(smsBaseMessage);
        TELEPHONY_LOGI("Received a new message and pushed it in queue");
    }
}

void SmsReceiveHandler::CombineMessagePart(const std::shared_ptr<SmsReceiveIndexer> &indexer)
{
    std::shared_ptr<vector<string>> pdus = make_shared<vector<string>>();
    if ((indexer == nullptr) || (pdus == nullptr)) {
        TELEPHONY_LOGE("indexer or pdus is nullptr");
        return;
    }
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if ((reliabilityHandler == nullptr)) {
        TELEPHONY_LOGE("reliabilityHandler is nullptr");
        return;
    }
    if (indexer->IsSingleMsg()) {
        string pdu = StringUtils::StringToHex(indexer->GetPdu());
        pdus->push_back(pdu);
    } else {
        if (!CombineMultiPageMessage(indexer, pdus, reliabilityHandler)) {
            TELEPHONY_LOGI("The multi-page text didn't all arrive");
            return;
        }
    }

    if (indexer->GetIsWapPushMsg()) {
        if (smsWapPushHandler_ != nullptr) {
            auto rawWapPushUserData = indexer->GetRawWapPushUserData();
            if (!smsWapPushHandler_->DecodeWapPushPdu(indexer, rawWapPushUserData)) {
                SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::WAP_PUSH,
                    SmsMmsErrorCode::SMS_ERROR_PDU_DECODE_FAIL, "Wap push decode wap push fail");
            }
        }
        return;
    }
    reliabilityHandler->SendBroadcast(indexer, pdus);
}

bool SmsReceiveHandler::CombineMultiPageMessage(const std::shared_ptr<SmsReceiveIndexer> &indexer,
    std::shared_ptr<std::vector<std::string>> pdus, std::shared_ptr<SmsReceiveReliabilityHandler> reliabilityHandler)
{
    int msgSeg = static_cast<int>(indexer->GetMsgCount());
    pdus->assign(msgSeg, "");
    int8_t notNullPart = msgSeg;
    std::vector<SmsReceiveIndexer> dbIndexers;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SmsSubsection::SENDER_NUMBER, indexer->GetOriginatingAddress())
        ->And()
        ->EqualTo(SmsSubsection::SMS_SUBSECTION_ID, std::to_string(indexer->GetMsgRefId()))
        ->And()
        ->EqualTo(SmsSubsection::SIZE, std::to_string(indexer->GetMsgCount()));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->Query(predicates, dbIndexers);
    int8_t count = 0;
    for (const auto &v : dbIndexers) {
        ++count;
        string pdu = StringUtils::StringToHex(v.GetPdu());
        if ((v.GetMsgSeqId() - PDU_POS_OFFSET >= msgSeg) || (v.GetMsgSeqId() - PDU_POS_OFFSET < 0)) {
            reliabilityHandler->DeleteMessageFormDb(indexer->GetMsgRefId(), indexer->GetDataBaseId(),
                indexer->GetMsgCount(), indexer->GetOriginatingAddress());
            return false;
        }
        pdus->at(v.GetMsgSeqId() - PDU_POS_OFFSET) = pdu;
        if (v.GetPdu().size() == 0) {
            --notNullPart;
        }
    }
    if ((count != msgSeg) || (pdus->empty()) || (notNullPart != msgSeg)) {
        return false;
    }
    UpdateMultiPageMessage(indexer, pdus);
    return true;
}

void SmsReceiveHandler::UpdateMultiPageMessage(
    const std::shared_ptr<SmsReceiveIndexer> &indexer, std::shared_ptr<std::vector<std::string>> pdus)
{
    if ((indexer == nullptr) || (pdus == nullptr) || (pdus->empty())) {
        TELEPHONY_LOGE("indexer or pdus is null");
        return;
    }
    std::string messagBody;
    std::string userDataRaw;
    std::string rawWapPushUserData;
    for (const auto &pdu : *pdus) {
        if (pdu.empty()) {
            continue;
        }
        std::shared_ptr<SmsBaseMessage> baseMessage = GsmSmsMessage::CreateMessage(pdu);
        if (baseMessage == nullptr) {
            continue;
        }
        messagBody.append(baseMessage->GetVisibleMessageBody());
        userDataRaw.append(baseMessage->GetRawUserData());
        if (!indexer->GetIsWapPushMsg()) {
            continue;
        }
        auto wapDataHex = StringUtils::StringToHex(baseMessage->GetRawWapPushUserData());
        if (wapDataHex.substr(0, WAP_SEQ_NUMBER_TAG.size()) == WAP_SEQ_NUMBER_TAG) {
            rawWapPushUserData.append(StringUtils::HexToString(wapDataHex.substr(WAP_SEQ_NUMBER_LEN)));
        } else {
            rawWapPushUserData.append(StringUtils::HexToString(wapDataHex));
        }
    }

    indexer->SetVisibleMessageBody(messagBody);
    indexer->SetRawUserData(userDataRaw);
    if (indexer->GetIsWapPushMsg()) {
        indexer->SetRawWapPushUserData(rawWapPushUserData);
    }
}

bool SmsReceiveHandler::IsRepeatedMessagePart(const shared_ptr<SmsReceiveIndexer> &smsIndexer)
{
    if (smsIndexer != nullptr) {
        std::vector<SmsReceiveIndexer> dbIndexers;
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(SmsSubsection::SENDER_NUMBER, smsIndexer->GetOriginatingAddress())
            ->And()
            ->EqualTo(SmsSubsection::SMS_SUBSECTION_ID, std::to_string(smsIndexer->GetMsgRefId()))
            ->And()
            ->EqualTo(SmsSubsection::SIZE, std::to_string(smsIndexer->GetMsgCount()));
        DelayedSingleton<SmsPersistHelper>::GetInstance()->Query(predicates, dbIndexers);

        for (const auto &it : dbIndexers) {
            if (it.GetMsgSeqId() == smsIndexer->GetMsgSeqId()) {
                return true;
            }
        }
    }
    return false;
}

bool SmsReceiveHandler::AddMsgToDB(const std::shared_ptr<SmsReceiveIndexer> indexer)
{
    if (indexer == nullptr) {
        TELEPHONY_LOGE("indexer is nullptr.");
        return false;
    }

    DataShare::DataShareValuesBucket bucket;
    bucket.Put(SmsSubsection::SLOT_ID, std::to_string(slotId_));
    bucket.Put(SmsSubsection::RECEIVER_NUMBER, indexer->GetOriginatingAddress());
    bucket.Put(SmsSubsection::SENDER_NUMBER, indexer->GetOriginatingAddress());
    bucket.Put(SmsSubsection::START_TIME, std::to_string(indexer->GetTimestamp()));
    bucket.Put(SmsSubsection::END_TIME, std::to_string(indexer->GetTimestamp()));
    bucket.Put(SmsSubsection::REW_PUD, StringUtils::StringToHex(indexer->GetPdu()));

    bucket.Put(SmsSubsection::FORMAT, indexer->GetIsCdma() ? SMS_TYPE_CDMA : SMS_TYPE_GSM);
    bucket.Put(SmsSubsection::DEST_PORT, indexer->GetDestPort());
    bucket.Put(SmsSubsection::SMS_SUBSECTION_ID, indexer->GetMsgRefId());
    bucket.Put(SmsSubsection::SIZE, indexer->GetMsgCount());
    bucket.Put(SmsSubsection::SUBSECTION_INDEX, indexer->GetMsgSeqId());
    int32_t dataBaseId = 0;
    bool ret = DelayedSingleton<SmsPersistHelper>::GetInstance()->Insert(bucket, dataBaseId);
    indexer->SetDataBaseId(dataBaseId);
    if (!ret) {
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_ADD_TO_DATABASE_FAIL, "add msg to database error");
    }
    return ret;
}
} // namespace Telephony
} // namespace OHOS