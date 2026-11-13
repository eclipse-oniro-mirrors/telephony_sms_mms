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

#include "mmsdecodebuffer_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
constexpr uint8_t VALUE_LENGTH_MARK = 0x1f;
constexpr uint8_t UINTVAR_CONTINUE_BYTE = 0x81;
constexpr uint8_t SAMPLE_ONE_BYTE = 0x01;
constexpr uint8_t SAMPLE_SHORT_INTEGER = 0x82;

static void ExerciseDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    uint8_t oneByte = SmsMmsFuzz::ZERO_BYTE;
    uint32_t value = SmsMmsFuzz::EMPTY_SIZE;
    uint32_t count = SmsMmsFuzz::EMPTY_SIZE;
    uint64_t integer = static_cast<uint64_t>(SmsMmsFuzz::EMPTY_SIZE);
    std::string text;
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->PeekOneByte(oneByte);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeIsShortInt();
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeIsString();
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeIsValueLength();

    auto markedBuffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    markedBuffer->MarkPosition();
    markedBuffer->GetOneByte(oneByte);
    markedBuffer->UnMarkPosition();

    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeShortLength(oneByte);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeValueLengthReturnLen(value, count);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeValueLength(value);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeUintvar(value, count);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeText(text, value);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeTokenText(text, value);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeQuotedText(text, value);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeShortInteger(oneByte);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeLongInteger(integer);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecodeInteger(integer);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->IncreasePointer(SmsMmsFuzz::EMPTY_SIZE);
    SmsMmsFuzz::MakeDecodeBuffer(bytes)->DecreasePointer(SmsMmsFuzz::EMPTY_SIZE);
}

static void RunMmsDecodeBufferFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider rawProvider(data, size);
    ExerciseDecodeBuffer(rawProvider.ConsumeRemainingBytes<uint8_t>());
    FuzzedDataProvider cursor(data, size);
    ExerciseDecodeBuffer(cursor.ConsumeBytes<uint8_t>(SmsMmsFuzz::MAX_BINARY_SIZE));
    ExerciseDecodeBuffer({VALUE_LENGTH_MARK, UINTVAR_CONTINUE_BYTE, SAMPLE_ONE_BYTE, 't', 'e', 'x', 't',
        SmsMmsFuzz::ZERO_BYTE});
    ExerciseDecodeBuffer({SmsMmsFuzz::SHORT_INTEGER_MASK, 'q', 'u', 'o', 't', 'e', SmsMmsFuzz::ZERO_BYTE});
    ExerciseDecodeBuffer({UINTVAR_CONTINUE_BYTE, SAMPLE_ONE_BYTE, SAMPLE_SHORT_INTEGER, SmsMmsFuzz::ZERO_BYTE});
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsDecodeBufferFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
