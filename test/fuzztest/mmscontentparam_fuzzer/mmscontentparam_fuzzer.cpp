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

#include "mmscontentparam_fuzzer.h"

#include "mms_fuzzer_utils.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
static uint8_t PickContentParamField(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<uint8_t, 10> PARAM_FIELDS = {
        static_cast<uint8_t>(ContentParam::CT_P_CHARSET),
        static_cast<uint8_t>(ContentParam::CT_P_TYPE),
        static_cast<uint8_t>(ContentParam::CT_P_NAME),
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME),
        static_cast<uint8_t>(ContentParam::CT_P_START),
        static_cast<uint8_t>(ContentParam::CT_P_START_INFO),
        static_cast<uint8_t>(ContentParam::CT_P_NAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_START_VALUE),
        0xff,
    };
    return PARAM_FIELDS[cursor.ConsumeUint32(0, static_cast<uint32_t>(PARAM_FIELDS.size() - 1))];
}

static void RunMmsContentParamFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    uint8_t opcode = cursor.ConsumeByte();
    bool useKnownField = cursor.ConsumeBool();
    uint32_t charset = SmsMmsFuzz::PickCharset(cursor);
    std::string type = SmsMmsFuzz::PickContentType(cursor);

    MmsContentParam param;
    SmsMmsFuzz::FillContentParam(param, cursor);
    param.DumpContentParam();
    param.SetCharSet(charset);
    param.SetType(type);
    param.SetFileName(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(32), "name.txt"));
    param.SetStart(SmsMmsFuzz::NonEmpty(cursor.ConsumeText(32), "<start>"));

    uint32_t count = cursor.ConsumeUint32(1, SmsMmsFuzz::MAX_PART_COUNT);
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t field = useKnownField ? PickContentParamField(cursor) : cursor.ConsumeByte();
        std::string value = SmsMmsFuzz::NonEmpty(cursor.ConsumeText(32), "value");
        if (((opcode + i) % 3) == 0) {
            value.clear();
        }
        param.AddNormalField(field, value);
        std::string out;
        param.GetNormalField(field, out);
        param.GetNormalField(static_cast<uint8_t>(field + 1), out);
    }

    switch (opcode % 4) {
        case 0:
            param.SetFileName("part.txt");
            break;
        case 1:
            param.SetStart("<smil>");
            break;
        case 2:
            param.SetType("application/vnd.wap.multipart.related");
            break;
        default:
            param.SetCharSet(0);
            break;
    }

    MmsContentParam assigned;
    assigned = param;
    std::string fileName;
    std::string start;
    uint32_t assignedCharset = assigned.GetCharSet();
    std::string assignedType = assigned.GetType();
    assigned.GetFileName(fileName);
    assigned.GetStart(start);
    assigned.GetParamMap();
    (void)assignedCharset;
    (void)assignedType;
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsContentParamFuzz(data, size);
    return 0;
}
