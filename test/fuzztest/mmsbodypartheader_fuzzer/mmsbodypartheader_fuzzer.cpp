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

#include "mmsbodypartheader_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void DecodeHeaderBytes(const std::vector<uint8_t> &bytes)
{
    auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    MmsBodyPartHeader header;
    uint32_t len = static_cast<uint32_t>(bytes.size());
    header.DecodeWellKnownHeader(*decodeBuffer, len);

    decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(bytes);
    len = static_cast<uint32_t>(bytes.size());
    header.DecodeApplicationHeader(*decodeBuffer, len);
}

static void RunMmsBodyPartHeaderFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    MmsBodyPartHeader header;
    header.SetContentId(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(SmsMmsFuzz::SHORT_TEXT_SIZE), "<cid>"));
    header.SetContentLocation(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(SmsMmsFuzz::SHORT_TEXT_SIZE), "part.txt"));
    header.SetContentDisposition(SmsMmsFuzz::PickDisposition(cursor));
    header.SetContentTransferEncoding(SmsMmsFuzz::PickTransferEncoding(cursor));
    std::string value;
    header.GetContentId(value);
    header.GetContentLocation(value);
    header.GetContentDisposition(value);
    header.GetContentTransferEncoding(value);

    MmsEncodeBuffer encodeBuffer;
    header.EncodeContentId(encodeBuffer);
    header.EncodeContentLocation(encodeBuffer);
    header.EncodeContentDisposition(encodeBuffer);
    header.EncodeContentTransferEncoding(encodeBuffer);
    header.EncodeMmsBodyPartHeader(encodeBuffer);
    DecodeHeaderBytes(SmsMmsFuzz::ReadEncodedBytes(encodeBuffer));

    std::vector<uint8_t> synthetic = {
        static_cast<uint8_t>(SmsMmsFuzz::SHORT_INTEGER_MARK | static_cast<uint8_t>(MmsHeaderParam::P_CONTENT_ID)),
        '"', 'f', 'u', 'z', 'z', SmsMmsFuzz::ZERO_BYTE,
    };
    DecodeHeaderBytes(synthetic);
    DecodeHeaderBytes(cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE));
    MmsBodyPartHeader copy(header);
    MmsBodyPartHeader assigned = copy;
    synthetic.clear();
    value.clear();
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsBodyPartHeaderFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
