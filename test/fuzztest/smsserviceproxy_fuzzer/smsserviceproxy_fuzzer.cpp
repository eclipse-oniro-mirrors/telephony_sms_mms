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

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_stub.h"
#include "i_sms_service_interface.h"
#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "send_short_message_callback_stub.h"
#include "short_message.h"
#include "sms_fuzzer_reply_remote.h"
#include "sms_fuzzer_utils.h"
#include "sms_service_ipc_interface_code.h"
#include "sms_service_proxy.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace SmsFuzz;

namespace {
constexpr uint32_t GROUP_TEXT_MESSAGE = 0;
constexpr uint32_t GROUP_DATA_MESSAGE = 1;
constexpr uint32_t GROUP_SIM_MESSAGE = 2;
constexpr uint32_t GROUP_CONFIG = 3;
constexpr uint32_t GROUP_QUERY = 4;
constexpr uint32_t GROUP_MMS_CODEC = 5;
constexpr uint32_t GROUP_COUNT = GROUP_MMS_CODEC + 1;

sptr<ISendShortMessageCallback> MakeSendCallback(FuzzedDataProvider &fdp)
{
    if (!fdp.ConsumeBool()) {
        return nullptr;
    }
    return new SendShortMessageCallbackStub();
}

sptr<IDeliveryShortMessageCallback> MakeDeliveryCallback(FuzzedDataProvider &fdp)
{
    if (!fdp.ConsumeBool()) {
        return nullptr;
    }
    return new DeliveryShortMessageCallbackStub();
}

void FuzzTextMessage(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string desAddr = ConsumeText(fdp);
    std::u16string scAddr = ConsumeText(fdp);
    std::u16string text = ConsumeText(fdp);
    sptr<ISendShortMessageCallback> sendCallback = MakeSendCallback(fdp);
    sptr<IDeliveryShortMessageCallback> deliveryCallback = MakeDeliveryCallback(fdp);
    proxy.SendMessage(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
    proxy.SendMessageWithoutSave(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
}

void FuzzDataMessage(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string desAddr = ConsumeText(fdp);
    std::u16string scAddr = ConsumeText(fdp);
    uint16_t port = fdp.ConsumeIntegral<uint16_t>();
    size_t payloadLen = fdp.ConsumeIntegralInRange<size_t>(0, MAX_DATA_SIZE);
    std::vector<uint8_t> payload = fdp.ConsumeBytes<uint8_t>(payloadLen);
    sptr<ISendShortMessageCallback> sendCallback = MakeSendCallback(fdp);
    sptr<IDeliveryShortMessageCallback> deliveryCallback = MakeDeliveryCallback(fdp);
    if (payload.empty()) {
        proxy.SendMessage(slotId, desAddr, scAddr, port, nullptr, 0, sendCallback, deliveryCallback);
        return;
    }
    proxy.SendMessage(slotId, desAddr, scAddr, port, payload.data(), static_cast<uint16_t>(payload.size()),
        sendCallback, deliveryCallback);
}

void FuzzSimMessage(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string text = ConsumeText(fdp);
    uint32_t msgIndex = fdp.ConsumeIntegral<uint32_t>();
    uint32_t status = fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS);
    proxy.AddSimMessage(slotId, text, text, static_cast<ISmsServiceInterface::SimMessageStatus>(status));
    proxy.DelSimMessage(slotId, msgIndex);
    proxy.UpdateSimMessage(slotId, msgIndex, static_cast<ISmsServiceInterface::SimMessageStatus>(status), text, text);
    proxy.SetSmscAddr(slotId, text);
    std::u16string smscAddress;
    proxy.GetSmscAddr(slotId, smscAddress);
    std::vector<ShortMessage> messages;
    proxy.GetAllSimMessages(slotId, messages);
}

void FuzzConfig(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    bool enable = fdp.ConsumeBool();
    uint32_t fromMsgId = fdp.ConsumeIntegral<uint32_t>();
    uint32_t toMsgId = fdp.ConsumeIntegral<uint32_t>();
    uint8_t ranType = fdp.ConsumeIntegral<uint8_t>();
    int32_t listSize = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_SPLIT_COUNT);
    std::vector<int32_t> messageIds;
    for (int32_t index = 0; index < listSize; ++index) {
        messageIds.emplace_back(fdp.ConsumeIntegral<int32_t>());
    }
    proxy.SetCBConfig(slotId, enable, fromMsgId, toMsgId, ranType);
    proxy.SetCBConfigList(slotId, messageIds, fdp.ConsumeIntegral<int32_t>());
    proxy.SetImsSmsConfig(slotId, fdp.ConsumeIntegral<int32_t>());
    proxy.SetDefaultSmsSlotId(slotId);
}

void FuzzQuery(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string text = ConsumeText(fdp);
    std::string desAddr = ConsumeString(fdp);
    proxy.GetDefaultSmsSlotId();
    int32_t simId = 0;
    proxy.GetDefaultSmsSimId(simId);
    std::vector<std::u16string> splitMessage;
    proxy.SplitMessage(text, splitMessage);
    ISmsServiceInterface::SmsSegmentsInfo segInfo;
    proxy.GetSmsSegmentsInfo(slotId, text, fdp.ConsumeBool(), segInfo);
    int32_t shortCodeType = 0;
    proxy.GetSmsShortCodeType(slotId, desAddr, shortCodeType);
    bool isSupported = false;
    proxy.IsImsSmsSupported(slotId, isSupported);
    std::u16string format;
    proxy.GetImsShortMessageFormat(format);
    proxy.HasSmsCapability();
}

void FuzzMmsAndCodec(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string mmsc = ConsumeText(fdp);
    std::u16string mmsData = ConsumeText(fdp);
    std::u16string ua = ConsumeText(fdp);
    std::u16string uaprof = ConsumeText(fdp);
    int64_t time = fdp.ConsumeIntegral<int64_t>();
    proxy.SendMms(slotId, mmsc, mmsData, ua, uaprof, time, fdp.ConsumeBool());
    proxy.DownloadMms(slotId, mmsc, mmsData, ua, uaprof);

    std::string pdu = ConsumeString(fdp);
    std::string specification = ConsumeString(fdp);
    ShortMessage message;
    proxy.CreateMessage(pdu, specification, message);
    std::string dest;
    proxy.GetBase64Encode(pdu, dest);
    proxy.GetBase64Decode(pdu, dest);
    std::string encodeString;
    proxy.GetEncodeStringFunc(encodeString, ConsumeCharset(fdp), static_cast<uint32_t>(pdu.size()), pdu);
}
} // namespace

void DoSmsServiceProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remote = new SmsServiceReplyRemote(fdp);
    SmsServiceProxy proxy(remote);
    int32_t slotId = ConsumeSlotId(fdp);

    switch (fdp.ConsumeIntegralInRange<uint32_t>(GROUP_TEXT_MESSAGE, GROUP_COUNT - 1)) {
        case GROUP_TEXT_MESSAGE:
            FuzzTextMessage(proxy, fdp, slotId);
            break;
        case GROUP_DATA_MESSAGE:
            FuzzDataMessage(proxy, fdp, slotId);
            break;
        case GROUP_SIM_MESSAGE:
            FuzzSimMessage(proxy, fdp, slotId);
            break;
        case GROUP_CONFIG:
            FuzzConfig(proxy, fdp, slotId);
            break;
        case GROUP_QUERY:
            FuzzQuery(proxy, fdp, slotId);
            break;
        default:
            FuzzMmsAndCodec(proxy, fdp, slotId);
            break;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsServiceProxyFuzzTest(data, size);
    return 0;
}
