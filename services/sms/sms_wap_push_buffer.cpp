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

#include "sms_wap_push_buffer.h"

#include "securec.h"
#include "sms_constants_utils.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
SmsWapPushBuffer::SmsWapPushBuffer() {}

SmsWapPushBuffer::~SmsWapPushBuffer()
{
    if (pduBuffer_ != nullptr) {
        pduBuffer_.reset();
    }
}

std::unique_ptr<char[]> SmsWapPushBuffer::ReadDataBuffer(uint32_t desLen)
{
    return ReadDataBuffer(curPosition_, desLen);
}

std::unique_ptr<char[]> SmsWapPushBuffer::ReadDataBuffer(uint32_t offset, uint32_t desLen)
{
    if (desLen > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("desLen over size error");
        return nullptr;
    }
    if ((desLen > totolLength_) || ((static_cast<uint64_t>(offset) + desLen) > totolLength_)) {
        return nullptr;
    }
    std::unique_ptr<char[]> result = std::make_unique<char[]>(desLen);
    if (result == nullptr) {
        return nullptr;
    }
    if (memcpy_s(result.get(), desLen, &pduBuffer_[offset], desLen) != EOK) {
        return nullptr;
    }
    return result;
}

bool SmsWapPushBuffer::WriteRawStringBuffer(const std::string &inSrc)
{
    uint32_t len = inSrc.length();
    if (len > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("create wappush input source string over length error.");
        return false;
    }

    if (pduBuffer_) {
        pduBuffer_.reset();
    }

    pduBuffer_ = std::make_unique<char[]>(len);
    if (!pduBuffer_) {
        TELEPHONY_LOGE("wap push make unique fail.");
        return false;
    }
    if (memcpy_s(pduBuffer_.get(), len, inSrc.data(), len) != EOK) {
        TELEPHONY_LOGE("wap push memcpy_s fail.");
        return false;
    }
    totolLength_ = len;
    return true;
}

bool SmsWapPushBuffer::WriteDataBuffer(std::unique_ptr<char[]> inBuff, uint32_t len)
{
    if (inBuff == nullptr) {
        TELEPHONY_LOGE("wap push inBuff is null.");
        return false;
    }
    if (len <= 0 || len > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("wap push len invalid .");
        return false;
    }

    if (pduBuffer_) {
        pduBuffer_.reset();
    }

    pduBuffer_ = std::make_unique<char[]>(len);
    if (!pduBuffer_) {
        TELEPHONY_LOGE("wap push make unique fail.");
        return false;
    }
    if (memcpy_s(pduBuffer_.get(), len, inBuff.get(), len) != EOK) {
        TELEPHONY_LOGE("wap push memcpy_s fail.");
        return false;
    }
    totolLength_ = len;
    return true;
}

uint32_t SmsWapPushBuffer::GetCurPosition() const
{
    return curPosition_;
}

uint32_t SmsWapPushBuffer::GetSize() const
{
    return totolLength_;
}

bool SmsWapPushBuffer::PeekOneByte(uint8_t &oneByte)
{
    if (curPosition_ >= totolLength_) {
        TELEPHONY_LOGE("wap push current position invalid.");
        return false;
    }
    oneByte = pduBuffer_[curPosition_];
    return true;
}

bool SmsWapPushBuffer::GetOneByte(uint8_t &oneByte)
{
    if (curPosition_ >= totolLength_) {
        TELEPHONY_LOGE("wap push current position invalid.");
        return false;
    }
    oneByte = pduBuffer_[curPosition_];
    curPosition_++;
    return true;
}

bool SmsWapPushBuffer::IncreasePointer(uint32_t offset)
{
    if ((offset > totolLength_) || ((curPosition_ + offset) > totolLength_)) {
        TELEPHONY_LOGE("wap push current position invalid.");
        return false;
    }
    curPosition_ += offset;
    return true;
}

bool SmsWapPushBuffer::DecreasePointer(uint32_t offset)
{
    if (offset > curPosition_) {
        TELEPHONY_LOGE("wap push current position invalid.");
        return false;
    }
    curPosition_ -= offset;
    return true;
}

/**
 * @brief DecodeUintvar
 * wap-230-wsp-20010705-a   section:8.4.2.1 Basic rules
 * Uintvar-integer = 1*5 OCTET
 * @param uintVar
 * @param count
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeUintvar(uint32_t &uintVar, uint32_t &count)
{
    const uint8_t setNotHighestBitZero = 0x80;
    const uint8_t setHighestBitZero = 0x7f;

    count = 0;
    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    count++;
    uintVar = 0;
    while ((oneByte & setNotHighestBitZero) != 0) {
        uintVar = uintVar << 0x07;
        uintVar |= oneByte & setHighestBitZero;

        if (!GetOneByte(oneByte)) {
            TELEPHONY_LOGE("wap push GetOneByte fail.");
            return false;
        }
        count++;
    }
    uintVar = uintVar << 0x07;
    uintVar |= oneByte & setHighestBitZero;
    return true;
}

/**
 * @brief DecodeShortLength
 * wap-230-wsp-20010705-a.pdf   section:8.4.2.2 Length
 * Short-length = <Any octet 0-30>
 * @param sValueLength
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeShortLength(uint8_t &sValueLength)
{
    const unsigned char minShortOctet = 30;

    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    if (oneByte <= minShortOctet) {
        sValueLength = oneByte;
        return true;
    }
    return false;
}

/**
 * @brief DecodeValueLengthReturnLen
 * value-length = short-length | (Length-quote Length)
 * short-length = 0~30
 * Length-quote = 31
 * (Length-quote Length) = Length-quote + Uintvar-length
 * @param valueLength
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeValueLengthReturnLen(uint32_t &valueLength, uint32_t &length)
{
    const uint8_t lengthQuote = 0x1f;
    const uint8_t maxShortLength = 30;

    length = 0;
    valueLength = 0;
    uint32_t uintvar = 0;
    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        DecreasePointer(1);
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }

    if (oneByte <= maxShortLength) {
        valueLength = oneByte;
        length = 1;
        return true;
    } else if (oneByte == lengthQuote) {
        /* Length-quote = 0x1F */
        uint32_t count = 0;
        if (!DecodeUintvar(uintvar, count)) {
            TELEPHONY_LOGE("wap push DecodeUintvar fail.");
            return false;
        }
        valueLength = uintvar;
        length = count + 1;
        return true;
    }
    return false;
}

/**
 * @brief DecodeValueLength
 * value-length = short-length | (Length-quote Length)
 * short-length = 0~30
 * Length-quote = 31
 * (Length-quote Length) = Length-quote + Uintvar-length
 * @param valueLength
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeValueLength(uint32_t &valueLength)
{
    const uint8_t lengthQuote = 0x1f;
    const uint8_t maxShortLength = 30;

    valueLength = 0;
    uint32_t uintvar = 0;
    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        DecreasePointer(1);
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }

    if (oneByte <= maxShortLength) {
        valueLength = oneByte;
        return true;
    } else if (oneByte == lengthQuote) {
        /* Length-quote = 0x1F */
        uint32_t count = 0;
        if (!DecodeUintvar(uintvar, count)) {
            TELEPHONY_LOGE("wap push DecodeUintvar fail.");
            return false;
        }
        valueLength = uintvar;
        return true;
    }
    return false;
}

/**
 * @brief CharIsToken
 * RFC2616   section:2.2 Basic Rules
 * CHAR           = <any US-ASCII character (octets 0 - 127)>
 * CTL            = <any US-ASCII control character
 *                   (octets 0 - 31) and DEL (127)>
 * separators     = "(" | ")" | "<" | ">" | "@"
 *                     | "," | ";" | ":" | "\" | <">
 *                     | "/" | "[" | "]" | "?" | "="
 *                     | "{" | "}" | SP | HT
 * @param oneByte
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::CharIsToken(uint8_t oneByte)
{
    const uint8_t minToken = 33;
    const uint8_t maxToken = 126;

    if (oneByte < minToken || oneByte > maxToken) {
        return false;
    }

    std::set<uint8_t> token = { '"', '(', ')', '/', ',', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '{', '}' };
    return !token.count(oneByte);
}

/**
 * @brief DecodeTokenText
 * wap-230-wsp-20010705-a   section:8.4.2.2 Length
 * Token-text = Token End-of-string
 * End-of-string = <Octet 0>
 * @param str
 * @param len
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeTokenText(std::string &str, uint32_t &len)
{
    const uint8_t minToken = 33;
    const uint8_t maxToken = 126;

    len = 0;
    str.clear();
    uint8_t oneByte = 0;
    if (!PeekOneByte(oneByte)) {
        TELEPHONY_LOGE("wap push PeekOneByte fail.");
        return false;
    }
    if (oneByte < minToken || oneByte > maxToken) {
        TELEPHONY_LOGE("wap push DecodeTokenText fail.");
        return false;
    }
    while (GetOneByte(oneByte)) {
        if (oneByte != 0) {
            if (CharIsToken(oneByte)) {
                str += oneByte;
                len++;
            }
        } else {
            break;
        }
    }
    return (len > 0) ? true : false;
}

/**
 * @brief DecodeText
 * wap-230-wsp-20010705-a   section:8.4.2.1 Basic rules
 * Text-string = [Quote] *TEXT End-of-string
 * Quote = <Octet 127>
 * End-of-string = <Octet 0>
 * @param str
 * @param len
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeText(std::string &str, uint32_t &len)
{
    const unsigned char quoteChar = 127;

    len = 0;
    str.clear();
    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }

    // ignore quote
    if (oneByte != quoteChar) {
        this->DecreasePointer(1);
    } else {
        len++;
    }

    while (GetOneByte(oneByte)) {
        if (oneByte != 0) {
            str += oneByte;
            len++;
        } else {
            break;
        }
    }
    return (len > 0) ? true : false;
}

/**
 * @brief DecodeQuotedText
 * wap-230-wsp-20010705-a   section:8.4.2.1 Basic rules
 * Quoted-string = <Octet 34> *TEXT End-of-string
 * The TEXT encodes an RFC2616 Quoted-string with the enclosing quotation-marks <"> removed
 * @param str
 * @param len
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeQuotedText(std::string &str, uint32_t &len)
{
    const uint8_t quotedFlag = 0x22;
    uint8_t oneByte = 0;
    len = 0;
    str.clear();
    while (GetOneByte(oneByte)) {
        if (oneByte != 0) {
            if (oneByte != quotedFlag) {
                str += oneByte;
            }
            len++;
        } else {
            break;
        }
    }
    return (len > 0) ? true : false;
}

/**
 * @brief DecodeShortInteger
 * wap-230-wsp-20010705-a   section:8.4.2.1 Basic rules
 * Short-integer = OCTET
 * Integers in range 0-127 shall be encoded as a one octet value with
 * the most significant bit set to one (1xxx xxxx) and with the value
 * in the remaining least significant bits
 * @param sInteger
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeShortInteger(uint8_t &sInteger)
{
    const uint8_t setHighestBitZero = 0x7f;
    const uint8_t maxShortInteger = 0x7f;

    sInteger = 0;
    uint8_t oneByte = 0;
    if (!GetOneByte(oneByte)) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    if (oneByte > maxShortInteger) {
        sInteger = oneByte & setHighestBitZero;
        return true;
    }
    return false;
}

/**
 * @brief DecodeLongInteger
 * Long-integer = [Short-length] [Multi-octet-integer]
 * Short-length = 1 byte
 * Multi-octet-integer = <Short-length> bytes
 * @param value
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeLongInteger(uint64_t &value)
{
    unsigned char oneByte = 0;
    uint64_t result = 0;
    uint32_t valuelength = 0;
    if (GetOneByte(oneByte) != true) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    if (oneByte > LONG_VALUE_LEN_MAX) {
        return false;
    }
    valuelength = oneByte;

    for (uint32_t i = 0; i < valuelength; i++) {
        if (GetOneByte(oneByte) != true) {
            TELEPHONY_LOGE("wap push GetOneByte fail.");
            return false;
        }
        result = (result << 0x08);
        result |= oneByte;
    }
    value = result;
    return true;
}

/**
 * @brief DecodeInteger
 * Integer-Value = Short-integer | Long-integer
 * @param iInteger
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeInteger(uint64_t &iInteger)
{
    iInteger = 0;
    if (DecodeIsShortInt()) {
        uint8_t sInteger = 0;
        if (DecodeShortInteger(sInteger)) {
            iInteger = sInteger;
            return true;
        }
    } else {
        if (DecodeLongInteger(iInteger)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief DecodeIsShortInt
 * (128 - 255) It is an encoded 7-bit value; this header has no more data
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeIsShortInt()
{
    uint8_t oneByte = 0;
    PeekOneByte(oneByte);
    if (oneByte > 0x7F) {
        return true;
    }
    return false;
}

/**
 * @brief DecodeIsString
 * (32 - 127) The value is a text string, terminated by a zero octet (NUL character)
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeIsString()
{
    uint8_t oneByte = 0;
    PeekOneByte(oneByte);
    if (0x20 <= oneByte && 0x7f >= oneByte) {
        return true;
    }
    return false;
}

/**
 * @brief DecodeIsValueLength
 * (0 - 30) This octet is followed by the indicated number (0 –30) of data octets
 * 31 This octet is followed by a uintvar, which indicates the number of data octets after it
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeIsValueLength()
{
    uint8_t oneByte = 0;
    PeekOneByte(oneByte);
    if (oneByte <= 0x1f) {
        return true;
    }
    return false;
}

/**
 * @brief DecodeExtensionMedia
 *
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeExtensionMedia()
{
    return false;
}

/**
 * @brief DecodeConstrainedEncoding
 *
 * @return true
 * @return false
 */
bool SmsWapPushBuffer::DecodeConstrainedEncoding()
{
    return false;
}

bool SmsWapPushBuffer::DecodeTextValue(std::string &str, bool &isNoValue)
{
    const uint8_t minTextToken = 31;

    str.clear();
    unsigned char oneByte = 0;
    if (GetOneByte(oneByte) != true) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    if ((oneByte & 0x80) == 0x00 && oneByte > minTextToken) {
        std::string field = "";
        uint32_t fieldLength = 0;
        if (!DecodeTokenText(field, fieldLength)) {
            TELEPHONY_LOGE("wap push DecodeTokenText fail.");
            return false;
        }
    } else {
        uint64_t fieldId = 0;
        if (!DecodeInteger(fieldId)) {
            TELEPHONY_LOGE("wap push DecodeInteger fail.");
            return false;
        }
    }
    bool bNoValue = false;
    if (!DecodeNoValue(bNoValue)) {
        TELEPHONY_LOGE("wap push DecodeNoValue fail.");
        return false;
    }
    if (bNoValue) {
        isNoValue = true;
        return true;
    } else {
        if (DecreasePointer(1) != true) {
            TELEPHONY_LOGE("wap push DecreasePointer fail.");
            return false;
        }
    }
    uint64_t fieldValue = 0;
    if (DecodeInteger(fieldValue)) {
        str = std::to_string(fieldValue);
        return true;
    }
    uint32_t valueLength = 0;
    if (DecodeTokenText(str, valueLength)) {
        if (str.length() > 1 && str.at(0) == '\"') {
            str = str.substr(1, str.length() - 1);
        }
        return true;
    }
    return false;
}

bool SmsWapPushBuffer::DecodeNoValue(bool &isNoValue)
{
    unsigned char oneByte = 0;
    if (GetOneByte(oneByte) != true) {
        TELEPHONY_LOGE("wap push GetOneByte fail.");
        return false;
    }
    isNoValue = (oneByte == 0) ? true : false;
    return true;
}

void SmsWapPushBuffer::MarkPosition()
{
    savePosition_ = curPosition_;
}

void SmsWapPushBuffer::UnMarkPosition()
{
    curPosition_ = savePosition_;
}
} // namespace Telephony
} // namespace OHOS
