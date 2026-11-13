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

#include "mmsbodypart_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void DecodePartBytes(const std::vector<uint8_t> &bytes)
{
    auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    MmsBodyPart decoded;
    decoded.DecodePart(*decodeBuffer, SmsMmsFuzz::MIN_BUFFER_SIZE);
}

static void RunMmsBodyPartFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider rawProvider(data, size);
    DecodePartBytes(rawProvider.ConsumeRemainingBytes<uint8_t>());
    FuzzedDataProvider cursor(data, size);
    MmsBodyPart bodyPart = SmsMmsFuzz::MakeBodyPart(cursor, cursor.ConsumeBool());
    bodyPart.SetContentType(SmsMmsFuzz::PickContentType(cursor));
    bodyPart.SetContentId(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    bodyPart.SetContentLocation(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    bodyPart.SetContentDisposition(SmsMmsFuzz::PickDisposition(cursor));
    bodyPart.SetSmilFile(cursor.ConsumeBool());
    bodyPart.SetFileName(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    std::string value;
    bodyPart.GetContentType(value);
    bodyPart.GetContentId(value);
    bodyPart.GetContentLocation(value);
    bodyPart.GetContentDisposition(value);
    bodyPart.GetPartFileName();
    bodyPart.GetContentType();
    bodyPart.GetPartHeader();

    MmsEncodeBuffer encodeBuffer;
    bodyPart.EncodeMmsBodyPart(encodeBuffer);
    DecodePartBytes(SmsMmsFuzz::ReadEncodedBytes(encodeBuffer));
    uint32_t len = SmsMmsFuzz::EMPTY_SIZE;
    auto body = bodyPart.ReadBodyPartBuffer(len);
    MmsBodyPart copy(bodyPart);
    MmsBodyPart assigned = copy;
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsBodyPartFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
