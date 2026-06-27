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

#include "mmsmsg_fuzzer.h"

#include "mms_fuzzer_utils.h"

#include <utility>

namespace OHOS {
namespace Telephony {
static void QueryMsg(MmsMsg &msg)
{
    msg.DumpMms();
    msg.GetMmsVersion();
    msg.GetMmsMessageType();
    msg.GetMmsTransactionId();
    msg.GetMmsDate();
    msg.GetMmsSubject();
    msg.GetMmsFrom();
    std::vector<MmsAddress> toAddrs;
    msg.GetMmsTo(toAddrs);
    msg.GetHeaderOctetValue(MMS_MESSAGE_TYPE);
    msg.GetHeaderIntegerValue(MMS_MMS_VERSION);
    msg.GetHeaderLongValue(MMS_DATE);
    msg.GetHeaderStringValue(MMS_TRANSACTION_ID);
    msg.GetHeaderContentType();
    msg.GetHeaderContentTypeStart();
    std::vector<MmsAttachment> attachments;
    msg.GetAllAttachment(attachments);
    msg.GetHeader();
}

static void RunMmsMsgFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    uint8_t messageType = SmsMmsFuzz::PickMessageType(cursor);
    MmsMsg msg;
    SmsMmsFuzz::FillMmsMsg(msg, cursor, messageType);
    msg.SetMmsVersion(static_cast<uint16_t>(cursor.ConsumeUint32(0, 0xff)));
    msg.SetMmsMessageType(messageType);
    msg.SetMmsTransactionId(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(32), "tx"));
    msg.SetMmsDate(static_cast<int64_t>(cursor.ConsumeUint64()));
    msg.SetMmsSubject(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(48), "subject"));
    MmsAddress from = SmsMmsFuzz::MakeAddress(cursor);
    msg.SetMmsFrom(from);
    std::vector<MmsAddress> toAddrs = { SmsMmsFuzz::MakeAddress(cursor), SmsMmsFuzz::MakeAddress(cursor) };
    msg.SetMmsTo(toAddrs);
    std::string text = SmsMmsFuzz::NonEmpty(cursor.ConsumeText(32), "header-text");
    msg.SetHeaderStringValue(MMS_CONTENT_LOCATION, text);
    msg.SetHeaderEncodedStringValue(MMS_RESPONSE_TEXT, text, SmsMmsFuzz::PickCharset(cursor));
    msg.SetHeaderOctetValue(MMS_CONTENT_CLASS, static_cast<uint8_t>(MmsContentClass::MMS_TEXT));
    msg.SetHeaderIntegerValue(MMS_MESSAGE_COUNT, static_cast<int32_t>(cursor.ConsumeUint32(0, 4096)));
    QueryMsg(msg);

    uint32_t outLen = 0;
    auto encoded = msg.EncodeMsg(outLen);
    if (encoded != nullptr && outLen > 0) {
        MmsMsg decoded;
        decoded.DecodeMsg(std::move(encoded), outLen);
        QueryMsg(decoded);
    }
    std::vector<uint8_t> raw = cursor.ConsumeBytes(SmsMmsFuzz::MAX_BINARY_SIZE);
    if (!raw.empty()) {
        MmsMsg rawDecoded;
        rawDecoded.DecodeMsg(SmsMmsFuzz::MakeCharBuffer(raw), static_cast<uint32_t>(raw.size()));
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsMsgFuzz(data, size);
    return 0;
}
