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

#include "gsm_cb_umts_codec.h"

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

GsmCbUmtsCodec::GsmCbUmtsCodec(std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> header,
    std::shared_ptr<GsmCbPduDecodeBuffer> buffer, std::shared_ptr<GsmCbCodec> cbCodec)
{
    cbHeader_ = header;
    cbPduBuffer_ = buffer;
    cbCodec_ = cbCodec;
}

GsmCbUmtsCodec::~GsmCbUmtsCodec() {}

/**
 * refer to 3GPP TS 23.041 V4.1.0 9.4.2.1 General Description
 * refer to 3GPP TS 23.041 V4.1.0 9.4.2.2 Message Parameter
 */
bool GsmCbUmtsCodec::Decode3gHeader()
{
    if (cbPduBuffer_ == nullptr || cbHeader_ == nullptr || cbCodec_ == nullptr || cbPduBuffer_->GetSize() == 0) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    cbPduBuffer_->IncreasePointer(1);
    uint8_t oneByte = 0;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint8_t temp = oneByte;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    cbHeader_->msgId = (temp << HEX_VALUE_08) | oneByte;
    bool isEtws;
    cbCodec_->IsEtwsMessage(isEtws);
    if (isEtws) {
        cbHeader_->cbMsgType = GsmCbCodec::GSM_ETWS;
    }
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    temp = oneByte;
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
    uint8_t dcs = oneByte;
    return Decode3gHeaderPartData(dcs);
}

bool GsmCbUmtsCodec::Decode3gHeaderPartData(uint8_t dcs)
{
    if (cbCodec_ == nullptr) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }
    uint8_t oneByte = 0;
    if (!cbPduBuffer_->GetOneByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    cbHeader_->totalPages = oneByte;

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
    if (cbPduBuffer_->GetCurPosition() >= HEX_VALUE_02) {
        cbPduBuffer_->SetPointer(cbPduBuffer_->GetCurPosition() - HEX_VALUE_02);
    }
    return true;
}

/**
 * refer to 3GPP TS 23.041 V4.1.0 9.4.2.2.5 CB Data
 */
bool GsmCbUmtsCodec::Decode3gCbMsg()
{
    if (cbHeader_ == nullptr) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    bool decodeResult = false;
    switch (cbHeader_->dcs.codingScheme) {
        case DATA_CODING_7BIT: {
            decodeResult = Decode3g7Bit();
            break;
        }
        case DATA_CODING_8BIT:
        case DATA_CODING_UCS2: {
            decodeResult = Decode3gUCS2();
            break;
        }
        default:
            break;
    }
    cbHeader_->totalPages = 1;
    return decodeResult;
}

bool GsmCbUmtsCodec::Decode3g7Bit()
{
    if (cbPduBuffer_ == nullptr || cbCodec_ == nullptr || cbPduBuffer_->GetSize() == 0) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    std::vector<unsigned char> dataPdu;
    cbCodec_->GetPduData(dataPdu);
    if (dataPdu.size() == 0 || cbPduBuffer_->GetCurPosition() >= cbPduBuffer_->GetSize()) {
        TELEPHONY_LOGE("dataPdu empty.");
        return false;
    }
    uint16_t pduLen = cbPduBuffer_->GetSize() - cbPduBuffer_->GetCurPosition();

    const uint8_t *tpdu = dataPdu.data();
    for (uint8_t i = 0; i < cbHeader_->totalPages; ++i) {
        uint16_t pageLenOffset = (i + 1) * MAX_PAGE_PDU_LEN + i;
        if (pduLen <= pageLenOffset) {
            TELEPHONY_LOGE("CB Msg Size err [%{public}d]", pduLen);
            messageRaw_.clear();
            return false;
        }
        uint16_t dataLen = tpdu[pageLenOffset];
        uint16_t offset = (i * MAX_PAGE_PDU_LEN) + i;
        if (dataLen > MAX_PAGE_PDU_LEN) {
            TELEPHONY_LOGE("CB Msg Size is over MAX [%{public}d]", dataLen);
            messageRaw_.clear();
            return false;
        }
        uint16_t unpackLen = 0;
        uint8_t pageData[MAX_PAGE_PDU_LEN * SMS_BYTE_BIT / GSM_CODE_BIT] = { 0 };
        unpackLen = SmsCommonUtils::Unpack7bitChar(
            &tpdu[offset], dataLen, 0x00, pageData, MAX_PAGE_PDU_LEN * SMS_BYTE_BIT / GSM_CODE_BIT);
        for (uint16_t position = 0; position < unpackLen; position++) {
            messageRaw_.push_back(pageData[position]);
        }
    }
    cbCodec_->SetCbMessageRaw(messageRaw_);
    return true;
}

bool GsmCbUmtsCodec::Decode3gUCS2()
{
    if (cbPduBuffer_ == nullptr || cbCodec_ == nullptr || cbPduBuffer_->GetSize() == 0) {
        TELEPHONY_LOGE("CB pdu data error.");
        return false;
    }

    std::vector<unsigned char> dataPdu;
    cbCodec_->GetPduData(dataPdu);
    if (dataPdu.size() == 0 || cbPduBuffer_->GetCurPosition() >= cbPduBuffer_->GetSize()) {
        TELEPHONY_LOGE("dataPdu empty.");
        return false;
    }
    uint16_t pduLen = cbPduBuffer_->GetSize() - cbPduBuffer_->GetCurPosition();

    uint8_t *tpdu = dataPdu.data();
    uint16_t tpduLen = dataPdu.size();
    for (uint8_t i = 0; i < cbHeader_->totalPages; ++i) {
        TELEPHONY_LOGI("cbHeader_->totalPages:%{public}d", cbHeader_->totalPages);
        uint16_t pageLenOffset = static_cast<uint16_t>((i + 1) * MAX_PAGE_PDU_LEN + i);
        if (pduLen <= pageLenOffset) {
            TELEPHONY_LOGE("pageLenOffset invalid.");
            messageRaw_.clear();
            return false;
        }
        uint16_t dataLen = 0;
        uint16_t offset = 0;
        if (cbHeader_->dcs.iso639Lang[0]) {
            if (tpdu[pageLenOffset] >= HEX_VALUE_02) {
                dataLen = tpdu[pageLenOffset] - HEX_VALUE_02;
            }
            offset = (i * MAX_PAGE_PDU_LEN) + i + HEX_VALUE_02;
        } else {
            dataLen = tpdu[pageLenOffset];
            offset = (i * MAX_PAGE_PDU_LEN) + i;
        }
        if (dataLen > 0 && dataLen <= MAX_PAGE_PDU_LEN && offset + dataLen <= tpduLen) {
            for (uint16_t position = offset; position < offset + dataLen; position++) {
                messageRaw_.push_back(static_cast<char>(tpdu[position]));
            }
        }
    }
    cbCodec_->SetCbMessageRaw(messageRaw_);
    return true;
}
} // namespace Telephony
} // namespace OHOS