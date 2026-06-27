/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "smsserviceproxy_fuzzer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_stub.h"
#include "securec.h"
#include "send_short_message_callback_stub.h"
#include "sms_service_proxy.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr size_t MAX_DATA_SIZE = 256;
constexpr size_t MAX_CB_ID_COUNT = 4;
constexpr uint32_t SIM_STATUS_MOD = 4;
constexpr uint32_t CHARSET_MOD = 128;
constexpr int32_t SLOT_COUNT = 2;
constexpr int32_t ENABLE_MOD = 2;
constexpr uint8_t GSM_NET_TYPE = 1;
constexpr uint16_t DEFAULT_PORT = 9200;
constexpr uint16_t PORT_RANGE = 4096;
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

int32_t MakeInt32(const uint8_t *data, size_t size)
{
    return static_cast<int32_t>(MakeUint32(data, size));
}

int64_t MakeInt64(const uint8_t *data, size_t size)
{
    int64_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0) {
        errno_t err = memcpy_s(&value, sizeof(value), data, copyLen);
        if (err != EOK) {
            return 0;
        }
    }
    return value;
}

uint16_t MakePort(const uint8_t *data, size_t size)
{
    return static_cast<uint16_t>(DEFAULT_PORT + (MakeUint32(data, size) % PORT_RANGE));
}

std::vector<uint8_t> MakeDataBuffer(const uint8_t *data, size_t size)
{
    size_t dataLen = std::min(size, MAX_DATA_SIZE);
    return std::vector<uint8_t>(data, data + dataLen);
}

std::vector<int32_t> MakeCellBroadcastIds(const uint8_t *data, size_t size)
{
    std::vector<int32_t> messageIds;
    size_t count = std::min(size, MAX_CB_ID_COUNT);
    for (size_t index = 0; index < count; ++index) {
        messageIds.emplace_back(static_cast<int32_t>(data[index]));
    }
    return messageIds;
}

ISmsServiceInterface::SimMessageStatus MakeSimMessageStatus(const uint8_t *data, size_t size)
{
    return static_cast<ISmsServiceInterface::SimMessageStatus>(MakeUint32(data, size) % SIM_STATUS_MOD);
}

void FuzzTextMessageProxy(SmsServiceProxy &proxy, int32_t slotId, const std::u16string &text)
{
    sptr<ISendShortMessageCallback> sendCallback = new SendShortMessageCallbackStub();
    sptr<IDeliveryShortMessageCallback> deliveryCallback = new DeliveryShortMessageCallbackStub();

    proxy.SendMessage(slotId, text, text, text, nullptr, nullptr);
    proxy.SendMessage(slotId, text, text, text, sendCallback, deliveryCallback);
    proxy.SendMessageWithoutSave(slotId, text, text, text, nullptr, nullptr);
    proxy.SendMessageWithoutSave(slotId, text, text, text, sendCallback, deliveryCallback);
}

void FuzzDataMessageProxy(SmsServiceProxy &proxy, const uint8_t *data, size_t size, int32_t slotId,
    const std::u16string &text)
{
    std::vector<uint8_t> dataBuffer = MakeDataBuffer(data, size);
    if (dataBuffer.empty()) {
        return;
    }

    sptr<ISendShortMessageCallback> sendCallback = new SendShortMessageCallbackStub();
    sptr<IDeliveryShortMessageCallback> deliveryCallback = new DeliveryShortMessageCallbackStub();
    uint16_t dataLen = static_cast<uint16_t>(dataBuffer.size());
    proxy.SendMessage(slotId, text, text, MakePort(data, size), dataBuffer.data(), dataLen, nullptr, deliveryCallback);
    proxy.SendMessage(slotId, text, text, MakePort(data, size), dataBuffer.data(), dataLen, sendCallback, nullptr);
    proxy.SendMessage(
        slotId, text, text, MakePort(data, size), dataBuffer.data(), dataLen, sendCallback, deliveryCallback);
}

void FuzzSimAndConfigProxy(SmsServiceProxy &proxy, const uint8_t *data, size_t size, int32_t slotId,
    const std::u16string &text)
{
    uint32_t msgIndex = MakeUint32(data, size);
    uint32_t fromMsgId = MakeUint32(data, size);
    uint32_t toMsgId = fromMsgId + static_cast<uint32_t>(size);
    std::vector<int32_t> messageIds = MakeCellBroadcastIds(data, size);

    proxy.SetSmscAddr(slotId, text);
    std::u16string smscAddress;
    proxy.GetSmscAddr(slotId, smscAddress);
    proxy.AddSimMessage(slotId, text, text, MakeSimMessageStatus(data, size));
    proxy.DelSimMessage(slotId, msgIndex);
    proxy.UpdateSimMessage(slotId, msgIndex, MakeSimMessageStatus(data, size), text, text);

    std::vector<ShortMessage> messages;
    proxy.GetAllSimMessages(slotId, messages);
    proxy.SetCBConfig(slotId, (size % ENABLE_MOD) == 0, fromMsgId, toMsgId, GSM_NET_TYPE);
    proxy.SetCBConfigList(slotId, messageIds, MakeInt32(data, size));
    proxy.SetImsSmsConfig(slotId, MakeInt32(data, size) % ENABLE_MOD);
    proxy.SetDefaultSmsSlotId(slotId);
}

void FuzzQueryProxy(SmsServiceProxy &proxy, const uint8_t *data, size_t size, int32_t slotId,
    const std::u16string &text)
{
    proxy.GetDefaultSmsSlotId();
    int32_t simId = 0;
    proxy.GetDefaultSmsSimId(simId);
    std::vector<std::u16string> splitMessage;
    proxy.SplitMessage(text, splitMessage);

    ISmsServiceInterface::SmsSegmentsInfo segInfo;
    proxy.GetSmsSegmentsInfo(slotId, text, (size % ENABLE_MOD) == 0, segInfo);
    int32_t shortCodeType = 0;
    proxy.GetSmsShortCodeType(slotId, MakeString(data, size), shortCodeType);
    bool isSupported = false;
    proxy.IsImsSmsSupported(slotId, isSupported);
    std::u16string format;
    proxy.GetImsShortMessageFormat(format);
    proxy.HasSmsCapability();
}

void FuzzMmsAndCodecProxy(SmsServiceProxy &proxy, const uint8_t *data, size_t size, int32_t slotId,
    const std::u16string &text)
{
    int64_t time = MakeInt64(data, size);
    proxy.SendMms(slotId, text, text, text, text, time, (size % ENABLE_MOD) == 0);
    std::u16string mmsData = text;
    proxy.DownloadMms(slotId, text, mmsData, text, text);

    std::string pdu = MakeString(data, size);
    ShortMessage message;
    proxy.CreateMessage(pdu, SPECIFICATION_3GPP, message);
    proxy.CreateMessage(pdu, SPECIFICATION_3GPP2, message);
    proxy.CreateMessage(pdu, pdu, message);

    std::string dest;
    proxy.GetBase64Encode(pdu, dest);
    proxy.GetBase64Decode(pdu, dest);
    std::string encodeString;
    uint32_t charset = MakeUint32(data, size) % CHARSET_MOD;
    proxy.GetEncodeStringFunc(encodeString, charset, static_cast<uint32_t>(pdu.size()), pdu);
}

void FuzzDeathRecipient()
{
    SmsServiceDeathRecipient recipient;
    wptr<IRemoteObject> remote;
    recipient.OnRemoteDied(remote);
    SmsServiceDeathRecipient::GotDeathRecipient();
}
} // namespace

void DoSmsServiceProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    sptr<IRemoteObject> remote = nullptr;
    SmsServiceProxy proxy(remote);
    int32_t slotId = static_cast<int32_t>(MakeUint32(data, size) % SLOT_COUNT);
    std::u16string text = MakeString16(data, size);

    FuzzTextMessageProxy(proxy, slotId, text);
    FuzzDataMessageProxy(proxy, data, size, slotId, text);
    FuzzSimAndConfigProxy(proxy, data, size, slotId, text);
    FuzzQueryProxy(proxy, data, size, slotId, text);
    FuzzMmsAndCodecProxy(proxy, data, size, slotId, text);
    FuzzDeathRecipient();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsServiceProxyFuzzTest(data, size);
    return 0;
}
