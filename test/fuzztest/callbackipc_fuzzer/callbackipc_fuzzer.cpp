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

#include "callbackipc_fuzzer.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_ipc_interface_code.h"
#include "delivery_short_message_callback_stub.h"
#include "ims_sms_callback_ipc_interface_code.h"
#include "ims_sms_callback_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "securec.h"
#include "send_short_message_callback_ipc_interface_code.h"
#include "send_short_message_callback_stub.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr uint32_t UNKNOWN_CODE_OFFSET = 100;
constexpr int32_t SLOT_COUNT = 2;
constexpr int32_t SALT_RADIO_FLAG = 1;
constexpr int32_t SALT_RADIO_SERIAL = 2;
constexpr int32_t SALT_RADIO_ERROR = 3;

std::string MakeString(const uint8_t *data, size_t size)
{
    return std::string(reinterpret_cast<const char *>(data), std::min(size, MAX_STRING_SIZE));
}

int32_t MakeInt32(const uint8_t *data, size_t size)
{
    int32_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0) {
        errno_t err = memcpy_s(&value, sizeof(value), data, copyLen);
        if (err != EOK) {
            return 0;
        }
    }
    return value;
}

int32_t MakeInt32WithSalt(const uint8_t *data, size_t size, int32_t salt)
{
    return MakeInt32(data, size) ^ salt;
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

std::vector<uint32_t> MakeCodes(uint32_t validCode, size_t size)
{
    std::vector<uint32_t> codes;
    codes.emplace_back(validCode);
    codes.emplace_back(validCode + UNKNOWN_CODE_OFFSET);
    codes.emplace_back(static_cast<uint32_t>(size));
    return codes;
}

void FuzzSendCallback(const uint8_t *data, size_t size)
{
    SendShortMessageCallbackStub stub;
    MessageOption option;
    uint32_t validCode = static_cast<uint32_t>(SendShortMessageCallbackInterfaceCode::ON_SMS_SEND_RESULT);
    for (uint32_t code : MakeCodes(validCode, size)) {
        MessageParcel parcel;
        MessageParcel reply;
        parcel.WriteInterfaceToken(SendShortMessageCallbackStub::GetDescriptor());
        parcel.WriteInt32(MakeInt32(data, size));
        stub.OnRemoteRequest(code, parcel, reply, option);
    }

    MessageParcel badParcel;
    MessageParcel badReply;
    badParcel.WriteInterfaceToken(u"bad.descriptor");
    badParcel.WriteInt32(MakeInt32(data, size));
    stub.OnRemoteRequest(validCode, badParcel, badReply, option);
    stub.OnSmsSendResult(static_cast<ISendShortMessageCallback::SmsSendResult>(MakeInt32(data, size)));
}

void FuzzDeliveryCallback(const uint8_t *data, size_t size)
{
    DeliveryShortMessageCallbackStub stub;
    MessageOption option;
    std::u16string pdu = StringUtils::ToUtf16(MakeString(data, size));
    uint32_t validCode = static_cast<uint32_t>(DeliveryShortMessageCallbackInterfaceCode::ON_SMS_DELIVERY_RESULT);
    for (uint32_t code : MakeCodes(validCode, size)) {
        MessageParcel parcel;
        MessageParcel reply;
        parcel.WriteInterfaceToken(DeliveryShortMessageCallbackStub::GetDescriptor());
        parcel.WriteString16(pdu);
        stub.OnRemoteRequest(code, parcel, reply, option);
    }

    MessageParcel badParcel;
    MessageParcel badReply;
    badParcel.WriteInterfaceToken(u"bad.descriptor");
    badParcel.WriteString16(pdu);
    stub.OnRemoteRequest(validCode, badParcel, badReply, option);
    stub.OnSmsDeliveryResult(pdu);
}

void WriteImsRadioResponseParcel(MessageParcel &parcel, int32_t slotId, const uint8_t *data, size_t size)
{
    parcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    parcel.WriteInt32(slotId);
    parcel.WriteInt32(MakeInt32(data, size));
    parcel.WriteInt32(MakeInt32WithSalt(data, size, SALT_RADIO_FLAG));
    parcel.WriteInt32(MakeInt32WithSalt(data, size, SALT_RADIO_SERIAL));
    parcel.WriteInt32(MakeInt32WithSalt(data, size, SALT_RADIO_ERROR));
}

void WriteImsSendResultParcel(MessageParcel &parcel, int32_t slotId, const uint8_t *data, size_t size)
{
    parcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    parcel.WriteInt32(slotId);
    parcel.WriteInt32(MakeInt32(data, size));
    parcel.WriteString(MakeString(data, size));
    parcel.WriteInt32(MakeInt32WithSalt(data, size, SALT_RADIO_FLAG));
    parcel.WriteInt64(MakeInt64(data, size));
}

void FuzzImsCallback(const uint8_t *data, size_t size)
{
    ImsSmsCallbackStub stub;
    MessageOption option;
    int32_t slotId = static_cast<int32_t>(size % SLOT_COUNT);
    uint32_t sendCode = static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_SEND_MESSAGE);
    uint32_t setCode = static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_SET_SMS_CONFIG);
    uint32_t getCode = static_cast<uint32_t>(ImsSmsCallbackInterfaceCode::IMS_GET_SMS_CONFIG);

    MessageParcel sendResultParcel;
    MessageParcel sendResultReply;
    WriteImsSendResultParcel(sendResultParcel, slotId, data, size);
    stub.OnRemoteRequest(sendCode, sendResultParcel, sendResultReply, option);

    MessageParcel sendRadioParcel;
    MessageParcel sendRadioReply;
    WriteImsRadioResponseParcel(sendRadioParcel, slotId, data, size);
    stub.OnRemoteRequest(sendCode, sendRadioParcel, sendRadioReply, option);

    MessageParcel setParcel;
    MessageParcel setReply;
    WriteImsRadioResponseParcel(setParcel, slotId, data, size);
    stub.OnRemoteRequest(setCode, setParcel, setReply, option);

    MessageParcel getParcel;
    MessageParcel getReply;
    getParcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    getParcel.WriteInt32(slotId);
    getParcel.WriteInt32(MakeInt32(data, size));
    stub.OnRemoteRequest(getCode, getParcel, getReply, option);

    MessageParcel unknownParcel;
    MessageParcel unknownReply;
    unknownParcel.WriteInterfaceToken(ImsSmsCallbackStub::GetDescriptor());
    unknownParcel.WriteInt32(slotId);
    stub.OnRemoteRequest(sendCode + UNKNOWN_CODE_OFFSET, unknownParcel, unknownReply, option);

    MessageParcel badParcel;
    MessageParcel badReply;
    badParcel.WriteInterfaceToken(u"bad.descriptor");
    badParcel.WriteInt32(slotId);
    stub.OnRemoteRequest(sendCode, badParcel, badReply, option);
}
} // namespace

void DoCallbackIpcFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }
    FuzzSendCallback(data, size);
    FuzzDeliveryCallback(data, size);
    FuzzImsCallback(data, size);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoCallbackIpcFuzzTest(data, size);
    return 0;
}
