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

#include "gsm_sms_sender.h"

#include "securec.h"

#include "core_manager_inner.h"
#include "radio_event.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
GsmSmsSender::GsmSmsSender(const std::shared_ptr<AppExecFwk::EventRunner> &runner, int32_t slotId,
    function<void(std::shared_ptr<SmsSendIndexer>)> sendRetryFun)
    : SmsSender(runner, slotId, sendRetryFun)
{}

GsmSmsSender::~GsmSmsSender() {}

void GsmSmsSender::Init()
{
    if (!RegisterHandler()) {
        TELEPHONY_LOGI("GsmSmsSender::Init Register RADIO_SMS_STATUS fail.");
    }
}

void GsmSmsSender::TextBasedSmsDelivery(const string &desAddr, const string &scAddr, const string &text,
    const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    bool isMore = false;
    bool isStatusReport = false;
    int ret = 0;
    int headerCnt;
    int cellsInfosSize;
    unsigned char msgRef8bit;
    SmsCodingScheme codingType;
    GsmSmsMessage gsmSmsMessage;
    std::vector<struct SplitInfo> cellsInfos;
    gsmSmsMessage.SplitMessage(cellsInfos, text, CheckForce7BitEncodeType(), codingType);
    isStatusReport = (deliveryCallback == nullptr) ? false : true;
    std::shared_ptr<struct SmsTpdu> tpdu =
        gsmSmsMessage.CreateDefaultSubmitSmsTpdu(desAddr, scAddr, text, isStatusReport, codingType);
    if (tpdu == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("TextBasedSmsDelivery tpdu nullptr error.");
        return;
    }
    cellsInfosSize = cellsInfos.size();
    if (cellsInfosSize > MAX_SEGMENT_NUM) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("message exceed the limit.");
        return;
    }
    msgRef8bit = GetMsgRef8Bit();
    isStatusReport = tpdu->data.submit.bStatusReport;

    TELEPHONY_LOGI("TextBasedSmsDelivery isStatusReport= %{public}d", isStatusReport);
    shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(cellsInfosSize);
    shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    if (unSentCellCount == nullptr || hasCellFailed == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < cellsInfosSize; i++) {
        std::shared_ptr<SmsSendIndexer> indexer = nullptr;
        std::string segmentText;
        segmentText.append((char *)(cellsInfos[i].encodeData.data()), cellsInfos[i].encodeData.size());
        indexer = make_shared<SmsSendIndexer>(desAddr, scAddr, segmentText, sendCallback, deliveryCallback);
        if (indexer == nullptr) {
            SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            return;
        }
        indexer->SetDcs(cellsInfos[i].encodeType);
        headerCnt = 0;
        (void)memset_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, 0x00, MAX_USER_DATA_LEN + 1);
        ret = memcpy_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, &cellsInfos[i].encodeData[0],
            cellsInfos[i].encodeData.size());
        if (ret != EOK || indexer == nullptr) {
            SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            return;
        }
        tpdu->data.submit.userData.length = cellsInfos[i].encodeData.size();
        tpdu->data.submit.userData.data[cellsInfos[i].encodeData.size()] = 0;
        tpdu->data.submit.msgRef = msgRef8bit;
        if (cellsInfosSize > 1) {
            indexer->SetIsConcat(true);
            SmsConcat concat;
            concat.is8Bits = true;
            concat.msgRef = msgRef8bit;
            concat.totalSeg = cellsInfosSize;
            concat.seqNum = i + 1;
            indexer->SetSmsConcat(concat);
            headerCnt += gsmSmsMessage.SetHeaderConcat(headerCnt, concat);
        }
        /* Set User Data Header for Alternate Reply Address */
        headerCnt += gsmSmsMessage.SetHeaderReply(headerCnt);
        /* Set User Data Header for National Language Single Shift */
        headerCnt += gsmSmsMessage.SetHeaderLang(headerCnt, codingType, cellsInfos[i].langId);
        indexer->SetLangId(cellsInfos[i].langId);
        tpdu->data.submit.userData.headerCnt = headerCnt;
        tpdu->data.submit.bHeaderInd = (headerCnt > 0) ? true : false;

        if (cellsInfosSize > 1 && i < (cellsInfosSize - 1)) {
            tpdu->data.submit.bStatusReport = false;
            isMore = true;
        } else {
            tpdu->data.submit.bStatusReport = isStatusReport;
            isMore = false;
        }
        std::shared_ptr<struct EncodeInfo> encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(scAddr, isMore);
        if (encodeInfo == nullptr) {
            SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            TELEPHONY_LOGE("create encodeInfo encodeInfo nullptr error.");
            continue;
        }
        SetSendIndexerInfo(indexer, encodeInfo, msgRef8bit);
        indexer->SetUnSentCellCount(unSentCellCount);
        indexer->SetHasCellFailed(hasCellFailed);
        SendSmsToRil(indexer);
    }
}

void GsmSmsSender::DataBasedSmsDelivery(const std::string &desAddr, const std::string &scAddr, int32_t port,
    const uint8_t *data, uint32_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    uint8_t msgRef8bit = GetMsgRef8Bit();
    GsmSmsMessage gsmSmsMessage;
    std::shared_ptr<struct SmsTpdu> tpdu = gsmSmsMessage.CreateDataSubmitSmsTpdu(
        desAddr, scAddr, port, data, dataLen, msgRef8bit, (deliveryCallback == nullptr) ? false : true);
    std::shared_ptr<struct EncodeInfo> encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(scAddr, false);
    if (encodeInfo == nullptr || tpdu == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("DataBasedSmsDelivery encodeInfo or tpdu nullptr error.");
        return;
    }
    shared_ptr<SmsSendIndexer> indexer = nullptr;
    indexer = make_shared<SmsSendIndexer>(desAddr, scAddr, port, data, dataLen, sendCallback, deliveryCallback);
    if (indexer == nullptr) {
        SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("DataBasedSmsDelivery create SmsSendIndexer nullptr");
        return;
    }

    std::vector<uint8_t> smca(encodeInfo->smcaData_, encodeInfo->smcaData_ + encodeInfo->smcaLen);
    std::vector<uint8_t> pdu(encodeInfo->tpduData_, encodeInfo->tpduData_ + encodeInfo->tpduLen);
    std::shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(1);
    std::shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    if (unSentCellCount == nullptr || hasCellFailed == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    indexer->SetUnSentCellCount(unSentCellCount);
    indexer->SetHasCellFailed(hasCellFailed);
    indexer->SetEncodeSmca(std::move(smca));
    indexer->SetEncodePdu(std::move(pdu));
    indexer->SetHasMore(encodeInfo->isMore_);
    indexer->SetMsgRefId(msgRef8bit);
    indexer->SetNetWorkType(NET_TYPE_GSM);
    indexer->SetTimeStamp(timeStamp);
    std::unique_lock<std::mutex> lock(mutex_);
    SendSmsToRil(indexer);
}

void GsmSmsSender::SendSmsToRil(const shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("gsm_sms_sender: SendSms smsIndexer nullptr");
        return;
    }
    if (!isImsNetDomain_ && (voiceServiceState_ != static_cast<int32_t>(RegServiceState::REG_STATE_IN_SERVICE))) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_SERVICE_UNAVAILABLE);
        TELEPHONY_LOGE("gsm_sms_sender: SendSms not in service");
        return;
    }
    int64_t refId = GetMsgRef64Bit();
    if (!SendCacheMapAddItem(refId, smsIndexer)) {
        TELEPHONY_LOGE("SendCacheMapAddItem Error!!");
        return;
    }

    if (!isImsNetDomain_ && smsIndexer->GetPsResendCount() == 0) {
        uint8_t tryCount = smsIndexer->GetCsResendCount();
        if (tryCount > 0) {
            smsIndexer->UpdatePduForResend();
        }
        GsmSimMessageParam smsData {
            .refId = refId,
            .smscPdu = StringUtils::StringToHex(smsIndexer->GetEncodeSmca()),
            .pdu = StringUtils::StringToHex(smsIndexer->GetEncodePdu())
        };
        if (tryCount == 0 && smsIndexer->GetHasMore()) {
            TELEPHONY_LOGI("SendSmsMoreMode pdu len = %{public}zu", smsIndexer->GetEncodePdu().size());
            CoreManagerInner::GetInstance().SendSmsMoreMode(slotId_,
                RadioEvent::RADIO_SEND_SMS_EXPECT_MORE, smsData, shared_from_this());
        } else {
            TELEPHONY_LOGI("SendSms pdu len = %{public}zu", smsIndexer->GetEncodePdu().size());
            CoreManagerInner::GetInstance().SendGsmSms(slotId_,
                RadioEvent::RADIO_SEND_SMS, smsData, shared_from_this());
        }
    } else {
        TELEPHONY_LOGI("ims network domain not support!");
        smsIndexer->SetPsResendCount(smsIndexer->GetPsResendCount() + 1);
    }
}

void GsmSmsSender::StatusReportAnalysis(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("gsm_sms_sender: StatusReportAnalysis event nullptr error.");
        return;
    }

    std::shared_ptr<SmsMessageInfo> statusInfo = event->GetSharedObject<SmsMessageInfo>();
    if (statusInfo == nullptr) {
        TELEPHONY_LOGE("gsm_sms_sender: StatusReportAnalysis statusInfo nullptr error.");
        return;
    }

    std::string pdu = StringUtils::StringToHex(statusInfo->pdu);
    TELEPHONY_LOGI("StatusReport pdu size == %{public}zu", pdu.length());
    std::shared_ptr<GsmSmsMessage> message = GsmSmsMessage::CreateMessage(pdu);
    if (message == nullptr) {
        TELEPHONY_LOGE("gsm_sms_sender: StatusReportAnalysis message nullptr error.");
        return;
    }
    sptr<IDeliveryShortMessageCallback> deliveryCallback = nullptr;
    auto oldIndexer = reportList_.begin();
    while (oldIndexer != reportList_.end()) {
        auto iter = oldIndexer++;
        TELEPHONY_LOGI("StatusReport %{public}d %{public}d", message->GetMsgRef(), (*iter)->GetMsgRefId());
        if (*iter != nullptr && (message->GetMsgRef() == (*iter)->GetMsgRefId())) {
            // save the message to db, or updata to db msg state(success or failed)
            deliveryCallback = (*iter)->GetDeliveryCallback();
            reportList_.erase(iter);
        }
    }
    CoreManagerInner::GetInstance().SendSmsAck(slotId_, 0, AckIncomeCause::SMS_ACK_PROCESSED, true, nullptr);
    if (deliveryCallback != nullptr) {
        std::string ackpdu = StringUtils::StringToHex(message->GetRawPdu());
        deliveryCallback->OnSmsDeliveryResult(StringUtils::ToUtf16(ackpdu));
        TELEPHONY_LOGI("gsm_sms_sender: StatusReportAnalysis %{public}s", pdu.c_str());
    }
}

void GsmSmsSender::SetSendIndexerInfo(const std::shared_ptr<SmsSendIndexer> &indexer,
    const std::shared_ptr<struct EncodeInfo> &encodeInfo, unsigned char msgRef8bit)
{
    if (encodeInfo == nullptr || indexer == nullptr) {
        TELEPHONY_LOGE("CreateSendIndexer encodeInfo nullptr");
        return;
    }

    std::vector<uint8_t> smca(encodeInfo->smcaData_, encodeInfo->smcaData_ + encodeInfo->smcaLen);
    std::vector<uint8_t> pdu(encodeInfo->tpduData_, encodeInfo->tpduData_ + encodeInfo->tpduLen);
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    indexer->SetTimeStamp(timeStamp);
    indexer->SetEncodeSmca(std::move(smca));
    indexer->SetEncodePdu(std::move(pdu));
    indexer->SetHasMore(encodeInfo->isMore_);
    indexer->SetMsgRefId(msgRef8bit);
    indexer->SetNetWorkType(NET_TYPE_GSM);
}

bool GsmSmsSender::RegisterHandler()
{
    CoreManagerInner::GetInstance().RegisterCoreNotify(
        slotId_, shared_from_this(), RadioEvent::RADIO_SMS_STATUS, nullptr);
    return true;
}

void GsmSmsSender::ResendTextDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("ResendTextDelivery smsIndexer == nullptr");
        return;
    }
    GsmSmsMessage gsmSmsMessage;
    bool isMore = false;
    if (!SetPduInfo(smsIndexer, gsmSmsMessage, isMore)) {
        TELEPHONY_LOGE("SetPduInfo fail.");
        return;
    }

    std::shared_ptr<struct EncodeInfo> encodeInfo = nullptr;
    encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(smsIndexer->GetSmcaAddr(), isMore);
    if (encodeInfo == nullptr) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("create encodeInfo indexer == nullptr\r\n");
        return;
    }
    SetSendIndexerInfo(smsIndexer, encodeInfo, smsIndexer->GetMsgRefId());
    std::unique_lock<std::mutex> lock(mutex_);
    SendSmsToRil(smsIndexer);
}

void GsmSmsSender::ResendDataDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("DataBasedSmsDelivery ResendDataDelivery smsIndexer nullptr");
        return;
    }

    bool isStatusReport = false;
    unsigned char msgRef8bit = 0;
    msgRef8bit = smsIndexer->GetMsgRefId();
    isStatusReport = (smsIndexer->GetDeliveryCallback() == nullptr) ? false : true;

    GsmSmsMessage gsmSmsMessage;
    std::shared_ptr<struct SmsTpdu> tpdu = nullptr;
    tpdu = gsmSmsMessage.CreateDataSubmitSmsTpdu(smsIndexer->GetDestAddr(), smsIndexer->GetSmcaAddr(),
        smsIndexer->GetDestPort(), smsIndexer->GetData().data(), smsIndexer->GetData().size(), msgRef8bit,
        isStatusReport);

    std::shared_ptr<struct EncodeInfo> encodeInfo = nullptr;
    encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(smsIndexer->GetSmcaAddr(), false);
    if (encodeInfo == nullptr || tpdu == nullptr) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("DataBasedSmsDelivery encodeInfo or tpdu nullptr");
        return;
    }
    std::vector<uint8_t> smca(encodeInfo->smcaData_, encodeInfo->smcaData_ + encodeInfo->smcaLen);
    std::vector<uint8_t> pdu(encodeInfo->tpduData_, encodeInfo->tpduData_ + encodeInfo->tpduLen);
    std::shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(1);
    std::shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();

    smsIndexer->SetUnSentCellCount(unSentCellCount);
    smsIndexer->SetHasCellFailed(hasCellFailed);
    smsIndexer->SetEncodeSmca(std::move(smca));
    smsIndexer->SetEncodePdu(std::move(pdu));
    smsIndexer->SetHasMore(encodeInfo->isMore_);
    smsIndexer->SetMsgRefId(msgRef8bit);
    smsIndexer->SetNetWorkType(NET_TYPE_GSM);
    smsIndexer->SetTimeStamp(timeStamp);
    std::unique_lock<std::mutex> lock(mutex_);
    SendSmsToRil(smsIndexer);
}

bool GsmSmsSender::SetPduInfo(
    const std::shared_ptr<SmsSendIndexer> &smsIndexer, GsmSmsMessage &gsmSmsMessage, bool &isMore)
{
    bool ret = false;
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("Indexer is nullptr");
        return ret;
    }
    SmsCodingScheme codingType = smsIndexer->GetDcs();
    std::shared_ptr<struct SmsTpdu> tpdu = nullptr;
    tpdu = gsmSmsMessage.CreateDefaultSubmitSmsTpdu(smsIndexer->GetDestAddr(), smsIndexer->GetSmcaAddr(),
        smsIndexer->GetText(), (smsIndexer->GetDeliveryCallback() == nullptr) ? false : true, codingType);
    if (tpdu == nullptr) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("ResendTextDelivery CreateDefaultSubmitSmsTpdu err.");
        return ret;
    }
    (void)memset_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, 0x00, MAX_USER_DATA_LEN + 1);
    if (memcpy_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, smsIndexer->GetText().c_str(),
        smsIndexer->GetText().length()) != EOK) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("TextBasedSmsDelivery memcpy_s error.");
        return ret;
    }
    int headerCnt = 0;
    tpdu->data.submit.userData.length = smsIndexer->GetText().length();
    tpdu->data.submit.userData.data[smsIndexer->GetText().length()] = 0;
    tpdu->data.submit.msgRef = smsIndexer->GetMsgRefId();
    if (smsIndexer->GetIsConcat()) {
        headerCnt += gsmSmsMessage.SetHeaderConcat(headerCnt, smsIndexer->GetSmsConcat());
    }
    /* Set User Data Header for Alternate Reply Address */
    headerCnt += gsmSmsMessage.SetHeaderReply(headerCnt);
    /* Set User Data Header for National Language Single Shift */
    headerCnt += gsmSmsMessage.SetHeaderLang(headerCnt, codingType, smsIndexer->GetLangId());
    tpdu->data.submit.userData.headerCnt = headerCnt;
    tpdu->data.submit.bHeaderInd = (headerCnt > 0) ? true : false;

    if ((smsIndexer->GetSmsConcat().totalSeg > 1) &&
        ((smsIndexer->GetSmsConcat().seqNum) < (smsIndexer->GetSmsConcat().totalSeg - 1))) {
        tpdu->data.submit.bStatusReport = false;
        isMore = true;
    } else {
        tpdu->data.submit.bStatusReport = (smsIndexer->GetDeliveryCallback() == nullptr) ? false : true;
        isMore = false;
    }
    return true;
}
} // namespace Telephony
} // namespace OHOS