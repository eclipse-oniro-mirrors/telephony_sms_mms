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

#include "mmscodecutils_fuzzer.h"

#include "mms_fuzzer_utils.h"
#include "singleton.h"
#include "utils/mms_charset.h"
#include "utils/mms_quoted_printable.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
constexpr size_t CODEC_SAMPLE_COUNT = 6;
constexpr size_t CHARSET_NAME_COUNT = 6;
constexpr uint32_t CODEC_OPERATION_KIND_COUNT = 4;
constexpr uint32_t CODEC_QP_DECODE = 0;
constexpr uint32_t CODEC_QP_ENCODE = 1;
constexpr uint32_t CODEC_KNOWN_CHARSET = 2;
constexpr uint32_t CODEC_RANDOM_CHARSET = 3;
constexpr const char *CODEC_SAMPLE_HEX_PAIR = "=41=42";
constexpr const char *CODEC_SAMPLE_SOFT_BREAK = "=\r\n";
constexpr const char *CODEC_SAMPLE_INVALID_ESCAPE = "=ZZ";
constexpr const char *CODEC_SAMPLE_PLAIN_TEXT = "quoted printable text";
constexpr const char *CODEC_SAMPLE_CHARSET_HEADER = "charset=UTF-8";
constexpr const char *CODEC_SAMPLE_CONTENT_TYPE = "text/plain; charset=UTF-8";
constexpr const char *CHARSET_NAME_UTF8 = "UTF-8";
constexpr const char *CHARSET_NAME_ASCII = "US-ASCII";
constexpr const char *CHARSET_NAME_ISO_8859_1 = "ISO-8859-1";
constexpr const char *CHARSET_NAME_UTF16 = "UTF-16";
constexpr const char *CHARSET_NAME_GB2312 = "GB2312";
constexpr const char *CHARSET_NAME_UNKNOWN = "x-unknown-fuzz-charset";

static std::string PickCodecSample(FuzzedDataProvider &cursor)
{
    static constexpr std::array<const char *, CODEC_SAMPLE_COUNT> CODEC_SAMPLES = {
        CODEC_SAMPLE_HEX_PAIR,
        CODEC_SAMPLE_SOFT_BREAK,
        CODEC_SAMPLE_INVALID_ESCAPE,
        CODEC_SAMPLE_PLAIN_TEXT,
        CODEC_SAMPLE_CHARSET_HEADER,
        CODEC_SAMPLE_CONTENT_TYPE,
    };
    return CODEC_SAMPLES[cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::EMPTY_SIZE,
        SmsMmsFuzz::LastIndex(CODEC_SAMPLES.size()))];
}

static std::string PickCharsetName(FuzzedDataProvider &cursor)
{
    static constexpr std::array<const char *, CHARSET_NAME_COUNT> CHARSET_NAMES = {
        CHARSET_NAME_UTF8,
        CHARSET_NAME_ASCII,
        CHARSET_NAME_ISO_8859_1,
        CHARSET_NAME_UTF16,
        CHARSET_NAME_GB2312,
        CHARSET_NAME_UNKNOWN,
    };
    return CHARSET_NAMES[cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::EMPTY_SIZE,
        SmsMmsFuzz::LastIndex(CHARSET_NAMES.size()))];
}

static void RunMmsCodecUtilsFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider cursor(data, size);
    uint8_t opcode = cursor.ConsumeIntegral<uint8_t>();
    bool useKnownSample = cursor.ConsumeBool();
    uint32_t operationCount = cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::MIN_BUFFER_SIZE,
        SmsMmsFuzz::MAX_PART_COUNT);
    std::string input = useKnownSample ? PickCodecSample(cursor) :
        cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE);

    std::string qpEncoded = MmsQuotedPrintable::Encode(input);
    std::string qpDecoded;
    MmsQuotedPrintable::Decode(qpEncoded, qpDecoded);

    auto charset = DelayedSingleton<MmsCharSet>::GetInstance();
    if (charset == nullptr) {
        return;
    }
    uint32_t charsetValue = SmsMmsFuzz::EMPTY_SIZE;
    std::string charsetName;
    for (uint32_t i = SmsMmsFuzz::EMPTY_SIZE; i < operationCount; ++i) {
        std::string sample = useKnownSample ? PickCodecSample(cursor) :
            cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE);
        switch ((opcode + i) % CODEC_OPERATION_KIND_COUNT) {
            case CODEC_QP_DECODE:
                MmsQuotedPrintable::Decode(sample, qpDecoded);
                break;
            case CODEC_QP_ENCODE:
                MmsQuotedPrintable::Encode(sample);
                break;
            case CODEC_KNOWN_CHARSET:
                charset->GetCharSetIntFromString(charsetValue, PickCharsetName(cursor));
                charset->GetCharSetStrFromInt(charsetName, charsetValue);
                break;
            case CODEC_RANDOM_CHARSET:
            default:
                charset->GetCharSetIntFromString(charsetValue,
                    cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
                charset->GetCharSetStrFromInt(charsetName,
                    cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::MAX_CHARSET_VALUE));
                break;
        }
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsCodecUtilsFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
