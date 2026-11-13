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

#include "addsmstoken_fuzzer.h"
#include "gsm_pdu_code_type.h"
#include "msg_text_convert_common.h"
#include "sms_common_utils.h"
#include "sms_pdu_code_type.h"
#include "string_utils.h"
#include "text_coder.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_TEXT_SIZE = 256;
constexpr size_t BUFFER_SIZE = 512;
constexpr uint8_t ASCII_PRINTABLE_BASE = 0x20;
constexpr uint8_t ASCII_PRINTABLE_RANGE = 0x5F;
constexpr uint8_t BIT_MOD = 8;
constexpr uint8_t MONTH_MOD = 12;
constexpr uint8_t DAY_MOD = 28;
constexpr uint8_t HOUR_MOD = 24;
constexpr uint8_t MINUTE_MOD = 60;
constexpr uint32_t CHARSET_MOD = 0x800;
constexpr uint32_t CODING_NATIONAL_MOD = 4;
constexpr uint32_t DATA_CODING_SCHEME_MOD = static_cast<uint32_t>(DATA_CODING_ASCII7BIT) + 1;
constexpr uint32_t LANG_ID_MOD = 32;
constexpr size_t BOOL_MOD = 2;
constexpr size_t LOCKING_SHIFT_MOD = 3;
constexpr uint32_t GROUP_COUNT = 3;
static_assert(DATA_CODING_SCHEME_MOD > 0, "Data coding scheme modulo must not be zero");

std::string MakeString(FuzzedDataProvider &fdp)
{
    size_t len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_TEXT_SIZE);
    std::vector<uint8_t> raw = fdp.ConsumeBytes<uint8_t>(len);
    std::string text;
    text.reserve(raw.size());
    for (uint8_t byte : raw) {
        text.push_back(static_cast<char>(ASCII_PRINTABLE_BASE + byte % ASCII_PRINTABLE_RANGE));
    }
    return text;
}

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
    timeAbs.month = fdp.ConsumeIntegralInRange<uint8_t>(0, MONTH_MOD - 1) + 1;
    timeAbs.day = fdp.ConsumeIntegralInRange<uint8_t>(0, DAY_MOD - 1) + 1;
    timeAbs.hour = fdp.ConsumeIntegralInRange<uint8_t>(0, HOUR_MOD - 1);
    timeAbs.minute = fdp.ConsumeIntegralInRange<uint8_t>(0, MINUTE_MOD - 1);
    timeAbs.second = fdp.ConsumeIntegralInRange<uint8_t>(0, MINUTE_MOD - 1);
    timeAbs.timeZone = fdp.ConsumeIntegral<int32_t>();
    return timeAbs;
}

void FuzzStringUtils(FuzzedDataProvider &fdp)
{
    std::string rawText = MakeString(fdp);
    std::vector<uint8_t> rawVector = MakeBytes(fdp, MAX_TEXT_SIZE);
    std::string url = MakeString(fdp);

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
    uint8_t fillBits = fdp.ConsumeIntegralInRange<uint8_t>(0, BIT_MOD - 1);
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
    std::vector<uint8_t> source = MakeBytes(fdp, MAX_TEXT_SIZE);
    std::array<uint8_t, BUFFER_SIZE> dest = {0};
    int srcLength = static_cast<int>(fdp.ConsumeIntegralInRange<size_t>(0, source.size()));
    int maxLength = static_cast<int>(fdp.ConsumeIntegralInRange<size_t>(0, dest.size()));
    std::string rawText = MakeString(fdp);
    std::string encodeString;
    TextCoder &textCoder = TextCoder::Instance();

    textCoder.GetEncodeString(encodeString, fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD),
        static_cast<uint32_t>(rawText.size()), rawText);
    MSG_LANGUAGE_ID_T langId = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, LANG_ID_MOD));
    textCoder.Utf8ToGsm7bit(dest.data(), maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToGsm7bit(nullptr, maxLength, source.data(), srcLength, langId);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, source.data(), srcLength);
    textCoder.Utf8ToUcs2(dest.data(), maxLength, nullptr, srcLength);

    auto scheme = static_cast<DataCodingScheme>(fdp.ConsumeIntegralInRange<uint32_t>(0, DATA_CODING_SCHEME_MOD));
    textCoder.CdmaUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme);
    uint32_t nationalIndex = fdp.ConsumeIntegralInRange<uint32_t>(0, CODING_NATIONAL_MOD);
    auto nationalType = static_cast<SmsCodingNationalType>(nationalIndex);
    textCoder.GsmUtf8ToAuto(dest.data(), maxLength, source.data(), srcLength, scheme, nationalType, langId);

    MsgLangInfo langInfo;
    langInfo.bSingleShift = fdp.ConsumeIntegralInRange<size_t>(0, BOOL_MOD - 1) == 0;
    langInfo.bLockingShift = fdp.ConsumeIntegralInRange<size_t>(0, LOCKING_SHIFT_MOD - 1) == 0;
    langInfo.singleLang = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, LANG_ID_MOD));
    langInfo.lockingLang = static_cast<MSG_LANGUAGE_ID_T>(fdp.ConsumeIntegralInRange<uint32_t>(0, LANG_ID_MOD));
    textCoder.Gsm7bitToUtf8(dest.data(), maxLength, source.data(), srcLength, langInfo);
    textCoder.Ucs2ToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.ShiftjisToUtf8(dest.data(), maxLength, source.data(), srcLength);
    textCoder.GetUCS2Value(fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD));
}
} // namespace

void DoSmsCodecUtilsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    switch (fdp.ConsumeIntegralInRange<uint32_t>(0, GROUP_COUNT - 1)) {
        case 0:
            FuzzStringUtils(fdp);
            break;
        case 1:
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
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsCodecUtilsFuzzTest(data, size);
    return 0;
}
