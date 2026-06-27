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

#include "mmsheader_fuzzer.h"

#include "mms_fuzzer_utils.h"

namespace OHOS {
namespace Telephony {
static void QueryHeader(MmsHeader &header)
{
    uint8_t octet = SmsMmsFuzz::ZERO_BYTE;
    int64_t longValue = static_cast<int64_t>(SmsMmsFuzz::EMPTY_SIZE);
    std::string textValue;
    std::vector<MmsAddress> addresses;
    header.GetOctetValue(MMS_MESSAGE_TYPE, octet);
    header.GetLongValue(MMS_DATE, longValue);
    header.GetTextValue(MMS_TRANSACTION_ID, textValue);
    header.GetHeaderAllAddressValue(MMS_TO, addresses);
    header.GetHeaderContentType();
    header.GetSmilFileName(textValue);
    header.IsHaveBody();
}

static void RunMmsHeaderFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    MmsHeader header;
    uint8_t messageType = SmsMmsFuzz::PickMessageType(cursor);
    SmsMmsFuzz::FillMmsHeader(header, cursor, messageType);
    header.SetOctetValue(MMS_PRIORITY, cursor.ConsumeByte());
    header.SetLongValue(MMS_MESSAGE_COUNT,
        cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::MAX_HEADER_VALUE));
    std::string text = SmsMmsFuzz::NonEmpty(cursor.ConsumeText(SmsMmsFuzz::SHORT_TEXT_SIZE), "cancel-id");
    header.SetTextValue(MMS_CANCEL_ID, text);
    MmsAddress address = SmsMmsFuzz::MakeAddress(cursor);
    header.AddHeaderAddressValue(MMS_BCC, address);
    QueryHeader(header);

    MmsEncodeBuffer encodeBuffer;
    header.EncodeMmsHeader(encodeBuffer);
    auto decodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(SmsMmsFuzz::ReadEncodedBytes(encodeBuffer));
    MmsHeader decoded;
    decoded.DecodeMmsHeader(*decodeBuffer);
    QueryHeader(decoded);

    auto rawDecodeBuffer = SmsMmsFuzz::MakeDecodeBuffer(cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE));
    decoded.DecodeMmsHeader(*rawDecodeBuffer);
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsHeaderFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
