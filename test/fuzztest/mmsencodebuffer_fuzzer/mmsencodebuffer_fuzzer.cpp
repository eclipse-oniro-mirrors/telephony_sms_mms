/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "mmsencodebuffer_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
constexpr uint32_t TOKEN_TEXT_MAX_SIZE = 24;
constexpr uint32_t MAX_UINTVAR_VALUE = 0x1fffff;
constexpr uint32_t MAX_SHORT_LENGTH_VALUE = 40;
constexpr uint32_t MAX_VALUE_LENGTH_VALUE = 256;

static std::string MakeTokenText(FuzzedDataProvider &cursor)
{
    std::string token = cursor.ConsumeRandomLengthString(TOKEN_TEXT_MAX_SIZE);
    return token;
}

static void RunMmsEncodeBufferFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider cursor(data, size);
    MmsEncodeBuffer encodeBuffer;
    encodeBuffer.WriteByte(cursor.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeUintvar(cursor.ConsumeIntegralInRange<uint32_t>(0, MAX_UINTVAR_VALUE));
    encodeBuffer.EncodeShortLength(static_cast<uint8_t>(
        cursor.ConsumeIntegralInRange<uint32_t>(0, MAX_SHORT_LENGTH_VALUE)));
    encodeBuffer.EncodeValueLength(cursor.ConsumeIntegralInRange<uint32_t>(0,
        MAX_VALUE_LENGTH_VALUE));
    encodeBuffer.EncodeOctet(cursor.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeShortInteger(cursor.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeInteger(cursor.ConsumeIntegralInRange<uint32_t>(0,
        SmsMmsFuzz::MAX_HEADER_VALUE));
    encodeBuffer.EncodeLongInteger(cursor.ConsumeIntegral<uint64_t>());
    encodeBuffer.EncodeText(cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeQuotedText(cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeTokenText("token");
    encodeBuffer.EncodeTokenText(MakeTokenText(cursor));

    MmsEncodeBuffer nested;
    nested.EncodeText("nested");
    encodeBuffer.WriteBuffer(nested);
    std::vector<uint8_t> bytes = cursor.ConsumeBytes<uint8_t>(SmsMmsFuzz::MAX_BINARY_SIZE);
    encodeBuffer.WriteBuffer(SmsMmsFuzz::MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    encodeBuffer.ReadDataBuffer(0, encodeBuffer.GetCurPosition());
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsEncodeBufferFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
