/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "smscodecutils_fuzzer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "gsm_pdu_code_type.h"
#include "msg_text_convert_common.h"
#include "securec.h"
#include "sms_common_utils.h"
#include "sms_policy_utils.h"
#include "sms_pdu_code_type.h"
#include "string_utils.h"
#include "text_coder.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_TEXT_SIZE = 256;
constexpr size_t BUFFER_SIZE = 512;
constexpr uint8_t BIT_MOD = 8;
constexpr uint8_t MONTH_MOD = 12;
constexpr uint8_t DAY_MOD = 28;
constexpr uint8_t HOUR_MOD = 24;
constexpr uint8_t MINUTE_MOD = 60;
constexpr uint8_t DTMF_CHAR_A = 0x0A;
constexpr uint8_t DTMF_CHAR_B = 0x0B;
constexpr uint8_t DTMF_CHAR_C = 0x0C;
constexpr uint32_t CHARSET_MOD = 0x800;
constexpr uint32_t CODING_NATIONAL_MOD = 4;
constexpr uint32_t DATA_CODING_SCHEME_MOD = static_cast<uint32_t>(DATA_CODING_ASCII7BIT) + 1;
static_assert(DATA_CODING_SCHEME_MOD > 0, "Data coding scheme modulo must not be zero");

std::string MakeString(const uint8_t *data, size_t size)
{
    return std::string(reinterpret_cast<const char *>(data), size);
}

std::vector<uint8_t> MakeVector(const uint8_t *data, size_t size)
{
    size_t copySize = std::min(size, MAX_TEXT_SIZE);
    return std::vector<uint8_t>(data, data + copySize);
}

uint32_t MakeUint32(const uint8_t *data, size_t size)
{
    uint32_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0) {
        errno_t err = memcpy_s(&value, sizeof(value), data, copyLen);
        if (err != EOK) {
            return 0;
        }
    }
    return value;
}

std::array<uint8_t, BUFFER_SIZE> MakeArray(const uint8_t *data, size_t size)
{
    std::array<uint8_t, BUFFER_SIZE> buffer = {0};
    size_t copySize = std::min(size, buffer.size());
    if (copySize > 0) {
        errno_t err = memcpy_s(buffer.data(), buffer.size(), data, copySize);
        if (err != EOK) {
            buffer.fill(0);
        }
    }
    return buffer;
}

SmsTimeAbs MakeSmsTimeAbs(const uint8_t *data, size_t size)
{
    SmsTimeAbs timeAbs;
    uint32_t value = MakeUint32(data, size);
    timeAbs.year = static_cast<uint8_t>(value);
    timeAbs.month = static_cast<uint8_t>((value % MONTH_MOD) + 1);
    timeAbs.day = static_cast<uint8_t>((value % DAY_MOD) + 1);
    timeAbs.hour = static_cast<uint8_t>(value % HOUR_MOD);
    timeAbs.minute = static_cast<uint8_t>(value % MINUTE_MOD);
    timeAbs.second = static_cast<uint8_t>((value / MINUTE_MOD) % MINUTE_MOD);
    timeAbs.timeZone = static_cast<int32_t>(value % HOUR_MOD);
    return timeAbs;
}

void FuzzStringUtils(const uint8_t *data, size_t size)
{
    std::string rawText = MakeString(data, std::min(size, MAX_TEXT_SIZE));
    std::vector<uint8_t> rawVector = MakeVector(data, size);
    StringUtils::StringToHex(rawText);
    StringUtils::StringToHex(rawVector);
    StringUtils::StringToHex(reinterpret_cast<const char *>(data), static_cast<int>(std::min(size, MAX_TEXT_SIZE)));
    StringUtils::StringToHex(nullptr, static_cast<int>(std::min(size, MAX_TEXT_SIZE)));
    std::string hexText = StringUtils::StringToHex(rawText);
    StringUtils::HexToString(rawText);
    StringUtils::HexToString(hexText);
    StringUtils::HexToByteVector(rawText);
    StringUtils::HexToByteVector(hexText);
    StringUtils::ToUtf8(StringUtils::ToUtf16(rawText));
    StringUtils::ToUtf16(rawText);
    StringUtils::IsEmpty(rawText);
    StringUtils::IsEmpty(" \t\r\n");

    std::string url = rawText + "://host:" + std::to_string(size) + "/path?query=" + rawText;
    StringUtils::GetProtocolFromURL(url);
    StringUtils::GetHostnameFromURL(url);
    StringUtils::GetHostnameWithPortFromURL(url);
    StringUtils::GetPortFromURL(url);
    StringUtils::GetProtocolFromURL(rawText);
    StringUtils::GetHostnameFromURL(rawText);
    StringUtils::GetHostnameWithPortFromURL(rawText);
    StringUtils::GetPortFromURL(rawText);
}

void FuzzSmsCommonUtils(const uint8_t *data, size_t size)
{
    auto source = MakeArray(data, size);
    std::array<uint8_t, BUFFER_SIZE> packed = {0};
    std::array<uint8_t, BUFFER_SIZE> unpacked = {0};
    uint16_t dataLen = static_cast<uint16_t>(std::min(size, static_cast<size_t>(MAX_USER_DATA_LEN)));
    uint16_t packLen = static_cast<uint16_t>(packed.size());
    uint8_t fillBits = static_cast<uint8_t>(size % BIT_MOD);
    uint16_t packedLen = SmsCommonUtils::Pack7bitChar(source.data(), dataLen, fillBits, packed.data(), packLen);
    if (packedLen == 0) {
        packedLen = dataLen;
    }
    SmsCommonUtils::Pack7bitChar(nullptr, dataLen, fillBits, packed.data(), packLen);
    SmsCommonUtils::Pack7bitChar(source.data(), dataLen, fillBits, nullptr, packLen);
    SmsCommonUtils::Unpack7bitChar(packed.data(), packedLen, fillBits, unpacked.data(), packLen);
    SmsCommonUtils::Unpack7bitChar(nullptr, packedLen, fillBits, unpacked.data(), packLen);
    SmsCommonUtils::Unpack7bitCharForCBPdu(packed.data(), packedLen, fillBits, unpacked.data(), packLen);
    SmsCommonUtils::Unpack7bitCharForCBPdu(packed.data(), 0, fillBits, unpacked.data(), packLen);
    SmsCommonUtils::DtmfCharToDigit(static_cast<uint8_t>(MakeUint32(data, size)));
    SmsCommonUtils::DtmfCharToDigit(DTMF_CHAR_A);
    SmsCommonUtils::DtmfCharToDigit(DTMF_CHAR_B);
    SmsCommonUtils::DtmfCharToDigit(DTMF_CHAR_C);
    SmsCommonUtils::DigitToDtmfChar(static_cast<uint8_t>(MakeUint32(data, size)));
    SmsCommonUtils::DigitToDtmfChar('0');
    SmsCommonUtils::DigitToDtmfChar('*');
    SmsCommonUtils::DigitToDtmfChar('#');
    SmsTimeAbs timeAbs = MakeSmsTimeAbs(data, size);
    SmsCommonUtils::ConvertTime(timeAbs);
}

void FuzzTextCoder(const uint8_t *data, size_t size)
{
    auto source = MakeArray(data, size);
    std::array<uint8_t, BUFFER_SIZE> dest = {0};
    int srcLength = static_cast<int>(std::min(size, MAX_TEXT_SIZE));
    int maxLength = static_cast<int>(dest.size());
    std::string rawText = MakeString(data, std::min(size, MAX_TEXT_SIZE));
    std::string encodeString;
    TextCoder &textCoder = TextCoder::Instance();
    textCoder.GetEncodeString(encodeString, MakeUint32(data, size) % CHARSET_MOD,
        static_cast<uint32_t>(rawText.size()), rawText);

    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    textCoder.Utf8ToGsm7bit(dest.data(), maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToGsm7bit(nullptr, maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, source.data(), srcLength);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, nullptr, srcLength);
    DataCodingScheme scheme = static_cast<DataCodingScheme>(size % DATA_CODING_SCHEME_MOD);
    textCoder.CdmaUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme);
    auto nationalType = static_cast<SmsCodingNationalType>(size % CODING_NATIONAL_MOD);
    textCoder.GsmUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme, nationalType, langId);

    MsgLangInfo langInfo;
    langInfo.bSingleShift = (size % 2) == 0;
    langInfo.bLockingShift = (size % 3) == 0;
    langInfo.singleLang = static_cast<MSG_LANGUAGE_ID_T>(size);
    langInfo.lockingLang = static_cast<MSG_LANGUAGE_ID_T>(size >> 1);
    textCoder.Gsm7bitToUtf8(dest.data(), maxLength, source.data(), srcLength, langInfo);
    textCoder.Ucs2ToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.ShiftjisToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.GetUCS2Value(MakeUint32(data, size) % CHARSET_MOD);
}

void FuzzPolicyUtils()
{
    SmsPolicyUtils::IsSmsPolicyDisable();
    SmsPolicyUtils::IsMmsPolicyDisable();
}
} // namespace

void DoSmsCodecUtilsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }
    FuzzStringUtils(data, size);
    FuzzSmsCommonUtils(data, size);
    FuzzTextCoder(data, size);
    FuzzPolicyUtils();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsCodecUtilsFuzzTest(data, size);
    return 0;
}
