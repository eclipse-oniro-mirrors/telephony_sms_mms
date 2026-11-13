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

#include "mmsaddress_fuzzer.h"

#include "mms_fuzzer_utils.h"

#include <array>
#include <string>

namespace OHOS {
namespace Telephony {
constexpr size_t ADDRESS_TEMPLATE_COUNT = 7;
constexpr uint32_t ADDRESS_OPERATION_KIND_COUNT = 4;
constexpr uint32_t ADDRESS_USE_INPUT = 0;
constexpr uint32_t ADDRESS_USE_TEMPLATE = 1;
constexpr uint32_t ADDRESS_USE_RANDOM_TEXT = 2;
constexpr const char *ADDRESS_TEMPLATE_PLMN = "+8613812345678/TYPE=PLMN";
constexpr const char *ADDRESS_TEMPLATE_IPV4 = "127.0.0.1/TYPE=IPv4";
constexpr const char *ADDRESS_TEMPLATE_IPV6 = "::1/TYPE=IPv6";
constexpr const char *ADDRESS_TEMPLATE_EMAIL = "user@example.com/TYPE=EMAIL";
constexpr const char *ADDRESS_TEMPLATE_INSERT = "Insert-address-token";
constexpr const char *ADDRESS_TEMPLATE_UNKNOWN = "/TYPE=UNKNOWN";
constexpr const char *ADDRESS_TEMPLATE_MALFORMED = "malformed-address";

static std::string PickAddressTemplate(FuzzedDataProvider &cursor)
{
    static constexpr std::array<const char *, ADDRESS_TEMPLATE_COUNT> ADDRESS_TEMPLATES = {
        ADDRESS_TEMPLATE_PLMN,
        ADDRESS_TEMPLATE_IPV4,
        ADDRESS_TEMPLATE_IPV6,
        ADDRESS_TEMPLATE_EMAIL,
        ADDRESS_TEMPLATE_INSERT,
        ADDRESS_TEMPLATE_UNKNOWN,
        ADDRESS_TEMPLATE_MALFORMED,
    };
    return ADDRESS_TEMPLATES[cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::EMPTY_SIZE,
        SmsMmsFuzz::LastIndex(ADDRESS_TEMPLATES.size()))];
}

static void RunMmsAddressFuzz(const uint8_t *data, size_t size)
{
    FuzzedDataProvider cursor(data, size);
    uint8_t opcode = cursor.ConsumeIntegral<uint8_t>();
    bool useTemplateAddress = cursor.ConsumeBool();
    uint32_t charset = SmsMmsFuzz::PickCharset(cursor);
    std::string inputAddress = useTemplateAddress ? PickAddressTemplate(cursor) :
        cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE);

    MmsAddress defaultAddress;
    defaultAddress.GetAddressString();
    defaultAddress.GetAddressCharset();
    defaultAddress.GetAddressType();

    MmsAddress directAddress(inputAddress, static_cast<MmsCharSets>(charset));
    directAddress.GetAddressString();
    directAddress.GetAddressCharset();
    directAddress.GetAddressType();

    uint32_t count = cursor.ConsumeIntegralInRange<uint32_t>(SmsMmsFuzz::MIN_BUFFER_SIZE, SmsMmsFuzz::MAX_PART_COUNT);
    for (uint32_t i = SmsMmsFuzz::EMPTY_SIZE; i < count; ++i) {
        MmsAddress address = SmsMmsFuzz::MakeAddress(cursor);
        switch ((opcode + i) % ADDRESS_OPERATION_KIND_COUNT) {
            case ADDRESS_USE_INPUT:
                address.SetMmsAddressString(inputAddress, static_cast<MmsCharSets>(charset));
                break;
            case ADDRESS_USE_TEMPLATE:
                address.SetMmsAddressString(PickAddressTemplate(cursor),
                    static_cast<MmsCharSets>(SmsMmsFuzz::PickCharset(cursor)));
                break;
            case ADDRESS_USE_RANDOM_TEXT:
                address.SetMmsAddressString(cursor.ConsumeRandomLengthString(SmsMmsFuzz::MAX_TEXT_SIZE),
                    static_cast<MmsCharSets>(SmsMmsFuzz::PickCharset(cursor)));
                break;
            default:
                address.SetMmsAddressString("", static_cast<MmsCharSets>(SmsMmsFuzz::EMPTY_SIZE));
                break;
        }
        address.GetAddressString();
        address.GetAddressCharset();
        address.GetAddressType();
    }
}
} // namespace Telephony
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Telephony::RunMmsAddressFuzz(data, size);
    return OHOS::Telephony::SmsMmsFuzz::FUZZER_SUCCESS;
}
