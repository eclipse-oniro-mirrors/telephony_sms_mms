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

#include "mmsencodebuffer_fuzzer.h"

#include "mms_fuzzer_utils.h"

#define MMS_TOKEN_TEXT_MAX_SIZE_VALUE 24
#define MMS_MAX_UINTVAR_VALUE_VALUE 0x1fffff
#define MMS_MAX_SHORT_LENGTH_VALUE_VALUE 40
#define MMS_MAX_VALUE_LENGTH_VALUE_VALUE 256

namespace OHOS {
namespace Telephony {
constexpr uint32_t TOKEN_TEXT_MAX_SIZE = MMS_TOKEN_TEXT_MAX_SIZE_VALUE;
constexpr uint32_t MAX_UINTVAR_VALUE = MMS_MAX_UINTVAR_VALUE_VALUE;
constexpr uint32_t MAX_SHORT_LENGTH_VALUE = MMS_MAX_SHORT_LENGTH_VALUE_VALUE;
constexpr uint32_t MAX_VALUE_LENGTH_VALUE = MMS_MAX_VALUE_LENGTH_VALUE_VALUE;

#undef MMS_TOKEN_TEXT_MAX_SIZE_VALUE
#undef MMS_MAX_UINTVAR_VALUE_VALUE
#undef MMS_MAX_SHORT_LENGTH_VALUE_VALUE
#undef MMS_MAX_VALUE_LENGTH_VALUE_VALUE

static std::string MakeTokenText(SmsMmsFuzz::DataCursor &cursor)
{
    std::string token = SmsMmsFuzz::NonEmpty(cursor.ConsumeText(TOKEN_TEXT_MAX_SIZE), "token");
    for (char &ch : token) {
        if (!MmsDecodeBuffer::CharIsToken(static_cast<uint8_t>(ch))) {
            ch = 'A';
        }
    }
    return token;
}

static void RunMmsEncodeBufferFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    MmsEncodeBuffer encodeBuffer;
    encodeBuffer.WriteByte(cursor.ConsumeByte());
    encodeBuffer.EncodeUintvar(cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, MAX_UINTVAR_VALUE));
    encodeBuffer.EncodeShortLength(static_cast<uint8_t>(
        cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, MAX_SHORT_LENGTH_VALUE)));
    encodeBuffer.EncodeValueLength(cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, MAX_VALUE_LENGTH_VALUE));
    encodeBuffer.EncodeOctet(cursor.ConsumeByte());
    encodeBuffer.EncodeShortInteger(cursor.ConsumeByte());
    encodeBuffer.EncodeInteger(cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::MAX_HEADER_VALUE));
    encodeBuffer.EncodeLongInteger(cursor.ConsumeUint64());
    encodeBuffer.EncodeText(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeQuotedText(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeTokenText(MakeTokenText(cursor));

    MmsEncodeBuffer nested;
    nested.EncodeText("nested");
    encodeBuffer.WriteBuffer(nested);
    std::vector<uint8_t> bytes = cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE);
    encodeBuffer.WriteBuffer(SmsMmsFuzz::MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    encodeBuffer.ReadDataBuffer(SmsMmsFuzz::EMPTY_SIZE, encodeBuffer.GetCurPosition());
    bytes.clear();
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsEncodeBufferFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
