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
#include "utils/mms_charset.h"
#include "utils/mms_quoted_printable.h"

#include <array>
#include <string>

#define MMS_CODEC_SAMPLE_COUNT_VALUE 6
#define MMS_CODEC_CHARSET_NAME_COUNT_VALUE 6
#define MMS_CODEC_OPERATION_KIND_COUNT_VALUE 4
#define MMS_CODEC_QP_DECODE_VALUE 0
#define MMS_CODEC_QP_ENCODE_VALUE 1
#define MMS_CODEC_KNOWN_CHARSET_VALUE 2
#define MMS_CODEC_RANDOM_CHARSET_VALUE 3
#define MMS_CODEC_SAMPLE_HEX_PAIR_VALUE "=41=42"
#define MMS_CODEC_SAMPLE_CHARSET_HEADER_VALUE "charset=UTF-8"
#define MMS_CODEC_SAMPLE_CONTENT_TYPE_VALUE "text/plain; charset=UTF-8"
#define MMS_CHARSET_NAME_UTF8_VALUE "UTF-8"
#define MMS_CHARSET_NAME_ISO_8859_1_VALUE "ISO-8859-1"
#define MMS_CHARSET_NAME_UTF16_VALUE "UTF-16"
#define MMS_CHARSET_NAME_GB2312_VALUE "GB2312"

namespace OHOS {
namespace Telephony {
constexpr size_t CODEC_SAMPLE_COUNT = MMS_CODEC_SAMPLE_COUNT_VALUE;
constexpr size_t CHARSET_NAME_COUNT = MMS_CODEC_CHARSET_NAME_COUNT_VALUE;
constexpr uint32_t CODEC_OPERATION_KIND_COUNT = MMS_CODEC_OPERATION_KIND_COUNT_VALUE;
constexpr uint32_t CODEC_QP_DECODE = MMS_CODEC_QP_DECODE_VALUE;
constexpr uint32_t CODEC_QP_ENCODE = MMS_CODEC_QP_ENCODE_VALUE;
constexpr uint32_t CODEC_KNOWN_CHARSET = MMS_CODEC_KNOWN_CHARSET_VALUE;
constexpr uint32_t CODEC_RANDOM_CHARSET = MMS_CODEC_RANDOM_CHARSET_VALUE;
constexpr const char *CODEC_SAMPLE_HEX_PAIR = MMS_CODEC_SAMPLE_HEX_PAIR_VALUE;
constexpr const char *CODEC_SAMPLE_SOFT_BREAK = "=\r\n";
constexpr const char *CODEC_SAMPLE_INVALID_ESCAPE = "=ZZ";
constexpr const char *CODEC_SAMPLE_PLAIN_TEXT = "quoted printable text";
constexpr const char *CODEC_SAMPLE_CHARSET_HEADER = MMS_CODEC_SAMPLE_CHARSET_HEADER_VALUE;
constexpr const char *CODEC_SAMPLE_CONTENT_TYPE = MMS_CODEC_SAMPLE_CONTENT_TYPE_VALUE;
constexpr const char *CHARSET_NAME_UTF8 = MMS_CHARSET_NAME_UTF8_VALUE;
constexpr const char *CHARSET_NAME_ASCII = "US-ASCII";
constexpr const char *CHARSET_NAME_ISO_8859_1 = MMS_CHARSET_NAME_ISO_8859_1_VALUE;
constexpr const char *CHARSET_NAME_UTF16 = MMS_CHARSET_NAME_UTF16_VALUE;
constexpr const char *CHARSET_NAME_GB2312 = MMS_CHARSET_NAME_GB2312_VALUE;
constexpr const char *CHARSET_NAME_UNKNOWN = "x-unknown-fuzz-charset";

#undef MMS_CODEC_SAMPLE_COUNT_VALUE
#undef MMS_CODEC_CHARSET_NAME_COUNT_VALUE
#undef MMS_CODEC_OPERATION_KIND_COUNT_VALUE
#undef MMS_CODEC_QP_DECODE_VALUE
#undef MMS_CODEC_QP_ENCODE_VALUE
#undef MMS_CODEC_KNOWN_CHARSET_VALUE
#undef MMS_CODEC_RANDOM_CHARSET_VALUE
#undef MMS_CODEC_SAMPLE_HEX_PAIR_VALUE
#undef MMS_CODEC_SAMPLE_CHARSET_HEADER_VALUE
#undef MMS_CODEC_SAMPLE_CONTENT_TYPE_VALUE
#undef MMS_CHARSET_NAME_UTF8_VALUE
#undef MMS_CHARSET_NAME_ISO_8859_1_VALUE
#undef MMS_CHARSET_NAME_UTF16_VALUE
#undef MMS_CHARSET_NAME_GB2312_VALUE

static std::string PickCodecSample(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<const char *, CODEC_SAMPLE_COUNT> CODEC_SAMPLES = {
        CODEC_SAMPLE_HEX_PAIR,
        CODEC_SAMPLE_SOFT_BREAK,
        CODEC_SAMPLE_INVALID_ESCAPE,
        CODEC_SAMPLE_PLAIN_TEXT,
        CODEC_SAMPLE_CHARSET_HEADER,
        CODEC_SAMPLE_CONTENT_TYPE,
    };
    return CODEC_SAMPLES[cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::LastIndex(CODEC_SAMPLES.size()))];
}

static std::string PickCharsetName(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<const char *, CHARSET_NAME_COUNT> CHARSET_NAMES = {
        CHARSET_NAME_UTF8,
        CHARSET_NAME_ASCII,
        CHARSET_NAME_ISO_8859_1,
        CHARSET_NAME_UTF16,
        CHARSET_NAME_GB2312,
        CHARSET_NAME_UNKNOWN,
    };
    return CHARSET_NAMES[cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::LastIndex(CHARSET_NAMES.size()))];
}

static void RunMmsCodecUtilsFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    uint8_t opcode = cursor.ConsumeByte();
    bool useKnownSample = cursor.ConsumeBool();
    uint32_t operationCount = cursor.ConsumeUint32(SmsMmsFuzz::MIN_BUFFER_SIZE, SmsMmsFuzz::MAX_PART_COUNT);
    std::string input = useKnownSample ? PickCodecSample(cursor) : cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE);

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
        std::string sample = useKnownSample ? PickCodecSample(cursor) : cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE);
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
                charset->GetCharSetIntFromString(charsetValue, cursor.ConsumeText(SmsMmsFuzz::SHORT_TEXT_SIZE));
                charset->GetCharSetStrFromInt(charsetName,
                    cursor.ConsumeUint32(SmsMmsFuzz::EMPTY_SIZE, SmsMmsFuzz::MAX_CHARSET_VALUE));
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
