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

#include "mmsattachment_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void RunMmsAttachmentFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider rawProvider(data, size);
    auto rawBuffer = SmsMmsFuzz::MakeDecodeBuffer(rawProvider.ConsumeRemainingBytes<uint8_t>());
    MmsBodyPart rawPart;
    rawPart.DecodePart(*rawBuffer, SmsMmsFuzz::MIN_BUFFER_SIZE);
    FuzzedDataProvider cursor(data, size);
    MmsAttachment attachment = SmsMmsFuzz::MakeAttachment(cursor);
    attachment.SetAttachmentFilePath(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE),
        cursor.ConsumeBool());
    attachment.SetContentId(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    attachment.SetContentLocation(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    attachment.SetContentDisposition(SmsMmsFuzz::PickDisposition(cursor));
    attachment.SetContentTransferEncoding(SmsMmsFuzz::PickTransferEncoding(cursor));
    attachment.SetContentType(SmsMmsFuzz::PickContentType(cursor));
    attachment.SetFileName(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    attachment.SetCharSet(SmsMmsFuzz::PickCharset(cursor));
    attachment.SetIsSmilFile(cursor.ConsumeBool());

    std::vector<uint8_t> bytes = cursor.ConsumeBytes<uint8_t>(SmsMmsFuzz::MAX_BINARY_SIZE);
    attachment.SetDataBuffer(SmsMmsFuzz::MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    uint32_t len = SmsMmsFuzz::EMPTY_SIZE;
    auto buffer = attachment.GetDataBuffer(len);
    MmsAttachment copy(attachment);
    copy.GetAttachmentFilePath();
    copy.GetContentId();
    copy.GetContentLocation();
    copy.GetContentDisposition();
    copy.GetContentTransferEncoding();
    copy.GetContentType();
    copy.GetFileName();
    copy.GetCharSet();
    copy.IsSmilFile();

    MmsBodyPart bodyPart;
    if (bodyPart.SetAttachment(copy)) {
        MmsEncodeBuffer encodeBuffer;
        if (bodyPart.EncodeMmsBodyPart(encodeBuffer)) {
            auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(SmsMmsFuzz::ReadEncodedBytes(encodeBuffer));
            MmsBodyPart decoded;
            decoded.DecodePart(*decodeBuffer, SmsMmsFuzz::MIN_BUFFER_SIZE);
        }
    }

}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsAttachmentFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
