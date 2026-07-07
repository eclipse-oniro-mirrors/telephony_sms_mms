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

#include "cdma_sms_sender.h"

#include <cinttypes>

#include "cdma_sms_message.h"
#include "core_manager_inner.h"
#include "radio_event.h"
#include "securec.h"
#include "sms_hisysevent.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint32_t CDMASMS_MESSAGE_ID_MAX = 65536;
static constexpr uint8_t CDMASMS_SEQ_NUM_MAX = 64;

CdmaSmsSender::CdmaSmsSender(int32_t slotId, function<void(shared_ptr<SmsSendIndexer>)> sendRetryFun)
    : SmsSender(slotId, sendRetryFun)
{}

CdmaSmsSender::~CdmaSmsSender() {}

void CdmaSmsSender::TextBasedSmsDelivery(const string &desAddr, const string &scAddr, const string &text,
    const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback, int32_t dataBaseId, bool isMmsApp)
{
    if (isImsNetDomain_ && imsSmsCfg_) {
        TextBasedSmsDeliveryViaIms(desAddr, scAddr, text, sendCallback, deliveryCallback, dataBaseId, isMmsApp);
        return;
    }
    CdmaSmsMessage message;
    DataCodingScheme codingType;
    std::vector<struct SplitInfo> splits;
    std::string addr;
    message.SplitMessage(splits, text, CheckForce7BitEncodeType(), codingType, false, addr);
    if (splits.size() > MAX_SEGMENT_NUM) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("message exceed the limit.");
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_EXCEED_MAX_SEGMENT_NUM, "text sms cdma message exceed the limit");
        return;
    }
    bool bStatusReport = (deliveryCallback == nullptr) ? false : true;
    std::unique_ptr<CdmaTransportMsg> transMsg =
        message.CreateSubmitTransMsg(desAddr, scAddr, text, bStatusReport, codingType);
    if (transMsg == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("CreateSubmitTransMsg nullptr fail.");
        return;
    }
    /* 1. Set Reply sequence number. */
    uint8_t msgRef8bit = GetSeqNum();
    transMsg->data.p2p.replySeq = msgRef8bit;
    /* 2. Set msg ID. */
    uint16_t msgId = GetSubmitMsgId();
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.msgId = msgId;
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    TextBasedSmsSplitDelivery(desAddr, scAddr, splits, std::move(transMsg), msgRef8bit, msgId, timeStamp,
        sendCallback, deliveryCallback, dataBaseId, isMmsApp);
}

void CdmaSmsSender::TextBasedSmsSplitDelivery(const std::string &desAddr, const std::string &scAddr,
    std::vector<struct SplitInfo> splits, std::unique_ptr<CdmaTransportMsg> transMsg, uint8_t msgRef8bit,
    uint16_t msgId, long timeStamp, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback, int32_t dataBaseId, bool isMmsApp)
{
    shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    if (hasCellFailed == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    for (std::size_t i = 0; i < splits.size(); i++) {
        (void)memset_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
            sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), 0x00,
            sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data));
        if (splits[i].encodeData.size() > sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data)) {
            TELEPHONY_LOGE("data length invalid");
            return;
        }
        int value = memcpy_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
            sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), splits[i].encodeData.data(),
            splits[i].encodeData.size());
        if (value != EOK) {
            SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            return;
        }
        std::string segmentText;
        segmentText.append((char *)(splits[i].encodeData.data()), splits[i].encodeData.size());
        auto indexer = make_shared<SmsSendIndexer>(desAddr, scAddr, segmentText, sendCallback, deliveryCallback);
        if (indexer == nullptr) {
            SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            return;
        }
        indexer->SetDcs(splits[i].encodeType);
        SetPduSeqInfo(indexer, splits.size(), transMsg, i, msgRef8bit);
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.length =
            static_cast<int>(splits[i].encodeData.size());
        /* encode msg data */
        std::unique_ptr<std::vector<uint8_t>> pdu = EncodeMsg(*transMsg.get());
        if (pdu == nullptr) {
            SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
            return;
        }
        indexer->SetEncodePdu(*pdu);
        UpdateIndexerInfo(indexer, msgRef8bit, splits.size(), hasCellFailed,
            timeStamp, msgId, isMmsApp, dataBaseId);
        SendSmsToRil(indexer);
    }
}

void CdmaSmsSender::UpdateIndexerInfo(
    shared_ptr<SmsSendIndexer> &indexer,
    uint8_t msgRef8bit,
    const uint8_t unSentCellCount,
    shared_ptr<bool> hasCellFailed,
    long timeStamp,
    uint16_t msgId,
    int32_t dataBaseId,
    bool isMmsApp)
{
    indexer->SetMsgRefId(msgRef8bit);
    indexer->SetNetWorkType(NET_TYPE_CDMA);
    indexer->SetUnSentCellCount(unSentCellCount);
    indexer->SetHasCellFailed(hasCellFailed);
    indexer->SetTimeStamp(timeStamp);
    indexer->SetMsgId(msgId);
    indexer->SetDataBaseId(dataBaseId);
    indexer->SetIsMmsApp(isMmsApp);
}

void CdmaSmsSender::TextBasedSmsDeliveryViaIms(const string &desAddr, const string &scAddr, const string &text,
    const sptr<ISendShortMessageCallback> &sendCallback, const sptr<IDeliveryShortMessageCallback> &deliveryCallback,
    int32_t dataBaseId, bool isMmsApp)
{
    DataCodingScheme codingType;
    GsmSmsMessage gsmSmsMessage;
    std::vector<struct SplitInfo> cellsInfos;
    gsmSmsMessage.SetSmsCodingNationalType(GetSmsCodingNationalType(slotId_));
    std::string addr;
    gsmSmsMessage.SplitMessage(cellsInfos, text, CheckForce7BitEncodeType(), codingType, false, addr);
    bool isStatusReport = (deliveryCallback == nullptr) ? false : true;
    std::shared_ptr<struct SmsTpdu> tpdu =
        gsmSmsMessage.CreateDefaultSubmitSmsTpdu(desAddr, scAddr, text, isStatusReport, codingType);
    int cellsInfosSize = static_cast<int>(cellsInfos.size());
    shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(cellsInfosSize);
    shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    if (TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, tpdu, unSentCellCount, hasCellFailed, sendCallback)) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    uint8_t msgRef8bit = GetMsgRef8Bit();
    TELEPHONY_LOGI("cdma text msgRef8bit = %{public}d", msgRef8bit);
    for (int i = 0; i < cellsInfosSize; i++) {
        SendSmsForEveryIndexer(i, cellsInfos, desAddr, scAddr, tpdu, gsmSmsMessage, unSentCellCount, hasCellFailed,
            codingType, msgRef8bit, sendCallback, deliveryCallback, dataBaseId, isMmsApp);
    }
    return;
}

void CdmaSmsSender::SendSmsForEveryIndexer(int &i, std::vector<struct SplitInfo> cellsInfos, const string &desAddr,
    const string &scAddr, std::shared_ptr<struct SmsTpdu> tpdu, GsmSmsMessage gsmSmsMessage,
    shared_ptr<uint8_t> unSentCellCount, shared_ptr<bool> hasCellFailed, DataCodingScheme codingType,
    uint8_t msgRef8bit, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback, int32_t dataBaseId, bool isMmsApp)
{
    std::string segmentText;
    segmentText.append((char *)(cellsInfos[i].encodeData.data()), cellsInfos[i].encodeData.size());
    std::shared_ptr<SmsSendIndexer> indexer =
        make_shared<SmsSendIndexer>(desAddr, scAddr, segmentText, sendCallback, deliveryCallback);
    if (indexer == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    indexer->SetDcs(cellsInfos[i].encodeType);
    (void)memset_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, 0x00, MAX_USER_DATA_LEN + 1);

    if (cellsInfos[i].encodeData.size() > MAX_USER_DATA_LEN) {
        return;
    }
    if (memcpy_s(tpdu->data.submit.userData.data, MAX_USER_DATA_LEN + 1, &cellsInfos[i].encodeData[0],
        cellsInfos[i].encodeData.size()) != EOK) {
        SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }

    tpdu->data.submit.userData.length = cellsInfos[i].encodeData.size();
    tpdu->data.submit.userData.data[cellsInfos[i].encodeData.size()] = 0;
    tpdu->data.submit.msgRef = msgRef8bit;
    int headerCnt = 0;
    bool isMore = false;

    int cellsInfosSize = static_cast<int>(cellsInfos.size());
    if (cellsInfosSize > 1) {
        indexer->SetIsConcat(true);
        SmsConcat concat;
        concat.is8Bits = true;
        concat.msgRef = msgRef8bit;
        concat.totalSeg = static_cast<uint16_t>(cellsInfosSize);
        concat.seqNum = static_cast<uint16_t>(i + 1);
        indexer->SetSmsConcat(concat);
        headerCnt += gsmSmsMessage.SetHeaderConcat(headerCnt, concat);
    }

    /* Set User Data Header for Alternate Reply Address */
    headerCnt += gsmSmsMessage.SetHeaderReply(headerCnt);
    /* Set User Data Header for National Language Single Shift */
    headerCnt += gsmSmsMessage.SetHeaderLang(headerCnt, codingType, cellsInfos[i].langId);
    indexer->SetLangId(cellsInfos[i].langId);
    indexer->SetIsMmsApp(isMmsApp);
    indexer->SetDataBaseId(dataBaseId);
    tpdu->data.submit.userData.headerCnt = headerCnt;
    tpdu->data.submit.bHeaderInd = (headerCnt > 0) ? true : false;

    if (cellsInfosSize > 1 && i < (cellsInfosSize - 1)) {
        tpdu->data.submit.bStatusReport = false;
        isMore = true;
    } else {
        tpdu->data.submit.bStatusReport = (deliveryCallback == nullptr) ? false : true;
    }
    ReadySendSms(gsmSmsMessage, scAddr, isMore, indexer, msgRef8bit, unSentCellCount, hasCellFailed);
}

void CdmaSmsSender::ReadySendSms(GsmSmsMessage gsmSmsMessage, const string &scAddr, bool isMore,
    std::shared_ptr<SmsSendIndexer> indexer, uint8_t msgRef8bit, shared_ptr<uint8_t> unSentCellCount,
    shared_ptr<bool> hasCellFailed)
{
    std::shared_ptr<struct EncodeInfo> encodeInfo = gsmSmsMessage.GetSubmitEncodeInfo(scAddr, isMore);
    if (encodeInfo == nullptr) {
        SendResultCallBack(indexer, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("create encodeInfo encodeInfo nullptr error.");
        return;
    }

    SetSendIndexerInfo(indexer, encodeInfo, msgRef8bit);
    indexer->SetUnSentCellCount(*unSentCellCount);
    indexer->SetHasCellFailed(hasCellFailed);
    indexer->SetImsSmsForCdma(true);
    SendSmsToRil(indexer);
}

bool CdmaSmsSender::TpduNullOrSmsPageOverNormalOrSmsEncodeFail(std::vector<struct SplitInfo> cellsInfos,
    std::shared_ptr<struct SmsTpdu> tpdu, shared_ptr<uint8_t> unSentCellCount, shared_ptr<bool> hasCellFailed,
    const sptr<ISendShortMessageCallback> &sendCallback)
{
    if (tpdu == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("TextBasedSmsDelivery tpdu nullptr error.");
        return true;
    }

    int cellsInfosSize = static_cast<int>(cellsInfos.size());
    if (cellsInfosSize > MAX_SEGMENT_NUM) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("message exceed the limit.");
        return true;
    }

    bool isStatusReport = tpdu->data.submit.bStatusReport;
    TELEPHONY_LOGI("TextBasedSmsDelivery isStatusReport= %{public}d", isStatusReport);
    if (unSentCellCount == nullptr || hasCellFailed == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return true;
    }
    return false;
}

void CdmaSmsSender::SetSendIndexerInfo(const std::shared_ptr<SmsSendIndexer> &indexer,
    const std::shared_ptr<struct EncodeInfo> &encodeInfo, uint8_t msgRef8bit)
{
    if (encodeInfo == nullptr || indexer == nullptr) {
        TELEPHONY_LOGE("CdmaSmsSender::SetSendIndexerInfo encodeInfo nullptr");
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
    indexer->SetNetWorkType(NET_TYPE_CDMA);
}

void CdmaSmsSender::SetPduSeqInfo(const std::shared_ptr<SmsSendIndexer> &smsIndexer, const std::size_t size,
    const std::unique_ptr<CdmaTransportMsg> &transMsg, const std::size_t index, const uint8_t msgRef8bit)
{
    if (size > 1) {
        smsIndexer->SetIsConcat(true);
        SmsConcat smsConcat;
        transMsg->data.p2p.teleserviceId = static_cast<uint16_t>(SmsTransTelsvcId::WEMT);
        transMsg->data.p2p.telesvcMsg.data.submit.msgId.headerInd = true;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.headerCnt = 1;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udhType = UDH_CONCAT_8BIT;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.msgRef = msgRef8bit;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.totalSeg =
            static_cast<uint8_t>(size);
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.seqNum = index + 1;
        smsConcat.msgRef = msgRef8bit;
        smsConcat.seqNum = index + 1;
        smsConcat.totalSeg = static_cast<uint8_t>(size);
        smsConcat.is8Bits = true;
        smsIndexer->SetSmsConcat(smsConcat);
    }
}

void CdmaSmsSender::DataBasedSmsDelivery(const string &desAddr, const string &scAddr, int32_t port, const uint8_t *data,
    uint32_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    if (isImsNetDomain_ && imsSmsCfg_) {
        DataBasedSmsDeliveryViaIms(desAddr, scAddr, port, data, dataLen, sendCallback, deliveryCallback);
        return;
    }
    CdmaSmsMessage message;
    DataCodingScheme codingType;
    std::vector<struct SplitInfo> splits;
    std::string text((char *)data, dataLen);
    std::string addr;
    message.SplitMessage(splits, text, false, codingType, true, addr);
    if (splits.size() == 0) {
        TELEPHONY_LOGE("splits fail.");
        return;
    }
    std::unique_ptr<CdmaTransportMsg> transMsg = nullptr;
    bool bStatusReport = (deliveryCallback == nullptr) ? false : true;
    transMsg = message.CreateSubmitTransMsg(desAddr, scAddr, port, data, dataLen, bStatusReport);
    if (transMsg == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("CreateSubmitTransMsg nullptr fail.");
        return;
    }
    /* Set Reply sequence number. */
    uint8_t msgRef8bit = GetSeqNum();
    transMsg->data.p2p.replySeq = msgRef8bit;
    /* Set msg ID. */
    uint16_t msgId = GetSubmitMsgId();
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.msgId = msgId;
    /* while user data header isn't exist, headerInd must be set false. */
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.headerInd = true;
    transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.length = static_cast<int>(splits[0].encodeData.size());
    if (splits[0].encodeData.size() > sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data)) {
        TELEPHONY_LOGE("DataBasedSmsDelivery data length invalid.");
        return;
    }
    if (memcpy_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
        sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), splits[0].encodeData.data(),
        splits[0].encodeData.size()) != EOK) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("memcpy_s return error.");
        return;
    }
    std::shared_ptr<SmsSendIndexer> indexer = make_shared<SmsSendIndexer>(desAddr, scAddr, port,
        splits[0].encodeData.data(), splits[0].encodeData.size(), sendCallback, deliveryCallback);
    EncodeMsgData(std::move(transMsg), indexer, msgRef8bit, sendCallback);
}

void CdmaSmsSender::EncodeMsgData(std::unique_ptr<CdmaTransportMsg> transMsg, std::shared_ptr<SmsSendIndexer> indexer,
    uint8_t msgRef8bit, const sptr<ISendShortMessageCallback> &sendCallback)
{
    /* encode msg data */
    SmsWriteBuffer pduBuffer;
    std::unique_ptr<CdmaSmsTransportMessage> msg = CdmaSmsTransportMessage::CreateTransportMessage(*transMsg.get());
    if (msg == nullptr || msg->IsEmpty() || !msg->Encode(pduBuffer)) {
        TELEPHONY_LOGE("EncodeMsg Error");
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_PDU_ENCODEING_FAIL, "cdma encode msg error");
        return;
    }

    const uint8_t segmentCount = 1;
    shared_ptr<uint8_t> unSentCellCount = make_shared<uint8_t>(segmentCount);
    shared_ptr<bool> hasCellFailed = make_shared<bool>(false);
    std::unique_ptr<std::vector<uint8_t>> pdu = pduBuffer.GetPduBuffer();
    if (indexer == nullptr || unSentCellCount == nullptr || hasCellFailed == nullptr || pdu == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("Init SmsSend Indexer Error.");
        return;
    }
    indexer->SetEncodePdu(*pdu);
    indexer->SetMsgRefId(msgRef8bit);
    indexer->SetNetWorkType(NET_TYPE_CDMA);
    indexer->SetUnSentCellCount(*unSentCellCount);
    indexer->SetHasCellFailed(hasCellFailed);
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    indexer->SetTimeStamp(timeStamp);
    uint16_t msgId = GetSubmitMsgId();
    indexer->SetMsgId(msgId);
    SendSmsToRil(indexer);
}

void CdmaSmsSender::DataBasedSmsDeliveryViaIms(const string &desAddr, const string &scAddr, int32_t port,
    const uint8_t *data, uint32_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    GsmSmsMessage gsmSmsMessage;
    std::vector<struct SplitInfo> cellsInfos;
    DataCodingScheme codingType;
    std::string dataStr;
    CharArrayToString(data, dataLen, dataStr);
    gsmSmsMessage.SetSmsCodingNationalType(GetSmsCodingNationalType(slotId_));
    gsmSmsMessage.SplitMessage(cellsInfos, dataStr, CheckForce7BitEncodeType(), codingType, true, desAddr);
    uint8_t msgRef8bit = GetMsgRef8Bit();
    TELEPHONY_LOGI("cdma data msgRef8bit = %{public}d", msgRef8bit);
    std::shared_ptr<struct SmsTpdu> tpdu = gsmSmsMessage.CreateDataSubmitSmsTpdu(
        desAddr, scAddr, port, data, dataLen, msgRef8bit, codingType, (deliveryCallback == nullptr) ? false : true);
    if (tpdu == nullptr) {
        SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("tpdu nullptr error.");
        return;
    }
    DataBasedSmsDeliverySplitPage(
        gsmSmsMessage, cellsInfos, tpdu, msgRef8bit, desAddr, scAddr, port, sendCallback, deliveryCallback);
}

void CdmaSmsSender::StatusReportAnalysis(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("cdma_sms_sender: StatusReportAnalysis event nullptr error.");
        return;
    }
    std::shared_ptr<SmsReceiveIndexer> statusInfo = event->GetSharedObject<SmsReceiveIndexer>();
    if (statusInfo == nullptr) {
        TELEPHONY_LOGE("cdma_sms_sender: StatusReportAnalysis statusInfo nullptr error.");
        return;
    }
    std::string pdu = StringUtils::StringToHex(statusInfo->GetPdu());
    std::shared_ptr<CdmaSmsMessage> message = CdmaSmsMessage::CreateMessage(pdu);
    if (message == nullptr) {
        TELEPHONY_LOGE("message is nullptr.");
        return;
    }
    sptr<IDeliveryShortMessageCallback> deliveryCallback = nullptr;
    auto oldIndexer = reportList_.begin();
    while (oldIndexer != reportList_.end()) {
        auto iter = oldIndexer++;
        if (*iter != nullptr) {
            if (message->GetMsgRef() == (*iter)->GetMsgRefId()) {
                // save the message to db, or updata to db msg state(success or fail)
                deliveryCallback = (*iter)->GetDeliveryCallback();
                reportList_.erase(iter);
            }
        }
    }
    if (deliveryCallback != nullptr) {
        std::string ackpdu = StringUtils::StringToHex(message->GetRawPdu());
        deliveryCallback->OnSmsDeliveryResult(StringUtils::ToUtf16(ackpdu));
        TELEPHONY_LOGI("gsm_sms_sender: StatusReportAnalysis %{private}s", pdu.c_str());
    }
}

void CdmaSmsSender::SendSmsToRil(const shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("cdma_sms_sender: SendSms smsIndexer nullptr");
        return;
    }
    if ((!isImsNetDomain_ && voiceServiceState_ != static_cast<int32_t>(RegServiceState::REG_STATE_IN_SERVICE))) {
        SendResultCallBack(smsIndexer, ISendShortMessageCallback::SEND_SMS_FAILURE_SERVICE_UNAVAILABLE);
        TELEPHONY_LOGE("cdma_sms_sender: SendSms not in service");
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId_, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_SENDSMS_NOT_IN_SERVICE, "cdma send sms not in service");
        return;
    }
    int64_t refId = GetMsgRef64Bit();
    TELEPHONY_LOGI("cdma refId = %{public}" PRId64 "", refId);
    if (!SendCacheMapAddItem(refId, smsIndexer)) {
        TELEPHONY_LOGE("SendCacheMapAddItem Error!!");
    }

    std::string pdu = StringUtils::StringToHex(smsIndexer->GetEncodePdu());
    bool sendImsSMS = smsIndexer->IsImsSmsForCdma();
    if (smsIndexer->GetPsResendCount() < MAX_SEND_RETRIES) {
        sendImsSMS = true;
    }

    if (sendImsSMS) {
        SendImsSms(smsIndexer, refId, pdu);
    } else {
        SendCsSms(smsIndexer, refId, pdu);
    }
}

void CdmaSmsSender::SendCsSms(const shared_ptr<SmsSendIndexer> &smsIndexer, int64_t &refId, std::string &pdu)
{
    lastSmsDomain_ = CS_DOMAIN;
    CoreManagerInner::GetInstance().SendCdmaSms(
        slotId_, RadioEvent::RADIO_SEND_CDMA_SMS, pdu, refId, shared_from_this());
    TELEPHONY_LOGI("SendCsSms pdu = %{private}s", pdu.c_str());
}

void CdmaSmsSender::SendImsSms(const shared_ptr<SmsSendIndexer> &smsIndexer, int64_t &refId, std::string &pdu)
{
    TELEPHONY_LOGI("ims network domain send sms interface.!");
    auto smsClient = DelayedSingleton<ImsSmsClient>::GetInstance();
    if (smsClient == nullptr) {
        TELEPHONY_LOGE("SendImsSms return, ImsSmsClient is nullptr.");
        return;
    }
    lastSmsDomain_ = IMS_DOMAIN;
    ImsMessageInfo imsMessageInfo;
    imsMessageInfo.refId = refId;
    imsMessageInfo.smscPdu = StringUtils::StringToHex(smsIndexer->GetEncodeSmca());
    imsMessageInfo.pdu = pdu;
    imsMessageInfo.tech = SMS_RADIO_TECH_3GPP;
    int32_t reply = smsClient->ImsSendMessage(slotId_, imsMessageInfo);
    HILOG_COMM_INFO("SendImsSms reply = %{public}d", reply);
}

int32_t CdmaSmsSender::IsImsSmsSupported(int32_t slotId, bool &isSupported)
{
    auto smsClient = DelayedSingleton<ImsSmsClient>::GetInstance();
    if (smsClient == nullptr) {
        TELEPHONY_LOGE("IsImsSmsSupported return, ImsSmsClient is nullptr.");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::unique_lock<std::mutex> lck(ctx_);
    resIsSmsReady_ = false;
    int32_t reply = smsClient->ImsGetSmsConfig(slotId);
    TELEPHONY_LOGI("IsImsSmsSupported reply = %{public}d", reply);
    while (resIsSmsReady_) {
        TELEPHONY_LOGI("IsImsSmsSupported::wait(), resIsSmsReady_ = false");
        if (cv_.wait_for(lck, std::chrono::seconds(WAIT_TIME_SECOND)) == std::cv_status::timeout) {
            break;
        }
    }
    TELEPHONY_LOGI("CdmaSmsSender::IsImsSmsSupported(), imsSmsCfg_:%{public}d", imsSmsCfg_);
    isSupported = (imsSmsCfg_ == IMS_SMS_ENABLE);
    return TELEPHONY_ERR_SUCCESS;
}

void CdmaSmsSender::StatusReportSetImsSms(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("cdma_sms_sender: StatusReportSetImsSms event nullptr error.");
        return;
    }
    std::shared_ptr<RadioResponseInfo> imsResponseInfo = event->GetSharedObject<RadioResponseInfo>();
    if (imsResponseInfo == nullptr) {
        TELEPHONY_LOGE("imsResponseInfo is nullptr error.");
        return;
    }
    if (imsResponseInfo->error != ErrType::NONE) {
        imsSmsCfg_ = IMS_SMS_DISABLE;
    }
}

void CdmaSmsSender::StatusReportGetImsSms(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        TELEPHONY_LOGE("CdmaSmsSender: StatusReportGetImsSms event nullptr error.");
        return;
    }
    std::shared_ptr<int32_t> imsSmsInfo = event->GetSharedObject<int32_t>();
    if (imsSmsInfo == nullptr) {
        TELEPHONY_LOGE("CdmaSmsSender: StatusReportGetImsSms imsSmsInfo nullptr error.");
        return;
    }
    imsSmsCfg_ = *imsSmsInfo;
}

void CdmaSmsSender::Init() {}

void CdmaSmsSender::ReceiveStatusReport(const std::shared_ptr<SmsReceiveIndexer> &smsIndexer)
{
    SendEvent(RadioEvent::RADIO_SMS_STATUS, smsIndexer);
}

void CdmaSmsSender::RegisterImsHandler()
{
    if (isImsCdmaHandlerRegistered) {
        return;
    }
    auto smsClient = DelayedSingleton<ImsSmsClient>::GetInstance();
    if (smsClient == nullptr) {
        TELEPHONY_LOGE("RegisterHandler return, ImsSmsClient is nullptr.");
        return;
    }
    smsClient->RegisterImsSmsCallbackHandler(slotId_, shared_from_this());
    TELEPHONY_LOGE("RegisterHandler  gsm ImsSmsClient successs");
    isImsCdmaHandlerRegistered = true;
}

uint8_t CdmaSmsSender::GetSeqNum()
{
    msgSeqNum_ = ((msgSeqNum_ + 1) % CDMASMS_SEQ_NUM_MAX);
    return msgSeqNum_;
}

uint8_t CdmaSmsSender::GetSubmitMsgId()
{
    msgSubmitId_ = ((msgSubmitId_ + 1) % CDMASMS_MESSAGE_ID_MAX);
    return msgSubmitId_;
}

std::unique_ptr<std::vector<uint8_t>> CdmaSmsSender::EncodeMsg(CdmaTransportMsg &transMsg)
{
    std::unique_ptr<CdmaSmsTransportMessage> msg = CdmaSmsTransportMessage::CreateTransportMessage(transMsg);
    SmsWriteBuffer pduBuffer;
    if (msg == nullptr || msg->IsEmpty() || !msg->Encode(pduBuffer)) {
        TELEPHONY_LOGE("encode msg error");
        return nullptr;
    }
    return pduBuffer.GetPduBuffer();
}

void CdmaSmsSender::ResendTextDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGE("smsIndexer is nullptr!!");
        return;
    }
    CdmaSmsMessage message;
    DataCodingScheme codingType = smsIndexer->GetDcs();
    std::unique_ptr<CdmaTransportMsg> transMsg = nullptr;
    bool bStatusReport = (smsIndexer->GetDeliveryCallback() == nullptr) ? false : true;
    transMsg = message.CreateSubmitTransMsg(
        smsIndexer->GetDestAddr(), smsIndexer->GetSmcaAddr(), smsIndexer->GetText(), bStatusReport, codingType);
    if (transMsg == nullptr) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("CreateSubmitTransMsg nullptr fail.");
        return;
    }
    /* 1. Set Reply sequence number. */
    uint8_t msgRef8bit = smsIndexer->GetMsgRefId();
    transMsg->data.p2p.replySeq = msgRef8bit;
    /* 2. Set msg ID. */
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.msgId = smsIndexer->GetMsgId();
    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    smsIndexer->SetTimeStamp(timeStamp);
    transMsg->data.p2p.telesvcMsg.data.submit.userData.encodeType = SmsEncodingType::OCTET;
    (void)memset_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
        sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), 0x00,
        sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data));
    if (smsIndexer->GetText().length() > sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data)) {
        TELEPHONY_LOGE("ResendTextDelivery data length invalid.");
        return;
    }
    if (memcpy_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
        sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), smsIndexer->GetText().data(),
        smsIndexer->GetText().length()) != EOK) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    SetConcact(smsIndexer, transMsg);
    /* encode msg data */
    std::unique_ptr<std::vector<uint8_t>> pdu = EncodeMsg(*transMsg.get());
    if (pdu == nullptr) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    smsIndexer->SetEncodePdu(*pdu);
    smsIndexer->SetNetWorkType(NET_TYPE_CDMA);
    smsIndexer->SetTimeStamp(timeStamp);
    SendSmsToRil(smsIndexer);
}

void CdmaSmsSender::ResendDataDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer)
{
    if (smsIndexer == nullptr) {
        TELEPHONY_LOGI("ResendDataDelivery::smsIndexer is nullptr error.");
        return;
    }

    CdmaSmsMessage message;
    std::unique_ptr<CdmaTransportMsg> transMsg = nullptr;
    bool bStatusReport = (smsIndexer->GetDeliveryCallback() == nullptr) ? false : true;
    transMsg = message.CreateSubmitTransMsg(smsIndexer->GetDestAddr(), smsIndexer->GetSmcaAddr(),
        smsIndexer->GetDestPort(), smsIndexer->GetData().data(), smsIndexer->GetData().size(), bStatusReport);
    if (transMsg == nullptr) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("CreateSubmitTransMsg nullptr fail.");
        return;
    }
    /* Set Reply sequence number. */
    uint8_t msgRef8bit = smsIndexer->GetMsgRefId();
    transMsg->data.p2p.replySeq = msgRef8bit;
    /* Set msg ID. */
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.msgId = smsIndexer->GetMsgId();
    /* while user data header isn't exist, headerInd must be set false. */
    transMsg->data.p2p.telesvcMsg.data.submit.msgId.headerInd = true;

    chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
    long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
    transMsg->data.p2p.telesvcMsg.data.submit.userData.encodeType = SmsEncodingType::OCTET;
    if (smsIndexer->GetData().size() > sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data)) {
        TELEPHONY_LOGE("ResendDataDelivery data length invalid.");
        return;
    }
    if (memcpy_s(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data,
        sizeof(transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.data), smsIndexer->GetData().data(),
        smsIndexer->GetData().size()) != EOK) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }

    transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.length = static_cast<int>(smsIndexer->GetData().size());
    /* encode msg data */
    std::unique_ptr<std::vector<uint8_t>> pdu = EncodeMsg(*transMsg.get());
    if (pdu == nullptr) {
        SendResultCallBack(smsIndexer->GetSendCallback(), ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return;
    }
    smsIndexer->SetEncodePdu(*pdu);
    smsIndexer->SetNetWorkType(NET_TYPE_CDMA);
    smsIndexer->SetTimeStamp(timeStamp);
    SendSmsToRil(smsIndexer);
}

void CdmaSmsSender::SetConcact(
    const std::shared_ptr<SmsSendIndexer> &smsIndexer, const std::unique_ptr<CdmaTransportMsg> &transMsg)
{
    if (smsIndexer->GetIsConcat()) {
        SmsConcat smsConcat = smsIndexer->GetSmsConcat();
        transMsg->data.p2p.teleserviceId = static_cast<uint16_t>(SmsTransTelsvcId::WEMT);
        transMsg->data.p2p.telesvcMsg.data.submit.msgId.headerInd = true;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.headerCnt = 1;
        if (smsConcat.is8Bits) {
            transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udhType = UDH_CONCAT_8BIT;
        } else {
            transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udhType = UDH_CONCAT_16BIT;
        }
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.msgRef = smsConcat.msgRef;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.totalSeg =
            smsConcat.totalSeg;
        transMsg->data.p2p.telesvcMsg.data.submit.userData.userData.header[0].udh.concat8bit.seqNum = smsConcat.seqNum;
    }
}
} // namespace Telephony
} // namespace OHOS
