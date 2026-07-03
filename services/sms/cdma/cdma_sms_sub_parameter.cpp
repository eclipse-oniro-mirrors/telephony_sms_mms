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

#include "cdma_sms_sub_parameter.h"

#include "gsm_user_data_pdu.h"
#include "securec.h"
#include "sms_common_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
static constexpr uint8_t HEX_STEP = 2;
static constexpr uint8_t DECIMAL_NUM = 10;
static constexpr uint8_t MESSAGE_ID_LEN = 3;
static constexpr uint8_t ABSOLUTE_TIME_LEN = 6;

bool CdmaSmsSubParameter::IsInvalidPdu(SmsReadBuffer &pdu)
{
    uint8_t id = RESERVED;
    if (pdu.IsEmpty() || !pdu.ReadByte(id) || !pdu.ReadByte(len_) || id != id_) {
        TELEPHONY_LOGE("sub parameter: pdu is invalid[%{public}d]: [%{public}d]", id_, id);
        return true;
    }
    return false;
}

CdmaSmsBaseParameter::CdmaSmsBaseParameter(uint8_t id, uint8_t &data) : data_(data)
{
    id_ = id;
    len_ = sizeof(id_);
}

bool CdmaSmsBaseParameter::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }
    if (pdu.WriteByte(id_) && pdu.WriteByte(len_) && pdu.WriteByte(data_)) {
        return true;
    }
    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsBaseParameter::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }
    return pdu.ReadByte(data_);
}

CdmaSmsReservedParameter::CdmaSmsReservedParameter(uint8_t id)
{
    id_ = id;
}

bool CdmaSmsReservedParameter::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }
    return true;
}

bool CdmaSmsReservedParameter::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }
    pdu.MoveForward(len_);
    return true;
}

CdmaSmsMessageId::CdmaSmsMessageId(SmsTeleSvcMsgId &msgId, uint8_t type) : msgId_(msgId)
{
    id_ = MESSAGE_IDENTIFIER;
    type_ = type; // 4 bits
    len_ = MESSAGE_ID_LEN;
}

bool CdmaSmsMessageId::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(type_, BIT4) && pdu.WriteBits(msgId_.msgId >> BIT8, BIT8) &&
            pdu.WriteBits(msgId_.msgId, BIT8) && pdu.WriteBits(msgId_.headerInd ? 0b1 : 0b0, BIT1)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsMessageId::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    if (!pdu.ReadBits(type_, BIT4)) {
        TELEPHONY_LOGE("type read error");
        return false;
    }
    uint8_t vh = 0;
    uint8_t v = 0;
    if (!pdu.ReadBits(vh, BIT8) || !pdu.ReadBits(v, BIT8)) {
        TELEPHONY_LOGE("msgid read error");
        return false;
    }
    msgId_.msgId = (vh << BIT8) | v;

    if (!pdu.ReadBits(v, BIT1)) {
        TELEPHONY_LOGE("header ind read error");
        return false;
    }
    msgId_.headerInd = v == 0b1 ? true : false;

    pdu.SkipBits();
    return true;
}

uint8_t CdmaSmsMessageId::GetMessageType()
{
    return type_;
}

CdmaSmsAbsoluteTime::CdmaSmsAbsoluteTime(uint8_t id, SmsTimeAbs &time) : time_(time)
{
    id_ = id;
    len_ = ABSOLUTE_TIME_LEN;
}

bool CdmaSmsAbsoluteTime::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteByte(EncodeBCD(time_.year)) && pdu.WriteByte(EncodeBCD(time_.month)) &&
            pdu.WriteByte(EncodeBCD(time_.day)) && pdu.WriteByte(EncodeBCD(time_.hour)) &&
            pdu.WriteByte(EncodeBCD(time_.minute)) && pdu.WriteByte(EncodeBCD(time_.second))) {
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsAbsoluteTime::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    uint8_t v3 = 0;
    uint8_t v4 = 0;
    uint8_t v5 = 0;
    uint8_t v6 = 0;
    if (!pdu.ReadByte(v1) || !pdu.ReadByte(v2) || !pdu.ReadByte(v3) || !pdu.ReadByte(v4) || !pdu.ReadByte(v5) ||
        !pdu.ReadByte(v6)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    time_.year = DecodeBCD(v1);
    time_.month = DecodeBCD(v2);
    time_.day = DecodeBCD(v3);
    time_.hour = DecodeBCD(v4);
    time_.minute = DecodeBCD(v5);
    time_.second = DecodeBCD(v6);
    return true;
}

uint8_t CdmaSmsAbsoluteTime::EncodeBCD(const uint8_t v)
{
    return ((v / DECIMAL_NUM) << BIT4) | (v % DECIMAL_NUM);
}

uint8_t CdmaSmsAbsoluteTime::DecodeBCD(const uint8_t v)
{
    return ((v & 0xf0) >> BIT4) * DECIMAL_NUM + (v & 0x0f);
}

CdmaSmsPriorityInd::CdmaSmsPriorityInd(SmsPriorityIndicator &priority) : priority_(priority)
{
    id_ = PRIORITY_INDICATOR;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsPriorityInd::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(static_cast<uint8_t>(priority_), BIT2)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsPriorityInd::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT2)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    pdu.SkipBits();
    priority_ = SmsPriorityIndicator(v);
    return true;
}

CdmaSmsPrivacyInd::CdmaSmsPrivacyInd(SmsPrivacyIndicator &privacy) : privacy_(privacy)
{
    id_ = PRIVACY_INDICATOR;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsPrivacyInd::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(static_cast<uint8_t>(privacy_), BIT2)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsPrivacyInd::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT2)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    pdu.SkipBits();
    privacy_ = SmsPrivacyIndicator(v);
    return true;
}

CdmaSmsReplyOption::CdmaSmsReplyOption(SmsReplyOption &replyOpt) : replyOpt_(replyOpt)
{
    id_ = REPLY_OPTION;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsReplyOption::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(replyOpt_.userAck ? 0b1 : 0b0, BIT1) && pdu.WriteBits(replyOpt_.dak ? 0b1 : 0b0, BIT1) &&
            pdu.WriteBits(replyOpt_.readAck ? 0b1 : 0b0, BIT1) && pdu.WriteBits(replyOpt_.report ? 0b1 : 0b0, BIT1)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsReplyOption::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v1 = 0;
    uint8_t v2 = 0;
    uint8_t v3 = 0;
    uint8_t v4 = 0;
    if (!pdu.ReadBits(v1, BIT1) || !pdu.ReadBits(v2, BIT1) || !pdu.ReadBits(v3, BIT1) || !pdu.ReadBits(v4, BIT1)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    replyOpt_.userAck = (v1 == 0b1) ? true : false;
    replyOpt_.dak = (v2 == 0b1) ? true : false;
    replyOpt_.readAck = (v3 == 0b1) ? true : false;
    replyOpt_.report = (v4 == 0b1) ? true : false;
    pdu.SkipBits();
    return true;
}

CdmaSmsUserData::CdmaSmsUserData(SmsTeleSvcUserData &data, bool &headerInd) : data_(data), headerInd_(headerInd)
{
    id_ = USER_DATA;
}

bool CdmaSmsUserData::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty() || !pdu.WriteByte(id_)) {
        TELEPHONY_LOGE("pdu is empty or id write error");
        return false;
    }
    uint16_t lenIndex = pdu.MoveForward();
    if (!pdu.WriteBits(static_cast<uint8_t>(data_.encodeType), BIT5)) {
        TELEPHONY_LOGE("encode type write error");
        return false;
    }
    if (data_.encodeType == SmsEncodingType::EPM || data_.encodeType == SmsEncodingType::GSMDCS) {
        if (!pdu.WriteBits(data_.msgType, BIT8)) {
            TELEPHONY_LOGE("msg type write error");
            return false;
        }
    }
    if (data_.encodeType == SmsEncodingType::ASCII_7BIT || data_.encodeType == SmsEncodingType::IA5) {
        if (!EncodeAscii7Bit(pdu)) {
            TELEPHONY_LOGE("ascii 7bit encode error");
            return false;
        }
    } else if (data_.encodeType == SmsEncodingType::GSM7BIT) {
        if (!EncodeGsm7Bit(pdu)) {
            TELEPHONY_LOGE("gsm 7bit encode error");
            return false;
        }
    } else if (data_.encodeType == SmsEncodingType::UNICODE) {
        if (!EncodeUnicode(pdu)) {
            TELEPHONY_LOGE("unicode encode error");
            return false;
        }
    } else {
        if (!Encode8BitData(pdu)) {
            TELEPHONY_LOGE("8bit encode error");
            return false;
        }
    }
    len_ = pdu.SkipBits() - lenIndex - 1;
    return pdu.InsertByte(len_, lenIndex);
}

bool CdmaSmsUserData::EncodeHeader7Bit(SmsWriteBuffer &pdu)
{
    if (static_cast<unsigned long>(data_.userData.headerCnt) >
        (sizeof(data_.userData.header) / sizeof(data_.userData.header[0]))) {
        TELEPHONY_LOGE("user data header length error");
        return false;
    }
    SmsWriteBuffer headerPdu;
    GsmUserDataPdu gsmUdPdu;
    for (uint8_t i = 0; i < data_.userData.headerCnt; i++) {
        gsmUdPdu.EncodeHeader(headerPdu, data_.userData.header[i]);
    }
    uint8_t udhBytes = headerPdu.GetIndex();
    if (udhBytes > 0) {
        uint8_t udhLen = (udhBytes + 1) * BIT8 / BIT7;
        // The header data is encoded in 7bit, the remaining bits need to be ignored.
        uint8_t ignoredBits = BIT7 - (udhBytes + 1) * BIT8 % BIT7;
        if (ignoredBits != BIT7) {
            udhLen++;
        }
        if (data_.encodeType == SmsEncodingType::GSM7BIT) {
            ignoredBits = BIT8;
            udhLen++;
        }
        uint8_t numFields = udhLen + data_.userData.length;
        if (!pdu.WriteBits(numFields, BIT8) || !pdu.WriteBits(udhBytes, BIT8)) {
            TELEPHONY_LOGE("num fields or udh bytes write error");
            return false;
        }
        uint8_t v = 0;
        for (uint8_t i = 0; i < udhBytes; i++) {
            if (!headerPdu.GetValueFromIndex(i, v) || !pdu.WriteBits(v, BIT8)) {
                TELEPHONY_LOGE("data write error");
                return false;
            }
        }
        if (ignoredBits != BIT7 && !pdu.WriteBits(0b0, ignoredBits)) {
            TELEPHONY_LOGE("write error");
            return false;
        }
    } else {
        return pdu.WriteBits(data_.userData.length, BIT8);
    }
    return true;
}

bool CdmaSmsUserData::EncodeAscii7Bit(SmsWriteBuffer &pdu)
{
    if (!EncodeHeader7Bit(pdu)) {
        TELEPHONY_LOGE("header encode error");
        return false;
    }
    if (static_cast<unsigned long>(data_.userData.length) >
        (sizeof(data_.userData.data) / sizeof(data_.userData.data[0]))) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    for (uint8_t i = 0; i < data_.userData.length; i++) {
        if (!pdu.WriteBits(data_.userData.data[i], BIT7)) {
            TELEPHONY_LOGE("data write error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsUserData::EncodeGsm7Bit(SmsWriteBuffer &pdu)
{
    if (!EncodeHeader7Bit(pdu)) {
        TELEPHONY_LOGE("header encode error");
        return false;
    }
    std::unique_ptr<uint8_t[]> destPtr = std::make_unique<uint8_t[]>(MAX_USER_DATA_LEN + 1);
    if (destPtr == nullptr) {
        TELEPHONY_LOGE("make_unique error");
        return false;
    }
    uint16_t size = SmsCommonUtils::Pack7bitChar(reinterpret_cast<const uint8_t *>(data_.userData.data),
        data_.userData.length, 0, destPtr.get(), MAX_USER_DATA_LEN + 1);
    if (size > MAX_USER_DATA_LEN + 1) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    for (uint16_t i = 0; i < size; i++) {
        if (!pdu.WriteBits(destPtr.get()[i], BIT8)) {
            TELEPHONY_LOGE("data write error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsUserData::EncodeHeaderUnicode(SmsWriteBuffer &pdu)
{
    if (static_cast<unsigned long>(data_.userData.headerCnt) >
        (sizeof(data_.userData.header) / sizeof(data_.userData.header[0]))) {
        TELEPHONY_LOGE("user data header length error");
        return false;
    }
    SmsWriteBuffer headerPdu;
    GsmUserDataPdu gsmUdPdu;
    for (uint8_t i = 0; i < data_.userData.headerCnt; i++) {
        gsmUdPdu.EncodeHeader(headerPdu, data_.userData.header[i]);
    }
    uint8_t udhBytes = headerPdu.GetIndex();
    uint8_t numFields = 0;
    if (udhBytes > 0) {
        numFields = (udhBytes + data_.userData.length) / HEX_STEP + 1;
        if (!pdu.WriteBits(numFields, BIT8) || !pdu.WriteBits(udhBytes, BIT8)) {
            TELEPHONY_LOGE("num fields or udh bytes write error");
            return false;
        }
        uint8_t v = 0;
        for (uint8_t i = 0; i < udhBytes; i++) {
            if (!headerPdu.GetValueFromIndex(i, v) || !pdu.WriteBits(v, BIT8)) {
                TELEPHONY_LOGE("data write error");
                return false;
            }
        }
    } else {
        numFields = data_.userData.length / HEX_STEP;
        return pdu.WriteBits(numFields, BIT8);
    }
    return true;
}

bool CdmaSmsUserData::EncodeUnicode(SmsWriteBuffer &pdu)
{
    if (!EncodeHeaderUnicode(pdu)) {
        TELEPHONY_LOGE("header encode error");
        return false;
    }
    if (static_cast<unsigned long>(data_.userData.length) >
        (sizeof(data_.userData.data) / sizeof(data_.userData.data[0]))) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    for (uint8_t i = 0; i < data_.userData.length; i++) {
        if (!pdu.WriteBits(data_.userData.data[i], BIT8)) {
            TELEPHONY_LOGE("user data write error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsUserData::Encode8BitData(SmsWriteBuffer &pdu)
{
    if (static_cast<unsigned long>(data_.userData.length) >
        (sizeof(data_.userData.data) / sizeof(data_.userData.data[0]))) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    if (!pdu.WriteBits(data_.userData.length, BIT8)) {
        TELEPHONY_LOGE("data length write error");
        return false;
    }
    for (uint8_t i = 0; i < data_.userData.length; i++) {
        if (!pdu.WriteBits(data_.userData.data[i], BIT8)) {
            TELEPHONY_LOGE("data write error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsUserData::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }
    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT5)) {
        TELEPHONY_LOGE("encode type read error");
        return false;
    }
    data_.encodeType = GetEncodingType(v);
    if (data_.encodeType == SmsEncodingType::EPM || data_.encodeType == SmsEncodingType::GSMDCS) {
        if (!pdu.ReadBits(data_.msgType, BIT8)) {
            TELEPHONY_LOGE("msg type read error");
            return false;
        }
    }
    uint8_t numFields = 0;
    if (!pdu.ReadBits(numFields, BIT8)) {
        TELEPHONY_LOGE("num fields read error");
        return false;
    }
    uint8_t udhBytes = DecodeHeader7Bit(pdu);
    if (data_.encodeType == SmsEncodingType::ASCII_7BIT || data_.encodeType == SmsEncodingType::IA5) {
        return DecodeAscii7Bit(pdu, numFields, udhBytes);
    } else if (data_.encodeType == SmsEncodingType::GSM7BIT) {
        return DecodeGsm7Bit(pdu, numFields, udhBytes);
    } else if (data_.encodeType == SmsEncodingType::UNICODE) {
        data_.userData.length = headerInd_ ? (numFields * HEX_STEP - udhBytes - 1) : (numFields * HEX_STEP);
        if (!Decode8BitData(pdu)) {
            TELEPHONY_LOGE("unicode decode error");
            return false;
        }
    } else if (data_.encodeType == SmsEncodingType::EPM || data_.encodeType == SmsEncodingType::GSMDCS) {
        TELEPHONY_LOGW("Encode type not support [%{public}d]", static_cast<int>(data_.encodeType));
        return false;
    } else {
        data_.userData.length = headerInd_ ? (numFields - udhBytes - 1) : numFields;
        if (!Decode8BitData(pdu)) {
            TELEPHONY_LOGE("8bit decode error");
            return false;
        }
    }
    pdu.SkipBits();
    return true;
}

uint8_t CdmaSmsUserData::DecodeHeader7Bit(SmsReadBuffer &pdu)
{
    uint8_t udhBytes = 0;
    if (headerInd_) {
        if (!pdu.ReadBits(udhBytes, BIT8) || (udhBytes > MAX_USER_DATA_LEN + 1)) {
            TELEPHONY_LOGE("udh bytes read error");
            return 0;
        }
    }
    data_.userData.headerCnt = 0;
    if (udhBytes > 0) {
        uint8_t index = 0;
        GsmUserDataPdu gsmUdPdu;
        for (uint8_t i = 0; index < udhBytes; i++) {
            if (static_cast<unsigned long>(i) >= (sizeof(data_.userData.header) / sizeof(data_.userData.header[0]))) {
                TELEPHONY_LOGE("user data header length error");
                return 0;
            }
            uint16_t length = 0;
            if (!gsmUdPdu.DecodeHeader(pdu, data_.userData.header[i], length)) {
                TELEPHONY_LOGE("decode header error");
                return 0;
            }
            if (length == 0) {
                TELEPHONY_LOGW("Decode header error, header len [%{public}d]", length);
                gsmUdPdu.ResetUserData(data_.userData);
                return 0;
            }
            index += length;
            if (index > udhBytes + HEX_STEP) {
                TELEPHONY_LOGW("Decode header error, over data [%{public}d > %{public}d + 2]", index, udhBytes);
                gsmUdPdu.ResetUserData(data_.userData);
                return 0;
            } else {
                data_.userData.headerCnt++;
            }
        }
    }
    return udhBytes;
}

bool CdmaSmsUserData::DecodeAscii7Bit(SmsReadBuffer &pdu, uint8_t numFields, uint8_t udhBytes)
{
    uint8_t v = 0;
    uint8_t udhLen = udhBytes == 0 ? 0 : (udhBytes + 1) * BIT8 / BIT7;
    if (udhLen > 0) {
        // The header data is encoded in 7bit, the remaining bits need to be ignored.
        uint8_t ignoredBits = BIT7 - (udhBytes + 1) * BIT8 % BIT7;
        if (ignoredBits < BIT7) {
            if (!pdu.ReadBits(v, ignoredBits)) {
                TELEPHONY_LOGE("read error");
                return false;
            }
            udhLen++;
        }
    }
    if (udhLen > numFields) {
        return false;
    }
    data_.userData.length = numFields - udhLen;
    if (static_cast<unsigned long>(data_.userData.length) >
        (sizeof(data_.userData.data) / sizeof(data_.userData.data[0]))) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    for (uint8_t i = 0; i < data_.userData.length; i++) {
        if (!pdu.ReadBits(v, BIT7)) {
            TELEPHONY_LOGE("data read error");
            return false;
        }
        data_.userData.data[i] = v;
    }
    pdu.SkipBits();
    return true;
}

bool CdmaSmsUserData::DecodeGsm7Bit(SmsReadBuffer &pdu, uint8_t numFields, uint8_t udhBytes)
{
    uint8_t udhLen = udhBytes == 0 ? 0 : (udhBytes + 1) * BIT8 / BIT7;
    if (udhLen > 0) {
        udhLen++;
        // The header data is encoded in 7bit, the remaining bits need to be ignored.
        uint8_t ignoredBits = BIT7 - (udhBytes + 1) * BIT8 % BIT7;
        if (ignoredBits != BIT7) {
            uint8_t v = 0;
            if (!pdu.ReadBits(v, BIT8)) {
                TELEPHONY_LOGE("read error");
                return false;
            }
            udhLen++;
        }
    }
    if (udhLen > numFields) {
        return false;
    }
    data_.userData.length = numFields - udhLen;
    uint8_t udBytes = data_.userData.length * BIT7 / BIT8;
    if (data_.userData.length * BIT7 % BIT8 > 0) {
        udBytes++;
    }
    if (udBytes > MAX_USER_DATA_LEN + 1) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    std::unique_ptr<uint8_t[]> dest = std::make_unique<uint8_t[]>(udBytes);
    if (dest == nullptr) {
        TELEPHONY_LOGE("make_unique error");
        return false;
    }
    for (uint8_t i = 0; i < udBytes; i++) {
        if (!pdu.ReadBits(dest[i], BIT8)) {
            TELEPHONY_LOGE("data read error");
            return false;
        }
    }
    SmsCommonUtils::Unpack7bitChar(
        dest.get(), udBytes, 0x00, reinterpret_cast<uint8_t *>(data_.userData.data), MAX_USER_DATA_LEN + 1);
    pdu.SkipBits();
    return true;
}

bool CdmaSmsUserData::Decode8BitData(SmsReadBuffer &pdu)
{
    if (static_cast<unsigned long>(data_.userData.length) >
        (sizeof(data_.userData.data) / sizeof(data_.userData.data[0]))) {
        TELEPHONY_LOGE("user data length error");
        return false;
    }
    uint8_t v = 0;
    for (uint8_t i = 0; i < data_.userData.length; i++) {
        if (!pdu.ReadBits(v, BIT8)) {
            TELEPHONY_LOGE("data read error");
            return false;
        }
        data_.userData.data[i] = v;
    }
    return true;
}

SmsEncodingType CdmaSmsUserData::GetEncodingType(uint8_t v)
{
    if (v <= static_cast<uint8_t>(SmsEncodingType::GSMDCS) || v == static_cast<uint8_t>(SmsEncodingType::EUCKR)) {
        return SmsEncodingType(v);
    } else {
        return SmsEncodingType::RESERVED;
    }
}

CdmaSmsCmasData::CdmaSmsCmasData(SmsTeleSvcCmasData &data) : data_(data)
{
    id_ = USER_DATA;
}

bool CdmaSmsCmasData::Encode(SmsWriteBuffer &pdu)
{
    TELEPHONY_LOGE("encode not support");
    return false;
}

bool CdmaSmsCmasData::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    if (memset_s(&data_, sizeof(SmsTeleSvcCmasData), 0x00, sizeof(SmsTeleSvcCmasData)) != EOK) {
        TELEPHONY_LOGE("memset_s fail");
        return false;
    }
    uint16_t index = pdu.GetIndex();
    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT5)) {
        TELEPHONY_LOGE("encode type read error");
        return false;
    }
    SmsEncodingType encodeType = CdmaSmsUserData::GetEncodingType(v);
    if (encodeType != SmsEncodingType::OCTET) {
        TELEPHONY_LOGE("wrong encode type [%{public}d], must be 0", v);
        return false;
    }
    if (!pdu.ReadBits(v, BIT8) || !pdu.ReadBits(v, BIT8)) {
        TELEPHONY_LOGE("protocol version read error");
        return false;
    }
    if (v != 0x00) {
        TELEPHONY_LOGE("wrong protocol version [%{public}d], must be 0", v);
        data_.isWrongRecodeType = true;
        return false;
    }

    while (pdu.GetIndex() < index + len_ - 1) {
        if (!DecodeData(pdu)) {
            TELEPHONY_LOGE("decode data error");
            return false;
        }
    }
    pdu.SkipBits();
    return true;
}

bool CdmaSmsCmasData::DecodeData(SmsReadBuffer &pdu)
{
    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT8)) {
        TELEPHONY_LOGE("type read error");
        return false;
    }
    if (v == 0x00) {
        if (!DecodeType0Data(pdu)) {
            TELEPHONY_LOGE("type 0 decode error");
            return false;
        }
    } else if (v == 0x01) {
        if (!DecodeType1Data(pdu)) {
            TELEPHONY_LOGE("type 1 decode error");
            return false;
        }
    } else if (v == 0x02) {
        if (!DecodeType2Data(pdu)) {
            TELEPHONY_LOGE("type 2 decode error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsCmasData::DecodeType0Data(SmsReadBuffer &pdu)
{
    uint8_t len = 0;
    uint8_t v = 0;
    if (!pdu.ReadBits(len, BIT8) || !pdu.ReadBits(v, BIT5)) {
        TELEPHONY_LOGE("len read error");
        return false;
    }
    SmsEncodingType encodeType = CdmaSmsUserData::GetEncodingType(v);
    if (encodeType == SmsEncodingType::ASCII_7BIT || encodeType == SmsEncodingType::IA5 ||
        encodeType == SmsEncodingType::GSM7BIT) {
        data_.dataLen = (len * BIT8 - BIT5) / BIT7;
        if (static_cast<unsigned long>(data_.dataLen) > (sizeof(data_.alertText) / sizeof(data_.alertText[0]))) {
            TELEPHONY_LOGE("alert text length error");
            return false;
        }
        for (uint16_t i = 0; i < data_.dataLen; i++) {
            if (!pdu.ReadBits(data_.alertText[i], BIT7)) {
                TELEPHONY_LOGE("alert text read error");
                return false;
            }
        }
        uint8_t ignoredBits = (len * BIT8 - BIT5) % BIT7;
        if (ignoredBits == BIT0) {
            return true;
        }
        return pdu.ReadBits(v, ignoredBits);
    } else if (encodeType == SmsEncodingType::EPM || encodeType == SmsEncodingType::GSMDCS) {
        TELEPHONY_LOGE("encode type not support");
        return false;
    } else {
        data_.dataLen = (len == 0) ? 0 : (len - 1);
        if (static_cast<unsigned long>(data_.dataLen) > (sizeof(data_.alertText) / sizeof(data_.alertText[0]))) {
            TELEPHONY_LOGE("alert text length invalid.");
            return false;
        }
        for (uint16_t i = 0; i < data_.dataLen; i++) {
            if (!pdu.ReadBits(data_.alertText[i], BIT8)) {
                TELEPHONY_LOGE("alert text read error");
                return false;
            }
        }
    }
    return true;
}

bool CdmaSmsCmasData::DecodeType1Data(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    uint8_t v3 = 0;
    uint8_t v4 = 0;
    uint8_t v5 = 0;
    if (!pdu.ReadBits(v1, BIT8)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    if (!pdu.ReadBits(v1, BIT8) || !pdu.ReadBits(v2, BIT8) || !pdu.ReadBits(v3, BIT4) || !pdu.ReadBits(v4, BIT4) ||
        !pdu.ReadBits(v5, BIT4)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    data_.category = static_cast<enum SmsCmaeCategory>(v1);
    data_.responseType = static_cast<enum SmsCmaeResponseType>(v2);
    data_.severity = static_cast<enum SmsCmaeSeverity>(v3);
    data_.urgency = static_cast<enum SmsCmaeUrgency>(v4);
    data_.certainty = static_cast<enum SmsCmaeCertainty>(v5);
    return pdu.ReadBits(v1, BIT4);
}

bool CdmaSmsCmasData::DecodeType2Data(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    if (!pdu.ReadBits(v1, BIT8)) {
        TELEPHONY_LOGE("read error");
        return false;
    }
    if (!pdu.ReadBits(v1, BIT8) || !pdu.ReadBits(v2, BIT8)) {
        TELEPHONY_LOGE("id read error");
        return false;
    }
    data_.id = (v1 << BIT8) + v2;
    if (!pdu.ReadBits(v1, BIT8)) {
        TELEPHONY_LOGE("alert handle read error");
        return false;
    }
    data_.alertHandle = static_cast<enum SmsCmaeAlertHandle>(v1);
    if (!DecodeAbsTime(pdu)) {
        TELEPHONY_LOGE("abs time decode error");
        return false;
    }
    if (!pdu.ReadBits(v1, BIT8)) {
        TELEPHONY_LOGE("language read error");
        return false;
    }
    data_.language = static_cast<enum SmsLanguageType>(v1);
    return true;
}

bool CdmaSmsCmasData::DecodeAbsTime(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("year read error");
        return false;
    }
    data_.expires.year = (v1 * DECIMAL_NUM) + v2;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("month read error");
        return false;
    }
    data_.expires.month = (v1 * DECIMAL_NUM) + v2;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("day read error");
        return false;
    }
    data_.expires.day = (v1 * DECIMAL_NUM) + v2;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("hour read error");
        return false;
    }
    data_.expires.hour = (v1 * DECIMAL_NUM) + v2;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("minute read error");
        return false;
    }
    data_.expires.minute = (v1 * DECIMAL_NUM) + v2;
    if (!pdu.ReadBits(v1, BIT4) || !pdu.ReadBits(v2, BIT4)) {
        TELEPHONY_LOGE("second read error");
        return false;
    }
    data_.expires.second = (v1 * DECIMAL_NUM) + v2;
    return true;
}

CdmaSmsAlertPriority::CdmaSmsAlertPriority(SmsAlertPriority &alertPriority) : alertPriority_(alertPriority)
{
    id_ = ALERT_ON_MSG_DELIVERY;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsAlertPriority::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(static_cast<uint8_t>(alertPriority_), BIT2)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsAlertPriority::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT2)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    pdu.SkipBits();
    alertPriority_ = SmsAlertPriority(v);
    return true;
}

CdmaSmsLanguageInd::CdmaSmsLanguageInd(SmsLanguageType &language) : language_(language)
{
    id_ = LANGUAGE_INDICATOR;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsLanguageInd::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_) && pdu.WriteByte(static_cast<uint8_t>(language_))) {
        return true;
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsLanguageInd::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadByte(v)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    language_ = SmsLanguageType(v);
    return true;
}

CdmaSmsCallbackNumber::CdmaSmsCallbackNumber(SmsTeleSvcAddr &address) : address_(address)
{
    id_ = CALLBACK_NUMBER;
}

bool CdmaSmsCallbackNumber::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (!pdu.WriteByte(id_)) {
        TELEPHONY_LOGE("id write error");
        return false;
    }
    uint16_t lenIndex = pdu.MoveForward();
    if (!pdu.WriteBits(address_.digitMode ? 0b1 : 0b0)) {
        TELEPHONY_LOGE("digit mode write error");
        return false;
    }
    if (static_cast<unsigned long>(address_.addrLen) > (sizeof(address_.szData) / sizeof(address_.szData[0]))) {
        TELEPHONY_LOGE("address length error");
        return false;
    }
    if (address_.digitMode) {
        // Type of number and Numbering plan
        if (!pdu.WriteBits(address_.numberType, BIT3) || !pdu.WriteBits(address_.numberPlan, BIT4) ||
            !pdu.WriteBits(address_.addrLen, BIT8)) {
            TELEPHONY_LOGE("number type, number plan or addrlen write error");
            return false;
        }

        for (uint8_t i = 0; i < address_.addrLen; i++) {
            if (!pdu.WriteBits(address_.szData[i], BIT8)) {
                TELEPHONY_LOGE("data write error");
                return false;
            }
        }
    } else {
        if (!pdu.WriteBits(address_.addrLen, BIT8)) {
            TELEPHONY_LOGE("addrlen write error");
            return false;
        }
        for (uint8_t i = 0; i < address_.addrLen; i++) {
            if (!pdu.WriteBits(SmsCommonUtils::DigitToDtmfChar(address_.szData[i]), BIT4)) {
                TELEPHONY_LOGE("data write error");
                return false;
            }
        }
    }
    len_ = pdu.SkipBits() - lenIndex - 1;
    return pdu.InsertByte(len_, lenIndex);
}

bool CdmaSmsCallbackNumber::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadBits(v)) {
        TELEPHONY_LOGE("digit mode read error");
        return false;
    }
    address_.digitMode = (v == 0b1) ? true : false;
    if (address_.digitMode) {
        // Type of number and Numbering plan
        if (!pdu.ReadBits(address_.numberType, BIT3) || !pdu.ReadBits(address_.numberPlan, BIT4)) {
            TELEPHONY_LOGE("number type or number plan read error");
            return false;
        }
    }

    if (!pdu.ReadBits(v, BIT8)) {
        TELEPHONY_LOGE("addrlen read error");
        return false;
    }
    address_.addrLen = v;
    if (static_cast<unsigned long>(address_.addrLen) > (sizeof(address_.szData) / sizeof(address_.szData[0]))) {
        TELEPHONY_LOGE("address length error");
        return false;
    }
    if (address_.digitMode) {
        for (uint8_t i = 0; i < address_.addrLen; i++) {
            if (!pdu.ReadBits(v, BIT8)) {
                TELEPHONY_LOGE("data read error");
                return false;
            }
            address_.szData[i] = v;
        }
    } else {
        for (uint8_t i = 0; i < address_.addrLen; i++) {
            if (!pdu.ReadBits(v, BIT4)) {
                TELEPHONY_LOGE("data read error");
                return false;
            }
            address_.szData[i] = SmsCommonUtils::DtmfCharToDigit(v);
        }
    }
    pdu.SkipBits();
    return true;
}

CdmaSmsDepositIndex::CdmaSmsDepositIndex(uint16_t &index) : index_(index)
{
    id_ = MESSAGE_DEPOSIT_INDEX;
    len_ = sizeof(index_);
}

bool CdmaSmsDepositIndex::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_) && pdu.WriteWord(index_)) {
        return true;
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsDepositIndex::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    return pdu.ReadWord(index_);
}

CdmaSmsDisplayMode::CdmaSmsDisplayMode(SmsDisplayMode &mode) : mode_(mode)
{
    id_ = MESSAGE_DISPLAY_MODE;
    len_ = sizeof(uint8_t);
}

bool CdmaSmsDisplayMode::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }

    if (pdu.WriteByte(id_) && pdu.WriteByte(len_)) {
        if (pdu.WriteBits(static_cast<uint8_t>(mode_), BIT2)) {
            pdu.SkipBits();
            return true;
        }
    }

    TELEPHONY_LOGE("encode error");
    return false;
}

bool CdmaSmsDisplayMode::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT2)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    pdu.SkipBits();
    mode_ = SmsDisplayMode(v);
    return true;
}

CdmaSmsMessageStatus::CdmaSmsMessageStatus(SmsStatusCode &status) : status_(status)
{
    id_ = MESSAGE_STATUS;
}

bool CdmaSmsMessageStatus::Encode(SmsWriteBuffer &pdu)
{
    TELEPHONY_LOGE("encode not support");
    return false;
}

bool CdmaSmsMessageStatus::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadByte(v)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    status_ = (SmsStatusCode)v;
    return true;
}

CdmaSmsNumberMessages::CdmaSmsNumberMessages(uint32_t &num) : num_(num)
{
    id_ = NUMBER_OF_MESSAGES;
}

bool CdmaSmsNumberMessages::Encode(SmsWriteBuffer &pdu)
{
    TELEPHONY_LOGE("encode not support");
    return false;
}

bool CdmaSmsNumberMessages::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    uint8_t v = 0;
    if (!pdu.ReadByte(v)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    num_ = (((v & 0xf0) >> BIT4) * DECIMAL_NUM) + (v & 0x0f);
    return true;
}

CdmaSmsEnhancedVmn::CdmaSmsEnhancedVmn(SmsEnhancedVmn &vmn) : vmn_(vmn)
{
    id_ = ENHANCED_VMN;
}

bool CdmaSmsEnhancedVmn::Encode(SmsWriteBuffer &pdu)
{
    TELEPHONY_LOGE("encode not support");
    return false;
}

bool CdmaSmsEnhancedVmn::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    if (memset_s(&vmn_, sizeof(SmsEnhancedVmn), 0x00, sizeof(SmsEnhancedVmn)) != EOK) {
        TELEPHONY_LOGE("memset_s fail");
        return false;
    }
    if (DecodeHeader(pdu) && DecodeVoiceMail(pdu) && DecodeAccessNumber(pdu) && DecodeCallingPartyNumber(pdu)) {
        pdu.SkipBits();
        return true;
    }

    TELEPHONY_LOGE("decode error");
    return false;
}

bool CdmaSmsEnhancedVmn::DecodeHeader(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    uint8_t v3 = 0;
    uint8_t v4 = 0;
    if (!pdu.ReadBits(v1, BIT2) || !pdu.ReadBits(v2, BIT1) || !pdu.ReadBits(v3, BIT1) || !pdu.ReadBits(v4, BIT1)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    vmn_.priority = SmsPriorityIndicator(v1);
    vmn_.passwordReq = (v2 == 0b1) ? true : false;
    vmn_.setupReq = (v3 == 0b1) ? true : false;
    vmn_.pwChangeReq = (v4 == 0b1) ? true : false;
    if (vmn_.setupReq || vmn_.pwChangeReq) {
        if (!pdu.ReadBits(vmn_.minPwLen, BIT4) || !pdu.ReadBits(vmn_.maxPwLen, BIT4)) {
            TELEPHONY_LOGE("min pwlen or max pwlen read error");
            return false;
        }
    }

    return true;
}

bool CdmaSmsEnhancedVmn::DecodeVoiceMail(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    uint8_t v3 = 0;
    uint8_t v4 = 0;
    if (!pdu.ReadBits(vmn_.vmNumUnheardMsg, BIT8)) {
        TELEPHONY_LOGE("num unheard msg read error");
        return false;
    }
    if (!pdu.ReadBits(v1, BIT1) || !pdu.ReadBits(v2, BIT1) || !pdu.ReadBits(v3, BIT1) || !pdu.ReadBits(v4, BIT1)) {
        TELEPHONY_LOGE("data read error");
        return false;
    }
    vmn_.vmMailboxAlmFull = (v1 == 0b1) ? true : false;
    vmn_.vmMailboxFull = (v2 == 0b1) ? true : false;
    vmn_.replyAllowed = (v3 == 0b1) ? true : false;
    vmn_.faxIncluded = (v4 == 0b1) ? true : false;

    if (!pdu.ReadBits(v1, BIT7) || !pdu.ReadBits(v2, BIT5) || !pdu.ReadBits(vmn_.vmRetDay, BIT7)) {
        TELEPHONY_LOGE("vmlen or vmretday read error");
        return false;
    }
    vmn_.vmLen = v1;
    vmn_.vmLen = (vmn_.vmLen << BIT5) | v2;

    if (!pdu.ReadBits(v1, BIT8) || !pdu.ReadBits(v2, BIT8) || !pdu.ReadBits(v3, BIT8) || !pdu.ReadBits(v4, BIT8)) {
        TELEPHONY_LOGE("msgid or mailboxid read error");
        return false;
    }
    vmn_.vmMsgId = v1;
    vmn_.vmMsgId = (vmn_.vmMsgId << BIT8) | v2;
    vmn_.vmMailboxId = v3;
    vmn_.vmMailboxId = (vmn_.vmMailboxId << BIT8) | v4;

    return true;
}

bool CdmaSmsEnhancedVmn::DecodeAccessNumber(SmsReadBuffer &pdu)
{
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    if (!pdu.ReadBits(v1, BIT1) || !pdu.ReadBits(v2, BIT3)) {
        TELEPHONY_LOGE("digitmode or numbertype read error");
        return false;
    }
    vmn_.anDigitMode = (v1 == 0b1) ? true : false;
    vmn_.anNumberType = v2;
    if (vmn_.anDigitMode) {
        if (!pdu.ReadBits(vmn_.anNumberPlan, BIT4) || !pdu.ReadBits(vmn_.anNumField, BIT8)) {
            TELEPHONY_LOGE("number plan or num field read error");
            return false;
        }
        if (static_cast<unsigned long>(vmn_.anNumField) >= (sizeof(vmn_.anChar) / sizeof(vmn_.anChar[0]))) {
            TELEPHONY_LOGE("enhancedVmn data length invalid.");
            return false;
        }
        for (uint8_t i = 0; i < vmn_.anNumField; i++) {
            if (!pdu.ReadBits(v1, BIT4)) {
                TELEPHONY_LOGE("char read error");
                return false;
            }
            vmn_.anChar[i] = SmsCommonUtils::DtmfCharToDigit(v1);
        }
        vmn_.anChar[vmn_.anNumField] = '\0';
    } else {
        if (!pdu.ReadBits(vmn_.anNumField, BIT8)) {
            TELEPHONY_LOGE("num field read error");
            return false;
        }
        if (static_cast<unsigned long>(vmn_.anNumField) >= (sizeof(vmn_.anChar) / sizeof(vmn_.anChar[0]))) {
            TELEPHONY_LOGE("enhancedVmn data length invalid.");
            return false;
        }
        for (uint8_t i = 0; i < vmn_.anNumField; i++) {
            if (!pdu.ReadBits(vmn_.anChar[i], BIT8)) {
                TELEPHONY_LOGE("char read error");
                return false;
            }
        }
    }
    return true;
}

bool CdmaSmsEnhancedVmn::DecodeCallingPartyNumber(SmsReadBuffer &pdu)
{
    uint8_t v = 0;
    if (!pdu.ReadBits(v, BIT1) || !pdu.ReadBits(vmn_.cliNumberType, BIT3)) {
        TELEPHONY_LOGE("digit mode or number type read error");
        return false;
    }
    vmn_.cliDigitMode = (v == 0b1) ? true : false;
    if (vmn_.cliDigitMode) {
        if (!pdu.ReadBits(vmn_.cliNumberPlan, BIT4) || !pdu.ReadBits(vmn_.cliNumField, BIT8)) {
            TELEPHONY_LOGE("number plan or num field read error");
            return false;
        }
        if (static_cast<unsigned long>(vmn_.cliNumField) >= (sizeof(vmn_.cliChar) / sizeof(vmn_.cliChar[0]))) {
            TELEPHONY_LOGE("enhancedVmn data length invalid.");
            return false;
        }
        for (uint8_t i = 0; i < vmn_.cliNumField; i++) {
            if (!pdu.ReadBits(v, BIT4)) {
                TELEPHONY_LOGE("char read error");
                return false;
            }
            vmn_.cliChar[i] = SmsCommonUtils::DtmfCharToDigit(v);
        }
        vmn_.cliChar[vmn_.cliNumField] = '\0';
    } else {
        if (!pdu.ReadBits(vmn_.cliNumField, BIT8)) {
            TELEPHONY_LOGE("num field read error");
            return false;
        }
        if (static_cast<unsigned long>(vmn_.cliNumField) >= (sizeof(vmn_.cliChar) / sizeof(vmn_.cliChar[0]))) {
            TELEPHONY_LOGE("enhancedVmn data length invalid.");
            return false;
        }
        for (uint8_t i = 0; i < vmn_.cliNumField; i++) {
            if (!pdu.ReadBits(vmn_.cliChar[i], BIT8)) {
                TELEPHONY_LOGE("char read error");
                return false;
            }
        }
    }
    return true;
}

CdmaSmsEnhancedVmnAck::CdmaSmsEnhancedVmnAck(SmsEnhancedVmnAck &ack) : ack_(ack)
{
    id_ = ENHANCED_VMN_ACK;
}

bool CdmaSmsEnhancedVmnAck::Encode(SmsWriteBuffer &pdu)
{
    TELEPHONY_LOGE("encode not support");
    return false;
}

bool CdmaSmsEnhancedVmnAck::Decode(SmsReadBuffer &pdu)
{
    if (IsInvalidPdu(pdu)) {
        TELEPHONY_LOGE("invalid pdu");
        return false;
    }

    if (memset_s(&ack_, sizeof(SmsEnhancedVmnAck), 0x00, sizeof(SmsEnhancedVmnAck)) != EOK) {
        TELEPHONY_LOGE("memset_s fail");
        return false;
    }
    if (!pdu.ReadWord(ack_.vmMailboxId) || !pdu.ReadByte(ack_.vmNumUnheardMsg)) {
        TELEPHONY_LOGE("mail boxid or num unheard msg read error");
        return false;
    }
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    if (!pdu.ReadBits(v1, BIT3) || !pdu.ReadBits(v2, BIT3)) {
        TELEPHONY_LOGE("delete ack or play ack read error");
        return false;
    }
    ack_.numDeleteAck = v1;
    ack_.numPlayAck = v2;
    if (static_cast<unsigned long>(ack_.numDeleteAck) > (sizeof(ack_.daVmMsgId) / sizeof(ack_.daVmMsgId[0]))) {
        TELEPHONY_LOGE("delect ack length error");
        return false;
    }
    for (uint8_t i = 0; i < ack_.numDeleteAck; i++) {
        if (!pdu.ReadBits(v1, BIT8) || !pdu.ReadBits(v2, BIT8)) {
            TELEPHONY_LOGE("msgid read error");
            return false;
        }
        ack_.daVmMsgId[i] = v1;
        ack_.daVmMsgId[i] = (ack_.daVmMsgId[i] << BIT8) | v2;
    }
    if (static_cast<unsigned long>(ack_.numPlayAck) > (sizeof(ack_.paVmMsgId) / sizeof(ack_.paVmMsgId[0]))) {
        TELEPHONY_LOGE("play ack length error");
        return false;
    }
    for (uint8_t i = 0; i < ack_.numPlayAck; i++) {
        if (!pdu.ReadBits(v1, BIT8) || !pdu.ReadBits(v2, BIT8)) {
            TELEPHONY_LOGE("msgid read error");
            return false;
        }
        ack_.paVmMsgId[i] = v1;
        ack_.paVmMsgId[i] = (ack_.paVmMsgId[i] << BIT8) | v2;
    }
    pdu.SkipBits();
    return true;
}

} // namespace Telephony
} // namespace OHOS
