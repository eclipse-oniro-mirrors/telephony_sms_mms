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

#include "smsinterfacestub_fuzzer.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fuzzer/FuzzedDataProvider.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "sms_fuzzer_utils.h"

#define private public

#include "addsmstoken_fuzzer.h"
#include "core_manager_inner.h"
#include "delivery_short_message_callback_stub.h"
#include "i_sms_service_interface.h"
#include "message_option.h"
#include "message_parcel.h"
#include "send_short_message_callback_stub.h"
#include "sms_interface_stub.h"
#include "sms_service.h"
#include "sms_service_ipc_interface_code.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace SmsFuzz;

namespace {
constexpr int32_t MAX_CB_LIST_SIZE = 16;
constexpr uint32_t UNKNOWN_CODE_OFFSET = 1000;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(SmsServiceInterfaceCode::GET_SMS_SHORT_CODE_TYPE) + 1;
constexpr uint8_t SPECIFICATION_SELECTOR_RANGE = 2;
constexpr int32_t INIT_WAIT_TIMES = 200;
constexpr auto INIT_WAIT_INTERVAL = std::chrono::milliseconds(1);

static bool g_isInited = false;
static bool g_initTried = false;

bool WaitInterfaceManagerReady(SmsService &service)
{
    for (int32_t attempt = 0; attempt < INIT_WAIT_TIMES; ++attempt) {
        if (service.GetSmsInterfaceManager(0) != nullptr) {
            return true;
        }
        std::this_thread::sleep_for(INIT_WAIT_INTERVAL);
    }
    return false;
}

// The whole process initialises the service at most once: a failed attempt must not be retried on
// every input, otherwise each iteration would re-run OnStart() and wait up to 200 ms again.
bool IsServiceInited()
{
    if (g_isInited) {
        return true;
    }
    if (g_initTried) {
        return false;
    }
    g_initTried = true;

    CoreManagerInner::GetInstance().isInitAllObj_ = true;
    auto service = DelayedSingleton<SmsService>::GetInstance();
    if (service == nullptr) {
        return false;
    }
    service->registerToService_ = true;
    service->WaitCoreServiceToInit();
    service->OnStart();
    if (service->GetServiceRunningState() != static_cast<int32_t>(ServiceRunningState::STATE_RUNNING) ||
        !WaitInterfaceManagerReady(*service)) {
        return false;
    }
    g_isInited = true;
    atexit([] {
        DelayedSingleton<SmsService>::GetInstance()->OnStop();
        _exit(0);
    });
    return true;
}

std::string MakeBundleName(FuzzedDataProvider &fdp)
{
    if (fdp.ConsumeBool()) {
        return std::string("com.ohos.mms");
    }
    return ConsumeString(fdp);
}

std::string MakeSpecification(FuzzedDataProvider &fdp)
{
    uint8_t type = fdp.ConsumeIntegralInRange<uint8_t>(0, SPECIFICATION_SELECTOR_RANGE);
    if (type == 0) {
        return std::string("3gpp");
    }
    if (type == 1) {
        return std::string("3gpp2");
    }
    return ConsumeString(fdp);
}

void WriteCallbacks(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    if (!fdp.ConsumeBool()) {
        return;
    }
    sptr<ISendShortMessageCallback> sendCallback = new SendShortMessageCallbackStub();
    parcel.WriteRemoteObject(sendCallback->AsObject().GetRefPtr());
    if (!fdp.ConsumeBool()) {
        return;
    }
    sptr<IDeliveryShortMessageCallback> deliveryCallback = new DeliveryShortMessageCallbackStub();
    parcel.WriteRemoteObject(deliveryCallback->AsObject().GetRefPtr());
}

void WriteTextBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    int32_t slotId = ConsumeSlotId(fdp);
    std::u16string desAddr = ConsumeText(fdp);
    std::u16string scAddr = ConsumeText(fdp);
    parcel.WriteInt32(slotId);
    parcel.WriteString16(desAddr);
    parcel.WriteString16(scAddr);
    if (code == SmsServiceInterfaceCode::DATA_BASED_SMS_DELIVERY) {
        parcel.WriteInt16(fdp.ConsumeIntegral<int16_t>());
        WriteCallbacks(parcel, fdp);
        int16_t dataLen = static_cast<int16_t>(fdp.ConsumeIntegralInRange<size_t>(0, MAX_DATA_SIZE));
        std::vector<uint8_t> payload = fdp.ConsumeBytes<uint8_t>(static_cast<size_t>(dataLen));
        parcel.WriteInt16(dataLen);
        if (!payload.empty()) {
            parcel.WriteRawData(payload.data(), payload.size());
        }
        return;
    }
    parcel.WriteString16(ConsumeText(fdp));
    WriteCallbacks(parcel, fdp);
    parcel.WriteString(MakeBundleName(fdp));
}

void WriteSimBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    parcel.WriteInt32(ConsumeSlotId(fdp));
    switch (code) {
        case SmsServiceInterfaceCode::SET_SMSC_ADDRESS:
            parcel.WriteString16(ConsumeText(fdp));
            break;
        case SmsServiceInterfaceCode::ADD_SIM_MESSAGE:
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteUint32(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS));
            break;
        case SmsServiceInterfaceCode::DEL_SIM_MESSAGE:
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            break;
        case SmsServiceInterfaceCode::UPDATE_SIM_MESSAGE:
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            parcel.WriteUint32(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            break;
        default:
            break;
    }
}

void WriteCbConfigList(MessageParcel &parcel, FuzzedDataProvider &fdp)
{
    int32_t listSize = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_CB_LIST_SIZE);
    parcel.WriteInt32(listSize);
    for (int32_t index = 0; index < listSize; ++index) {
        parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
    }
    parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
}

void WriteConfigBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case SmsServiceInterfaceCode::SET_CB_CONFIG:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteBool(fdp.ConsumeBool());
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            parcel.WriteUint8(fdp.ConsumeIntegral<uint8_t>());
            break;
        case SmsServiceInterfaceCode::SET_CB_CONFIG_LIST:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            WriteCbConfigList(parcel, fdp);
            break;
        case SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG:
        case SmsServiceInterfaceCode::SET_DEFAULT_SMS_SLOT_ID:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteInt32(fdp.ConsumeIntegral<int32_t>());
            break;
        default:
            break;
    }
}

void WriteQueryBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case SmsServiceInterfaceCode::SPLIT_MESSAGE:
            parcel.WriteString16(ConsumeText(fdp));
            break;
        case SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteBool(fdp.ConsumeBool());
            break;
        case SmsServiceInterfaceCode::GET_SMS_SHORT_CODE_TYPE:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteString(ConsumeString(fdp));
            break;
        case SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            break;
        default:
            break;
    }
}

void WriteMmsBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case SmsServiceInterfaceCode::SEND_MMS:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteInt64(fdp.ConsumeIntegral<int64_t>());
            parcel.WriteString(MakeBundleName(fdp));
            break;
        case SmsServiceInterfaceCode::DOWNLOAD_MMS:
            parcel.WriteInt32(ConsumeSlotId(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            parcel.WriteString16(ConsumeText(fdp));
            break;
        case SmsServiceInterfaceCode::CREATE_MESSAGE:
            parcel.WriteString(ConsumeString(fdp));
            parcel.WriteString(MakeSpecification(fdp));
            break;
        case SmsServiceInterfaceCode::MMS_BASE64_ENCODE:
        case SmsServiceInterfaceCode::MMS_BASE64_DECODE:
            parcel.WriteString16(ConsumeText(fdp));
            break;
        case SmsServiceInterfaceCode::GET_ENCODE_STRING:
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            parcel.WriteUint32(fdp.ConsumeIntegral<uint32_t>());
            parcel.WriteString16(ConsumeText(fdp));
            break;
        default:
            break;
    }
}

void WriteParcelBody(MessageParcel &parcel, SmsServiceInterfaceCode code, FuzzedDataProvider &fdp)
{
    if (code <= SmsServiceInterfaceCode::DATA_BASED_SMS_DELIVERY) {
        WriteTextBody(parcel, code, fdp);
        return;
    }
    if (code <= SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE) {
        WriteSimBody(parcel, code, fdp);
        return;
    }
    if (code <= SmsServiceInterfaceCode::GET_DEFAULT_SMS_SIM_ID) {
        WriteConfigBody(parcel, code, fdp);
        return;
    }
    if (code <= SmsServiceInterfaceCode::HAS_SMS_CAPABILITY) {
        WriteQueryBody(parcel, code, fdp);
        return;
    }
    WriteMmsBody(parcel, code, fdp);
}
} // namespace

void DoSmsInterfaceStubFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE || !IsServiceInited()) {
        return;
    }
    auto service = DelayedSingleton<SmsService>::GetInstance();
    if (service == nullptr) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    uint32_t code = fdp.ConsumeIntegralInRange<uint32_t>(0, CODE_MAX - 1);
    if (fdp.ConsumeBool()) {
        code += UNKNOWN_CODE_OFFSET;
    }

    MessageParcel parcel;
    if (fdp.ConsumeBool()) {
        parcel.WriteInterfaceToken(SmsInterfaceStub::GetDescriptor());
    } else {
        parcel.WriteInterfaceToken(u"bad.descriptor");
    }
    if (code < CODE_MAX) {
        WriteParcelBody(parcel, static_cast<SmsServiceInterfaceCode>(code), fdp);
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    service->OnRemoteRequest(code, parcel, reply, option);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSmsInterfaceStubFuzzTest(data, size);
    return 0;
}
