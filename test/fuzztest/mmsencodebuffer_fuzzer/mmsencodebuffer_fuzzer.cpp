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

namespace OHOS {
namespace Telephony {
static std::string MakeTokenText(SmsMmsFuzz::DataCursor &cursor)
{
    std::string token = SmsMmsFuzz::NonEmpty(cursor.ConsumeText(24), "token");
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
    encodeBuffer.EncodeUintvar(cursor.ConsumeUint32(0, 0x1fffff));
    encodeBuffer.EncodeShortLength(static_cast<uint8_t>(cursor.ConsumeUint32(0, 40)));
    encodeBuffer.EncodeValueLength(cursor.ConsumeUint32(0, 256));
    encodeBuffer.EncodeOctet(cursor.ConsumeByte());
    encodeBuffer.EncodeShortInteger(cursor.ConsumeByte());
    encodeBuffer.EncodeInteger(cursor.ConsumeUint32(0, 4096));
    encodeBuffer.EncodeLongInteger(cursor.ConsumeUint64());
    encodeBuffer.EncodeText(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeQuotedText(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE));
    encodeBuffer.EncodeTokenText(MakeTokenText(cursor));

    MmsEncodeBuffer nested;
    nested.EncodeText("nested");
    encodeBuffer.WriteBuffer(nested);
    std::vector<uint8_t> bytes = cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE);
    encodeBuffer.WriteBuffer(SmsMmsFuzz::MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    encodeBuffer.ReadDataBuffer(0, encodeBuffer.GetCurPosition());
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsEncodeBufferFuzz(data, size);
    return 0;
}
