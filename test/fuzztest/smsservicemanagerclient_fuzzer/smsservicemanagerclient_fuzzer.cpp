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
#include <memory>
#include <mutex>
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
#include "sms_service_ipc_interface_code.h"
#include "sms_service_proxy.h"
#include "singleton.h"

#define private public
#include "sms_service_manager_client.h"
#undef private

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr size_t MAX_DATA_SIZE = 256;
constexpr uint8_t ASCII_PRINTABLE_BASE = 0x20;
constexpr uint8_t ASCII_PRINTABLE_RANGE = 0x5F;
constexpr int32_t MAX_SLOT_ID = 2;
constexpr int32_t INVALID_SLOT_ID = -1;
constexpr uint32_t MAX_SIM_STATUS = 8;
constexpr uint32_t MAX_CHARSET = 0x800;
constexpr uint32_t CHARSET_BASE = 0x3A;
constexpr uint32_t RESULT_ERROR_MOD = 4;
constexpr int32_t MAX_LIST_COUNT = 4;
constexpr int32_t MAX_SEGMENT_COUNT = 8;
constexpr uint32_t GROUP_COUNT = 4;
constexpr int32_t REPLY_RESULT_SUCCESS = 0;
constexpr int32_t REPLY_RESULT_FAILURE = -1;
constexpr int32_t REMOTE_CALL_SUCCESS = 0;

std::u16string MakeText(FuzzedDataProvider &fdp)
{
    size_t len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_STRING_SIZE);
    std::vector<uint8_t> raw = fdp.ConsumeBytes<uint8_t>(len);
    std::u16string text;
    text.reserve(raw.size());
    for (uint8_t byte : raw) {
        text.push_back(static_cast<char16_t>(ASCII_PRINTABLE_BASE + byte % ASCII_PRINTABLE_RANGE));
    }
    return text;
}

std::string MakeString(FuzzedDataProvider &fdp)
{
    size_t len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_STRING_SIZE);
    std::vector<uint8_t> raw = fdp.ConsumeBytes<uint8_t>(len);
    std::string text;
    text.reserve(raw.size());
    for (uint8_t byte : raw) {
        text.push_back(static_cast<char>(ASCII_PRINTABLE_BASE + byte % ASCII_PRINTABLE_RANGE));
    }
    return text;
}

class ClientReplyRemote final : public IRemoteObject {
public:
    explicit ClientReplyRemote(FuzzedDataProvider &fdp) : IRemoteObject(u""), fdp_(fdp) {}

    int32_t GetObjectRefCount() override
    {
        return 1;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        FillReply(static_cast<SmsServiceInterfaceCode>(code), reply);
        return REMOTE_CALL_SUCCESS;
    }

private:
    void FillStringReply(MessageParcel &reply)
    {
        int32_t result = MakeResult();
        reply.WriteInt32(result);
        if (result == REPLY_RESULT_SUCCESS) {
            reply.WriteString16(MakeText(fdp_));
        }
    }

    void WriteShortMessage(MessageParcel &reply)
    {
        ShortMessage message;
        message.visibleMessageBody_ = MakeText(fdp_);
        message.visibleRawAddress_ = MakeText(fdp_);
        message.scAddress_ = MakeText(fdp_);
        message.scTimestamp_ = fdp_.ConsumeIntegral<int64_t>();
        message.status_ = fdp_.ConsumeIntegral<int32_t>();
        message.Marshalling(reply);
    }

    void FillListReply(SmsServiceInterfaceCode code, MessageParcel &reply)
    {
        int32_t count = fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_LIST_COUNT);
        reply.WriteInt32(REPLY_RESULT_SUCCESS);
        reply.WriteInt32(count);
        for (int32_t index = 0; index < count; ++index) {
            if (code == SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE) {
                WriteShortMessage(reply);
            } else {
                reply.WriteString16(MakeText(fdp_));
            }
        }
    }

    void FillSegmentsReply(MessageParcel &reply)
    {
        reply.WriteInt32(REPLY_RESULT_SUCCESS);
        for (int32_t index = 0; index < MAX_LIST_COUNT; ++index) {
            reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        }
    }

    void FillShortMessageReply(MessageParcel &reply)
    {
        reply.WriteInt32(MakeResult());
        WriteShortMessage(reply);
    }

    void FillReply(SmsServiceInterfaceCode code, MessageParcel &reply)
    {
        switch (code) {
            case SmsServiceInterfaceCode::GET_SMSC_ADDRESS:
            case SmsServiceInterfaceCode::GET_IMS_SHORT_MESSAGE_FORMAT:
                FillStringReply(reply);
                break;
            case SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE:
            case SmsServiceInterfaceCode::SPLIT_MESSAGE:
                FillListReply(code, reply);
                break;
            case SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO:
                FillSegmentsReply(reply);
                break;
            case SmsServiceInterfaceCode::CREATE_MESSAGE:
                FillShortMessageReply(reply);
                break;
            default:
                reply.WriteInt32(MakeResult());
                break;
        }
    }

    int32_t MakeResult()
    {
        if (fdp_.ConsumeIntegralInRange<uint32_t>(0, RESULT_ERROR_MOD) == 0) {
            return REPLY_RESULT_FAILURE;
        }
        return REPLY_RESULT_SUCCESS;
    }

    FuzzedDataProvider &fdp_;
};

int32_t MakeSlotId(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeIntegralInRange<int32_t>(INVALID_SLOT_ID, MAX_SLOT_ID);
}

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
    int32_t slotId = MakeSlotId(fdp);
    std::u16string desAddr = MakeText(fdp);
    std::u16string scAddr = MakeText(fdp);
    std::u16string text = MakeText(fdp);
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
    int32_t slotId = MakeSlotId(fdp);
    std::u16string smsc = MakeText(fdp);
    std::u16string pdu = MakeText(fdp);
    uint32_t msgIndex = fdp.ConsumeIntegral<uint32_t>();
    uint32_t status = fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS);
    client.SetScAddress(slotId, smsc);
    client.AddSimMessage(slotId, smsc, pdu, static_cast<ISmsServiceInterface::SimMessageStatus>(status));
    client.DelSimMessage(slotId, msgIndex);
    client.UpdateSimMessage(slotId, msgIndex, static_cast<ISmsServiceInterface::SimMessageStatus>(status), pdu, smsc);
}

void FuzzClientSimConfig(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = MakeSlotId(fdp);
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
    int32_t slotId = MakeSlotId(fdp);
    std::u16string text = MakeText(fdp);
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
    int32_t slotId = MakeSlotId(fdp);
    int32_t shortCodeType = 0;
    client.GetSmsShortCodeType(slotId, MakeString(fdp), shortCodeType);
    bool isSupported = false;
    client.IsImsSmsSupported(slotId, isSupported);
}

void FuzzClientCodec(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    std::string pdu = MakeString(fdp);
    ShortMessage message;
    client.CreateMessage(pdu, MakeString(fdp), message);
    std::string dest;
    client.GetBase64Encode(pdu, dest);
    client.GetBase64Decode(pdu, dest);
    std::string encodeString;
    uint32_t charset = CHARSET_BASE + fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CHARSET);
    client.GetEncodeStringFunc(encodeString, charset, static_cast<uint32_t>(pdu.size()), pdu);
}

void FuzzClientMms(SmsServiceManagerClient &client, FuzzedDataProvider &fdp)
{
    int32_t slotId = MakeSlotId(fdp);
    std::u16string mmsc = MakeText(fdp);
    std::u16string mmsData = MakeText(fdp);
    std::u16string ua = MakeText(fdp);
    std::u16string uaprof = MakeText(fdp);
    client.SendMms(slotId, mmsc, mmsData, ua, uaprof, fdp.ConsumeIntegral<int64_t>(), fdp.ConsumeBool());
    client.DownloadMms(slotId, mmsc, mmsData, ua, uaprof);
}
} // namespace

void DoSmsServiceManagerClientFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remoteObject = new ClientReplyRemote(fdp);
    SmsServiceManagerClient &client = Singleton<SmsServiceManagerClient>::GetInstance();
    client.smsServiceInterface_ = new SmsServiceProxy(remoteObject);
    if (client.smsServiceInterface_ == nullptr) {
        return;
    }

    switch (fdp.ConsumeIntegralInRange<uint32_t>(0, GROUP_COUNT - 1)) {
        case 0:
            FuzzClientSendMessage(client, fdp);
            break;
        case 1:
            FuzzClientSimMessage(client, fdp);
            FuzzClientSimConfig(client, fdp);
            break;
        case 2:
            FuzzClientQuery(client, fdp);
            FuzzClientStatusQuery(client, fdp);
            break;
        default:
            FuzzClientCodec(client, fdp);
            FuzzClientMms(client, fdp);
            break;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsServiceManagerClientFuzzTest(data, size);
    return 0;
}
