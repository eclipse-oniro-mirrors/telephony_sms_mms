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

#include "mmscontenttype_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void DecodeContentTypeBytes(const std::vector<uint8_t> &bytes)
{
    auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    MmsContentType contentType;
    int32_t len = static_cast<int32_t>(SmsMmsFuzz::EMPTY_SIZE);
    contentType.DecodeMmsContentType(*decodeBuffer, len);
}

static void RunMmsContentTypeFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    MmsContentType contentType = SmsMmsFuzz::MakeContentType(cursor);
    std::string contentTypeText;
    contentType.GetContentType(contentTypeText);
    MmsContentType::GetContentTypeFromString(contentTypeText);
    MmsContentType::GetContentTypeFromInt(cursor.ConsumeByte());

    MmsEncodeBuffer encodeBuffer;
    contentType.EncodeTextField(encodeBuffer);
    contentType.EncodeCharsetField(encodeBuffer);
    contentType.EncodeTypeField(encodeBuffer);
    contentType.EncodeMmsBodyPartContentParam(encodeBuffer);

    MmsEncodeBuffer contentTypeBuffer;
    if (contentType.EncodeMmsBodyPartContentType(contentTypeBuffer)) {
        DecodeContentTypeBytes(SmsMmsFuzz::ReadEncodedBytes(contentTypeBuffer));
    }

    std::vector<uint8_t> shortForm = { static_cast<uint8_t>(SmsMmsFuzz::SHORT_INTEGER_MARK |
        (cursor.ConsumeByte() & SmsMmsFuzz::SHORT_INTEGER_MASK)) };
    DecodeContentTypeBytes(shortForm);
    std::vector<uint8_t> raw = cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE);
    DecodeContentTypeBytes(raw);
    raw.clear();
    shortForm.clear();
    contentTypeText.clear();
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsContentTypeFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
