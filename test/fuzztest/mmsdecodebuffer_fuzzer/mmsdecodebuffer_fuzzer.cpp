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

#include "mmsdecodebuffer_fuzzer.h"

#include "mms_fuzzer_utils.h"

#define MMS_VALUE_LENGTH_MARK_VALUE 0x1f
#define MMS_UINTVAR_CONTINUE_BYTE_VALUE 0x81
#define MMS_SAMPLE_ONE_BYTE_VALUE 0x01
#define MMS_SAMPLE_SHORT_INTEGER_VALUE 0x82

namespace OHOS {
namespace Telephony {
constexpr uint8_t VALUE_LENGTH_MARK = MMS_VALUE_LENGTH_MARK_VALUE;
constexpr uint8_t UINTVAR_CONTINUE_BYTE = MMS_UINTVAR_CONTINUE_BYTE_VALUE;
constexpr uint8_t SAMPLE_ONE_BYTE = MMS_SAMPLE_ONE_BYTE_VALUE;
constexpr uint8_t SAMPLE_SHORT_INTEGER = MMS_SAMPLE_SHORT_INTEGER_VALUE;

#undef MMS_VALUE_LENGTH_MARK_VALUE
#undef MMS_UINTVAR_CONTINUE_BYTE_VALUE
#undef MMS_SAMPLE_ONE_BYTE_VALUE
#undef MMS_SAMPLE_SHORT_INTEGER_VALUE

static void ExerciseDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    auto buffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    uint8_t oneByte = SmsMmsFuzz::ZERO_BYTE;
    uint32_t value = SmsMmsFuzz::EMPTY_SIZE;
    uint32_t count = SmsMmsFuzz::EMPTY_SIZE;
    uint64_t integer = static_cast<uint64_t>(SmsMmsFuzz::EMPTY_SIZE);
    std::string text;
    buffer->PeekOneByte(oneByte);
    buffer->DecodeIsShortInt();
    buffer->DecodeIsString();
    buffer->DecodeIsValueLength();
    buffer->MarkPosition();
    buffer->GetOneByte(oneByte);
    buffer->UnMarkPosition();
    buffer->DecodeShortLength(oneByte);
    buffer->DecodeValueLengthReturnLen(value, count);
    buffer->DecodeValueLength(value);
    buffer->DecodeUintvar(value, count);
    buffer->DecodeText(text, value);
    buffer->DecodeTokenText(text, value);
    buffer->DecodeQuotedText(text, value);
    buffer->DecodeShortInteger(oneByte);
    buffer->DecodeLongInteger(integer);
    buffer->DecodeInteger(integer);
    buffer->IncreasePointer(SmsMmsFuzz::EMPTY_SIZE);
    buffer->DecreasePointer(SmsMmsFuzz::EMPTY_SIZE);
}

static void RunMmsDecodeBufferFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    ExerciseDecodeBuffer(cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE));
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
