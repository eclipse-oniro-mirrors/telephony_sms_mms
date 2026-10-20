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

#include "gsm_cb_gsm_codec.h"

#include "cdma_sms_common.h"
#include "gsm_pdu_hex_value.h"
#include "securec.h"
#include "sms_common_utils.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"
#include "text_coder.h"

namespace OHOS {
namespace Telephony {
static constexpr uint8_t SMS_BYTE_BIT = 8;
static constexpr uint8_t GSM_CODE_BIT = 7;
static constexpr uint8_t MAX_PAGE_PDU_LEN = 82;
static constexpr uint8_t MAX_ETWS_PDU_LEN = 56;
static constexpr uint8_t MAX_PAGE_NUM = 15;
static constexpr uint16_t GSM_ETWS_BASE_MASK = 0x1100;

GsmCbGsmCodec::GsmCbGsmCodec(std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> header,
    std::shared_ptr<GsmCbPduDecodeBuffer> buffer, std::shared_ptr<GsmCbCodec> cbCodec)
{
    cbHeader_ = header;
    cbPduBuffer_ = buffer;
    cbCodec_ = cbCodec;
}

GsmCbGsmCodec::~GsmCbGsmCodec() {}

/**
 * refer to 3GPP TS 23.041 V4.1.0 9.4 Message Format on the Radio Network  – MS/UE Interface
 * refer to 3GPP TS 23.041 V4.1.0 9.4.1.2 Message Parameter
 */
bool GsmCbGsmCodec::Decode2gHeader()
{
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbPduBuffer_->GetSize() == 0) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    cbHeader_->bEtwsMessage = false;
    uint8_t oneByte = 0;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint8_t temp = oneByte;
    cbHeader_->serialNum.geoScope = (temp & HEX_VALUE_C0) >> HEX_VALUE_06;
    cbHeader_->serialNum.msgCode = (temp & HEX_VALUE_3F) << HEX_VALUE_04;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    temp = oneByte;
    cbHeader_->serialNum.msgCode |= (temp & HEX_VALUE_F0) >> HEX_VALUE_04;
    cbHeader_->serialNum.updateNum = temp & HEX_VALUE_0F;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    temp = oneByte;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    cbHeader_->msgId = (temp << HEX_VALUE_08) | oneByte;
    bool isEtws;
    if (cbCodec_ == nullptr) {
        TELEPHONY_LOGE("cbCodec_ nullptr error.");
        return false;
    }
    cbCodec_->IsEtwsMessage(isEtws);
    if (isEtws && cbPduBuffer_->GetSize() <= MAX_ETWS_PDU_LEN) {
        if (!Decode2gHeaderEtws()) {
            TELEPHONY_LOGE("etws head error.");
            return false;
        }
    } else {
        if (!Decode2gHeaderCommonCb()) {
            TELEPHONY_LOGE("common cb head error.");
            return false;
        }
    }
    return true;
}

bool GsmCbGsmCodec::Decode2gHeaderEtws()
{
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbCodec_ == nullptr) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    uint8_t oneByte = 0;
    cbHeader_->cbEtwsType = GsmCbCodec::ETWS_PRIMARY;
    cbHeader_->bEtwsMessage = true;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    cbHeader_->warningType = (oneByte & HEX_VALUE_FE) >> 1;
    if (cbHeader_->msgId >= GSM_ETWS_BASE_MASK) {
        cbCodec_->GetWarningType(cbHeader_->warningType);
    }
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    cbHeader_->totalPages = 1;
    bool activatePopup = (oneByte & HEX_VALUE_80) != 0;
    TELEPHONY_LOGI("activatePopup:%{public}d.", activatePopup);
    return true;
}

bool GsmCbGsmCodec::Decode2gHeaderCommonCb()
{
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbCodec_ == nullptr || cbPduBuffer_->GetSize() == 0) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    uint8_t oneByte = 0;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint8_t dcs = oneByte;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint8_t temp = oneByte;
    cbHeader_->totalPages = temp & HEX_VALUE_0F;
    cbHeader_->page = (temp & HEX_VALUE_F0) >> HEX_VALUE_04;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    unsigned short iosTemp = oneByte;

    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    iosTemp |= (oneByte << SMS_BYTE_BIT);
    cbCodec_->DecodeCbMsgDCS(dcs, iosTemp, cbHeader_->dcs);
    cbHeader_->langType = cbHeader_->dcs.langType;
    cbHeader_->recvTime = static_cast<time_t>(cbCodec_->GetRecvTime());
    if (cbHeader_->totalPages > MAX_PAGE_NUM) {
        TELEPHONY_LOGE("CB Page Count is over MAX[%{public}d]", cbHeader_->totalPages);
        cbPduBuffer_->SetPointer(0);
    }
    bool isEtws;
    cbCodec_->IsEtwsMessage(isEtws);
    if (isEtws) {
        cbHeader_->bEtwsMessage = true;
        cbHeader_->cbEtwsType = GsmCbCodec::ETWS_GSM;
        if (cbHeader_->msgId >= GSM_ETWS_BASE_MASK) {
            cbCodec_->GetWarningType(cbHeader_->warningType);
        }
    }
    if (cbPduBuffer_->GetCurPosition() >= HEX_VALUE_02) {
        cbPduBuffer_->SetPointer(cbPduBuffer_->GetCurPosition() - HEX_VALUE_02);
    }
    return true;
}

/**
 * refer to 3GPP TS 23.041 V4.1.0 9.4.2.2 Message Parameter
 */
bool GsmCbGsmCodec::Decode2gCbMsg()
{
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbCodec_ == nullptr ||
        cbPduBuffer_->GetCurPosition() >= cbPduBuffer_->GetSize()) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    uint16_t dataLen = cbPduBuffer_->GetSize() - cbPduBuffer_->GetCurPosition();
    switch (cbHeader_->dcs.codingScheme) {
        case DATA_CODING_7BIT: {
            if (!Decode2gCbMsg7bit(dataLen)) {
                TELEPHONY_LOGE("decode cb 7bit error.");
                return false;
            }
            break;
        }
        case DATA_CODING_8BIT:
        case DATA_CODING_UCS2: {
            if (dataLen < HEX_VALUE_02) {
                TELEPHONY_LOGE("dataLen error.");
                return false;
            }
            if (cbHeader_->dcs.iso639Lang[0]) {
                TELEPHONY_LOGI("dcs.iso639Lang");
                cbPduBuffer_->SetPointer(cbPduBuffer_->GetCurPosition() + HEX_VALUE_02);
                dataLen -= HEX_VALUE_02;
            }

            if (dataLen == 0 || cbPduBuffer_->pduBuffer_ == nullptr ||
                (cbPduBuffer_->GetCurPosition() + dataLen) > cbPduBuffer_->GetSize()) {
                TELEPHONY_LOGE("CB pdu data error.");
                return false;
            }
            for (uint32_t i = cbPduBuffer_->GetCurPosition(); i < cbPduBuffer_->GetSize(); i++) {
                messageRaw_.push_back(cbPduBuffer_->pduBuffer_[i]);
            }
            cbCodec_->SetCbMessageRaw(messageRaw_);
            break;
        }
        default:
            break;
    }
    return true;
}

bool GsmCbGsmCodec::Decode2gCbMsg7bit(uint16_t dataLen)
{
    if (cbHeader_ == nullptr || cbCodec_ == nullptr) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    uint8_t pageData[MAX_PAGE_PDU_LEN * SMS_BYTE_BIT / GSM_CODE_BIT + 1] = { 0 };
    std::vector<uint8_t> dataPdu;
    cbCodec_->GetPduData(dataPdu);

    if (dataPdu.size() == 0) {
        TELEPHONY_LOGE("dataPdu empty.");
        return false;
    }
    uint16_t unpackLen = SmsCommonUtils::Unpack7bitCharForCBPdu(
        dataPdu.data(), dataLen, 0x00, pageData, MAX_PAGE_PDU_LEN * SMS_BYTE_BIT / GSM_CODE_BIT + 1);

    uint16_t offset = 0;
    if (cbHeader_->dcs.iso639Lang[0] && unpackLen >= GsmCbCodec::CB_IOS639_LANG_SIZE) {
        unpackLen = unpackLen - GsmCbCodec::CB_IOS639_LANG_SIZE;
        offset = GsmCbCodec::CB_IOS639_LANG_SIZE;
    }
    if (offset + unpackLen >= (MAX_PAGE_PDU_LEN * SMS_BYTE_BIT / GSM_CODE_BIT) + 1) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    for (uint16_t i = 0; i < unpackLen; i++) {
        messageRaw_.push_back(static_cast<char>(pageData[i]));
    }
    cbCodec_->SetCbMessageRaw(messageRaw_);
    return true;
}

bool GsmCbGsmCodec::DecodeEtwsMsg()
{
    if (cbPduBuffer_ == nullptr || cbPduBuffer_->pduBuffer_ == nullptr || cbPduBuffer_->GetSize() == 0 ||
        cbPduBuffer_->GetSize() > MAX_ETWS_PDU_LEN) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    uint8_t total = cbPduBuffer_->GetSize() - cbPduBuffer_->GetCurPosition();
    return Decode2gCbMsg7bit(total);
}
} // namespace Telephony
} // namespace OHOS