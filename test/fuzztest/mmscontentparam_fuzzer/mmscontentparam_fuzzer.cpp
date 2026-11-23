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

#include "mmscontentparam_fuzzer.h"

#include "mms_fuzzer_utils.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
constexpr size_t CONTENT_PARAM_FIELD_COUNT = 10;
constexpr uint32_t CONTENT_PARAM_VALUE_CLEAR_INTERVAL = 3;
constexpr uint32_t CONTENT_PARAM_OPERATION_KIND_COUNT = 4;
constexpr uint32_t CONTENT_PARAM_SET_FILE_NAME = 0;
constexpr uint32_t CONTENT_PARAM_SET_START = 1;
constexpr uint32_t CONTENT_PARAM_SET_TYPE = 2;

static uint8_t PickContentParamField(FuzzedDataProvider &cursor)
{
    static constexpr std::array<uint8_t, CONTENT_PARAM_FIELD_COUNT> PARAM_FIELDS = {
        static_cast<uint8_t>(ContentParam::CT_P_CHARSET),
        static_cast<uint8_t>(ContentParam::CT_P_TYPE),
        static_cast<uint8_t>(ContentParam::CT_P_NAME),
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME),
        static_cast<uint8_t>(ContentParam::CT_P_START),
        static_cast<uint8_t>(ContentParam::CT_P_START_INFO),
        static_cast<uint8_t>(ContentParam::CT_P_NAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_START_VALUE),
        SmsMmsFuzz::INVALID_FIELD_ID,
    };
    return PARAM_FIELDS[cursor.ConsumeIntegralInRange<uint32_t>(0,
        SmsMmsFuzz::LastIndex(PARAM_FIELDS.size()))];
}

static void RunMmsContentParamFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider cursor(data, size);
    uint8_t opcode = cursor.ConsumeIntegral<uint8_t>();
    bool useKnownField = cursor.ConsumeBool();
    uint32_t charset = SmsMmsFuzz::PickCharset(cursor);
    std::string type = SmsMmsFuzz::PickContentType(cursor);

    MmsContentParam param;
    SmsMmsFuzz::FillContentParam(param, cursor);
    param.SetCharSet(charset);
    param.SetType(type);
    param.SetFileName(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));
    param.SetStart(cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE));

    uint32_t count = cursor.ConsumeIntegralInRange<uint32_t>(1, SmsMmsFuzz::MAX_PART_COUNT);
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t field = useKnownField ? PickContentParamField(cursor) : cursor.ConsumeIntegral<uint8_t>();
        std::string value = cursor.ConsumeRandomLengthString(SmsMmsFuzz::SHORT_TEXT_SIZE);
        if (((opcode + i) % CONTENT_PARAM_VALUE_CLEAR_INTERVAL) == 0) {
            value.clear();
        }
        param.AddNormalField(field, value);
        std::string out;
        param.GetNormalField(field, out);
        param.GetNormalField(static_cast<uint8_t>(field + 1), out);
    }

    switch (opcode % CONTENT_PARAM_OPERATION_KIND_COUNT) {
        case CONTENT_PARAM_SET_FILE_NAME:
            param.SetFileName("part.txt");
            break;
        case CONTENT_PARAM_SET_START:
            param.SetStart("<smil>");
            break;
        case CONTENT_PARAM_SET_TYPE:
            param.SetType("application/vnd.wap.multipart.related");
            break;
        default:
            param.SetCharSet(SmsMmsFuzz::UNSPECIFIED_CHARSET);
            break;
    }

    MmsContentParam assigned = param;
    std::string fileName;
    std::string start;
    (void)assigned.GetCharSet();
    (void)assigned.GetType();
    assigned.GetFileName(fileName);
    assigned.GetStart(start);
    assigned.GetParamMap();
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsContentParamFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
