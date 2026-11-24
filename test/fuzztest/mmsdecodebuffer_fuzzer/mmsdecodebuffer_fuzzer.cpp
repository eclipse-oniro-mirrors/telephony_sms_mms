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
static void ExerciseDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    auto buffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    uint8_t oneByte = 0;
    uint32_t value = 0;
    uint32_t count = 0;
    uint64_t integer = 0;
    std::string text;

    buffer->PeekOneByte(oneByte);
    buffer->DecodeIsShortInt();
    buffer->DecodeIsString();
    buffer->DecodeIsValueLength();
    buffer->MarkPosition();
    buffer->GetOneByte(oneByte);

    // Give each read the same starting position as a fresh buffer.
    buffer->UnMarkPosition();
    buffer->DecodeShortLength(oneByte);
    buffer->UnMarkPosition();
    buffer->DecodeValueLengthReturnLen(value, count);
    buffer->UnMarkPosition();
    buffer->DecodeValueLength(value);
    buffer->UnMarkPosition();
    buffer->DecodeUintvar(value, count);
    buffer->UnMarkPosition();
    buffer->DecodeText(text, value);
    buffer->UnMarkPosition();
    buffer->DecodeTokenText(text, value);
    buffer->UnMarkPosition();
    buffer->DecodeQuotedText(text, value);
    buffer->UnMarkPosition();
    buffer->DecodeShortInteger(oneByte);
    buffer->UnMarkPosition();
    buffer->DecodeLongInteger(integer);
    buffer->UnMarkPosition();
    buffer->DecodeInteger(integer);
    buffer->UnMarkPosition();
    buffer->IncreasePointer(0);
    buffer->DecreasePointer(0);
}

static void RunMmsDecodeBufferFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider cursor(data, size);
    ExerciseDecodeBuffer(cursor.ConsumeRemainingBytes<uint8_t>());
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsDecodeBufferFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
