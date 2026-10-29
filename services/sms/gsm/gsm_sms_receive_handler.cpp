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

#include "gsm_sms_receive_handler.h"

#include "core_manager_inner.h"
#include "gsm_sms_message.h"
#include "radio_event.h"
#include "sms_common.h"
#ifdef SMS_MMS_SATELLITE
#include "satellite_sms_client.h"
#endif // SMS_MMS_SATELLITE
#include "sms_hisysevent.h"
#include "sms_persist_helper.h"
#include "sms_receive_reliability_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr int16_t INVALID_SMS_PORT = 0x157b;

namespace {
/*
 * Messages that HandleSmsByType / HandleNormalSmsByType acknowledge but never store or show: a special message
 * (e.g. Type 0, 3GPP TS 23.040 9.2.3.9) and a message to INVALID_SMS_PORT. A copy queued before DataShare was
 * ready is dropped in the same way.
 */
bool IsAckedAndDropped(GsmSmsMessage &message)
{
    if (message.IsSpecialMessage()) {
        TELEPHONY_LOGI("GsmSmsReceiveHandler:: IsSpecialMessage");
        return true;
    }
    if (static_cast<int16_t>(message.GetDestPort()) == INVALID_SMS_PORT) {
        TELEPHONY_LOGI("[invalid sms port]");
        return true;
    }
    return false;
}
} // namespace

GsmSmsReceiveHandler::GsmSmsReceiveHandler(int32_t slotId) : SmsReceiveHandler(slotId) {}

GsmSmsReceiveHandler::~GsmSmsReceiveHandler()
{
    if (smsCbHandler_ != nullptr) {
        smsCbHandler_->UnRegisterHandler();
    }
}

void GsmSmsReceiveHandler::Init()
{
    if (!RegisterHandler()) {
        TELEPHONY_LOGI("GsmSmsSender::Init Register RADIO_SMS_STATUS fail.");
    }
    smsCbHandler_ = std::make_shared<GsmSmsCbHandler>(slotId_);
    if (smsCbHandler_ == nullptr) {
        TELEPHONY_LOGE("failed to create GsmSmsCbHandler");
        return;
    }
    smsCbHandler_->Init();
    TELEPHONY_LOGI("smsCbHandler_->Run().");
}

bool GsmSmsReceiveHandler::RegisterHandler()
{
    TELEPHONY_LOGI("GsmSmsReceiveHandler::RegisteHandler Register RADIO_GSM_SMS ok.");
#ifdef SMS_MMS_SATELLITE
    auto &satelliteSmsClient = SatelliteSmsClient::GetInstance();
    if (satelliteSmsClient.GetSatelliteSupported()) {
        TELEPHONY_LOGI("satellite is supported.");
        satelliteSmsClient.AddReceiveHandler(slotId_, std::static_pointer_cast<TelEventHandler>(shared_from_this()));
    }
#endif // SMS_MMS_SATELLITE
    CoreManagerInner::GetInstance().RegisterCoreNotify(slotId_, shared_from_this(), RadioEvent::RADIO_GSM_SMS, nullptr);

    return true;
}

void GsmSmsReceiveHandler::UnRegisterHandler()
{
    TELEPHONY_LOGI("SmsReceiveHandler::UnRegisterHandler::slotId= %{public}d", slotId_);
    CoreManagerInner::GetInstance().UnRegisterCoreNotify(slotId_, shared_from_this(), RadioEvent::RADIO_GSM_SMS);
#ifdef SMS_MMS_SATELLITE
    auto &satelliteSmsClient = SatelliteSmsClient::GetInstance();
    if (satelliteSmsClient.GetSatelliteSupported() && satelliteCallback_ != nullptr) {
        TELEPHONY_LOGI("satellite is supported.");
        satelliteSmsClient.UnRegisterSmsNotify(slotId_, RadioEvent::RADIO_GSM_SMS);
        satelliteSmsClient.UnRegisterSmsNotify(slotId_, SMS_EVENT_NEW_SMS_REPLY);
    }
#endif // SMS_MMS_SATELLITE
}

#ifdef SMS_MMS_SATELLITE
void GsmSmsReceiveHandler::RegisterSatelliteCallback()
{
    auto &satelliteSmsClient = SatelliteSmsClient::GetInstance();
    if (satelliteSmsClient.GetSatelliteSupported()) {
        TELEPHONY_LOGI("gsm receiver register satellite notify");
        satelliteCallback_ =
            std::make_unique<SatelliteSmsCallback>(std::static_pointer_cast<TelEventHandler>(shared_from_this()))
                .release();
        satelliteSmsClient.RegisterSmsNotify(slotId_, RadioEvent::RADIO_GSM_SMS, satelliteCallback_);
        satelliteSmsClient.RegisterSmsNotify(slotId_, SMS_EVENT_NEW_SMS_REPLY, satelliteCallback_);
    }
}

void GsmSmsReceiveHandler::UnregisterSatelliteCallback()
{
    auto &satelliteSmsClient = SatelliteSmsClient::GetInstance();
    if (satelliteSmsClient.GetSatelliteSupported()) {
        TELEPHONY_LOGI("gsm receiver unregister satellite notify");
        satelliteCallback_ = nullptr;
    }
}
#endif // SMS_MMS_SATELLITE

int32_t GsmSmsReceiveHandler::HandleSmsByType(const shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("BaseMessage is null.");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    GsmSmsMessage *message = (GsmSmsMessage *)smsBaseMessage.get();
    if (message->IsSpecialMessage()) {
        TELEPHONY_LOGI("GsmSmsReceiveHandler:: IsSpecialMessage");
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    int ret = CheckSmsSupport();
    if (ret != AckIncomeCause::SMS_ACK_RESULT_OK) {
        return ret;
    }
    return HandleNormalSmsByType(smsBaseMessage);
}

int32_t GsmSmsReceiveHandler::HandleAck(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("BaseMessage is null");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    GsmSmsMessage *message = (GsmSmsMessage *)smsBaseMessage.get();
    if (message->IsSpecialMessage()) {
        TELEPHONY_LOGI("GsmSmsReceiveHandler:: IsSpecialMessage");
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if (reliabilityHandler == nullptr) {
        TELEPHONY_LOGE("reliabilityHandler nullptr");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    if (!(reliabilityHandler->CheckSmsCapable())) {
        TELEPHONY_LOGE("sms receive capable unSupport");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_EMPTY_INPUT_PARAMETER, "sms receive capable unsupported");
        return AckIncomeCause::SMS_ACK_PROCESSED;
    }
    if (message->IsConcatMsg()) {
        std::shared_ptr<SmsConcat> smsConcat = message->GetConcatMsg();
        if (smsConcat == nullptr) {
            TELEPHONY_LOGE("Concat is null.");
            return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
        }
    }
    return AckIncomeCause::SMS_ACK_RESULT_OK;
}

void GsmSmsReceiveHandler::HandleRemainDataShare(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("BaseMessage is null");
        return;
    }
    GsmSmsMessage *message = (GsmSmsMessage *)smsBaseMessage.get();
    if (IsAckedAndDropped(*message)) {
        return;
    }
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if (reliabilityHandler != nullptr) {
        if (!reliabilityHandler->DeleteExpireSmsFromDB()) {
            TELEPHONY_LOGE("DeleteExpireSmsFromDB fail");
        }
    }
    shared_ptr<SmsReceiveIndexer> indexer;
    std::shared_ptr<SmsConcat> smsConcat = GetValidConcatMsg(smsBaseMessage);
    if (smsConcat == nullptr) {
        indexer = make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), !message->GetGsm(), false, message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), message->GetVisibleMessageBody());
    } else {
        indexer = make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), !message->GetGsm(), message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), smsConcat->msgRef, smsConcat->seqNum, smsConcat->totalSeg,
            false, message->GetVisibleMessageBody());
    }
    if (indexer == nullptr) {
        return;
    }
    indexer->SetRawUserData(message->GetRawUserData());
    indexer->SetRawWapPushUserData(message->GetRawWapPushUserData());
    HILOG_COMM_INFO("received a gsm sms, the refid is %{public}d, this is %{public}d, a total of %{public}d",
        indexer->GetMsgRefId(), indexer->GetMsgSeqId(), indexer->GetMsgCount());
    if (indexer->GetIsText() && (smsConcat != nullptr) && IsRepeatedMessagePart(indexer)) {
        TELEPHONY_LOGE("Ack repeated error.");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_REPEATED_ERROR, "gsm message repeated error");
        return;
    }
    if (!AddMsgToDB(indexer)) {
        return;
    }
    CombineMessagePart(indexer);
}

int32_t GsmSmsReceiveHandler::CheckSmsSupport()
{
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if (reliabilityHandler == nullptr) {
        TELEPHONY_LOGE("reliabilityHandler nullptr");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    if (!(reliabilityHandler->CheckSmsCapable())) {
        TELEPHONY_LOGI("sms receive capable unSupport");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_EMPTY_INPUT_PARAMETER, "sms receive capable unsupported");
        return AckIncomeCause::SMS_ACK_PROCESSED;
    }
    if (!reliabilityHandler->DeleteExpireSmsFromDB()) {
        TELEPHONY_LOGE("DeleteExpireSmsFromDB fail");
    }
    return AckIncomeCause::SMS_ACK_RESULT_OK;
}

int32_t GsmSmsReceiveHandler::HandleNormalSmsByType(const shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("BaseMessage is null.");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    GsmSmsMessage *message = (GsmSmsMessage *)smsBaseMessage.get();

    shared_ptr<SmsReceiveIndexer> indexer;
    std::shared_ptr<SmsConcat> smsConcat = GetValidConcatMsg(smsBaseMessage);
    if (smsConcat == nullptr) {
        indexer = make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), !message->GetGsm(), false, message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), message->GetVisibleMessageBody());
    } else {
        indexer = make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), !message->GetGsm(), message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), smsConcat->msgRef, smsConcat->seqNum, smsConcat->totalSeg,
            false, message->GetVisibleMessageBody());
    }
    if (indexer == nullptr) {
        TELEPHONY_LOGE("indexer is null.");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    indexer->SetRawUserData(message->GetRawUserData());
    indexer->SetRawWapPushUserData(message->GetRawWapPushUserData());

    HILOG_COMM_INFO("received a gsm sms, the refid is %{public}d, this is %{public}d, a total of %{public}d",
        indexer->GetMsgRefId(), indexer->GetMsgSeqId(), indexer->GetMsgCount());
    if (indexer->GetIsText() && (smsConcat != nullptr) && IsRepeatedMessagePart(indexer)) {
        TELEPHONY_LOGE("Ack repeated error.");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_REPEATED_ERROR, "gsm message repeated error");
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    auto destPort = indexer->GetDestPort();
    if (destPort == INVALID_SMS_PORT) {
        TELEPHONY_LOGI("[invalid sms port]");
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    if (!AddMsgToDB(indexer)) {
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    CombineMessagePart(indexer);
    return AckIncomeCause::SMS_ACK_RESULT_OK;
}

bool GsmSmsReceiveHandler::ReplySmsToSmsc(int result)
{
    TELEPHONY_LOGI("GsmSmsReceiveHandler::ReplySmsToSmsc ackResult %{public}d", result);
#ifdef SMS_MMS_SATELLITE
    auto &satelliteSmsClient = SatelliteSmsClient::GetInstance();
    if (satelliteSmsClient.IsSatelliteEnabled()) {
        TELEPHONY_LOGI("send smsack through satellite");
        satelliteSmsClient.SendSmsAck(
            slotId_, SMS_EVENT_NEW_SMS_REPLY, result == AckIncomeCause::SMS_ACK_RESULT_OK, result);
        return result == AckIncomeCause::SMS_ACK_RESULT_OK;
    }
#endif // SMS_MMS_SATELLITE
    CoreManagerInner::GetInstance().SendSmsAck(
        slotId_, SMS_EVENT_NEW_SMS_REPLY, result == AckIncomeCause::SMS_ACK_RESULT_OK, result, shared_from_this());
    return result == AckIncomeCause::SMS_ACK_RESULT_OK;
}

shared_ptr<SmsBaseMessage> GsmSmsReceiveHandler::TransformMessageInfo(const shared_ptr<SmsMessageInfo> info)
{
    std::shared_ptr<SmsBaseMessage> baseMessage = nullptr;
    if (info == nullptr) {
        TELEPHONY_LOGE("MessageInfo is null.");
        return baseMessage;
    }
    std::string pdu = StringUtils::StringToHex(info->pdu);
    baseMessage = GsmSmsMessage::CreateMessage(pdu);
    return baseMessage;
}
} // namespace Telephony
} // namespace OHOS
