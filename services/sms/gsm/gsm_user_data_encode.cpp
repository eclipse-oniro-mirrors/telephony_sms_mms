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

#include "gsm_user_data_encode.h"

#include "gsm_pdu_hex_value.h"
#include "gsm_sms_common_utils.h"
#include "securec.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint8_t NORMAL_BYTE_BITS = 8;
static constexpr uint8_t GSM_ENCODE_BITS = 7;
static constexpr uint8_t SLIDE_DATA_STEP = 2;
static constexpr uint8_t MAX_TPDU_LEN = 255;
static constexpr uint8_t HEX_07 = 0x07;
static constexpr uint8_t HEX_08 = 0x08;
static constexpr uint16_t MAX_7BIT_UDL = 160;
static constexpr uint16_t MAX_OCTET_UDL = 140;
const std::string CT_SMSC = "10659401";

static uint16_t GetOctetUdl(uint16_t udhl, uint8_t bodyLen)
{
    return (udhl > 0) ? static_cast<uint16_t>(udhl + 1 + bodyLen) : bodyLen;
}

static uint16_t GetSeptetUdl(uint16_t udhl, uint8_t fillBits, uint8_t bodyLen)
{
    if (udhl == 0) {
        return bodyLen;
    }
    return static_cast<uint16_t>((((udhl + 1) * HEX_08) + fillBits + (bodyLen * HEX_07)) / HEX_07);
}

GsmUserDataEncode::GsmUserDataEncode(std::shared_ptr<GsmUserDataPdu> data)
{
    userData_ = data;
}

GsmUserDataEncode::~GsmUserDataEncode() {}

bool GsmUserDataEncode::EncodeGsmPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData)
{
    uint8_t fillBits = 0;
    if (!EncodeGsmHeadPdu(buffer, userData, fillBits)) {
        return false;
    }
    return EncodeGsmBodyPdu(buffer, userData, fillBits);
}

bool GsmUserDataEncode::EncodeGsmHeadPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData, uint8_t &fillBits)
{
    if (userData == nullptr || userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    uint16_t location = buffer.GetIndex();
    if (userData->headerCnt > 0) {
        buffer.MoveForward(SLIDE_DATA_STEP);
    } else {
        buffer.MoveForward(HEX_VALUE_01);
    }
    uint16_t udhl = buffer.GetIndex();
    for (uint8_t i = 0; i < userData->headerCnt; i++) {
        userData_->EncodeHeader(buffer, userData->header[i]);
    }

    if (buffer.GetIndex() > udhl) {
        udhl = buffer.GetIndex() - udhl;
    } else {
        udhl = 0;
    }
    if (udhl > 0) {
        fillBits = ((udhl + 1) * NORMAL_BYTE_BITS) % GSM_ENCODE_BITS; /* + UDHL */
    }
    if (fillBits > 0 && GSM_ENCODE_BITS > fillBits) {
        fillBits = GSM_ENCODE_BITS - fillBits;
    } else {
        fillBits = 0;
    }

    /* Set UDL, UDHL; 3GPP TS 23.040 9.2.3.16: 7bit TP-UDL counts septets of UDHL, UDH, fill bits and body, max 160 */
    uint16_t udl = GetSeptetUdl(udhl, fillBits, userData->length);
    if (udl > MAX_7BIT_UDL || !buffer.InsertByte(static_cast<uint8_t>(udl), location)) {
        TELEPHONY_LOGE("7bit udl over limit or write data error.");
        return false;
    }
    if (udhl > 0 && !buffer.InsertByte(static_cast<uint8_t>(udhl), location + 1)) {
        TELEPHONY_LOGE("write data error.");
        return false;
    }
    return true;
}

bool GsmUserDataEncode::EncodeGsmBodyPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData, uint8_t fillBits)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    GsmSmsCommonUtils utils;
    bool packResult =
        utils.Pack7bitChar(buffer, reinterpret_cast<const uint8_t *>(userData->data), userData->length, fillBits);
    return packResult;
}

bool GsmUserDataEncode::Encode8bitPdu(
    SmsWriteBuffer &buffer, const struct SmsUDPackage *userData, std::string &destAddr)
{
    if (!Encode8bitHeadPdu(buffer, userData, destAddr)) {
        return false;
    }
    return Encode8bitBodyPdu(buffer, userData);
}

bool GsmUserDataEncode::Encode8bitHeadPdu(
    SmsWriteBuffer &buffer, const struct SmsUDPackage *userData, std::string &destAddr)
{
    if (userData == nullptr || userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    uint16_t location = buffer.GetIndex();
    if (userData->headerCnt > 0 && CT_SMSC.compare(destAddr) != 0) {
        buffer.MoveForward(SLIDE_DATA_STEP);
    } else {
        buffer.MoveForward(HEX_VALUE_01);
    }

    /* Encode User Data Header */
    uint16_t udhl = buffer.GetIndex();
    for (uint8_t index = 0; index < userData->headerCnt && CT_SMSC.compare(destAddr) != 0; index++) {
        userData_->EncodeHeader(buffer, userData->header[index]);
    }
    if (buffer.GetIndex() > udhl) {
        udhl = buffer.GetIndex() - udhl;
    } else {
        udhl = 0;
    }
    /* Set UDL, UDHL; 3GPP TS 23.040 9.2.3.16: 8bit TP-UDL counts octets of UDHL, UDH and body, max 140 */
    if (GetOctetUdl(udhl, userData->length) > MAX_OCTET_UDL) {
        TELEPHONY_LOGE("Encode8bitHeadPdu udl over limit.");
        return false;
    }
    if (udhl > 0) {
        if (!buffer.InsertByte(static_cast<uint8_t>(udhl + 1 + userData->length), location)) {
            TELEPHONY_LOGE("Encode8bitHeadPdu write data error.");
            return false;
        }
        if (!buffer.InsertByte(static_cast<uint8_t>(udhl), location + 1)) {
            TELEPHONY_LOGE("Encode8bitHeadPdu write data error.");
            return false;
        }
    } else {
        if (!buffer.InsertByte(userData->length, location)) {
            TELEPHONY_LOGE("Encode8bitHeadPdu write data error.");
            return false;
        }
    }
    return true;
}

bool GsmUserDataEncode::Encode8bitBodyPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData)
{
    if (userData == nullptr || buffer.data_ == nullptr) {
        TELEPHONY_LOGE("nullptr error");
        return false;
    }

    if (buffer.GetIndex() >= MAX_TPDU_LEN || userData->length + buffer.GetIndex() > MAX_TPDU_LEN) {
        TELEPHONY_LOGE(" userData length error.");
        return false;
    }
    if (buffer.GetIndex() >= buffer.GetSize() || userData->length + buffer.GetIndex() > buffer.GetSize()) {
        TELEPHONY_LOGE("buffer error.");
        return false;
    }
    uint16_t destLen = MAX_TPDU_LEN - buffer.GetIndex() - HEX_VALUE_01;
    if (memcpy_s(buffer.data_.get() + buffer.GetIndex(), destLen, userData->data, userData->length) != EOK) {
        TELEPHONY_LOGE("memcpy_s error");
        return false;
    }
    buffer.MoveForward(userData->length);
    return true;
}

bool GsmUserDataEncode::EncodeUcs2Pdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData)
{
    if (!EncodeUcs2HeadPdu(buffer, userData)) {
        return false;
    }
    return EncodeUcs2BodyPdu(buffer, userData);
}

bool GsmUserDataEncode::EncodeUcs2HeadPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData)
{
    if (userData == nullptr || userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error");
        return false;
    }
    uint16_t location = buffer.GetIndex();
    if (userData->headerCnt == 0) {
        buffer.MoveForward(1);
    } else {
        buffer.MoveForward(SLIDE_DATA_STEP);
    }

    /* Encode User Data Header */
    uint16_t udhl = buffer.GetIndex();
    for (uint8_t j = 0; j < userData->headerCnt; j++) {
        userData_->EncodeHeader(buffer, userData->header[j]);
    }
    if (buffer.GetIndex() > udhl) {
        udhl = buffer.GetIndex() - udhl;
    } else {
        udhl = 0;
    }

    /* Set UDL, UDHL; 3GPP TS 23.040 9.2.3.16: UCS2 TP-UDL counts octets of UDHL, UDH and body, max 140 */
    if (GetOctetUdl(udhl, userData->length) > MAX_OCTET_UDL) {
        TELEPHONY_LOGE("EncodeUcs2HeadPdu udl over limit.");
        return false;
    }
    if (udhl > 0) {
        if (!buffer.InsertByte(static_cast<uint8_t>(udhl + 1 + userData->length), location)) {
            TELEPHONY_LOGE("EncodeUcs2HeadPdu write data error.");
            return false;
        }
        if (!buffer.InsertByte(static_cast<uint8_t>(udhl), location + 1)) {
            TELEPHONY_LOGE("EncodeUcs2HeadPdu write data error.");
            return false;
        }
    } else {
        if (!buffer.InsertByte(userData->length, location)) {
            TELEPHONY_LOGE("EncodeUcs2HeadPdu write data error.");
            return false;
        }
    }
    return true;
}

bool GsmUserDataEncode::EncodeUcs2BodyPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *userData)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error");
        return false;
    }
    if (userData->length + buffer.GetIndex() >= MAX_TPDU_LEN) {
        TELEPHONY_LOGE("data length error");
        return false;
    }
    if (buffer.data_ == nullptr || (buffer.GetIndex() + userData->length) > buffer.GetSize()) {
        TELEPHONY_LOGE("buffer error.");
        return false;
    }
    uint16_t destLen = MAX_TPDU_LEN - buffer.GetIndex() - HEX_VALUE_01;
    if (memcpy_s(buffer.data_.get() + buffer.GetIndex(), destLen, userData->data, userData->length) != EOK) {
        TELEPHONY_LOGE("memcpy_s error");
        return false;
    }
    buffer.MoveForward(userData->length);
    return true;
}
} // namespace Telephony
} // namespace OHOS