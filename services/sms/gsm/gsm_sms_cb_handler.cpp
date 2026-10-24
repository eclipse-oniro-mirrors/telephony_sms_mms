/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#include "gsm_sms_cb_handler.h"

#include "cb_start_ability.h"
#include "common_event_support.h"
#include "core_manager_inner.h"
#include "radio_event.h"
#include "securec.h"
#include "singleton.h"
#include "sms_hisysevent.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"

namespace OHOS {
namespace Telephony {
using namespace OHOS::EventFwk;
GsmSmsCbHandler::GsmSmsCbHandler(int32_t slotId) : TelEventHandler("GsmSmsCbHandler"), slotId_(slotId)
{
    TELEPHONY_LOGI("GsmSmsCbHandler Create slotId (%{public}d)", slotId_);
}

void GsmSmsCbHandler::Init()
{
    cbMsgList_.clear();
    CoreManagerInner::GetInstance().RegisterCoreNotify(
        slotId_, shared_from_this(), RadioEvent::RADIO_CELL_BROADCAST, nullptr);
    TELEPHONY_LOGI("GsmSmsCbHandler::RegisterHandler::slotId= %{public}d", slotId_);
}

void GsmSmsCbHandler::UnRegisterHandler()
{
    CoreManagerInner::GetInstance().UnRegisterCoreNotify(slotId_, shared_from_this(), RadioEvent::RADIO_CELL_BROADCAST);
}

bool GsmSmsCbHandler::CheckCbActive(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("CbMessage is null!");
        return false;
    }
    return true;
}

uint8_t GsmSmsCbHandler::CheckCbMessage(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    uint8_t currPageCnt = 0;
    bool bFind = false;
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("CheckCbMessage cbMessage nullptr err.");
        return currPageCnt;
    }

    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> cbHeader = cbMessage->GetCbHeader();
    if (cbHeader == nullptr) {
        TELEPHONY_LOGE("CheckCbMessage GetCbHeader err.");
        return currPageCnt;
    }

    for (std::size_t i = 0; i < cbMsgList_.size(); i++) {
        SmsCbInfo &cbInfo = cbMsgList_[i];
        if (*cbInfo.header.get() == *cbHeader.get()) {
            int updateNum = cbHeader->serialNum.updateNum - cbInfo.header->serialNum.updateNum;
            if (updateNum != 0) {
                break;
            }

            if (cbInfo.cbMsgs.count(cbHeader->page)) {
                currPageCnt = 0x01;
                return currPageCnt;
            }
            cbInfo.cbMsgs.insert(std::make_pair(cbHeader->page, cbMessage));
            currPageCnt = cbInfo.cbMsgs.size();
            bFind = true;
            break;
        }
    }

    if (bFind == false || cbHeader->totalPages == 0x01) {
        AddCbMessageToList(cbMessage);
        currPageCnt = 0x01;
    }

    return currPageCnt;
}

std::unique_ptr<SmsCbInfo> GsmSmsCbHandler::FindCbMessage(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    std::unique_ptr<SmsCbInfo> cbInfo = nullptr;
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("FindCbMessage cbMessage nullptr err.");
        return cbInfo;
    }

    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> cbHeader = cbMessage->GetCbHeader();
    if (cbHeader == nullptr) {
        TELEPHONY_LOGE("FindCbMessage header err.");
        return cbInfo;
    }

    for (std::size_t i = 0; i < cbMsgList_.size(); i++) {
        SmsCbInfo &info = cbMsgList_[i];
        if (*info.header.get() == *cbHeader.get()) {
            cbInfo = std::make_unique<SmsCbInfo>(info.header, info.cbMsgs);
            if (cbInfo == nullptr) {
                TELEPHONY_LOGE("Make SmsCbInfo err.");
                return cbInfo;
            }
        }
    }
    return cbInfo;
}

bool GsmSmsCbHandler::AddCbMessageToList(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("AddCbMessageToList cbMessage nullptr err.");
        return false;
    }
    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> cbHeader = cbMessage->GetCbHeader();
    if (cbHeader == nullptr) {
        TELEPHONY_LOGE("AddCbMessageToList header err.");
        return false;
    }
    std::unique_ptr<SmsCbInfo> cbInfo = FindCbMessage(cbMessage);
    if (cbInfo == nullptr) {
        SmsCbInfo info;
        info.header = cbHeader;
        info.cbMsgs.insert(std::make_pair(cbHeader->page, cbMessage));
        InitLocation(info);
        auto size = cbMsgList_.size();
        if (size > MAX_CB_MSG_LIST_SIZE) {
            auto halfSize = size / 2;
            cbMsgList_.erase(cbMsgList_.begin(), cbMsgList_.begin() + halfSize);
            TELEPHONY_LOGI("AddCbMessageToList clear message count: %{public}zu", halfSize);
        }
        cbMsgList_.push_back(info);
        return true;
    }
    return false;
}

void GsmSmsCbHandler::ClearExpiredMessage()
{
    if (cbMsgList_.empty()) {
        return;
    }
    std::time_t timep;
    int64_t currentTime = time(&timep);
    for (auto i = cbMsgList_.size() - 1; i > 0; i--) {
        if (currentTime - cbMsgList_[i].header->recvTime > static_cast<long long>(DEFAULT_EXPIRED_TIME)) {
            cbMsgList_.erase(cbMsgList_.begin(), cbMsgList_.begin() + i + 1);
            TELEPHONY_LOGI("ClearExpiredMessage clear message count: %{public}zu", i);
            return;
        }
    }
}

bool GsmSmsCbHandler::InitLocation(SmsCbInfo &info)
{
    const uint8_t cellWideImmediate = 0;
    const uint8_t plmnWide = 1;
    const uint8_t LaWide = 2;
    const uint8_t cellWide = 3;
    const int32_t defaultValue = -1;
    info.plmn_ = CoreManagerInner::GetInstance().GetOperatorNumeric(slotId_);
    sptr<CellLocation> location = CoreManagerInner::GetInstance().GetCellLocation(slotId_);
    if (location == nullptr) {
        TELEPHONY_LOGE("location is nullptr.");
        return false;
    }
    if (location->GetCellLocationType() != CellLocation::CellType::CELL_TYPE_GSM) {
        TELEPHONY_LOGE("location type isn't GSM.");
        return false;
    }
    sptr<GsmCellLocation> gsmLocation = sptr<GsmCellLocation>(static_cast<GsmCellLocation *>(location.GetRefPtr()));
    info.lac_ = gsmLocation->GetLac();
    info.cid_ = gsmLocation->GetCellId();
    switch (info.header->serialNum.geoScope) {
        case LaWide:
            info.cid_ = defaultValue;
            break;
        case cellWide:
        case cellWideImmediate:
            break;
        case plmnWide:
        default:
            info.cid_ = defaultValue;
            info.lac_ = defaultValue;
            break;
    }
    plmn_ = info.plmn_;
    cid_ = info.cid_;
    lac_ = info.lac_;
    return true;
}

bool GsmSmsCbHandler::RemoveCbMessageFromList(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    bool result = false;
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("RemoveCbMessageFromList cbMessage nullptr err.");
        return false;
    }

    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> header = cbMessage->GetCbHeader();
    if (header == nullptr) {
        TELEPHONY_LOGE("RemoveCbMessageFromList header err.");
        return false;
    }
    auto it = cbMsgList_.begin();
    while (it != cbMsgList_.end()) {
        SmsCbInfo &info = *it;
        if (*info.header.get() == *header.get() || !info.MatchLocation(plmn_, lac_, cid_)) {
            it = cbMsgList_.erase(it);
            result = true;
        } else {
            ++it;
        }
    }
    return result;
}

void GsmSmsCbHandler::HandleCbMessage(std::shared_ptr<CBConfigReportInfo> &message)
{
    if (message == nullptr) {
        TELEPHONY_LOGE("GsmSmsCbHandler HandleCbMessage message == nullptr");
        return;
    }

    std::string pdu(message->pdu);
    std::shared_ptr<GsmCbCodec> cbMessage = GsmCbCodec::CreateCbMessage(pdu);
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("create Sms CbMessage fail");
        SmsHiSysEvent::WriteSmsReceiveFaultEvent(slotId_, SmsMmsMessageType::CELL_BROAD_CAST,
            SmsMmsErrorCode::SMS_ERROR_CELL_BROADCAST_PUD_ANALYSIS_FAIL, "publish cell broadcast event fail");
        return;
    }
    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> header = cbMessage->GetCbHeader();
    if (header == nullptr) {
        TELEPHONY_LOGE("HandleCbMessage header is null.");
        return;
    }

    if (!CheckCbActive(cbMessage)) {
        TELEPHONY_LOGE("The Cell Broadcast msg is not active");
        return;
    }

    ClearExpiredMessage();
    uint8_t pageCnt = CheckCbMessage(cbMessage);
    if (header->totalPages == pageCnt) {
        SendCbMessageBroadcast(cbMessage);
        RemoveCbMessageFromList(cbMessage);
    }
}

bool GsmSmsCbHandler::SendCbMessageBroadcast(const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("SendCbMessageBroadcast cbMessage nullptr err.");
        return false;
    }
    EventFwk::Want want;
    EventFwk::CommonEventData data;
    if (!SetWantData(want, cbMessage)) {
        TELEPHONY_LOGE("SendCbMessageBroadcast SetWantData fail.");
        return false;
    }
    DelayedSingleton<CbStartAbility>::GetInstance()->StartAbility(want);
    data.SetWant(want);
    EventFwk::CommonEventPublishInfo publishInfo;
    publishInfo.SetOrdered(true);
    std::vector<std::string> gsmCbPermissions;
    gsmCbPermissions.emplace_back(Permission::RECEIVE_MESSAGES);
    publishInfo.SetSubscriberPermissions(gsmCbPermissions);
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

void GsmSmsCbHandler::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("GsmSmsCbHandler event nullptr error.");
        return;
    }

    uint32_t eventId = 0;
    eventId = event->GetInnerEventId();
    TELEPHONY_LOGI("ProcessEvent eventId = %{public}d", eventId);
    switch (eventId) {
        case RadioEvent::RADIO_CELL_BROADCAST: {
            std::shared_ptr<CBConfigReportInfo> cbMessage = nullptr;
            cbMessage = event->GetSharedObject<CBConfigReportInfo>();
            HandleCbMessage(cbMessage);
            break;
        }
        default:
            TELEPHONY_LOGE("GsmSmsCbHandler eventId unknown error.");
            break;
    }
}

bool GsmSmsCbHandler::SetWantData(EventFwk::Want &want, const std::shared_ptr<GsmCbCodec> &cbMessage)
{
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("cbMessage is nullptr.");
        return false;
    }
    SmsCbData::CbData sendData;
    std::unique_ptr<SmsCbInfo> info = FindCbMessage(cbMessage);
    if (info == nullptr) {
        TELEPHONY_LOGE("find cb message fail.");
        return false;
    }
    std::string rawMsgBody;
    TELEPHONY_LOGI("cbMsgs size:%{public}zu.", info->cbMsgs.size());
    for (auto it = info->cbMsgs.begin(); it != info->cbMsgs.end(); it++) {
        rawMsgBody.append(it->second->GetCbMessageRaw());
    }
    GetCbData(cbMessage, sendData);
    cbMessage->ConvertToUTF8(rawMsgBody, sendData.msgBody);
    PackageWantData(sendData, want);
    if (sendData.isPrimary && !StringUtils::IsEmpty(sendData.msgBody)) {
        TELEPHONY_LOGI("secondary cb msg");
        sendData.isPrimary = false;
        want.SetParam(SmsCbData::IS_ETWS_PRIMARY, sendData.isPrimary);
    }
    if (sendData.isPrimary) {
        want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_EMERGENCY_CB_RECEIVE_COMPLETED);
    } else {
        want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_CB_RECEIVE_COMPLETED);
    }
    return true;
}

void GsmSmsCbHandler::GetCbData(const std::shared_ptr<GsmCbCodec> &cbMessage, SmsCbData::CbData &sendData)
{
    if (cbMessage == nullptr) {
        TELEPHONY_LOGE("Get Cb Data error.");
        return;
    }
    cbMessage->GetMsgType(sendData.msgType);
    cbMessage->GetLangType(sendData.langType);
    cbMessage->GetLanguageCode(sendData.languageCode);
    uint8_t codingGroup = 0;
    if (cbMessage->GetCodingGroup(codingGroup)) {
        if (codingGroup == 0x00 || codingGroup == 0x02 || codingGroup == 0x03) {
            sendData.langType &= 0x0F;
        } else {
            sendData.langType = GsmCbCodec::CB_MSG_UNSPECIFIED;
        }
    }
    cbMessage->GetDcs(sendData.dcs);
    cbMessage->GetPriority(sendData.priority);
    cbMessage->GetCmasMessageClass(sendData.cmasClass);
    cbMessage->GetCmasResponseType(sendData.cmasRes);
    cbMessage->GetCmasSeverity(sendData.severity);
    cbMessage->GetCmasUrgency(sendData.urgency);
    cbMessage->GetCmasCertainty(sendData.certainty);
    cbMessage->IsEtwsMessage(sendData.isEtws);
    cbMessage->GetWarningType(sendData.warnType);
    cbMessage->IsCmasMessage(sendData.isCmas);
    cbMessage->GetSerialNum(sendData.serial);
    cbMessage->GetReceiveTime(sendData.recvTime);
    cbMessage->GetMessageId(sendData.msgId);
    cbMessage->GetServiceCategory(sendData.category);
    cbMessage->GetFormat(sendData.format);
    cbMessage->IsEtwsPrimary(sendData.isPrimary);
    cbMessage->GetGeoScope(sendData.geoScope);
    cbMessage->GetCmasCategory(sendData.cmasCate);
}

void GsmSmsCbHandler::PackageWantData(SmsCbData::CbData &sendData, EventFwk::Want &want)
{
    want.SetParam(SmsCbData::GEO_SCOPE, static_cast<char>(sendData.geoScope));
    want.SetParam(SmsCbData::CMAS_RESPONSE, static_cast<char>(sendData.cmasRes));
    want.SetParam(SmsCbData::SLOT_ID, static_cast<int>(slotId_));
    want.SetParam(SmsCbData::FORMAT, static_cast<char>(sendData.format));
    want.SetParam(SmsCbData::CB_MSG_TYPE, static_cast<char>(sendData.msgType));
    want.SetParam(SmsCbData::MSG_ID, static_cast<int>(sendData.msgId));
    want.SetParam(SmsCbData::SERVICE_CATEGORY, static_cast<int>(sendData.category));
    want.SetParam(SmsCbData::LANG_TYPE, static_cast<char>(sendData.langType));
    want.SetParam(SmsCbData::LANGUAGE_CODE, sendData.languageCode);
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
    want.SetParam(SmsCbData::LAC, static_cast<int>(lac_));
    want.SetParam(SmsCbData::CID, static_cast<int>(cid_));
    want.SetParam(SmsCbData::WARNING_TYPE, static_cast<int>(sendData.warnType));
}
} // namespace Telephony
} // namespace OHOS