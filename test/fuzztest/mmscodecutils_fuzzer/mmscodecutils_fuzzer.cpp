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

#include "mmscodecutils_fuzzer.h"

#include "mms_fuzzer_utils.h"
#include "singleton.h"
#include "utils/mms_base64.h"
#include "utils/mms_charset.h"
#include "utils/mms_quoted_printable.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
static std::string PickCodecSample(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<const char *, 8> CODEC_SAMPLES = {
        "TWFu",
        "SGVsbG8=",
        "====",
        "A===",
        "=41=42",
        "=\r\n",
        "=ZZ",
        "text/plain; charset=UTF-8",
    };
    return CODEC_SAMPLES[cursor.ConsumeUint32(0, static_cast<uint32_t>(CODEC_SAMPLES.size() - 1))];
}

static std::string PickCharsetName(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<const char *, 6> CHARSET_NAMES = {
        "UTF-8",
        "US-ASCII",
        "ISO-8859-1",
        "UTF-16",
        "GB2312",
        "x-unknown-fuzz-charset",
    };
    return CHARSET_NAMES[cursor.ConsumeUint32(0, static_cast<uint32_t>(CHARSET_NAMES.size() - 1))];
}

static void RunMmsCodecUtilsFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    uint8_t opcode = cursor.ConsumeByte();
    bool useKnownSample = cursor.ConsumeBool();
    uint32_t operationCount = cursor.ConsumeUint32(1, SmsMmsFuzz::MAX_PART_COUNT);
    std::string input = useKnownSample ? PickCodecSample(cursor) : cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE);

    std::string encoded64 = MmsBase64::Encode(input);
    MmsBase64::Decode(encoded64);

    std::string qpEncoded = MmsQuotedPrintable::Encode(input);
    std::string qpDecoded;
    MmsQuotedPrintable::Decode(qpEncoded, qpDecoded);

    auto charset = DelayedSingleton<MmsCharSet>::GetInstance();
    if (charset == nullptr) {
        return;
    }
    uint32_t charsetValue = 0;
    std::string charsetName;
    for (uint32_t i = 0; i < operationCount; ++i) {
        std::string sample = useKnownSample ? PickCodecSample(cursor) : cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE);
        switch ((opcode + i) % 5) {
            case 0:
                MmsBase64::Decode(sample);
                break;
            case 1:
                MmsQuotedPrintable::Decode(sample, qpDecoded);
                break;
            case 2:
                charset->GetCharSetIntFromString(charsetValue, PickCharsetName(cursor));
                charset->GetCharSetStrFromInt(charsetName, charsetValue);
                break;
            case 3:
                charset->GetCharSetIntFromString(charsetValue, cursor.ConsumeText(32));
                charset->GetCharSetStrFromInt(charsetName, cursor.ConsumeUint32(0, 0x1000));
                break;
            default:
                MmsBase64::Encode(sample);
                MmsQuotedPrintable::Encode(sample);
                break;
        }
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsCodecUtilsFuzz(data, size);
    return 0;
}
