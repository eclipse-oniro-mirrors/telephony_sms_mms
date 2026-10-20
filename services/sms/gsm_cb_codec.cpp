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

#include "cdma_sms_common.h"
#include "gsm_cb_gsm_codec.h"
#include "gsm_cb_umts_codec.h"
#include "gsm_pdu_hex_value.h"
#include "securec.h"
#include "sms_common_utils.h"
#include "string_utils.h"
#include "telephony_ext_wrapper.h"
#include "telephony_log_wrapper.h"
#include "text_coder.h"

namespace OHOS {
namespace Telephony {
static constexpr uint8_t SMS_BYTE_BIT = 8;
static constexpr uint8_t GSM_CB_HEADER_LEN = 6;
static constexpr uint16_t MAX_CB_MSG_LEN = 4200;
static constexpr uint8_t MAX_PAGE_PDU_LEN = 82;
static constexpr uint8_t CB_FORMAT_3GPP = 1;
static constexpr uint16_t OTHER_TYPE = -2;
static constexpr uint16_t ETWS_TYPE = 0x1100;
static constexpr uint16_t ETWS_TYPE_MASK = 0xFFF8;
static constexpr uint16_t CMAS_FIRST_ID = 0x1112;
static constexpr uint16_t CMAS_LAST_ID = 0x112F;
static constexpr uint16_t EMERGENCY_USER_ALERT = 0x2000;
static constexpr uint16_t ETWS_POPUP = 0x1000;
static constexpr uint16_t PWS_FIRST_ID = 0x1100;
static constexpr uint16_t PWS_LAST_ID = 0x18FF;

bool GsmCbCodec::operator==(const GsmCbCodec &codec) const
{
    if (cbHeader_ == nullptr || codec.cbHeader_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    return cbHeader_->serialNum.geoScope == codec.cbHeader_->serialNum.geoScope &&
           cbHeader_->serialNum.msgCode == codec.cbHeader_->serialNum.msgCode &&
           cbHeader_->msgId == codec.cbHeader_->msgId;
}

std::shared_ptr<GsmCbCodec> GsmCbCodec::CreateCbMessage(const std::string &pdu)
{
    bool result = false;
    std::shared_ptr<GsmCbCodec> message = std::make_shared<GsmCbCodec>();
    if (message == nullptr) {
        TELEPHONY_LOGE("message is nullptr.");
        return nullptr;
    }
    result = message->PduAnalysis(StringUtils::HexToByteVector(pdu));
    return (result ? message : nullptr);
}

std::shared_ptr<GsmCbCodec> GsmCbCodec::CreateCbMessage(const std::vector<unsigned char> &pdu)
{
    bool result = false;
    std::shared_ptr<GsmCbCodec> message = std::make_shared<GsmCbCodec>();
    if (message == nullptr) {
        TELEPHONY_LOGE("message is nullptr.");
        return nullptr;
    }
    result = message->PduAnalysis(pdu);
    return (result ? message : nullptr);
}

std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> GsmCbCodec::GetCbHeader() const
{
    return cbHeader_;
}

std::string GsmCbCodec::GetCbMessageRaw() const
{
    return messageRaw_;
}

void GsmCbCodec::SetCbMessageRaw(std::string &raw)
{
    messageRaw_ = raw;
}

bool GsmCbCodec::IsSinglePageMsg() const
{
    return cbHeader_ != nullptr && (cbHeader_->totalPages == 1);
}

/**
 * refer to 3GPP TS 23.041 V4.1.0 9.1.1 9.1.2 section Protocols and Protocol Architecture
 * refer to 3GPP TS 23.041 V4.1.0 9.3 Parameters
 */
bool GsmCbCodec::PduAnalysis(const std::vector<unsigned char> &pdu)
{
    if (!ParamsCheck(pdu)) {
        TELEPHONY_LOGE("params error.");
        return false;
    }
    auto gsmCodec = std::make_shared<GsmCbGsmCodec>(cbHeader_, cbPduBuffer_, shared_from_this());
    auto umtsCodec = std::make_shared<GsmCbUmtsCodec>(cbHeader_, cbPduBuffer_, shared_from_this());
    if (gsmCodec == nullptr || umtsCodec == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    // refer to 3GPP TS 23.041 V4.1.0 9.1.1 9.1.2 section Protocols and Protocol Architecture
    bool decodeResult = false;
    if (pdu.size() <= (MAX_PAGE_PDU_LEN + GSM_CB_HEADER_LEN)) {
        cbHeader_->cbMsgType = GSM_CBS;
        cbHeader_->cbNetType = GSM_NET_CB;
        decodeResult = gsmCodec->Decode2gHeader();
    } else if (pdu.size() <= MAX_CB_MSG_LEN) {
        uint8_t oneByte = 0;
        if (!cbPduBuffer_->PickOneByte(oneByte)) {
            TELEPHONY_LOGE("get data error.");
            return false;
        }
        cbHeader_->cbMsgType = oneByte;
        cbHeader_->cbNetType = UMTS_NET_CB;
        decodeResult = umtsCodec->Decode3gHeader();
    } else {
        TELEPHONY_LOGE("pdu size over max.");
        return false;
    }

    if (!decodeResult || cbPduBuffer_->GetCurPosition() >= pdu.size()) {
        TELEPHONY_LOGE("CB decode head fail.");
        return false;
    }
    if (cbHeader_->bEtwsMessage && cbHeader_->cbEtwsType == ETWS_PRIMARY) {
        PduAfterDiscardNullBlock(pdu);
        return gsmCodec->DecodeEtwsMsg();
    }

    if (cbHeader_->cbNetType == GSM_NET_CB) {
        PduAfterDiscardNullBlock(pdu);
        decodeResult = gsmCodec->Decode2gCbMsg();
    } else if (cbHeader_->cbNetType == UMTS_NET_CB) {
        decodeResult = umtsCodec->Decode3gCbMsg();
    }
    TELEPHONY_LOGI("CB decode result:%{public}d.", decodeResult);
    return decodeResult;
}

void GsmCbCodec::PduAfterDiscardNullBlock(const std::vector<unsigned char> &pdu)
{
    if (cbPduBuffer_ == nullptr) {
        TELEPHONY_LOGE("cbPduBuffer_ nullptr error");
        return;
    }
    if (TELEPHONY_EXT_WRAPPER.getCbsPduLength_ != nullptr) {
        uint32_t len = TELEPHONY_EXT_WRAPPER.getCbsPduLength_(pdu);
        cbPduBuffer_ -> SetSize(len);
    }
}

bool GsmCbCodec::ParamsCheck(const std::vector<unsigned char> &pdu)
{
    if (pdu.size() == 0 || pdu.size() > MAX_CB_MSG_LEN) {
        TELEPHONY_LOGE("pdu data error.");
        return false;
    }
    cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbPduBuffer_->pduBuffer_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    for (size_t index = 0; index < pdu.size() && index < cbPduBuffer_->GetSize(); index++) {
        cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    return true;
}

void GsmCbCodec::GetPduData(std::vector<unsigned char> &dataPdu)
{
    if (cbPduBuffer_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return;
    }

    uint32_t current = cbPduBuffer_->GetCurPosition();
    uint8_t oneByte = 0;
    while (cbPduBuffer_->GetCurPosition() < cbPduBuffer_->GetSize()) {
        if (!cbPduBuffer_->GetOneByte(oneByte)) {
            TELEPHONY_LOGE("get data error.");
        }
        dataPdu.push_back(oneByte);
    }
    cbPduBuffer_->SetPointer(current);
}

void GsmCbCodec::ConvertToUTF8(const std::string &raw, std::string &message) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("ConvertToUTF8 cbHeader null err.");
        return;
    }
    if (cbHeader_->bEtwsMessage && cbHeader_->cbEtwsType == ETWS_PRIMARY) {
        message.assign(raw);
    } else {
        uint16_t codeSize = 0;
        MsgLangInfo langInfo;
        uint8_t outBuf[MAX_CB_MSG_LEN + 1] = { 0 };
        const uint8_t *src = reinterpret_cast<const uint8_t *>(raw.data());
        if (cbHeader_->dcs.codingScheme == DATA_CODING_7BIT) {
            codeSize = TextCoder::Instance().Gsm7bitToUtf8(outBuf, sizeof(outBuf), src, raw.length(), langInfo);
        } else if (cbHeader_->dcs.codingScheme == DATA_CODING_UCS2) {
            codeSize = TextCoder::Instance().Ucs2ToUtf8(outBuf, sizeof(outBuf), src, raw.length());
        } else {
            TELEPHONY_LOGI("CB message data encoding 8bit");
            message.assign(raw);
            return;
        }
        if (codeSize >= MAX_CB_MSG_LEN) {
            TELEPHONY_LOGE("codeSize over size.");
            return;
        }
        outBuf[codeSize] = '\0';
        for (uint16_t index = 0; index < codeSize; index++) {
            message.push_back(static_cast<char>(outBuf[index]));
        }
    }
}

unsigned short GsmCbCodec::EncodeCbSerialNum(const GsmCBMsgSerialNum &snFields)
{
    unsigned short serialNum = ((snFields.geoScope & HEX_VALUE_03) << HEX_VALUE_0E) |
                               ((snFields.msgCode & HEX_VALUE_03FF) << HEX_VALUE_04) |
                               (snFields.updateNum & HEX_VALUE_0F);
    return serialNum;
}

uint8_t GsmCbCodec::CMASClass(const uint16_t messageId) const
{
    uint8_t ret = 0;
    switch (messageId) {
        case PRESIDENTIAL:
        case PRESIDENTIAL_SPANISH:
            ret = CMAS_PRESIDENTIAL;
            break;
        case EXTREME_OBSERVED:
        case EXTREME_OBSERVED_SPANISH:
        case EXTREME_LIKELY:
        case EXTREME_LIKELY_SPANISH:
            ret = CMAS_EXTREME;
            break;
        case SEVERE_OBSERVED:
        case SEVERE_OBSERVED_SPANISH:
        case SEVERE_LIKELY:
        case SEVERE_LIKELY_SPANISH:
        case ALERT_OBSERVED_DEFUALT:
        case ALERT_OBSERVED_SPANISH:
        case ALERT_LIKELY:
        case ALERT_LIKELY_SPANISH:
        case EXPECTED_OBSERVED:
        case EXPECTED_OBSERVED_SPANISH:
        case EXPECTED_LIKELY:
        case EXPECTED_LIKELY_SPANISH:
            ret = CMAS_SEVERE;
            break;
        case AMBER_ALERT:
        case AMBER_ALERT_SPANISH:
            ret = CMAS_AMBER;
            break;
        case MONTHLY_ALERT:
        case MONTHLY_ALERT_SPANISH:
            ret = CMAS_TEST;
            break;
        case EXERCISE_ALERT:
        case EXERCISE_ALERT_SPANISH:
            ret = CMAS_EXERCISE;
            break;
        case OPERATOR_ALERT:
        case OPERATOR_ALERT_SPANISH:
            ret = CMAS_OPERATOR_DEFINED;
            break;
        default:
            break;
    }
    return ret;
}

/**
 *  refer to 3GPP TS 23.038 V4.3.0 6.1.1 section
 */
void GsmCbCodec::DecodeIos639Dcs(const uint8_t dcsData, const unsigned short iosData, GsmCbMsgDcs &dcs) const
{
    uint8_t dcsLow = (dcsData & HEX_VALUE_0F);
    switch (dcsLow) {
        case 0x00:
        case HEX_VALUE_01: {
            dcs.codingGroup = SMS_CBMSG_CODGRP_GENERAL_DCS;
            dcs.codingScheme = (dcsData & HEX_VALUE_01) ? DATA_CODING_UCS2 : DATA_CODING_7BIT;
            dcs.langType = CB_LANG_ISO639;
            uint8_t hight = (iosData >> SMS_BYTE_BIT);
            uint8_t low = iosData;

            // refer to 3GPP TS 23.038 V4.3.0 6.1.1 section Control characters
            if (hight && low) {
                dcs.iso639Lang[0x00] = hight & HEX_VALUE_7F;
                dcs.iso639Lang[HEX_VALUE_01] = (hight & HEX_VALUE_80) >> HEX_VALUE_07;
                dcs.iso639Lang[HEX_VALUE_01] |= (low & HEX_VALUE_3F) << HEX_VALUE_01;
                dcs.iso639Lang[HEX_VALUE_02] = HEX_VALUE_13; /* CR */
            } else {
                dcs.iso639Lang[0x00] = HEX_VALUE_45; /* E */
                dcs.iso639Lang[HEX_VALUE_01] = HEX_VALUE_4E; /* N */
                dcs.iso639Lang[HEX_VALUE_02] = HEX_VALUE_13; /* CR */
            }
            break;
        }
        default:
            break;
    }
}

/**
 *  refer to 3GPP TS 23.038 V4.3.0 6 section
 */
void GsmCbCodec::DecodeGeneralDcs(const uint8_t dcsData, GsmCbMsgDcs &dcs) const
{
    dcs.codingGroup = SMS_CBMSG_CODGRP_GENERAL_DCS;
    dcs.bCompressed = (dcsData & HEX_VALUE_20) ? true : false;
    if (dcsData & HEX_VALUE_10) {
        dcs.classType = (dcsData & HEX_VALUE_03);
    }
    uint8_t tmpScheme = (dcsData & HEX_VALUE_0C) >> HEX_VALUE_02;
    switch (tmpScheme) {
        case 0x00:
            dcs.codingScheme = DATA_CODING_7BIT;
            break;
        case HEX_VALUE_01:
            dcs.codingScheme = DATA_CODING_8BIT;
            break;
        case HEX_VALUE_02:
            dcs.codingScheme = DATA_CODING_UCS2;
            break;
        default:
            break;
    }
}

/**
 *  refer to 3GPP TS 23.038 V4.3.0 6 section
 */
void GsmCbCodec::DecodeCbMsgDCS(const uint8_t dcsData, const unsigned short iosData, GsmCbMsgDcs &dcs) const
{
    dcs.codingGroup = SMS_CBMSG_CODGRP_GENERAL_DCS;
    dcs.classType = SMS_CLASS_UNKNOWN;
    dcs.bCompressed = false;
    dcs.codingScheme = DATA_CODING_7BIT;
    dcs.langType = CB_MSG_UNSPECIFIED;
    if (memset_s(dcs.iso639Lang, sizeof(dcs.iso639Lang), 0x00, sizeof(dcs.iso639Lang)) != EOK) {
        TELEPHONY_LOGE("memset_s error!");
        return;
    }
    dcs.bUDH = false;
    dcs.rawData = dcsData;
    uint8_t codingGroup = (dcsData & HEX_VALUE_F0) >> HEX_VALUE_04;
    switch (codingGroup) {
        case 0x00:
        case HEX_VALUE_02:
        case HEX_VALUE_03:
            dcs.codingGroup = SMS_CBMSG_CODGRP_GENERAL_DCS;
            dcs.langType = dcsData;
            break;
        case HEX_VALUE_01:
            DecodeIos639Dcs(dcsData, iosData, dcs);
            break;
        case HEX_VALUE_04:
        case HEX_VALUE_05:
        case HEX_VALUE_06:
        case HEX_VALUE_07:
            DecodeGeneralDcs(dcsData, dcs);
            break;
        case HEX_VALUE_09:
            dcs.bUDH = true;
            dcs.classType = dcsData & HEX_VALUE_03;
            dcs.codingScheme = (dcsData & HEX_VALUE_0C) >> HEX_VALUE_02;
            break;
        case HEX_VALUE_0E:
            dcs.codingGroup = SMS_CBMSG_CODGRP_WAP;
            break;
        case HEX_VALUE_0F:
            dcs.codingGroup = SMS_CBMSG_CODGRP_CLASS_CODING;
            dcs.codingScheme = (dcsData & HEX_VALUE_04) ? DATA_CODING_8BIT : DATA_CODING_7BIT;
            dcs.classType = dcsData & HEX_VALUE_03;
            break;
        default:
            TELEPHONY_LOGI("codingGrp: [0x%{public}x]", codingGroup);
            break;
    }
}

int64_t GsmCbCodec::GetRecvTime() const
{
    time_t recvTime = time(NULL);
    return static_cast<int64_t>(recvTime);
}

std::string GsmCbCodec::ToString() const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return "GsmCbCodec Header nullptr";
    }
    std::string cbMsgId("msgId:" + std::to_string(cbHeader_->msgId));
    std::string cbLangType("\nlangType:" + std::to_string(cbHeader_->langType));
    std::string cbIsEtwsMessage("\nisEtws:" + std::to_string(cbHeader_->bEtwsMessage));
    std::string cbMsgType("\ncbMsgType:" + std::to_string(cbHeader_->cbMsgType));
    std::string cbWarningType("\nwarningType:" + std::to_string(cbHeader_->warningType));
    std::string cbSerialNum("\nserialNum: geoScope " + std::to_string(cbHeader_->serialNum.geoScope) + "| updateNum " +
                            std::to_string(cbHeader_->serialNum.updateNum) + "| msgCode " +
                            std::to_string(cbHeader_->serialNum.msgCode));
    std::string cbPage(
        "\ntotal pages: " + std::to_string(cbHeader_->totalPages) + " page:" + std::to_string(cbHeader_->page));
    std::string cbRecvTime("\nrecv time: " + std::to_string(cbHeader_->recvTime));
    std::string cbDcsRaw("\ndcs: " + std::to_string(cbHeader_->dcs.rawData));
    std::string cbMsgBody;
    ConvertToUTF8(messageRaw_, cbMsgBody);
    return cbMsgId.append(cbLangType)
        .append(cbIsEtwsMessage)
        .append(cbMsgType)
        .append(cbWarningType)
        .append(cbSerialNum)
        .append(cbPage)
        .append(cbRecvTime)
        .append(cbDcsRaw)
        .append("\nbody:")
        .append(cbMsgBody);
}

bool GsmCbCodec::GetFormat(int8_t &cbFormat) const
{
    cbFormat = CB_FORMAT_3GPP;
    return true;
}

bool GsmCbCodec::GetPriority(int8_t &cbPriority) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    const int8_t normal = 0x00;
    const int8_t emergency = HEX_VALUE_03;
    if (cbHeader_->msgId >= PWS_FIRST_ID && cbHeader_->msgId <= PWS_LAST_ID) {
        cbPriority = emergency;
    } else {
        cbPriority = normal;
    }
    return true;
}

bool GsmCbCodec::GetGeoScope(uint8_t &geoScope) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    geoScope = cbHeader_->serialNum.geoScope;
    return true;
}

bool GsmCbCodec::GetSerialNum(uint16_t &cbSerial) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    cbSerial = ((cbHeader_->serialNum.geoScope & HEX_VALUE_03) << HEX_VALUE_0E) |
               ((cbHeader_->serialNum.msgCode & HEX_VALUE_03FF) << HEX_VALUE_04) |
               (cbHeader_->serialNum.updateNum & HEX_VALUE_0F);
    return true;
}

bool GsmCbCodec::GetServiceCategory(uint16_t &cbCategoty) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    cbCategoty = cbHeader_->msgId;
    return true;
}

__attribute__((no_sanitize("cfi")))
bool GsmCbCodec::GetWarningType(uint16_t &type) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    if (TELEPHONY_EXT_WRAPPER.getCustEtwsType_  != nullptr) {
        TELEPHONY_LOGI("GetWarningType getCustEtwsType_  not null");
        TELEPHONY_EXT_WRAPPER.getCustEtwsType_(cbHeader_->msgId, type);
        if (type != OTHER_TYPE) {
            TELEPHONY_LOGI("getCustEtwsType_ type not OTHER_TYPE.");
            return true;
        }
    }
    type = cbHeader_->msgId - ETWS_TYPE;
    return true;
}

bool GsmCbCodec::IsEtwsPrimary(bool &primary) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    primary = (cbHeader_->cbEtwsType == ETWS_PRIMARY);
    return true;
}

__attribute__((no_sanitize("cfi")))
bool GsmCbCodec::IsEtwsMessage(bool &etws) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    etws = ((cbHeader_->msgId & ETWS_TYPE_MASK) == ETWS_TYPE);
    if (!etws && TELEPHONY_EXT_WRAPPER.isCustEtwsMessage_  != nullptr) {
        TELEPHONY_LOGI("IsEtwsMessage isCustEtwsMessage_  not null");
        if (TELEPHONY_EXT_WRAPPER.isCustEtwsMessage_(cbHeader_->msgId)) {
            TELEPHONY_LOGI("isCustEtwsMessage_ is true.");
            etws = true;
        }
    }
    return true;
}

bool GsmCbCodec::IsCmasMessage(bool &cmas) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    cmas = ((cbHeader_->msgId >= CMAS_FIRST_ID) && (cbHeader_->msgId <= CMAS_LAST_ID));
    return true;
}

bool GsmCbCodec::IsEtwsEmergencyUserAlert(bool &alert) const
{
    uint16_t serial = 0;
    if (!GetSerialNum(serial)) {
        TELEPHONY_LOGE("Get serial num fail.");
        return false;
    }
    alert = ((serial & EMERGENCY_USER_ALERT) != 0);
    return true;
}

bool GsmCbCodec::IsEtwsPopupAlert(bool &alert) const
{
    uint16_t serial = 0;
    if (!GetSerialNum(serial)) {
        TELEPHONY_LOGE("Get serial num fail.");
        return false;
    }
    alert = ((serial & ETWS_POPUP) != 0);
    return true;
}

bool GsmCbCodec::GetCmasSeverity(uint8_t &severity) const
{
    uint16_t msgId = 0;
    if (!GetMessageId(msgId)) {
        TELEPHONY_LOGE("Get message id fail.");
        return false;
    }
    switch (static_cast<CmasMsgType>(msgId)) {
        case CmasMsgType::EXTREME_OBSERVED:
        case CmasMsgType::EXTREME_OBSERVED_SPANISH:
        case CmasMsgType::EXTREME_LIKELY:
        case CmasMsgType::EXTREME_LIKELY_SPANISH:
        case CmasMsgType::SEVERE_OBSERVED:
        case CmasMsgType::SEVERE_OBSERVED_SPANISH:
        case CmasMsgType::SEVERE_LIKELY:
        case CmasMsgType::SEVERE_LIKELY_SPANISH:
            severity = static_cast<uint8_t>(SmsCmaeSeverity::EXTREME);
            break;
        case CmasMsgType::ALERT_OBSERVED_DEFUALT:
        case CmasMsgType::ALERT_OBSERVED_SPANISH:
        case CmasMsgType::ALERT_LIKELY:
        case CmasMsgType::ALERT_LIKELY_SPANISH:
        case CmasMsgType::EXPECTED_OBSERVED:
        case CmasMsgType::EXPECTED_OBSERVED_SPANISH:
        case CmasMsgType::EXPECTED_LIKELY:
        case CmasMsgType::EXPECTED_LIKELY_SPANISH:
            severity = static_cast<uint8_t>(SmsCmaeSeverity::SEVERE);
            break;
        default:
            severity = static_cast<uint8_t>(SmsCmaeSeverity::RESERVED);
            break;
    }
    return true;
}

bool GsmCbCodec::GetCmasUrgency(uint8_t &urgency) const
{
    uint16_t msgId = 0;
    if (!GetMessageId(msgId)) {
        TELEPHONY_LOGE("Get message id fail.");
        return false;
    }
    switch (static_cast<CmasMsgType>(msgId)) {
        case CmasMsgType::EXTREME_OBSERVED:
        case CmasMsgType::EXTREME_OBSERVED_SPANISH:
        case CmasMsgType::EXTREME_LIKELY:
        case CmasMsgType::EXTREME_LIKELY_SPANISH:
        case CmasMsgType::ALERT_OBSERVED_DEFUALT:
        case CmasMsgType::ALERT_OBSERVED_SPANISH:
        case CmasMsgType::ALERT_LIKELY:
        case CmasMsgType::ALERT_LIKELY_SPANISH:
            urgency = static_cast<uint8_t>(SmsCmaeUrgency::IMMEDIATE);
            break;
        case CmasMsgType::SEVERE_OBSERVED:
        case CmasMsgType::SEVERE_OBSERVED_SPANISH:
        case CmasMsgType::SEVERE_LIKELY:
        case CmasMsgType::SEVERE_LIKELY_SPANISH:
        case CmasMsgType::EXPECTED_OBSERVED:
        case CmasMsgType::EXPECTED_OBSERVED_SPANISH:
        case CmasMsgType::EXPECTED_LIKELY:
        case CmasMsgType::EXPECTED_LIKELY_SPANISH:
            urgency = static_cast<uint8_t>(SmsCmaeUrgency::EXPECTED);
            break;
        default:
            urgency = static_cast<uint8_t>(SmsCmaeUrgency::RESERVED);
            break;
    }
    return true;
}

bool GsmCbCodec::GetCmasCertainty(uint8_t &certainty) const
{
    uint16_t msgId = 0;
    if (!GetMessageId(msgId)) {
        TELEPHONY_LOGE("Get message id fail.");
        return false;
    }
    switch (static_cast<CmasMsgType>(msgId)) {
        case CmasMsgType::EXTREME_OBSERVED:
        case CmasMsgType::EXTREME_OBSERVED_SPANISH:
        case CmasMsgType::SEVERE_OBSERVED:
        case CmasMsgType::SEVERE_OBSERVED_SPANISH:
        case CmasMsgType::ALERT_OBSERVED_DEFUALT:
        case CmasMsgType::ALERT_OBSERVED_SPANISH:
        case CmasMsgType::EXPECTED_OBSERVED:
        case CmasMsgType::EXPECTED_OBSERVED_SPANISH:
            certainty = static_cast<uint8_t>(SmsCmaeCertainty::OBSERVED);
            break;
        case CmasMsgType::EXTREME_LIKELY:
        case CmasMsgType::EXTREME_LIKELY_SPANISH:
        case CmasMsgType::SEVERE_LIKELY:
        case CmasMsgType::SEVERE_LIKELY_SPANISH:
        case CmasMsgType::ALERT_LIKELY:
        case CmasMsgType::ALERT_LIKELY_SPANISH:
        case CmasMsgType::EXPECTED_LIKELY:
        case CmasMsgType::EXPECTED_LIKELY_SPANISH:
            certainty = static_cast<uint8_t>(SmsCmaeCertainty::LIKELY);
            break;
        default:
            certainty = static_cast<uint8_t>(SmsCmaeCertainty::RESERVED);
            break;
    }
    return true;
}

bool GsmCbCodec::GetCmasCategory(uint8_t &cmasCategory) const
{
    cmasCategory = static_cast<uint8_t>(SmsCmaeCategory::RESERVED);
    return true;
}

bool GsmCbCodec::GetCmasResponseType(uint8_t &cmasRes) const
{
    cmasRes = static_cast<uint8_t>(SmsCmaeResponseType::RESERVED);
    return true;
}

bool GsmCbCodec::GetMessageId(uint16_t &msgId) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    msgId = cbHeader_->msgId;
    return true;
}

bool GsmCbCodec::GetCmasMessageClass(uint8_t &cmasClass) const
{
    uint16_t cbMsgId = 0;
    if (!GetMessageId(cbMsgId)) {
        TELEPHONY_LOGE("get cb id fail.");
        return false;
    }
    cmasClass = CMASClass(cbMsgId);
    return true;
}

bool GsmCbCodec::GetMsgType(uint8_t &msgType) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    msgType = cbHeader_->cbMsgType;
    return true;
}

bool GsmCbCodec::GetLangType(uint8_t &lan) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    lan = cbHeader_->langType;
    return true;
}

bool GsmCbCodec::GetDcs(uint8_t &dcs) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("cbHeader_ is nullptr");
        return false;
    }
    dcs = cbHeader_->dcs.codingScheme;
    return true;
}

bool GsmCbCodec::GetReceiveTime(int64_t &receiveTime) const
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    receiveTime = cbHeader_->recvTime;
    if (receiveTime == 0) {
        TELEPHONY_LOGI("receiveTime = 0");
        time_t recvTime = time(NULL);
        receiveTime = static_cast<int64_t>(recvTime);
    }
    return true;
}
} // namespace Telephony
} // namespace OHOS