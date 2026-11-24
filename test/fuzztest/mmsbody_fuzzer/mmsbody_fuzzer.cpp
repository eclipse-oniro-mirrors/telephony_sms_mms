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

#include "mmsbody_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void RunMmsBodyFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider rawProvider(data, size);
    auto rawBuffer = SmsMmsFuzz::MakeDecodeBuffer(rawProvider.ConsumeRemainingBytes<uint8_t>());
    MmsHeader rawHeader;
    rawHeader.SetOctetValue(MMS_MESSAGE_TYPE, MMS_MSGTYPE_RETRIEVE_CONF);
    rawHeader.GetHeaderContentType().SetContentType("application/vnd.wap.multipart.related");
    MmsBody rawBody;
    rawBody.DecodeMmsBody(*rawBuffer, rawHeader);
    FuzzedDataProvider cursor(data, size);
    MmsBody body;
    uint32_t count = cursor.ConsumeIntegralInRange<uint32_t>(1, SmsMmsFuzz::MAX_PART_COUNT);
    for (uint32_t i = 0; i < count; ++i) {
        MmsBodyPart part = SmsMmsFuzz::MakeBodyPart(cursor, i == 0 && cursor.ConsumeBool());
        body.IsBodyPartExist(part);
        body.AddMmsBodyPart(part);
    }
    body.GetBodyPartCount();
    std::vector<MmsBodyPart> parts;
    body.GetMmsBodyPart(parts);

    MmsHeader header;
    SmsMmsFuzz::FillMmsHeader(header, cursor, MMS_MSGTYPE_SEND_REQ);
    MmsEncodeBuffer encodeBuffer;
    bool headerEncoded = body.EncodeMmsHeaderContentType(header, encodeBuffer);
    bool bodyEncoded = body.EncodeMmsBody(encodeBuffer);
    if (headerEncoded && bodyEncoded) {
        auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(SmsMmsFuzz::ReadEncodedBytes(encodeBuffer));
        body.DecodeMmsBody(*decodeBuffer, header);
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsBodyFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
