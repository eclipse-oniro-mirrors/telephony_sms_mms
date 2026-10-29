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

#include <regex>

#include "gsm_sms_message.h"

#include "core_manager_inner.h"
#include "securec.h"
#include "telephony_log_wrapper.h"
#include "text_coder.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint16_t DEFAULT_PORT = -1;
static constexpr uint8_t MAX_GSM_7BIT_DATA_LEN = 160;
static constexpr uint8_t MAX_SMSC_LEN = 20;
static constexpr uint16_t MAX_TPDU_DATA_LEN = 255;
static constexpr uint16_t TAPI_TEXT_SIZE_MAX = 520;
static constexpr uint16_t ADDRESS_LEN = 4;
static constexpr uint16_t MATCH_TWO = 2;
static constexpr int MAX_TP_UD_OCTETS = 140;
static constexpr int OCTET_BITS = 8;
static constexpr int GSM_7BIT_BITS = 7;
static constexpr int UCS2_CHAR_OCTETS = 2;
static constexpr int UDHL_OCTETS = 1;
static constexpr int CONCAT_8BIT_IE_OCTETS = 5;
static constexpr int APP_PORT_16BIT_IE_OCTETS = 6;
static constexpr int SINGLE_SHIFT_IE_OCTETS = 3;
static std::string g_nameAddrEmailRegex = "\\s*(\"[^\"]*\"|[^<>\"]+)\\s*<([^<>]+)>\\s*";
static std::string g_addrEmailRegex =
    "^([A-Za-z0-9_\\-\\.\u4e00-\u9fa5])+\\@([A-Za-z0-9_\\-\\.])+\\.([A-Za-z]{2,63})$";

uint8_t GsmSmsMessage::CalcReplyEncodeAddress(const std::string &replyAddress)
{
    std::string encodedAddr;
    if (replyAddress.length() > 0) {
        struct AddressNumber replyAddr = {};
        replyAddr.ton = TYPE_NATIONAL;
        replyAddr.npi = SMS_NPI_ISDN;
        int ret = memset_s(replyAddr.address, sizeof(replyAddr.address), 0x00, MAX_ADDRESS_LEN + 1);
        if (ret != EOK) {
            TELEPHONY_LOGE("CalcReplyEncodeAddress memset_s error!");
            return 0;
        }
        ret = memcpy_s(replyAddr.address, sizeof(replyAddr.address), replyAddress.c_str(), MAX_ADDRESS_LEN);
        if (ret != EOK) {
            TELEPHONY_LOGE("CalcReplyEncodeAddress memory_s error!");
            return 0;
        }
        GsmSmsParamCodec codec;
        codec.EncodeAddressPdu(&replyAddr, encodedAddr);
    }
    return encodedAddr.size();
}

int GsmSmsMessage::SetSmsTpduDestAddress(std::shared_ptr<struct SmsTpdu> &tPdu, const std::string &desAddr)
{
    int ret = 0;
    int addLen = 0;
    if (tPdu == nullptr) {
        TELEPHONY_LOGE("TPdu is null.");
        return addLen;
    }
    addLen = static_cast<int>(desAddr.length());
    tPdu->data.submit.destAddress.ton = TYPE_UNKNOWN;
    tPdu->data.submit.destAddress.npi = SMS_NPI_ISDN;
    if (addLen < MAX_ADDRESS_LEN) {
        ret = memcpy_s(tPdu->data.submit.destAddress.address, sizeof(tPdu->data.submit.destAddress.address),
            desAddr.c_str(), addLen);
        if (ret != EOK) {
            TELEPHONY_LOGE("SetSmsTpduDestAddress memcpy_s error!");
            return addLen;
        }
        tPdu->data.submit.destAddress.address[addLen] = '\0';
    } else {
        if (desAddr[0] == '+') {
            ret = memcpy_s(tPdu->data.submit.destAddress.address, sizeof(tPdu->data.submit.destAddress.address),
                desAddr.c_str(), MAX_ADDRESS_LEN);
        } else {
            ret = memcpy_s(tPdu->data.submit.destAddress.address, sizeof(tPdu->data.submit.destAddress.address),
                desAddr.c_str(), MAX_ADDRESS_LEN - 1);
        }
        if (ret != EOK) {
            TELEPHONY_LOGE("SetSmsTpduDestAddress memcpy_s error!");
            return addLen;
        }
        tPdu->data.submit.destAddress.address[MAX_ADDRESS_LEN] = '\0';
    }
    return addLen;
}

int GsmSmsMessage::SetHeaderLang(int index, const DataCodingScheme codingType, const MSG_LANGUAGE_ID_T langId)
{
    int ret = 0;
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("TPdu is null.");
        return ret;
    }
    if (index < 0 || index >= MAX_UD_HEADER_NUM) {
        TELEPHONY_LOGE("index is invalid");
        return ret;
    }
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_SUBMIT:
            if (codingType == DATA_CODING_7BIT && langId != MSG_ID_RESERVED_LANG) {
                smsTpdu_->data.submit.userData.header[index].udhType = UDH_SINGLE_SHIFT;
                smsTpdu_->data.submit.userData.header[index].udh.singleShift.langId = langId;
                ret++;
            }
            break;
        default:
            break;
    }
    return ret;
}

int GsmSmsMessage::SetHeaderConcat(int index, const SmsConcat &concat)
{
    int ret = 0;
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("TPdu is null.");
        return ret;
    }
    if (index < 0 || index >= MAX_UD_HEADER_NUM) {
        TELEPHONY_LOGE("index is invalid");
        return ret;
    }
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_SUBMIT:
            if (concat.is8Bits) {
                smsTpdu_->data.submit.userData.header[index].udhType = UDH_CONCAT_8BIT;
                smsTpdu_->data.submit.userData.header[index].udh.concat8bit.msgRef = concat.msgRef;
                smsTpdu_->data.submit.userData.header[index].udh.concat8bit.totalSeg = concat.totalSeg;
                smsTpdu_->data.submit.userData.header[index].udh.concat8bit.seqNum = concat.seqNum;
            } else {
                smsTpdu_->data.submit.userData.header[index].udhType = UDH_CONCAT_16BIT;
                smsTpdu_->data.submit.userData.header[index].udh.concat16bit.msgRef = concat.msgRef;
                smsTpdu_->data.submit.userData.header[index].udh.concat16bit.totalSeg = concat.totalSeg;
                smsTpdu_->data.submit.userData.header[index].udh.concat16bit.seqNum = concat.seqNum;
            }
            ret++;
            break;
        default:
            break;
    }
    return ret;
}

int GsmSmsMessage::SetHeaderReply(int index)
{
    int ret = 0;
    std::string reply = GetReplyAddress();
    if (reply.length() == 0) {
        TELEPHONY_LOGE("address is null.");
        return ret;
    }
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("smsTpdu_ is null.");
        return ret;
    }
    if (index < 0 || index >= MAX_UD_HEADER_NUM) {
        TELEPHONY_LOGE("index is invalid");
        return ret;
    }
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_SUBMIT: {
            smsTpdu_->data.submit.bReplyPath = true;
            smsTpdu_->data.submit.userData.header[index].udhType = UDH_ALTERNATE_REPLY_ADDRESS;
            smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.ton = TYPE_NATIONAL;
            smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.npi = SMS_NPI_ISDN;
            ret = memset_s(smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.address,
                sizeof(smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.address), 0x00,
                MAX_ADDRESS_LEN + 1);
            if (ret != EOK) {
                TELEPHONY_LOGE("SetHeaderReply memset_s error!");
                return ret;
            }
            if (sizeof(smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.address) < reply.length()) {
                TELEPHONY_LOGE("reply length exceed maxinum");
                return ret;
            }
            ret = memcpy_s(smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.address,
                sizeof(smsTpdu_->data.submit.userData.header[index].udh.alternateAddress.address), reply.c_str(),
                reply.length());
            if (ret != EOK) {
                TELEPHONY_LOGE("SetHeaderReply memcpy_s error!");
                return ret;
            }
            break;
        }
        default:
            break;
    }
    return ret;
}

void GsmSmsMessage::CreateDefaultSubmit(bool bStatusReport, const DataCodingScheme codingScheme)
{
    smsTpdu_ = std::make_shared<struct SmsTpdu>();
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("Make tPdu is fail.");
        return;
    }
    smsTpdu_->tpduType = SMS_TPDU_SUBMIT;
    smsTpdu_->data.submit.bHeaderInd = false;
    smsTpdu_->data.submit.bRejectDup = false;
    smsTpdu_->data.submit.bStatusReport = bStatusReport;
    smsTpdu_->data.submit.bReplyPath = false;
    smsTpdu_->data.submit.msgRef = 0;
    smsTpdu_->data.submit.dcs.bCompressed = false;
    smsTpdu_->data.submit.dcs.msgClass = SmsMessageClass::SMS_CLASS_UNKNOWN;
    smsTpdu_->data.submit.dcs.codingGroup = CODING_GENERAL_GROUP;
    smsTpdu_->data.submit.dcs.codingScheme = codingScheme;
    smsTpdu_->data.submit.pid = SMS_NORMAL_PID;
    smsTpdu_->data.submit.vpf = SMS_VPF_NOT_PRESENT;
}

std::shared_ptr<struct SmsTpdu> GsmSmsMessage::CreateDefaultSubmitSmsTpdu(const std::string &dest,
    const std::string &sc, const std::string &text, bool bStatusReport,
    const DataCodingScheme codingScheme = DATA_CODING_7BIT)
{
    SetFullText(text);
    SetSmscAddr(sc);
    SetDestAddress(dest);
    CreateDefaultSubmit(bStatusReport, codingScheme);
    SetSmsTpduDestAddress(smsTpdu_, dest);
    return smsTpdu_;
}

std::shared_ptr<struct SmsTpdu> GsmSmsMessage::CreateDataSubmitSmsTpdu(const std::string &desAddr,
    const std::string &scAddr, int32_t port, const uint8_t *data, uint32_t dataLen, uint8_t msgRef8bit,
    DataCodingScheme codingType, bool bStatusReport)
{
    SetSmscAddr(scAddr);
    SetDestAddress(desAddr);
    CreateDefaultSubmit(bStatusReport, codingType);
    SetSmsTpduDestAddress(smsTpdu_, desAddr);
    int endcodeLen = 0;
    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    uint8_t encodeData[(MAX_GSM_7BIT_DATA_LEN * MAX_SEGMENT_NUM) + 1];
    if (memset_s(encodeData, sizeof(encodeData), 0x00, sizeof(encodeData)) != EOK) {
        TELEPHONY_LOGE("failed to initialize!");
        return nullptr;
    }
    const uint8_t *pMsgText = data;
    uint8_t *pDestText = encodeData;
    endcodeLen = TextCoder::Instance().Utf8ToGsm7bit(
        pDestText, ((MAX_GSM_7BIT_DATA_LEN * MAX_SEGMENT_NUM) + 1), pMsgText, static_cast<int16_t>(dataLen), langId);
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("smsTpdu_ is nullptr!");
        return nullptr;
    }
    if (memset_s(smsTpdu_->data.submit.userData.data, sizeof(smsTpdu_->data.submit.userData.data), 0x00,
            sizeof(smsTpdu_->data.submit.userData.data)) != EOK) {
        TELEPHONY_LOGE("memset_s is error!");
        return nullptr;
    }
    if ((unsigned int)endcodeLen >= sizeof(smsTpdu_->data.submit.userData.data)) {
        endcodeLen = sizeof(smsTpdu_->data.submit.userData.data) -1;
        if (memcpy_s(smsTpdu_->data.submit.userData.data, sizeof(smsTpdu_->data.submit.userData.data), encodeData,
                sizeof(smsTpdu_->data.submit.userData.data)) != EOK) {
            TELEPHONY_LOGE("memcpy_s is error!");
            return nullptr;
        }
    } else {
        if (memcpy_s(smsTpdu_->data.submit.userData.data, sizeof(smsTpdu_->data.submit.userData.data), encodeData,
                endcodeLen) != EOK) {
            TELEPHONY_LOGE("memcpy_s is error!");
            return nullptr;
        }
    }
    smsTpdu_->data.submit.userData.data[endcodeLen] = 0;
    smsTpdu_->data.submit.userData.length = (int)dataLen;
    smsTpdu_->data.submit.msgRef = msgRef8bit;
    return smsTpdu_;
}

std::shared_ptr<struct EncodeInfo> GsmSmsMessage::GetSubmitEncodeInfo(const std::string &sc, bool bMore)
{
    uint8_t encodeSmscAddr[MAX_SMSC_LEN];
    if (memset_s(encodeSmscAddr, sizeof(encodeSmscAddr), 0x00, sizeof(encodeSmscAddr)) != EOK) {
        TELEPHONY_LOGE("memset_s error.");
        return nullptr;
    }

    uint8_t encodeSmscLen = 0;
    if ((!sc.empty()) && (sc.length() < MAX_SMSC_LEN)) {
        struct AddressNumber pAddress;
        if (memset_s(&pAddress.address, sizeof(pAddress.address), 0x00, sizeof(pAddress.address)) != EOK) {
            TELEPHONY_LOGE("GetSubmitEncodeInfo memset_s error!");
            return nullptr;
        }
        if (sc.length() > sizeof(pAddress.address)) {
            return nullptr;
        }
        if (memcpy_s(&pAddress.address, sizeof(pAddress.address), sc.data(), sc.length()) != EOK) {
            TELEPHONY_LOGE("GetSubmitEncodeInfo memcpy_s error!");
            return nullptr;
        }
        pAddress.address[sc.length()] = '\0';
        if (sc[0] == '+') {
            pAddress.ton = TYPE_INTERNATIONAL;
        } else {
            pAddress.ton = TYPE_UNKNOWN;
        }
        pAddress.npi = SMS_NPI_ISDN; /* app cannot set this value */
        GsmSmsParamCodec codec;
        encodeSmscLen = codec.EncodeSmscPdu(&pAddress, encodeSmscAddr, sizeof(encodeSmscAddr));
    }
    return GetSubmitEncodeInfoPartData(encodeSmscAddr, encodeSmscLen, bMore);
}

std::shared_ptr<struct EncodeInfo> GsmSmsMessage::GetSubmitEncodeInfoPartData(
    uint8_t *encodeSmscAddr, uint8_t encodeSmscLen, bool bMore)
{
    std::shared_ptr<struct EncodeInfo> info = std::make_shared<struct EncodeInfo>();
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    if (tpduCodec == nullptr) {
        TELEPHONY_LOGE("tpduCodec nullptr.");
        return nullptr;
    }
    char tpduBuf[MAX_TPDU_DATA_LEN];
    if (memset_s(tpduBuf, sizeof(tpduBuf), 0x00, sizeof(tpduBuf)) != EOK) {
        TELEPHONY_LOGE("memset_s error.");
        return nullptr;
    }

    uint16_t bufLen = 0;
    bool ret = tpduCodec->EncodeSmsPdu(smsTpdu_, tpduBuf, sizeof(tpduBuf), bufLen);
    if (ret && bufLen > 0 && info != nullptr) {
        if (encodeSmscLen > sizeof(info->smcaData_)) {
            TELEPHONY_LOGE("GetSubmitEncodeInfo data length invalid.");
            return nullptr;
        }
        if (memcpy_s(info->smcaData_, sizeof(info->smcaData_), encodeSmscAddr, encodeSmscLen) != EOK) {
            TELEPHONY_LOGE("GetSubmitEncodeInfo encodeSmscAddr memcpy_s error!");
            return nullptr;
        }
        if (memcpy_s(info->tpduData_, sizeof(info->tpduData_), tpduBuf, bufLen) != EOK) {
            TELEPHONY_LOGE("GetSubmitEncodeInfo memcpy_s error!");
            return nullptr;
        }
        info->smcaLen = encodeSmscLen;
        info->tpduLen = bufLen;
        info->isMore_ = bMore;
    }
    return info;
}

std::shared_ptr<struct SmsTpdu> GsmSmsMessage::CreateDeliverSmsTpdu()
{
    smsTpdu_ = std::make_shared<struct SmsTpdu>();
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("Make smsTpdu fail.");
        return smsTpdu_;
    }
    smsTpdu_->tpduType = SMS_TPDU_DELIVER;
    smsTpdu_->data.deliver.bHeaderInd = false;
    return smsTpdu_;
}

std::shared_ptr<struct SmsTpdu> GsmSmsMessage::CreateDeliverReportSmsTpdu()
{
    smsTpdu_ = std::make_shared<struct SmsTpdu>();
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("Make smsTpdu fail.");
        return smsTpdu_;
    }
    smsTpdu_->tpduType = SMS_TPDU_DELIVER_REP;
    smsTpdu_->data.deliverRep.bHeaderInd = false;
    smsTpdu_->data.deliverRep.paramInd = 0x00;
    return smsTpdu_;
}

std::shared_ptr<struct SmsTpdu> GsmSmsMessage::CreateStatusReportSmsTpdu()
{
    smsTpdu_ = std::make_shared<struct SmsTpdu>();
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("Make smsTpdu fail.");
        return smsTpdu_;
    }
    smsTpdu_->tpduType = SMS_TPDU_STATUS_REP;
    return smsTpdu_;
}

std::shared_ptr<GsmSmsMessage> GsmSmsMessage::CreateMessage(const std::string &pdu)
{
    std::shared_ptr<GsmSmsMessage> message = std::make_shared<GsmSmsMessage>();
    if (message == nullptr) {
        TELEPHONY_LOGE("Make message fail.");
        return message;
    }
    message->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    if (message->smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("Make smsTpdu fail.");
        return message;
    }
    (void)memset_s(message->smsTpdu_.get(), sizeof(struct SmsTpdu), 0x00, sizeof(struct SmsTpdu));
    std::string pduData = StringUtils::HexToString(pdu);
    message->rawPdu_ = StringUtils::HexToByteVector(pdu);
    if (message->PduAnalysis(pduData)) {
        return message;
    }
    return nullptr;
}

bool GsmSmsMessage::PduAnalysis(const string &pdu)
{
    if (smsTpdu_ == nullptr || pdu.empty() || pdu.length() > MAX_TPDU_DATA_LEN) {
        TELEPHONY_LOGE("GsmSmsMessage::PduAnalysis smsTpdu is null");
        return false;
    }
    struct AddressNumber smsc;
    if (memset_s(&smsc, sizeof(struct AddressNumber), 0x00, sizeof(struct AddressNumber)) != EOK) {
        TELEPHONY_LOGE("PduAnalysis memset_s error!");
        return false;
    }
    GsmSmsParamCodec codec;
    uint8_t smscLen = codec.DecodeSmscPdu(reinterpret_cast<const uint8_t *>(pdu.c_str()), pdu.length(), smsc);
    if (smscLen >= pdu.length()) {
        TELEPHONY_LOGE("PduAnalysis pdu is invalid!");
        return false;
    } else if (smscLen > 0) {
        scAddress_ = smsc.address;
    }
    uint8_t tempPdu[TAPI_TEXT_SIZE_MAX + 1] = { 0 };
    if (sizeof(tempPdu) + smscLen < pdu.length()) {
        TELEPHONY_LOGE("pdu length exceed maxinum");
        return false;
    }
    if (memcpy_s(tempPdu, sizeof(tempPdu), (pdu.c_str() + smscLen), pdu.length() - smscLen) != EOK) {
        TELEPHONY_LOGE("PduAnalysis memset_s error!");
        return false;
    }
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    if (!tpduCodec->DecodeSmsPdu(tempPdu, pdu.length() - smscLen, smsTpdu_.get())) {
        TELEPHONY_LOGE("DecodeSmsPdu fail");
        return false;
    }
    return PduAnalysisMsg();
}

bool GsmSmsMessage::PduAnalysisMsg()
{
    bool result = true;
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_DELIVER:
            AnalysisMsgDeliver(smsTpdu_->data.deliver);
            break;
        case SMS_TPDU_STATUS_REP:
            AnalysisMsgStatusReport(smsTpdu_->data.statusRep);
            break;
        case SMS_TPDU_SUBMIT:
            AnalysisMsgSubmit(smsTpdu_->data.submit);
            break;
        default:
            TELEPHONY_LOGE("tpduType is unknown.");
            result = false;
            break;
    }
    return result;
}

void GsmSmsMessage::AnalysisMsgDeliver(const SmsDeliver &deliver)
{
    protocolId_ = (int)(deliver.pid);
    hasReplyPath_ = deliver.bReplyPath;
    bStatusReportMessage_ = deliver.bStatusReport;
    bMoreMsg_ = deliver.bMoreMsg;
    bHeaderInd_ = deliver.bHeaderInd;
    originatingAddress_ = deliver.originAddress.address;
    headerCnt_ = deliver.userData.headerCnt;
    ConvertMsgTimeStamp(deliver.timeStamp);
    ConvertMessageDcs();
    ConvertUserData();
}

void GsmSmsMessage::AnalysisMsgStatusReport(const SmsStatusReport &statusRep)
{
    protocolId_ = (int)(statusRep.pid);
    msgRef_ = statusRep.msgRef;
    bMoreMsg_ = statusRep.bMoreMsg;
    bStatusReportMessage_ = statusRep.bStatusReport;
    bHeaderInd_ = statusRep.bHeaderInd;
    status_ = statusRep.status;
    ConvertMsgTimeStamp(statusRep.timeStamp);
    ConvertMessageDcs();
    ConvertUserData();
}

void GsmSmsMessage::AnalysisMsgSubmit(const SmsSubmit &submit)
{
    protocolId_ = static_cast<int>(submit.pid);
    hasReplyPath_ = submit.bReplyPath;
    msgRef_ = submit.msgRef;
    bStatusReportMessage_ = submit.bStatusReport;
    bHeaderInd_ = submit.bHeaderInd;
    ConvertMsgTimeStamp(submit.validityPeriod);
    ConvertMessageDcs();
    ConvertUserData();
}

void GsmSmsMessage::ConvertMessageDcs()
{
    if (smsTpdu_ == nullptr) {
        TELEPHONY_LOGE("GsmSmsMessage::ConvertMessageDcs smsTpdu is null");
        return;
    }
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_DELIVER:
            bCompressed_ = smsTpdu_->data.deliver.dcs.bCompressed;
            codingScheme_ = smsTpdu_->data.deliver.dcs.codingScheme;
            codingGroup_ = smsTpdu_->data.deliver.dcs.codingGroup;
            bIndActive_ = smsTpdu_->data.deliver.dcs.bIndActive;
            bMwi_ = smsTpdu_->data.deliver.dcs.bMWI;
            bMwiSense_ = smsTpdu_->data.deliver.dcs.bIndActive; /* Indicates vmail notification set/clear */
            ConvertMessageClass(smsTpdu_->data.deliver.dcs.msgClass);
            break;
        case SMS_TPDU_STATUS_REP:
            bCompressed_ = smsTpdu_->data.statusRep.dcs.bCompressed;
            codingScheme_ = smsTpdu_->data.statusRep.dcs.codingScheme;
            codingGroup_ = smsTpdu_->data.statusRep.dcs.codingGroup;
            bIndActive_ = smsTpdu_->data.statusRep.dcs.bIndActive;
            ConvertMessageClass(smsTpdu_->data.statusRep.dcs.msgClass);
            break;
        case SMS_TPDU_SUBMIT:
            bCompressed_ = smsTpdu_->data.submit.dcs.bCompressed;
            codingScheme_ = smsTpdu_->data.submit.dcs.codingScheme;
            codingGroup_ = smsTpdu_->data.submit.dcs.codingGroup;
            bIndActive_ = smsTpdu_->data.submit.dcs.bIndActive;
            bMwi_ = smsTpdu_->data.submit.dcs.bMWI;
            bMwiSense_ = smsTpdu_->data.submit.dcs.bIndActive;
            ConvertMessageClass(smsTpdu_->data.submit.dcs.msgClass);
            break;
        default:
            break;
    }
}

void GsmSmsMessage::ConvertUserData()
{
    int ret = 0;
    if (smsTpdu_ == nullptr ||
        (memset_s(&smsUserData_, sizeof(struct SmsUDPackage), 0x00, sizeof(struct SmsUDPackage)) != EOK)) {
        TELEPHONY_LOGE("nullptr or memset_s error.");
        return;
    }
    size_t udLen = sizeof(SmsUDPackage);
    size_t tpduLen = sizeof(SmsTpud);
    switch (smsTpdu_->tpduType) {
        case SMS_TPDU_DELIVER:
            headerDataLen_ = smsTpdu_->data.deliver.userData.length;
            ret = memcpy_s(&smsUserData_, udLen, &(smsTpdu_->data.deliver.userData), udLen);
            if (ret == EOK) {
                ret = memcpy_s(&smsWapPushUserData_, tpduLen, &(smsTpdu_->data.deliver.udData), tpduLen);
            }
            break;
        case SMS_TPDU_STATUS_REP:
            headerDataLen_ = smsTpdu_->data.statusRep.userData.length;
            ret = memcpy_s(&smsUserData_, udLen, &(smsTpdu_->data.statusRep.userData), udLen);
            break;
        case SMS_TPDU_SUBMIT:
            headerDataLen_ = smsTpdu_->data.submit.userData.length;
            ret = memcpy_s(&smsUserData_, udLen, &(smsTpdu_->data.submit.userData), udLen);
            break;
        default:
            break;
    }
    if (ret != EOK) {
        TELEPHONY_LOGE("memset_s error.");
        return;
    }
    ConvertUserPartData();
}

/*
 * 3GPP TS 23.040 9.2.3.24.15 / 9.2.3.24.16: the national language tables are selected by the single shift
 * IE (0x24) and the locking shift IE (0x25) wherever they appear in the UDH; the last occurrence applies.
 * Locking shift is only applied for the tables TextCoder implements, others fall back to the default alphabet.
 */
MsgLangInfo GsmSmsMessage::GetNationalLangInfo() const
{
    MsgLangInfo langInfo;
    for (uint8_t i = 0; i < smsUserData_.headerCnt && i < MAX_UD_HEADER_NUM; i++) {
        const struct SmsUDH &header = smsUserData_.header[i];
        if (header.udhType == UDH_SINGLE_SHIFT) {
            langInfo.bSingleShift = true;
            langInfo.singleLang = header.udh.singleShift.langId;
        } else if (header.udhType == UDH_LOCKING_SHIFT) {
            bool supported = header.udh.lockingShift.langId == MSG_ID_TURKISH_LANG ||
                header.udh.lockingShift.langId == MSG_ID_PORTUGUESE_LANG;
            langInfo.bLockingShift = supported;
            langInfo.lockingLang =
                supported ? header.udh.lockingShift.langId : static_cast<MSG_LANGUAGE_ID_T>(MSG_ID_RESERVED_LANG);
        }
    }
    return langInfo;
}

void GsmSmsMessage::ConvertUserPartData()
{
    if (smsUserData_.length == 0) {
        TELEPHONY_LOGE("user data length error.");
        return;
    }
    uint8_t buff[MAX_MSG_TEXT_LEN + 1] = { 0 };
    if (codingScheme_ == DATA_CODING_7BIT) {
        MsgLangInfo langInfo = GetNationalLangInfo();
        int dataSize = TextCoder::Instance().Gsm7bitToUtf8(
            buff, MAX_MSG_TEXT_LEN, reinterpret_cast<uint8_t *>(smsUserData_.data), smsUserData_.length, langInfo);
        visibleMessageBody_.insert(0, reinterpret_cast<char *>(buff), dataSize);
    } else if (codingScheme_ == DATA_CODING_UCS2) {
        int dataSize = TextCoder::Instance().Ucs2ToUtf8(
            buff, MAX_MSG_TEXT_LEN, reinterpret_cast<uint8_t *>(smsUserData_.data), smsUserData_.length);
        visibleMessageBody_.insert(0, reinterpret_cast<char *>(buff), dataSize);
    } else if (codingScheme_ == DATA_CODING_8BIT) {
        visibleMessageBody_.insert(0, static_cast<char *>(smsUserData_.data), smsUserData_.length);
    } else if (codingScheme_ == DATA_CODING_EUCKR) {
        int dataSize = TextCoder::Instance().EuckrToUtf8(
            buff, MAX_MSG_TEXT_LEN, reinterpret_cast<uint8_t *>(smsUserData_.data), smsUserData_.length);
        visibleMessageBody_.insert(0, reinterpret_cast<char *>(buff), dataSize);
    }
    rawUserData_.insert(0, static_cast<char *>(smsUserData_.data), smsUserData_.length);
    rawWapPushUserData_.insert(0, smsWapPushUserData_.ud, smsWapPushUserData_.udl);

    if (!visibleMessageBody_.empty()) {
        ParseEmailFromMessageBody();
    }
}

void GsmSmsMessage::SetFullText(const std::string &text)
{
    fullText_ = text;
}

void GsmSmsMessage::SetDestAddress(const std::string &address)
{
    destAddress_ = address;
}

void GsmSmsMessage::SetDestPort(uint32_t port)
{
    destPort_ = port;
}

std::string GsmSmsMessage::GetFullText() const
{
    return fullText_;
}

std::string GsmSmsMessage::GetReplyAddress() const
{
    return replyAddress_;
}

std::string GsmSmsMessage::GetDestAddress() const
{
    return destAddress_;
}

uint16_t GsmSmsMessage::GetDestPort()
{
    std::shared_ptr<SmsAppPortAddr> portAddress = GetPortAddress();
    if (portAddress == nullptr) {
        TELEPHONY_LOGE("PortAddress is null!");
        return DEFAULT_PORT;
    }
    destPort_ = static_cast<uint16_t>(portAddress->destPort);
    return destPort_;
}

bool GsmSmsMessage::GetIsSmsText() const
{
    return bSmsText_;
}

bool GsmSmsMessage::GetGsm() const
{
    return true;
}

bool GsmSmsMessage::GetIsTypeZeroInd() const
{
    return (GetProtocolId() == 0x40);
}

bool GsmSmsMessage::GetIsSIMDataTypeDownload() const
{
    int protocolId = GetProtocolId();
    return GetMessageClass() == SMS_SIM_MESSAGE && (protocolId == 0x7f || protocolId == 0x7c);
}

void GsmSmsMessage::ConvertMsgTimeStamp(const struct SmsTimeStamp &times)
{
    if (times.format == SMS_TIME_ABSOLUTE) {
        scTimestamp_ = SmsCommonUtils::ConvertTime(times.time.absolute);
    } else {
        scTimestamp_ = time(nullptr);
    }
}

// from 3GPP TS 23.040 V5.1.0 9.2.3.24.2 Special SMS Message Indication
bool GsmSmsMessage::IsSpecialMessage() const
{
    bool result = false;
    if (GetIsTypeZeroInd()) {
        TELEPHONY_LOGI("GsmSmsMessage:: IsTypeZeroInd");
        result = true;
    }
    // 9.2.3.9 TP Protocol Identifier (TP PID)
    if (GetIsSIMDataTypeDownload()) {
        TELEPHONY_LOGI("GsmSmsMessage:: GetIsSIMDataTypeDownload");
        result = true;
    }
    if (IsMwiSet() || IsMwiClear()) {
        TELEPHONY_LOGI("GsmSmsMessage::Mwi Message");
        result = true;
    }
    return result;
}

void GsmSmsMessage::SetSmsCodingNationalType(SmsCodingNationalType smsCodingNationalType)
{
    smsCodingNationalType_ = smsCodingNationalType;
}

/*
 * 3GPP TS 23.040 9.2.3.16 / 9.2.3.24: TP-UD holds at most 140 octets (160 septets), UDHL and all
 * information elements included; with 7bit coding the UDH occupies ceil((UDHL + 1) * 8 / 7) septets.
 */
static int GetUserDataCapacity(DataCodingScheme codingScheme, int ieOctets)
{
    int udhOctets = (ieOctets > 0) ? (UDHL_OCTETS + ieOctets) : 0;
    switch (codingScheme) {
        case DATA_CODING_7BIT:
        case DATA_CODING_ASCII7BIT:
            return ((MAX_TP_UD_OCTETS - udhOctets) * OCTET_BITS) / GSM_7BIT_BITS;
        case DATA_CODING_8BIT:
            return MAX_TP_UD_OCTETS - udhOctets;
        case DATA_CODING_UCS2:
            return ((MAX_TP_UD_OCTETS - udhOctets) / UCS2_CHAR_OCTETS) * UCS2_CHAR_OCTETS;
        default:
            return 0;
    }
}

int GsmSmsMessage::GetSegmentSize(
    DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId) const
{
    // count the IEs the GSM senders add to every segment: 16bit port IE, national language single shift IE
    int ieOctets = 0;
    if (bPortNum) {
        ieOctets += APP_PORT_16BIT_IE_OCTETS;
    }
    if (codingScheme == DATA_CODING_7BIT && langId != MSG_ID_RESERVED_LANG) {
        ieOctets += SINGLE_SHIFT_IE_OCTETS;
    }
    if (dataLen <= GetUserDataCapacity(codingScheme, ieOctets)) {
        return dataLen;
    }
    // more than one segment: every segment also carries the 8bit concatenation IE
    return GetUserDataCapacity(codingScheme, ieOctets + CONCAT_8BIT_IE_OCTETS);
}

int GsmSmsMessage::DecodeMessage(uint8_t *decodeData, unsigned int len, DataCodingScheme &codingType,
    const std::string &msgText, bool &bAbnormal, MSG_LANGUAGE_ID_T &langId)
{
    int decodeLen = 0;
    int dataLen = static_cast<int>(msgText.length());
    const unsigned int maxDecodeLen = len;
    const uint8_t *pMsgText = reinterpret_cast<const uint8_t *>(msgText.c_str());

    if (msgText.empty()) {
        TELEPHONY_LOGE("msgText empty.");
        return decodeLen;
    }

    switch (codingType) {
        case DATA_CODING_7BIT: {
            decodeLen = TextCoder::Instance().Utf8ToGsm7bit(
                decodeData, maxDecodeLen, const_cast<uint8_t *>(pMsgText), dataLen, langId);
            break;
        }
        case DATA_CODING_8BIT: {
            if (static_cast<unsigned int>(dataLen) > maxDecodeLen) {
                TELEPHONY_LOGE("DecodeMessage data length invalid.");
                return decodeLen;
            }
            if (memcpy_s(decodeData, maxDecodeLen, pMsgText, dataLen) != EOK) {
                TELEPHONY_LOGE("SplitMessage SMS_CHARSET_8BIT memcpy_s error!");
                return decodeLen;
            }
            decodeLen = dataLen;
            break;
        }
        case DATA_CODING_UCS2: {
            decodeLen = TextCoder::Instance().Utf8ToUcs2(decodeData, maxDecodeLen, pMsgText, dataLen);
            break;
        }
        case DATA_CODING_AUTO:
        default: {
            DataCodingScheme encodeType = DATA_CODING_AUTO;
            decodeLen = TextCoder::Instance().GsmUtf8ToAuto(decodeData, maxDecodeLen, pMsgText, dataLen,
                encodeType, smsCodingNationalType_, langId);
            codingType = encodeType;
            break;
        }
    }
    TELEPHONY_LOGI("DecodeMessage, message coding type is %{public}d", codingType);
    return decodeLen;
}

void GsmSmsMessage::ParseEmailFromMessageBody()
{
    if (originatingAddress_.empty()) {
        return;
    }
    if (originatingAddress_.length() > ADDRESS_LEN) {
        return;
    }
    std::regex re("( /)|( )");
    std::smatch match;
    if (visibleMessageBody_.empty() || !std::regex_search(visibleMessageBody_, match, re)) {
        return;
    }
    size_t pos = static_cast<size_t>(match.position());
    emailFrom_ = visibleMessageBody_.substr(0, pos);
    std::string emailBody = visibleMessageBody_.substr(pos + 1);
    if (!emailBody.empty() && emailBody.front() == '/') {
        emailBody = emailBody.substr(1);
    }
    emailBody_ = emailBody;
    isEmail_ = IsEmailAddress(emailFrom_);
}

bool GsmSmsMessage::IsEmailAddress(std::string emailFrom)
{
    if (emailFrom.empty()) {
        return false;
    }
    std::regex nameAddrEmailRegex(g_nameAddrEmailRegex);
    std::smatch match;
    std::string emailAddress;
    if (std::regex_search(emailFrom, match, nameAddrEmailRegex)) {
        emailAddress = match[MATCH_TWO].str();
    } else {
        emailAddress = emailFrom;
    }
    std::regex addrEmailRegex(g_addrEmailRegex);
    if (emailAddress.empty()) {
        return false;
    }
    return std::regex_match(emailAddress, addrEmailRegex);
}
} // namespace Telephony
} // namespace OHOS