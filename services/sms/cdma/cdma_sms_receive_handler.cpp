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

#include "cdma_sms_receive_handler.h"

#include "cdma_sms_common.h"
#include "cdma_sms_message.h"
#include "cdma_sms_sender.h"
#include "common_event.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "core_manager_inner.h"
#include "radio_event.h"
#include "singleton.h"
#include "sms_common.h"
#include "sms_hisysevent.h"
#include "sms_persist_helper.h"
#include "sms_receive_reliability_handler.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"
#include "want.h"

namespace OHOS {
namespace Telephony {
using namespace OHOS::EventFwk;
CdmaSmsReceiveHandler::CdmaSmsReceiveHandler(int32_t slotId) : SmsReceiveHandler(slotId)
{
    TELEPHONY_LOGI("%{public}d", slotId_);
}

int32_t CdmaSmsReceiveHandler::HandleSmsByType(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("SmsBaseMessage is null!");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    CdmaSmsMessage *message = static_cast<CdmaSmsMessage *>(smsBaseMessage.get());
    if (message->IsBroadcastMsg()) {
        SendCBBroadcast(smsBaseMessage);
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    int service = message->GetTransTeleService();
    if (static_cast<int>(SmsTransTelsvcId::WEMT) != service && static_cast<int>(SmsTransTelsvcId::CMT_95) != service) {
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    if (message->IsStatusReport() && !cdmaSmsSender_.expired()) {
        std::shared_ptr<SmsSender> smsSender = cdmaSmsSender_.lock();
        CdmaSmsSender *cdmaSend = static_cast<CdmaSmsSender *>(smsSender.get());
        std::shared_ptr<SmsReceiveIndexer> statusInfo = std::make_shared<SmsReceiveIndexer>();
        if (statusInfo == nullptr) {
            TELEPHONY_LOGE("statusInfo is null!");
            return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
        }
        statusInfo->SetMsgRefId(message->GetMsgRef());
        statusInfo->SetPdu(message->GetRawPdu());
        cdmaSend->ReceiveStatusReport(statusInfo);
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    return HandleSmsOtherSvcid(smsBaseMessage);
}

int32_t CdmaSmsReceiveHandler::HandleAck(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("SmsBaseMessage is null!");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    CdmaSmsMessage *message = static_cast<CdmaSmsMessage *>(smsBaseMessage.get());
    if (message->IsBroadcastMsg()) {
        SendCBBroadcast(smsBaseMessage);
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    int service = message->GetTransTeleService();
    if (static_cast<int>(SmsTransTelsvcId::WEMT) != service && static_cast<int>(SmsTransTelsvcId::CMT_95) != service) {
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    if (message->IsStatusReport() && !cdmaSmsSender_.expired()) {
        std::shared_ptr<SmsSender> smsSender = cdmaSmsSender_.lock();
        CdmaSmsSender *cdmaSend = static_cast<CdmaSmsSender *>(smsSender.get());
        std::shared_ptr<SmsReceiveIndexer> statusInfo = std::make_shared<SmsReceiveIndexer>();
        if (statusInfo == nullptr) {
            TELEPHONY_LOGE("statusInfo is null!");
            return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
        }
        statusInfo->SetMsgRefId(message->GetMsgRef());
        statusInfo->SetPdu(message->GetRawPdu());
        cdmaSend->ReceiveStatusReport(statusInfo);
        return AckIncomeCause::SMS_ACK_RESULT_OK;
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

void CdmaSmsReceiveHandler::HandleRemainDataShare(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("SmsBaseMessage is null!");
        return;
    }
    CdmaSmsMessage *message = static_cast<CdmaSmsMessage *>(smsBaseMessage.get());
    std::shared_ptr<SmsReceiveIndexer> indexer;
    std::shared_ptr<SmsConcat> smsConcat = GetValidConcatMsg(smsBaseMessage);
    if (smsConcat == nullptr) {
        indexer = std::make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), true, false, message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), message->GetVisibleMessageBody());
    } else {
        indexer = std::make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), true, message->GetOriginatingAddress(), message->GetVisibleOriginatingAddress(),
            smsConcat->msgRef, smsConcat->seqNum, smsConcat->totalSeg, false, message->GetVisibleMessageBody());
    }
    if (indexer == nullptr) {
        TELEPHONY_LOGE("indexer is nullptr.");
        return;
    }
    HILOG_COMM_INFO("received a cdma sms, the refid is %{public}d, this is %{public}d, a total of %{public}d",
        indexer->GetMsgRefId(), indexer->GetMsgSeqId(), indexer->GetMsgCount());
    if (indexer->GetIsText() && (smsConcat != nullptr) && IsRepeatedMessagePart(indexer)) {
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_REPEATED_ERROR, "cdma message repeated error");
        return;
    }
    if (!AddMsgToDB(indexer)) {
        TELEPHONY_LOGE("AddMsgToDB failed");
        return;
    }
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if (reliabilityHandler == nullptr) {
        TELEPHONY_LOGE("reliabilityHandler nullptr");
        return;
    }
    if (!reliabilityHandler->DeleteExpireSmsFromDB()) {
        TELEPHONY_LOGE("DeleteExpireSmsFromDB fail");
        return;
    }
    CombineMessagePart(indexer);
}

int32_t CdmaSmsReceiveHandler::HandleSmsOtherSvcid(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("SmsBaseMessage is null!");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    CdmaSmsMessage *message = static_cast<CdmaSmsMessage *>(smsBaseMessage.get());
    // Encapsulate key information to Tracker
    std::shared_ptr<SmsReceiveIndexer> indexer;
    std::shared_ptr<SmsConcat> smsConcat = GetValidConcatMsg(smsBaseMessage);
    if (smsConcat == nullptr) {
        indexer = std::make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), true, false, message->GetOriginatingAddress(),
            message->GetVisibleOriginatingAddress(), message->GetVisibleMessageBody());
    } else {
        indexer = std::make_shared<SmsReceiveIndexer>(message->GetRawPdu(), message->GetScTimestamp(),
            message->GetDestPort(), true, message->GetOriginatingAddress(), message->GetVisibleOriginatingAddress(),
            smsConcat->msgRef, smsConcat->seqNum, smsConcat->totalSeg, false, message->GetVisibleMessageBody());
    }
    // add messages to the database
    if (indexer == nullptr) {
        TELEPHONY_LOGE("indexer is nullptr.");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    HILOG_COMM_INFO("received a cdma sms, the refid is %{public}d, this is %{public}d, a total of %{public}d",
        indexer->GetMsgRefId(), indexer->GetMsgSeqId(), indexer->GetMsgCount());
    if (indexer->GetIsText() && (smsConcat != nullptr) && IsRepeatedMessagePart(indexer)) {
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_REPEATED_ERROR, "cdma message repeated error");
        return AckIncomeCause::SMS_ACK_RESULT_OK;
    }
    if (!AddMsgToDB(indexer)) {
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId_);
    if (reliabilityHandler == nullptr) {
        TELEPHONY_LOGE("reliabilityHandler nullptr");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    if (!reliabilityHandler->DeleteExpireSmsFromDB()) {
        TELEPHONY_LOGE("DeleteExpireSmsFromDB fail");
        return AckIncomeCause::SMS_ACK_UNKNOWN_ERROR;
    }
    CombineMessagePart(indexer);
    return AckIncomeCause::SMS_ACK_RESULT_OK;
}

bool CdmaSmsReceiveHandler::ReplySmsToSmsc(int result)
{
    TELEPHONY_LOGI("Reply To Smsc ackResult %{public}d", result);
    CoreManagerInner::GetInstance().SendSmsAck(
        slotId_, SMS_EVENT_NEW_SMS_REPLY, result == AckIncomeCause::SMS_ACK_RESULT_OK, result, shared_from_this());
    return result == AckIncomeCause::SMS_ACK_RESULT_OK;
}

void CdmaSmsReceiveHandler::SetCdmaSender(const std::weak_ptr<SmsSender> &smsSender)
{
    cdmaSmsSender_ = smsSender;
}

std::shared_ptr<SmsBaseMessage> CdmaSmsReceiveHandler::TransformMessageInfo(const std::shared_ptr<SmsMessageInfo> info)
{
    std::shared_ptr<SmsBaseMessage> baseMessage = nullptr;
    if (info == nullptr) {
        TELEPHONY_LOGE("SmsBaseMessage is null!");
        return baseMessage;
    }
    std::string pdu = StringUtils::StringToHex(info->pdu);
    baseMessage = CdmaSmsMessage::CreateMessage(pdu);
    return baseMessage;
}

void CdmaSmsReceiveHandler::Init()
{
    if (!RegisterHandler()) {
        TELEPHONY_LOGI("GsmSmsSender::Init Register RADIO_SMS_STATUS fail.");
    }
}

bool CdmaSmsReceiveHandler::RegisterHandler()
{
    TELEPHONY_LOGI("CdmaSmsReceiveHandler::RegisteHandler Register RADIO_CDMA_SMS ok.");
    CoreManagerInner::GetInstance().RegisterCoreNotify(
        slotId_, shared_from_this(), RadioEvent::RADIO_CDMA_SMS, nullptr);
    return true;
}

void CdmaSmsReceiveHandler::UnRegisterHandler()
{
    TELEPHONY_LOGI("CdmaSmsReceiveHandler::UnRegisterHandler::slotId= %{public}d", slotId_);
    CoreManagerInner::GetInstance().UnRegisterCoreNotify(slotId_, shared_from_this(), RadioEvent::RADIO_CDMA_SMS);
}

bool CdmaSmsReceiveHandler::SendCBBroadcast(const std::shared_ptr<SmsBaseMessage> smsBaseMessage)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("smsBaseMessage is nullptr.");
        return false;
    }

    bool isMergency = false;
    SmsCbData::CbData sendData;
    GetCBData(smsBaseMessage, sendData, isMergency);
    EventFwk::Want want;
    EventFwk::CommonEventData data;
    if (isMergency) {
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_EMERGENCY_CB_RECEIVE_COMPLETED);
    } else {
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_CB_RECEIVE_COMPLETED);
    }
    SetCBBroadcastParam(want, sendData);
    data.SetWant(want);
    EventFwk::CommonEventPublishInfo publishInfo;
    publishInfo.SetOrdered(true);
    std::vector<std::string> cdmaCbPermissions;
    cdmaCbPermissions.emplace_back(Permission::RECEIVE_MESSAGES);
    publishInfo.SetSubscriberPermissions(cdmaCbPermissions);
    bool publishResult = EventFwk::CommonEventManager::PublishCommonEvent(data, publishInfo, nullptr);
    if (!publishResult) {
        TELEPHONY_LOGE("SendBroadcast PublishBroadcastEvent result fail");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::CELL_BROAD_CAST,
            SmsMmsErrorCode::SMS_ERROR_PUBLISH_COMMON_EVENT_FAIL, "publish cell broadcast event fail");
        return false;
    }
    DelayedSingleton<SmsHiSysEvent>::GetInstance()->SetCbBroadcastStartTime();
    return true;
}

bool CdmaSmsReceiveHandler::SetCBBroadcastParam(AppExecFwk::Want &want, SmsCbData::CbData &sendData)
{
    const int lac = -1;
    const int cid = -1;
    want.SetParam(SmsCbData::GEO_SCOPE, static_cast<char>(sendData.geoScope));
    want.SetParam(SmsCbData::CMAS_RESPONSE, static_cast<char>(sendData.cmasRes));
    want.SetParam(SmsCbData::SLOT_ID, static_cast<int>(slotId_));
    want.SetParam(SmsCbData::FORMAT, static_cast<char>(sendData.format));
    want.SetParam(SmsCbData::CB_MSG_TYPE, static_cast<char>(sendData.msgType));
    want.SetParam(SmsCbData::MSG_ID, static_cast<int>(sendData.msgId));
    want.SetParam(SmsCbData::SERVICE_CATEGORY, static_cast<int>(sendData.category));
    want.SetParam(SmsCbData::LANG_TYPE, static_cast<char>(sendData.langType));
    want.SetParam(SmsCbData::PRIORITY, static_cast<char>(sendData.priority));
    want.SetParam(SmsCbData::MSG_BODY, sendData.msgBody);
    want.SetParam(SmsCbData::CMAS_CLASS, static_cast<char>(sendData.cmasClass));
    want.SetParam(SmsCbData::CMAS_CATEGORY, static_cast<char>(sendData.cmasCate));
    want.SetParam(SmsCbData::SEVERITY, static_cast<char>(sendData.severity));
    want.SetParam(SmsCbData::URGENCY, static_cast<char>(sendData.urgency));
    want.SetParam(SmsCbData::CERTAINTY, static_cast<char>(sendData.certainty));
    want.SetParam(SmsCbData::IS_CMAS_MESSAGE, sendData.isCmas);
    want.SetParam(SmsCbData::SERIAL_NUM, static_cast<int>(sendData.serial));
    want.SetParam(SmsCbData::RECV_TIME, std::to_string(sendData.recvTime));
    want.SetParam(SmsCbData::DCS, static_cast<char>(sendData.dcs));
    want.SetParam(SmsCbData::IS_ETWS_PRIMARY, sendData.isPrimary);
    want.SetParam(SmsCbData::IS_ETWS_MESSAGE, sendData.isEtws);
    want.SetParam(SmsCbData::PLMN, StringUtils::ToUtf8(plmn_));
    want.SetParam(SmsCbData::LAC, static_cast<int>(lac));
    want.SetParam(SmsCbData::CID, static_cast<int>(cid));
    want.SetParam(SmsCbData::WARNING_TYPE, static_cast<int>(sendData.warnType));
    return true;
}

void CdmaSmsReceiveHandler::GetCBData(
    const std::shared_ptr<SmsBaseMessage> smsBaseMessage, SmsCbData::CbData &sendData, bool &isEmergency)
{
    if (smsBaseMessage == nullptr) {
        TELEPHONY_LOGE("smsBaseMessage is nullptr.");
        return;
    }
    CdmaSmsMessage *message = static_cast<CdmaSmsMessage *>(smsBaseMessage.get());
    if (message == nullptr) {
        TELEPHONY_LOGE("message is nullptr.");
        return;
    }

    sendData.format = message->GetFormat();
    sendData.geoScope = message->GetGeoScope();
    sendData.msgId = message->GetMessageId();
    sendData.serial = message->GetMessageId();
    sendData.category = message->GetServiceCategoty();
    sendData.langType = static_cast<uint8_t>(message->GetLanguage());
    sendData.msgBody = message->GetVisibleMessageBody();
    sendData.priority = message->GetPriority();
    sendData.isCmas = message->IsCMAS();
    sendData.cmasClass = message->GetCMASMessageClass();
    sendData.cmasCate = message->GetCMASCategory();
    sendData.severity = message->GetCMASSeverity();
    sendData.urgency = message->GetCMASUrgency();
    sendData.certainty = message->GetCMASCertainty();
    sendData.recvTime = message->GetReceTime();
    sendData.langType = static_cast<uint8_t>(message->GetLanguage());
    isEmergency = message->IsEmergencyMsg();
    plmn_ = CoreManagerInner::GetInstance().GetOperatorNumeric(slotId_);
}
} // namespace Telephony
} // namespace OHOS
