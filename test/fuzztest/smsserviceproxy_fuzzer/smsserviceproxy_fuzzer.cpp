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
#include "sms_service_ipc_interface_code.h"
#include "sms_service_proxy.h"

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
constexpr uint32_t MAX_MESSAGE_CLASS = 4;
constexpr uint32_t MAX_CHARSET = 0x800;
constexpr uint32_t CHARSET_BASE = 0x3A;
constexpr uint32_t IPC_ERROR_MOD = 8;
constexpr uint32_t RESULT_ERROR_MOD = 4;
constexpr int32_t MAX_SIM_MESSAGE_COUNT = 2;
constexpr int32_t MAX_SPLIT_COUNT = 4;
constexpr int32_t MAX_SEGMENT_COUNT = 8;
constexpr uint32_t GROUP_TEXT_MESSAGE = 0;
constexpr uint32_t GROUP_DATA_MESSAGE = 1;
constexpr uint32_t GROUP_SIM_MESSAGE = 2;
constexpr uint32_t GROUP_CONFIG = 3;
constexpr uint32_t GROUP_QUERY = 4;
constexpr uint32_t GROUP_MMS_CODEC = 5;
constexpr uint32_t GROUP_COUNT = GROUP_MMS_CODEC + 1;
constexpr int32_t REMOTE_CALL_SUCCESS = 0;
constexpr int32_t REMOTE_CALL_FAILURE = -1;
constexpr int32_t REPLY_RESULT_SUCCESS = 0;
constexpr int32_t REPLY_RESULT_FAILURE = -1;

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

int32_t MakeSlotId(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeIntegralInRange<int32_t>(INVALID_SLOT_ID, MAX_SLOT_ID);
}

uint32_t MakeCharset(FuzzedDataProvider &fdp)
{
    return CHARSET_BASE + fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CHARSET);
}

class SmsReplyRemote final : public IRemoteObject {
public:
    explicit SmsReplyRemote(FuzzedDataProvider &fdp) : IRemoteObject(u""), fdp_(fdp) {}

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
        if (fdp_.ConsumeIntegralInRange<uint32_t>(0, IPC_ERROR_MOD) == 0) {
            return REMOTE_CALL_FAILURE;
        }
        return REMOTE_CALL_SUCCESS;
    }

private:
    int32_t MakeResult()
    {
        if (fdp_.ConsumeIntegralInRange<uint32_t>(0, RESULT_ERROR_MOD) == 0) {
            return REPLY_RESULT_FAILURE;
        }
        return REPLY_RESULT_SUCCESS;
    }

    void FillShortMessage(MessageParcel &reply)
    {
        ShortMessage message;
        message.visibleMessageBody_ = MakeText(fdp_);
        message.visibleRawAddress_ = MakeText(fdp_);
        message.scAddress_ = MakeText(fdp_);
        message.scTimestamp_ = fdp_.ConsumeIntegral<int64_t>();
        message.status_ = fdp_.ConsumeIntegral<int32_t>();
        message.messageClass_ = static_cast<ShortMessage::SmsMessageClass>(
            fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_MESSAGE_CLASS));
        message.simMessageStatus_ = static_cast<ShortMessage::SmsSimMessageStatus>(
            fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS));
        message.Marshalling(reply);
    }

    void FillSimMessageList(MessageParcel &reply)
    {
        int32_t count = fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SIM_MESSAGE_COUNT);
        reply.WriteInt32(count);
        for (int32_t index = 0; index < count; ++index) {
            FillShortMessage(reply);
        }
    }

    void FillSplitMessage(MessageParcel &reply)
    {
        int32_t count = fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SPLIT_COUNT);
        reply.WriteInt32(count);
        for (int32_t index = 0; index < count; ++index) {
            reply.WriteString16(MakeText(fdp_));
        }
    }

    void FillSegmentsInfo(MessageParcel &reply)
    {
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegral<int32_t>());
    }

    void FillResultAndString(MessageParcel &reply)
    {
        int32_t result = MakeResult();
        reply.WriteInt32(result);
        if (result == REPLY_RESULT_SUCCESS) {
            reply.WriteString16(MakeText(fdp_));
        }
    }

    void FillBoolAndString(MessageParcel &reply)
    {
        bool result = fdp_.ConsumeBool();
        reply.WriteBool(result);
        if (result) {
            reply.WriteString16(MakeText(fdp_));
        }
    }

    void FillReply(SmsServiceInterfaceCode code, MessageParcel &reply)
    {
        switch (code) {
            case SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG:
            case SmsServiceInterfaceCode::HAS_SMS_CAPABILITY:
                reply.WriteBool(fdp_.ConsumeBool());
                break;
            case SmsServiceInterfaceCode::GET_SMSC_ADDRESS:
            case SmsServiceInterfaceCode::GET_IMS_SHORT_MESSAGE_FORMAT:
            case SmsServiceInterfaceCode::DOWNLOAD_MMS:
                FillResultAndString(reply);
                break;
            case SmsServiceInterfaceCode::MMS_BASE64_ENCODE:
            case SmsServiceInterfaceCode::MMS_BASE64_DECODE:
            case SmsServiceInterfaceCode::GET_ENCODE_STRING:
                FillBoolAndString(reply);
                break;
            case SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE:
                FillSimMessageList(reply);
                break;
            case SmsServiceInterfaceCode::SPLIT_MESSAGE:
                FillSplitMessage(reply);
                break;
            case SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO:
                FillSegmentsInfo(reply);
                break;
            case SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED:
                reply.WriteInt32(MakeResult());
                reply.WriteBool(fdp_.ConsumeBool());
                break;
            case SmsServiceInterfaceCode::CREATE_MESSAGE:
                reply.WriteInt32(MakeResult());
                FillShortMessage(reply);
                break;
            default:
                reply.WriteInt32(MakeResult());
                break;
        }
    }

    FuzzedDataProvider &fdp_;
};

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
    std::u16string desAddr = MakeText(fdp);
    std::u16string scAddr = MakeText(fdp);
    std::u16string text = MakeText(fdp);
    sptr<ISendShortMessageCallback> sendCallback = MakeSendCallback(fdp);
    sptr<IDeliveryShortMessageCallback> deliveryCallback = MakeDeliveryCallback(fdp);
    proxy.SendMessage(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
    proxy.SendMessageWithoutSave(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
}

void FuzzDataMessage(SmsServiceProxy &proxy, FuzzedDataProvider &fdp, int32_t slotId)
{
    std::u16string desAddr = MakeText(fdp);
    std::u16string scAddr = MakeText(fdp);
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
    std::u16string text = MakeText(fdp);
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
    std::u16string text = MakeText(fdp);
    std::string desAddr = MakeString(fdp);
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
    std::u16string mmsc = MakeText(fdp);
    std::u16string mmsData = MakeText(fdp);
    std::u16string ua = MakeText(fdp);
    std::u16string uaprof = MakeText(fdp);
    int64_t time = fdp.ConsumeIntegral<int64_t>();
    proxy.SendMms(slotId, mmsc, mmsData, ua, uaprof, time, fdp.ConsumeBool());
    proxy.DownloadMms(slotId, mmsc, mmsData, ua, uaprof);

    std::string pdu = MakeString(fdp);
    std::string specification = MakeString(fdp);
    ShortMessage message;
    proxy.CreateMessage(pdu, specification, message);
    std::string dest;
    proxy.GetBase64Encode(pdu, dest);
    proxy.GetBase64Decode(pdu, dest);
    std::string encodeString;
    proxy.GetEncodeStringFunc(encodeString, MakeCharset(fdp), static_cast<uint32_t>(pdu.size()), pdu);
}
} // namespace

void DoSmsServiceProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remote = new SmsReplyRemote(fdp);
    SmsServiceProxy proxy(remote);
    int32_t slotId = MakeSlotId(fdp);

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
