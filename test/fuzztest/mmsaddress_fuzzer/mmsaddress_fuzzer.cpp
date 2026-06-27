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

#include "mmsaddress_fuzzer.h"

#include "mms_fuzzer_utils.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
static std::string PickAddressTemplate(SmsMmsFuzz::DataCursor &cursor)
{
    static constexpr std::array<const char *, 7> ADDRESS_TEMPLATES = {
        "+8613812345678/TYPE=PLMN",
        "127.0.0.1/TYPE=IPv4",
        "::1/TYPE=IPv6",
        "user@example.com/TYPE=EMAIL",
        "Insert-address-token",
        "/TYPE=UNKNOWN",
        "malformed-address",
    };
    return ADDRESS_TEMPLATES[cursor.ConsumeUint32(0, static_cast<uint32_t>(ADDRESS_TEMPLATES.size() - 1))];
}

static void RunMmsAddressFuzz(const uint8_t *data, size_t size)
{
    SmsMmsFuzz::DataCursor cursor(data, size);
    uint8_t opcode = cursor.ConsumeByte();
    bool useTemplateAddress = cursor.ConsumeBool();
    uint32_t charset = SmsMmsFuzz::PickCharset(cursor);
    std::string inputAddress = useTemplateAddress ? PickAddressTemplate(cursor) :
        SmsMmsFuzz::NonEmpty(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE), "+8613812345678/TYPE=PLMN");

    MmsAddress defaultAddress;
    defaultAddress.GetAddressString();
    defaultAddress.GetAddressCharset();
    defaultAddress.GetAddressType();

    MmsAddress directAddress(inputAddress, static_cast<MmsCharSets>(charset));
    directAddress.GetAddressString();
    directAddress.GetAddressCharset();
    directAddress.GetAddressType();

    uint32_t count = cursor.ConsumeUint32(1, SmsMmsFuzz::MAX_PART_COUNT);
    for (uint32_t i = 0; i < count; ++i) {
        MmsAddress address = SmsMmsFuzz::MakeAddress(cursor);
        switch ((opcode + i) % 4) {
            case 0:
                address.SetMmsAddressString(inputAddress, static_cast<MmsCharSets>(charset));
                break;
            case 1:
                address.SetMmsAddressString(PickAddressTemplate(cursor),
                    static_cast<MmsCharSets>(SmsMmsFuzz::PickCharset(cursor)));
                break;
            case 2:
                address.SetMmsAddressString(cursor.ConsumeText(SmsMmsFuzz::MAX_TEXT_SIZE),
                    static_cast<MmsCharSets>(SmsMmsFuzz::PickCharset(cursor)));
                break;
            default:
                address.SetMmsAddressString("", static_cast<MmsCharSets>(0));
                break;
        }
        std::string current = address.GetAddressString();
        MmsCharSets currentCharset = address.GetAddressCharset();
        MmsAddress::MmsAddressType currentType = address.GetAddressType();
        (void)current;
        (void)currentCharset;
        (void)currentType;
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsAddressFuzz(data, size);
    return 0;
}
