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

#include "mms_encode_string.h"

#include "mms_charset.h"
#include "sms_service_manager_client.h"
#include "telephony_log_wrapper.h"
#include "utils/mms_charset.h"

namespace OHOS {
namespace Telephony {
MmsEncodeString::MmsEncodeString() : charset_(0), strEncodeString_("") {}

MmsEncodeString::MmsEncodeString(const MmsEncodeString &obj)
{
    this->charset_ = obj.charset_;
    this->valLength_ = obj.valLength_;
    this->strEncodeString_ = obj.strEncodeString_;
}

MmsEncodeString::~MmsEncodeString() {}

/**
 * @brief DecodeEncodeString
 * OMA-TS-MMS_CONF-V1_3-20110913-A   section:7.3.19 Encoded-String-Value
 * Encoded-string-value = Text-string | Value-length Char-set Text-string
 * End-of-string = <Octet 0>
 * @param decodeBuffer
 * @return true
 * @return false
 */
bool MmsEncodeString::DecodeEncodeString(MmsDecodeBuffer &decodeBuffer)
{
    uint8_t oneByte = 0;
    const uint8_t maxHasCharsetNum = 30;
    const uint8_t lengthQuote = 0x1f;
    strEncodeString_.clear();
    if (!decodeBuffer.PeekOneByte(oneByte)) {
        TELEPHONY_LOGE("Decode encodeString PeekOneByte fail.");
        return false;
    }

    if (oneByte == 0) {
        strEncodeString_.clear();
        decodeBuffer.IncreasePointer(1);
        TELEPHONY_LOGE("Decode encodeString DecodeEncodeString fail.");
        return true;
    }

    if (oneByte <= maxHasCharsetNum || oneByte == lengthQuote) {
        if (!decodeBuffer.DecodeValueLength(valLength_)) {
            TELEPHONY_LOGE("Decode encodeString DecodeValueLength fail.");
            return false;
        }
        uint64_t charset = 0;
        if (!decodeBuffer.DecodeInteger(charset)) {
            TELEPHONY_LOGE("Decode encodeString DecodeInteger fail.");
            return false;
        }
        charset_ = static_cast<uint32_t>(charset);
    }

    uint32_t len = 0;
    if (!decodeBuffer.DecodeText(strEncodeString_, len)) {
        TELEPHONY_LOGE("Decode encodeString DecodeText fail.");
        return false;
    }
    valLength_ = len;
    return true;
}

/**
 * @brief EncodeEncodeString
 * OMA-TS-MMS_CONF-V1_3-20110913-A   section:7.3.19 Encoded-String-Value
 * Encoded-string-value = Text-string | Value-length Char-set Text-string
 * End-of-string = <Octet 0>
 * @param encodeBuffer
 * @return true
 * @return false
 */
bool MmsEncodeString::EncodeEncodeString(MmsEncodeBuffer &encodeBuffer)
{
    MmsEncodeBuffer tempBuffer;
    if (charset_ != 0) {
        if ((charset_ & 0xFF00) == 0) {
            tempBuffer.EncodeShortInteger(charset_);
        } else {
            tempBuffer.EncodeInteger(charset_);
        }
        if (!tempBuffer.EncodeText(strEncodeString_)) {
            TELEPHONY_LOGE("EncodeString EncodeText fail.");
            return false;
        }
        if (!encodeBuffer.EncodeValueLength(tempBuffer.GetCurPosition())) {
            TELEPHONY_LOGE("EncodeString EncodeValueLength fail.");
            return false;
        }
        if (!encodeBuffer.WriteBuffer(tempBuffer)) {
            TELEPHONY_LOGE("EncodeString WriteBuffer fail.");
            return false;
        }
        return true;
    }
    if (!encodeBuffer.EncodeText(strEncodeString_)) {
        TELEPHONY_LOGE("EncodeString EncodeText fail.");
        return false;
    }
    return true;
}

bool MmsEncodeString::GetEncodeString(std::string &encodeString)
{
    bool ret = Singleton<SmsServiceManagerClient>::GetInstance().
        GetEncodeStringFunc(encodeString, charset_, valLength_, strEncodeString_);
    if (encodeString.empty()) {
        encodeString = strEncodeString_;
    }
    return ret;
}

bool MmsEncodeString::SetEncodeString(uint32_t charset, const std::string &encodeString)
{
    valLength_ = encodeString.length();
    strEncodeString_ = encodeString;

    if (charset == 0) {
        charset = CHARSET_UTF8;
    }
    charset_ = charset;
    return true;
}

bool MmsEncodeString::SetAddressString(MmsAddress &addrsss)
{
    std::string enString = addrsss.GetAddressString();
    MmsCharSets chartSets = addrsss.GetAddressCharset();
    return SetEncodeString(static_cast<uint32_t>(chartSets), enString);
}

std::string MmsEncodeString::GetStrEncodeString()
{
    return strEncodeString_;
}
} // namespace Telephony
} // namespace OHOS
