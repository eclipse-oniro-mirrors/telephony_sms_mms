/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "smsservicemanagerclient_fuzzer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "securec.h"
#include "sms_service_manager_client.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr uint32_t CHARSET_MOD = 128;
constexpr int32_t SLOT_COUNT = 2;
constexpr int32_t ENABLE_MOD = 2;
constexpr char SPECIFICATION_3GPP[] = "3gpp";
constexpr char SPECIFICATION_3GPP2[] = "3gpp2";

std::string MakeString(const uint8_t *data, size_t size)
{
    return std::string(reinterpret_cast<const char *>(data), std::min(size, MAX_STRING_SIZE));
}

std::u16string MakeString16(const uint8_t *data, size_t size)
{
    return StringUtils::ToUtf16(MakeString(data, size));
}

uint32_t MakeUint32(const uint8_t *data, size_t size)
{
    uint32_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0) {
        errno_t err = memcpy_s(&value, sizeof(value), data, copyLen);
        if (err != EOK) {
            return 0;
        }
    }
    return value;
}

void FuzzSafeQueries(SmsServiceManagerClient &client, const uint8_t *data, size_t size, int32_t slotId,
    const std::u16string &text)
{
    client.GetDefaultSmsSlotId();
    int32_t simId = 0;
    client.GetDefaultSmsSimId(simId);
    std::u16string smscAddress;
    client.GetScAddress(slotId, smscAddress);
    std::vector<ShortMessage> messages;
    client.GetAllSimMessages(slotId, messages);

    std::vector<std::u16string> splitMessage;
    client.SplitMessage(text, splitMessage);
    ISmsServiceInterface::SmsSegmentsInfo segInfo;
    client.GetSmsSegmentsInfo(slotId, text, (size % ENABLE_MOD) == 0, segInfo);
    int32_t shortCodeType = 0;
    client.GetSmsShortCodeType(slotId, MakeString(data, size), shortCodeType);
    bool isSupported = false;
    client.IsImsSmsSupported(slotId, isSupported);
    std::u16string format;
    client.GetImsShortMessageFormat(format);
    client.HasSmsCapability();
}

void FuzzCodecHelpers(SmsServiceManagerClient &client, const uint8_t *data, size_t size)
{
    std::string pdu = MakeString(data, size);
    ShortMessage message;
    client.CreateMessage(pdu, SPECIFICATION_3GPP, message);
    client.CreateMessage(pdu, SPECIFICATION_3GPP2, message);
    client.CreateMessage(pdu, pdu, message);

    std::string dest;
    client.GetBase64Encode(pdu, dest);
    client.GetBase64Decode(pdu, dest);
    std::string encodeString;
    uint32_t charset = MakeUint32(data, size) % CHARSET_MOD;
    client.GetEncodeStringFunc(encodeString, charset, static_cast<uint32_t>(pdu.size()), pdu);
}
} // namespace

void DoSmsServiceManagerClientFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    SmsServiceManagerClient &client = Singleton<SmsServiceManagerClient>::GetInstance();
    int32_t slotId = static_cast<int32_t>(MakeUint32(data, size) % SLOT_COUNT);
    std::u16string text = MakeString16(data, size);

    client.InitSmsServiceProxy();
    FuzzSafeQueries(client, data, size, slotId, text);
    FuzzCodecHelpers(client, data, size);
    client.ResetSmsServiceProxy();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsServiceManagerClientFuzzTest(data, size);
    return 0;
}
