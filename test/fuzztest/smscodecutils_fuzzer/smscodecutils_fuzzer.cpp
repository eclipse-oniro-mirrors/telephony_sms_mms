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
#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <vector>

#include "gsm_pdu_code_type.h"
#include "msg_text_convert_common.h"
#include "sms_common_utils.h"
#include "sms_fuzzer_utils.h"
#include "sms_pdu_code_type.h"
#include "string_utils.h"
#include "text_coder.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace SmsFuzz;

namespace {
constexpr size_t BUFFER_SIZE = 512;
constexpr uint8_t MAX_FILL_BITS = 7;
constexpr uint8_t MAX_MONTH = 11;
constexpr uint8_t MAX_DAY = 27;
constexpr uint8_t MAX_HOUR = 23;
constexpr uint8_t MAX_MINUTE = 59;
constexpr uint32_t MAX_CODING_NATIONAL = 4;
constexpr uint32_t MAX_DATA_CODING_SCHEME = static_cast<uint32_t>(DATA_CODING_ASCII7BIT) + 1;
constexpr uint32_t MAX_LANG_ID = 32;
constexpr size_t MAX_LOCKING_SHIFT = 2;
constexpr uint32_t GROUP_STRING_UTILS = 0;
constexpr uint32_t GROUP_SMS_COMMON_UTILS = 1;
constexpr uint32_t GROUP_TEXT_CODER = 2;
constexpr uint32_t GROUP_COUNT = GROUP_TEXT_CODER + 1;
static_assert(MAX_DATA_CODING_SCHEME > 0, "Data coding scheme upper bound must not be zero");

std::vector<uint8_t> MakeBytes(FuzzedDataProvider &fdp, size_t maxLen)
{
    size_t len = fdp.ConsumeIntegralInRange<size_t>(0, maxLen);
    return fdp.ConsumeBytes<uint8_t>(len);
}

uint16_t MakeDataLen(FuzzedDataProvider &fdp, size_t bufferSize, size_t maxLen)
{
    size_t limit = std::min(bufferSize, maxLen);
    return static_cast<uint16_t>(fdp.ConsumeIntegralInRange<size_t>(0, limit));
}

SmsTimeAbs MakeSmsTimeAbs(FuzzedDataProvider &fdp)
{
    SmsTimeAbs timeAbs;
    timeAbs.year = fdp.ConsumeIntegral<uint8_t>();
    timeAbs.month = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_MONTH) + 1;
    timeAbs.day = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_DAY) + 1;
    timeAbs.hour = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_HOUR);
    timeAbs.minute = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_MINUTE);
    timeAbs.second = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_MINUTE);
    timeAbs.timeZone = fdp.ConsumeIntegral<int32_t>();
    return timeAbs;
}

void FuzzStringUtils(FuzzedDataProvider &fdp)
{
    std::string rawText = ConsumeString(fdp);
    std::vector<uint8_t> rawVector = MakeBytes(fdp, MAX_STRING_SIZE);
    std::string url = ConsumeString(fdp);

    StringUtils::StringToHex(rawText);
    StringUtils::StringToHex(rawVector);
    StringUtils::StringToHex(rawText.c_str(), static_cast<int>(rawText.length()));
    StringUtils::StringToHex(nullptr, static_cast<int>(rawText.length()));
    std::string hexText = StringUtils::StringToHex(rawText);
    StringUtils::HexToString(rawText);
    StringUtils::HexToString(hexText);
    StringUtils::HexToByteVector(rawText);
    StringUtils::HexToByteVector(hexText);
    StringUtils::ToUtf8(StringUtils::ToUtf16(rawText));
    StringUtils::ToUtf16(rawText);
    StringUtils::IsEmpty(rawText);
    StringUtils::IsEmpty(" \t\r\n");

    StringUtils::GetProtocolFromURL(url);
    StringUtils::GetHostnameFromURL(url);
    StringUtils::GetHostnameWithPortFromURL(url);
    StringUtils::GetPortFromURL(url);
    StringUtils::GetProtocolFromURL(rawText);
    StringUtils::GetHostnameFromURL(rawText);
    StringUtils::GetHostnameWithPortFromURL(rawText);
    StringUtils::GetPortFromURL(rawText);
}

void FuzzSevenBitCodec(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> source = MakeBytes(fdp, BUFFER_SIZE);
    std::array<uint8_t, BUFFER_SIZE> packed = {0};
    std::array<uint8_t, BUFFER_SIZE> unpacked = {0};
    uint8_t fillBits = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_FILL_BITS);
    uint16_t dataLen = MakeDataLen(fdp, source.size(), MAX_USER_DATA_LEN);
    uint16_t packLen = fdp.ConsumeIntegralInRange<uint16_t>(0, static_cast<uint16_t>(packed.size()));
    SmsCommonUtils::Pack7bitChar(source.data(), dataLen, fillBits, packed.data(), packLen);
    SmsCommonUtils::Pack7bitChar(nullptr, dataLen, fillBits, packed.data(), packLen);
    SmsCommonUtils::Pack7bitChar(source.data(), dataLen, fillBits, nullptr, packLen);

    std::vector<uint8_t> rawTpdu = MakeBytes(fdp, BUFFER_SIZE);
    uint16_t tpduLen = MakeDataLen(fdp, rawTpdu.size(), MAX_USER_DATA_LEN);
    uint16_t unpackLen = fdp.ConsumeIntegralInRange<uint16_t>(0, static_cast<uint16_t>(unpacked.size()));
    SmsCommonUtils::Unpack7bitChar(rawTpdu.data(), tpduLen, fillBits, unpacked.data(), unpackLen);
    SmsCommonUtils::Unpack7bitChar(nullptr, tpduLen, fillBits, unpacked.data(), unpackLen);
    SmsCommonUtils::Unpack7bitCharForCBPdu(rawTpdu.data(), tpduLen, fillBits, unpacked.data(), unpackLen);
    SmsCommonUtils::Unpack7bitCharForCBPdu(rawTpdu.data(), 0, fillBits, unpacked.data(), unpackLen);
}

void FuzzSmsCommonUtils(FuzzedDataProvider &fdp)
{
    FuzzSevenBitCodec(fdp);
    SmsCommonUtils::DtmfCharToDigit(fdp.ConsumeIntegral<uint8_t>());
    SmsCommonUtils::DigitToDtmfChar(fdp.ConsumeIntegral<uint8_t>());
    SmsCommonUtils::ConvertTime(MakeSmsTimeAbs(fdp));
}

void FuzzTextCoder(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> source = MakeBytes(fdp, MAX_STRING_SIZE);
    std::array<uint8_t, BUFFER_SIZE> dest = {0};
    int srcLength = static_cast<int>(fdp.ConsumeIntegralInRange<size_t>(0, source.size()));
    int maxLength = static_cast<int>(fdp.ConsumeIntegralInRange<size_t>(0, dest.size()));
    std::string rawText = ConsumeString(fdp);
    std::string encodeString;
    TextCoder &textCoder = TextCoder::Instance();

    textCoder.GetEncodeString(encodeString, fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CHARSET),
        static_cast<uint32_t>(rawText.size()), rawText);
    MSG_LANGUAGE_ID_T langId = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_LANG_ID));
    textCoder.Utf8ToGsm7bit(dest.data(), maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToGsm7bit(nullptr, maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, source.data(), srcLength);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, nullptr, srcLength);

    auto scheme = static_cast<DataCodingScheme>(
        fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_DATA_CODING_SCHEME));
    textCoder.CdmaUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme);
    uint32_t nationalIndex = fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CODING_NATIONAL);
    auto nationalType = static_cast<SmsCodingNationalType>(nationalIndex);
    textCoder.GsmUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme, nationalType, langId);

    MsgLangInfo langInfo;
    langInfo.bSingleShift = fdp.ConsumeBool();
    langInfo.bLockingShift = fdp.ConsumeIntegralInRange<size_t>(0, MAX_LOCKING_SHIFT) == 0;
    langInfo.singleLang = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_LANG_ID));
    langInfo.lockingLang = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_LANG_ID));
    textCoder.Gsm7bitToUtf8(dest.data(), maxLength, source.data(), srcLength, langInfo);
    textCoder.Ucs2ToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.ShiftjisToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.GetUCS2Value(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CHARSET));
}
} // namespace

void DoSmsCodecUtilsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    switch (fdp.ConsumeIntegralInRange<uint32_t>(GROUP_STRING_UTILS, GROUP_COUNT - 1)) {
        case GROUP_STRING_UTILS:
            FuzzStringUtils(fdp);
            break;
        case GROUP_SMS_COMMON_UTILS:
            FuzzSmsCommonUtils(fdp);
            break;
        default:
            FuzzTextCoder(fdp);
            break;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DoSmsCodecUtilsFuzzTest(data, size);
    return 0;
}
