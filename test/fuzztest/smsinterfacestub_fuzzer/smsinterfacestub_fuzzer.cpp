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

#include "smsinterfacestub_fuzzer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "delivery_short_message_callback_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "send_short_message_callback_stub.h"
#include "sms_interface_stub.h"
#include "sms_service.h"
#include "securec.h"
#include "sms_service_ipc_interface_code.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr size_t MAX_DATA_SIZE = 256;
constexpr int32_t VALID_SLOT_ID = 0;
constexpr int32_t INVALID_SLOT_ID = -1;
constexpr int32_t ENABLE_MOD = 2;
constexpr uint32_t SIM_STATUS_MOD = 4;
constexpr uint32_t INVALID_SIM_STATUS = 8;
constexpr int32_t MAX_CB_LIST_SIZE = 256;
constexpr int32_t CB_LIST_SAMPLE_SIZE = 4;
constexpr uint32_t UNKNOWN_CODE_OFFSET = 1000;
constexpr uint16_t DEFAULT_PORT = 9200;
constexpr uint16_t PORT_RANGE = 4096;
constexpr char SPECIFICATION_3GPP[] = "3gpp";

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

void WriteDescriptor(MessageParcel &parcel)
{
    parcel.WriteInterfaceToken(SmsInterfaceStub::GetDescriptor());
}

void WriteTextMessageParcel(MessageParcel &parcel, const std::u16string &text, int32_t slotId)
{
    WriteDescriptor(parcel);
    parcel.WriteInt32(slotId);
    parcel.WriteString16(text);
    parcel.WriteString16(text);
    parcel.WriteString16(text);
    sptr<ISendShortMessageCallback> sendCallback = new SendShortMessageCallbackStub();
    sptr<IDeliveryShortMessageCallback> deliveryCallback = new DeliveryShortMessageCallbackStub();
    parcel.WriteRemoteObject(sendCallback->AsObject().GetRefPtr());
    parcel.WriteRemoteObject(deliveryCallback->AsObject().GetRefPtr());
    parcel.WriteString("com.ohos.mms");
}

void WriteDataMessageParcel(MessageParcel &parcel, const uint8_t *data, size_t size, const std::u16string &text,
    int32_t slotId)
{
    WriteDescriptor(parcel);
    parcel.WriteInt32(slotId);
    parcel.WriteString16(text);
    parcel.WriteString16(text);
    parcel.WriteInt16(static_cast<int16_t>(MakePort(data, size)));
    sptr<ISendShortMessageCallback> sendCallback = new SendShortMessageCallbackStub();
    sptr<IDeliveryShortMessageCallback> deliveryCallback = new DeliveryShortMessageCallbackStub();
    parcel.WriteRemoteObject(sendCallback->AsObject().GetRefPtr());
    parcel.WriteRemoteObject(deliveryCallback->AsObject().GetRefPtr());
    std::vector<uint8_t> dataBuffer = MakeDataBuffer(data, size);
    parcel.WriteInt16(static_cast<int16_t>(dataBuffer.size()));
    parcel.WriteRawData(dataBuffer.data(), dataBuffer.size());
}

void WriteSimMessageParcel(MessageParcel &parcel, const std::u16string &text, uint32_t status)
{
    WriteDescriptor(parcel);
    parcel.WriteInt32(VALID_SLOT_ID);
    parcel.WriteString16(text);
    parcel.WriteString16(text);
    parcel.WriteUint32(status);
}

void WriteSetCbConfigListParcel(MessageParcel &parcel, const uint8_t *data, size_t size, int32_t listSize)
{
    if (size == 0) {
        return;
    }
    WriteDescriptor(parcel);
    parcel.WriteInt32(VALID_SLOT_ID);
    parcel.WriteInt32(listSize);
    for (int32_t index = 0; index < listSize && index < CB_LIST_SAMPLE_SIZE; ++index) {
        parcel.WriteInt32(static_cast<int32_t>(data[index % size]));
    }
    parcel.WriteInt32(MakeInt32(data, size));
}

void WriteUpdateSimOrSetCbParcel(MessageParcel &parcel, const uint8_t *data, size_t size,
    const std::u16string &text, SmsServiceInterfaceCode code)
{
    switch (code) {
        case SmsServiceInterfaceCode::UPDATE_SIM_MESSAGE:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteUint32(MakeUint32(data, size));
            parcel.WriteUint32(MakeUint32(data, size) % SIM_STATUS_MOD);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            break;
        case SmsServiceInterfaceCode::SET_CB_CONFIG:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteBool((size % ENABLE_MOD) == 0);
            parcel.WriteUint32(MakeUint32(data, size));
            parcel.WriteUint32(MakeUint32(data, size) + static_cast<uint32_t>(size));
            parcel.WriteUint8(static_cast<uint8_t>(size));
            break;
        default:
            break;
    }
}

void WriteCommonParcel(MessageParcel &parcel, const uint8_t *data, size_t size, const std::u16string &text,
    SmsServiceInterfaceCode code)
{
    WriteDescriptor(parcel);
    switch (code) {
        case SmsServiceInterfaceCode::SET_SMSC_ADDRESS:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteString16(text);
            break;
        case SmsServiceInterfaceCode::GET_SMSC_ADDRESS:
        case SmsServiceInterfaceCode::DEL_SIM_MESSAGE:
        case SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE:
        case SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED:
            parcel.WriteInt32(VALID_SLOT_ID);
            if (code == SmsServiceInterfaceCode::DEL_SIM_MESSAGE) {
                parcel.WriteUint32(MakeUint32(data, size));
            }
            break;
        case SmsServiceInterfaceCode::UPDATE_SIM_MESSAGE:
        case SmsServiceInterfaceCode::SET_CB_CONFIG:
            WriteUpdateSimOrSetCbParcel(parcel, data, size, text, code);
            break;
        case SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG:
        case SmsServiceInterfaceCode::SET_DEFAULT_SMS_SLOT_ID:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteInt32(MakeInt32(data, size) % ENABLE_MOD);
            break;
        case SmsServiceInterfaceCode::SPLIT_MESSAGE:
            parcel.WriteString16(text);
            break;
        case SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteString16(text);
            parcel.WriteBool((size % ENABLE_MOD) == 0);
            break;
        case SmsServiceInterfaceCode::GET_SMS_SHORT_CODE_TYPE:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteString(MakeString(data, size));
            break;
        default:
            break;
    }
}

void WriteCodecParcel(MessageParcel &parcel, const uint8_t *data, size_t size, const std::u16string &text,
    SmsServiceInterfaceCode code)
{
    WriteDescriptor(parcel);
    std::string src = MakeString(data, size);
    switch (code) {
        case SmsServiceInterfaceCode::CREATE_MESSAGE:
            parcel.WriteString(src);
            parcel.WriteString(SPECIFICATION_3GPP);
            break;
        case SmsServiceInterfaceCode::MMS_BASE64_ENCODE:
        case SmsServiceInterfaceCode::MMS_BASE64_DECODE:
            parcel.WriteString16(text);
            break;
        case SmsServiceInterfaceCode::GET_ENCODE_STRING:
            parcel.WriteUint32(MakeUint32(data, size));
            parcel.WriteUint32(static_cast<uint32_t>(src.length()));
            parcel.WriteString16(text);
            break;
        case SmsServiceInterfaceCode::SEND_MMS:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            parcel.WriteInt64(MakeInt64(data, size));
            parcel.WriteString("com.ohos.mms");
            break;
        case SmsServiceInterfaceCode::DOWNLOAD_MMS:
            parcel.WriteInt32(VALID_SLOT_ID);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            parcel.WriteString16(text);
            break;
        default:
            break;
    }
}

void CallOnRemoteRequest(SmsInterfaceStub &stub, uint32_t code, MessageParcel &parcel)
{
    MessageParcel reply;
    MessageOption option;
    stub.OnRemoteRequest(code, parcel, reply, option);
}

void FuzzDescriptorAndUnknownCode(SmsInterfaceStub &stub, const uint8_t *data, size_t size)
{
    MessageParcel badDescriptorParcel;
    badDescriptorParcel.WriteInterfaceToken(u"bad.descriptor");
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::HAS_SMS_CAPABILITY), badDescriptorParcel);

    MessageParcel unknownCodeParcel;
    WriteDescriptor(unknownCodeParcel);
    uint32_t code = static_cast<uint32_t>(SmsServiceInterfaceCode::HAS_SMS_CAPABILITY) + UNKNOWN_CODE_OFFSET;
    CallOnRemoteRequest(stub, code + MakeUint32(data, size), unknownCodeParcel);
}

void FuzzMessageDispatch(SmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
{
    MessageParcel textParcel;
    WriteTextMessageParcel(textParcel, text, VALID_SLOT_ID);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::TEXT_BASED_SMS_DELIVERY), textParcel);

    MessageParcel invalidTextParcel;
    WriteTextMessageParcel(invalidTextParcel, text, INVALID_SLOT_ID);
    CallOnRemoteRequest(
        stub, static_cast<uint32_t>(SmsServiceInterfaceCode::SEND_SMS_TEXT_WITHOUT_SAVE), invalidTextParcel);

    MessageParcel dataParcel;
    WriteDataMessageParcel(dataParcel, data, size, text, VALID_SLOT_ID);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::DATA_BASED_SMS_DELIVERY), dataParcel);
}

void FuzzSimDispatch(SmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
{
    MessageParcel addParcel;
    WriteSimMessageParcel(addParcel, text, MakeUint32(data, size) % SIM_STATUS_MOD);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::ADD_SIM_MESSAGE), addParcel);

    MessageParcel invalidAddParcel;
    WriteSimMessageParcel(invalidAddParcel, text, INVALID_SIM_STATUS);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::ADD_SIM_MESSAGE), invalidAddParcel);

    MessageParcel cbListParcel;
    WriteSetCbConfigListParcel(cbListParcel, data, size, CB_LIST_SAMPLE_SIZE);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::SET_CB_CONFIG_LIST), cbListParcel);

    MessageParcel longCbListParcel;
    WriteSetCbConfigListParcel(longCbListParcel, data, size, MAX_CB_LIST_SIZE + 1);
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::SET_CB_CONFIG_LIST), longCbListParcel);
}

void FuzzCommonDispatch(SmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
{
    std::vector<SmsServiceInterfaceCode> codes = {
        SmsServiceInterfaceCode::SET_SMSC_ADDRESS,
        SmsServiceInterfaceCode::GET_SMSC_ADDRESS,
        SmsServiceInterfaceCode::DEL_SIM_MESSAGE,
        SmsServiceInterfaceCode::UPDATE_SIM_MESSAGE,
        SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE,
        SmsServiceInterfaceCode::SET_CB_CONFIG,
        SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG,
        SmsServiceInterfaceCode::SET_DEFAULT_SMS_SLOT_ID,
        SmsServiceInterfaceCode::GET_DEFAULT_SMS_SLOT_ID,
        SmsServiceInterfaceCode::GET_DEFAULT_SMS_SIM_ID,
        SmsServiceInterfaceCode::SPLIT_MESSAGE,
        SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO,
        SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED,
        SmsServiceInterfaceCode::GET_IMS_SHORT_MESSAGE_FORMAT,
        SmsServiceInterfaceCode::GET_SMS_SHORT_CODE_TYPE,
        SmsServiceInterfaceCode::HAS_SMS_CAPABILITY,
    };
    for (SmsServiceInterfaceCode code : codes) {
        MessageParcel parcel;
        WriteCommonParcel(parcel, data, size, text, code);
        CallOnRemoteRequest(stub, static_cast<uint32_t>(code), parcel);
    }
}

void FuzzCodecDispatch(SmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
{
    std::vector<SmsServiceInterfaceCode> codes = {
        SmsServiceInterfaceCode::CREATE_MESSAGE,
        SmsServiceInterfaceCode::MMS_BASE64_ENCODE,
        SmsServiceInterfaceCode::MMS_BASE64_DECODE,
        SmsServiceInterfaceCode::GET_ENCODE_STRING,
        SmsServiceInterfaceCode::SEND_MMS,
        SmsServiceInterfaceCode::DOWNLOAD_MMS,
    };
    for (SmsServiceInterfaceCode code : codes) {
        MessageParcel parcel;
        WriteCodecParcel(parcel, data, size, text, code);
        CallOnRemoteRequest(stub, static_cast<uint32_t>(code), parcel);
    }

    MessageParcel mismatchEncodeParcel;
    WriteDescriptor(mismatchEncodeParcel);
    mismatchEncodeParcel.WriteUint32(MakeUint32(data, size));
    mismatchEncodeParcel.WriteUint32(static_cast<uint32_t>(MakeString(data, size).length() + 1));
    mismatchEncodeParcel.WriteString16(text);
    CallOnRemoteRequest(
        stub, static_cast<uint32_t>(SmsServiceInterfaceCode::GET_ENCODE_STRING), mismatchEncodeParcel);
}
} // namespace

void DoSmsInterfaceStubFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    auto service = DelayedSingleton<SmsService>::GetInstance();
    if (service == nullptr) {
        return;
    }
    SmsInterfaceStub &stub = *service;
    std::u16string text = MakeString16(data, size);
    FuzzDescriptorAndUnknownCode(stub, data, size);
    FuzzMessageDispatch(stub, data, size, text);
    FuzzSimDispatch(stub, data, size, text);
    FuzzCommonDispatch(stub, data, size, text);
    FuzzCodecDispatch(stub, data, size, text);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsInterfaceStubFuzzTest(data, size);
    return 0;
}
