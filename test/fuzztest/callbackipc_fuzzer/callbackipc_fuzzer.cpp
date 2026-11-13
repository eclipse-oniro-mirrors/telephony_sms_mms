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

#include "callbackipc_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_ipc_interface_code.h"
#include "delivery_short_message_callback_stub.h"
#include "ims_sms_callback_ipc_interface_code.h"
#include "ims_sms_callback_stub.h"
#include "ipc_object_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "send_short_message_callback_ipc_interface_code.h"
#include "send_short_message_callback_stub.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr uint8_t ASCII_PRINTABLE_BASE = 0x20;
constexpr uint8_t ASCII_PRINTABLE_RANGE = 0x5F;
constexpr int32_t MAX_SLOT_ID = 2;
constexpr int32_t INVALID_SLOT_ID = -1;
constexpr uint32_t UNKNOWN_CODE_OFFSET = 100;
constexpr uint32_t STUB_FAMILY_COUNT = 3;

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

uint32_t MakeCode(uint32_t validCode, FuzzedDataProvider &fdp)
{
    uint32_t selector = fdp.ConsumeIntegralInRange<uint32_t>(0, 2);
    if (selector == 0) {
        return validCode;
    }
    if (selector == 1) {
        return validCode + UNKNOWN_CODE_OFFSET + fdp.ConsumeIntegralInRange<uint32_t>(0, UNKNOWN_CODE_OFFSET);
    }
    return fdp.ConsumeIntegral<uint32_t>();
}

void CallRemoteRequest(
    IPCObjectStub &stub, uint32_t code, MessageParcel &data, FuzzedDataProvider &fdp, bool withDescriptor)
{
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!withDescriptor) {
        MessageParcel badData;
        badData.WriteInterfaceToken(u"bad.descriptor");
        badData.WriteInt32(MakeSlotId(fdp));
        stub.OnRemoteRequest(code, badData, reply, option);
        return;
    }
    stub.OnRemoteRequest(code, data, reply, option);
}

void WriteSendResultParcel(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteString16(MakeText(fdp));
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
}

void WriteDeliveryResultParcel(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    parcel.WriteString16(MakeText(fdp));
}

void FuzzSendCallback(FuzzedDataProvider &fdp)
{
    SendShortMessageCallbackStub stub;
    uint32_t code = MakeCode(
        static_cast<uint32_t>(SendShortMessageCallbackInterfaceCode::ON_SMS_SEND_RESULT), fdp);
    MessageParcel parcel;
    parcel.WriteInterfaceToken(SendShortMessageCallbackStub::GetDescriptor());
    WriteSendResultParcel(parcel, fdp);
    CallRemoteRequest(stub, code, parcel, fdp, fdp.ConsumeBool());
}

void FuzzDeliveryCallback(FuzzedDataProvider &fdp)
{
    DeliveryShortMessageCallbackStub stub;
    uint32_t code = MakeCode(
        static_cast<uint32_t>(DeliveryShortMessageCallbackInterfaceCode::ON_SMS_DELIVERY_RESULT), fdp);
    MessageParcel parcel;
    parcel.WriteInterfaceToken(DeliveryShortMessageCallbackStub::GetDescriptor());
    WriteDeliveryResultParcel(parcel, fdp);
    CallRemoteRequest(stub, code, parcel, fdp, fdp.ConsumeBool());
}

void WriteRadioResponseParcel(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
}

void WriteSendSmsResultParcel(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteString(MakeString(fdp));
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    parcel.WriteInt64(fdp.ConsumeIntegral<int64_t>());
}

void FuzzImsSendCallback(FuzzedDataProvider &fdp)
{
    ImsSmsCallbackStub stub;
    uint32_t code = MakeCode(static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_SEND_MESSAGE), fdp);
    MessageParcel parcel;
    parcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    parcel.WriteInt32(MakeSlotId(fdp));
    if (fdp.ConsumeBool()) {
        WriteRadioResponseParcel(parcel, fdp);
    } else {
        WriteSendSmsResultParcel(parcel, fdp);
    }
    CallRemoteRequest(stub, code, parcel, fdp, fdp.ConsumeBool());
}

void FuzzImsConfigCallback(FuzzedDataProvider &fdp)
{
    ImsSmsCallbackStub stub;
    uint32_t setCode = MakeCode(static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_SET_SMS_CONFIG), fdp);
    MessageParcel setParcel;
    setParcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    setParcel.WriteInt32(MakeSlotId(fdp));
    WriteRadioResponseParcel(setParcel, fdp);
    CallRemoteRequest(stub, setCode, setParcel, fdp, true);

    uint32_t getCode = MakeCode(static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_GET_SMS_CONFIG), fdp);
    MessageParcel getParcel;
    getParcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    getParcel.WriteInt32(MakeSlotId(fdp));
    getParcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    CallRemoteRequest(stub, getCode, getParcel, fdp, fdp.ConsumeBool());
}
} // namespace

void DoCallbackIpcFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    switch (fdp.ConsumeIntegralInRange<uint32_t>(0, STUB_FAMILY_COUNT - 1)) {
        case 0:
            FuzzSendCallback(fdp);
            break;
        case 1:
            FuzzDeliveryCallback(fdp);
            break;
        default:
            FuzzImsSendCallback(fdp);
            FuzzImsConfigCallback(fdp);
            break;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoCallbackIpcFuzzTest(data, size);
    return 0;
}
