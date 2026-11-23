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

#include "smsservicemanagerclient_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <mutex>
#include <string>
#include <vector>

#include "sms_fuzzer_reply_remote.h"
#include "sms_fuzzer_utils.h"

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_stub.h"
#include "i_sms_service_interface.h"
#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "send_short_message_callback_stub.h"
#include "short_message.h"
#include "sms_service_ipc_interface_code.h"
#include "sms_service_manager_client.h"
#include "sms_service_proxy.h"
#include "singleton.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace SmsFuzz;

namespace {
constexpr uint32_t GROUP_SEND_MESSAGE = 0;
constexpr uint32_t GROUP_SIM = 1;
constexpr uint32_t GROUP_QUERY = 2;
constexpr uint32_t GROUP_CODEC = 3;
constexpr uint32_t GROUP_COUNT = GROUP_CODEC + 1;

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

void FuzzClientSendMessage(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::u16string desAddr = ConsumeText(fdp);
    std::u16string scAddr = ConsumeText(fdp);
    std::u16string text = ConsumeText(fdp);
    sptr<ISendShortMessageCallback> sendCallback = MakeSendCallback(fdp);
    sptr<IDeliveryShortMessageCallback> deliveryCallback = MakeDeliveryCallback(fdp);
    client.SendMessage(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
    client.SendMessageWithoutSave(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
    size_t payloadLen = fdp.ConsumeIntegralInRange<size_t>(0, MAX_DATA_SIZE);
    std::vector<uint8_t> payload = fdp.ConsumeBytes<uint8_t>(payloadLen);
    uint16_t port = fdp.ConsumeIntegral<uint16_t>();
    client.SendMessage(slotId, desAddr, scAddr, port, payload.data(), static_cast<uint16_t>(payload.size()),
        sendCallback, deliveryCallback);
}

void FuzzClientSimMessage(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::u16string smsc = ConsumeText(fdp);
    std::u16string pdu = ConsumeText(fdp);
    uint32_t msgIndex = fdp.ConsumeIntegral<uint32_t>();
    uint32_t status = fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS);
    client.SetScAddress(slotId, smsc);
    client.AddSimMessage(slotId, smsc, pdu, static_cast<ISmsServiceInterface::SimMessageStatus>(status));
    client.DelSimMessage(slotId, msgIndex);
    client.UpdateSimMessage(slotId, msgIndex, static_cast<ISmsServiceInterface::SimMessageStatus>(status), pdu, smsc);
}

void FuzzClientSimConfig(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::vector<int32_t> messageIds;
    int32_t listSize = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_LIST_COUNT);
    for (int32_t index = 0; index < listSize; ++index) {
        messageIds.emplace_back(fdp.ConsumeIntegral<int32_t>());
    }
    client.SetCBConfig(slotId, fdp.ConsumeBool(), fdp.ConsumeIntegral<uint32_t>(), fdp.ConsumeIntegral<uint32_t>(),
        fdp.ConsumeIntegral<uint8_t>());
    client.SetCBConfigList(slotId, messageIds, fdp.ConsumeIntegral<int32_t>());
    client.SetImsSmsConfig(slotId, fdp.ConsumeIntegral<int32_t>());
    client.SetDefaultSmsSlotId(slotId);
}

void FuzzClientQuery(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::u16string text = ConsumeText(fdp);
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
    client.GetSmsSegmentsInfo(slotId, text, fdp.ConsumeBool(), segInfo);
    std::u16string format;
    client.GetImsShortMessageFormat(format);
    client.HasSmsCapability();
}

void FuzzClientStatusQuery(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    int32_t shortCodeType = 0;
    client.GetSmsShortCodeType(slotId, ConsumeString(fdp), shortCodeType);
    bool isSupported = false;
    client.IsImsSmsSupported(slotId, isSupported);
}

void FuzzClientCodec(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    std::string pdu = ConsumeString(fdp);
    ShortMessage message;
    client.CreateMessage(pdu, ConsumeString(fdp), message);
    std::string dest;
    client.GetBase64Encode(pdu, dest);
    client.GetBase64Decode(pdu, dest);
    std::string encodeString;
    client.GetEncodeStringFunc(encodeString, ConsumeCharset(fdp), static_cast<uint32_t>(pdu.size()), pdu);
}

void FuzzClientMms(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::u16string mmsc = ConsumeText(fdp);
    std::u16string mmsData = ConsumeText(fdp);
    std::u16string ua = ConsumeText(fdp);
    std::u16string uaprof = ConsumeText(fdp);
    int64_t time = fdp.ConsumeIntegral<int64_t>();
    client.SendMms(slotId, mmsc, mmsData, ua, uaprof, time);
    client.DownloadMms(slotId, mmsc, mmsData, ua, uaprof);
}

void FuzzClientGroups(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    switch (fdp.ConsumeIntegralInRange<uint32_t>(GROUP_SEND_MESSAGE, GROUP_COUNT - 1)) {
        case GROUP_SEND_MESSAGE:
            FuzzClientSendMessage(client, fdp);
            break;
        case GROUP_SIM:
            FuzzClientSimMessage(client, fdp);
            FuzzClientSimConfig(client, fdp);
            break;
        case GROUP_QUERY:
            FuzzClientQuery(client, fdp);
            FuzzClientStatusQuery(client, fdp);
            break;
        default:
            FuzzClientCodec(client, fdp);
            FuzzClientMms(client, fdp);
            break;
    }
}
} // namespace

void DoSmsServiceManagerClientFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remoteObject = new SmsServiceReplyRemote(fdp);
    SmsServiceManagerClient &client = Singleton<SmsServiceManagerClient>::GetInstance();
    {
        std::lock_guard<std::mutex> lock(client.mutex_);
        client.smsServiceInterface_ = new SmsServiceProxy(remoteObject);
    }
    FuzzClientGroups(client, fdp);
    {
        // The mock holds a reference to the stack fdp, so the interface must be released before
        // this iteration returns; otherwise the singleton keeps a dangling reference.
        std::lock_guard<std::mutex> lock(client.mutex_);
        client.smsServiceInterface_ = nullptr;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsServiceManagerClientFuzzTest(data, size);
    return 0;
}
