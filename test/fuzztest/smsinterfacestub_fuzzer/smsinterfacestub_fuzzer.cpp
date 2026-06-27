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
#include "sms_mms_errors.h"
#include "securec.h"
#include "sms_service_ipc_interface_code.h"
#include "string_utils.h"
#include "telephony_errors.h"

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
constexpr char ENCODED_DEST[] = "encoded";
constexpr char SMSC_ADDRESS[] = "+8613800000000";
constexpr char IMS_FORMAT[] = "3gpp";

class FakeSmsInterfaceStub : public SmsInterfaceStub {
public:
    int32_t SendMessage(int32_t slotId, const std::u16string desAddr, const std::u16string scAddr,
        const std::u16string text, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliverCallback, bool isMmsApp = true) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SendMessageWithoutSave(int32_t slotId, const std::u16string desAddr, const std::u16string scAddr,
        const std::u16string text, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliverCallback) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SendMessage(int32_t slotId, const std::u16string desAddr, const std::u16string scAddr, uint16_t port,
        const uint8_t *data, uint16_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliverCallback) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SetSmscAddr(int32_t slotId, const std::u16string &scAddr) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetSmscAddr(int32_t slotId, std::u16string &smscAddress) override
    {
        smscAddress = StringUtils::ToUtf16(SMSC_ADDRESS);
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t AddSimMessage(
        int32_t slotId, const std::u16string &smsc, const std::u16string &pdu, SimMessageStatus status) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t DelSimMessage(int32_t slotId, uint32_t msgIndex) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t UpdateSimMessage(int32_t slotId, uint32_t msgIndex, SimMessageStatus newStatus,
        const std::u16string &pdu, const std::u16string &smsc) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetAllSimMessages(int32_t slotId, std::vector<ShortMessage> &message) override
    {
        message.emplace_back();
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SetCBConfig(int32_t slotId, bool enable, uint32_t fromMsgId, uint32_t toMsgId, uint8_t netType) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SetCBConfigList(int32_t slotId, const std::vector<int32_t> &messageIds, int32_t ranType) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    bool SetImsSmsConfig(int32_t slotId, int32_t enable) override
    {
        return (enable % ENABLE_MOD) == 0;
    }

    int32_t SetDefaultSmsSlotId(int32_t slotId) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetDefaultSmsSlotId() override
    {
        return VALID_SLOT_ID;
    }

    int32_t GetDefaultSmsSimId(int32_t &simId) override
    {
        simId = VALID_SLOT_ID;
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t SplitMessage(const std::u16string &message, std::vector<std::u16string> &splitMessage) override
    {
        splitMessage.emplace_back(message);
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetSmsSegmentsInfo(int32_t slotId, const std::u16string &message, bool force7BitCode,
        SmsSegmentsInfo &info) override
    {
        info.msgSegCount = 1;
        info.msgEncodingCount = static_cast<int32_t>(message.size());
        info.msgRemainCount = 0;
        info.msgCodeScheme = SmsSegmentsInfo::SmsSegmentCodeScheme::SMS_ENCODING_7BIT;
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t IsImsSmsSupported(int32_t slotId, bool &isSupported) override
    {
        isSupported = true;
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetImsShortMessageFormat(std::u16string &format) override
    {
        format = StringUtils::ToUtf16(IMS_FORMAT);
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t GetSmsShortCodeType(int32_t slotId, const std::string &desAddr, int32_t &smsShortCodeType) override
    {
        smsShortCodeType = 0;
        return TELEPHONY_ERR_SUCCESS;
    }

    bool HasSmsCapability() override
    {
        return true;
    }

    int32_t CreateMessage(std::string pdu, std::string specification, ShortMessage &message) override
    {
        return specification.empty() ? TELEPHONY_ERR_ARGUMENT_INVALID : TELEPHONY_ERR_SUCCESS;
    }

    bool GetBase64Encode(std::string src, std::string &dest) override
    {
        dest = ENCODED_DEST;
        return !src.empty();
    }

    bool GetBase64Decode(std::string src, std::string &dest) override
    {
        dest = src;
        return !src.empty();
    }

    bool GetEncodeStringFunc(
        std::string &encodeString, uint32_t charset, uint32_t valLength, std::string strEncodeString) override
    {
        encodeString = strEncodeString;
        return valLength == strEncodeString.length();
    }

    int32_t SendMms(int32_t slotId, const std::u16string &mmsc, const std::u16string &data,
        const std::u16string &ua, const std::u16string &uaprof, int64_t &time, bool isMmsApp = false) override
    {
        return TELEPHONY_ERR_SUCCESS;
    }

    int32_t DownloadMms(int32_t slotId, const std::u16string &mmsc, std::u16string &data,
        const std::u16string &ua, const std::u16string &uaprof) override
    {
        data = mmsc;
        return TELEPHONY_ERR_SUCCESS;
    }
};

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

void CallOnRemoteRequest(FakeSmsInterfaceStub &stub, uint32_t code, MessageParcel &parcel)
{
    MessageParcel reply;
    MessageOption option;
    stub.OnRemoteRequest(code, parcel, reply, option);
}

void FuzzDescriptorAndUnknownCode(FakeSmsInterfaceStub &stub, const uint8_t *data, size_t size)
{
    MessageParcel badDescriptorParcel;
    badDescriptorParcel.WriteInterfaceToken(u"bad.descriptor");
    CallOnRemoteRequest(stub, static_cast<uint32_t>(SmsServiceInterfaceCode::HAS_SMS_CAPABILITY), badDescriptorParcel);

    MessageParcel unknownCodeParcel;
    WriteDescriptor(unknownCodeParcel);
    uint32_t code = static_cast<uint32_t>(SmsServiceInterfaceCode::HAS_SMS_CAPABILITY) + UNKNOWN_CODE_OFFSET;
    CallOnRemoteRequest(stub, code + MakeUint32(data, size), unknownCodeParcel);
}

void FuzzMessageDispatch(FakeSmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
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

void FuzzSimDispatch(FakeSmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
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

void FuzzCommonDispatch(FakeSmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
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

void FuzzCodecDispatch(FakeSmsInterfaceStub &stub, const uint8_t *data, size_t size, const std::u16string &text)
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

    FakeSmsInterfaceStub stub;
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
