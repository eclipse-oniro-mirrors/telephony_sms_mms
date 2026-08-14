/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "addsimmessage_fuzzer.h"

#define private public
#include "addsmstoken_fuzzer.h"
#include "core_manager_inner.h"
#include "i_sms_service_interface.h"
#include "sms_service.h"
#include "cdma_sms_message.h"

using namespace OHOS::Telephony;
namespace OHOS {
static bool g_isInited = false;
constexpr int32_t SLOT_NUM = 2;
static int32_t STATUS_COUNT = 4;

bool IsServiceInited()
{
    if (!g_isInited) {
        CoreManagerInner::GetInstance().isInitAllObj_ = true;
        DelayedSingleton<SmsService>::GetInstance()->registerToService_ = true;
        DelayedSingleton<SmsService>::GetInstance()->WaitCoreServiceToInit();
        DelayedSingleton<SmsService>::GetInstance()->OnStart();
        if (DelayedSingleton<SmsService>::GetInstance()->GetServiceRunningState() ==
            static_cast<int32_t>(Telephony::ServiceRunningState::STATE_RUNNING)) {
            g_isInited = true;
            atexit([] {
                DelayedSingleton<SmsService>::GetInstance()->OnStop();
                _exit(0);
            });
        }
    }
    return g_isInited;
}

void OnRemoteRequest(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    if (!dataParcel.WriteInterfaceToken(SmsInterfaceStub::GetDescriptor())) {
        TELEPHONY_LOGE("OnRemoteRequest WriteInterfaceToken is false");
        return;
    }

    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    uint32_t code = static_cast<uint32_t>(size);

    DelayedSingleton<SmsService>::GetInstance()->OnRemoteRequest(code, dataParcel, replyParcel, option);
    return;
}

void GetAllSimMessages(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    dataParcel.WriteInt32(slotId);
    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnGetAllSimMessages(dataParcel, replyParcel, option);

    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr");
        return;
    }

    interfaceManager->InitInterfaceManager();
    std::vector<ShortMessage> message;
    interfaceManager->GetAllSimMessages(message);

    std::shared_ptr<SmsMiscManager> smsMiscManager = std::make_shared<SmsMiscManager>(slotId);
    if (smsMiscManager == nullptr) {
        TELEPHONY_LOGE("smsMiscManager nullptr");
        return;
    }
    smsMiscManager->GetAllSimMessages(message);
}

void AddSimMessage(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    std::string smsc(reinterpret_cast<const char *>(data), size);
    std::string pdu(reinterpret_cast<const char *>(data), size);
    auto smscU16 = Str8ToStr16(smsc);
    auto pduU16 = Str8ToStr16(pdu);
    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    auto status = static_cast<ISmsServiceInterface::SimMessageStatus>(size % STATUS_COUNT);

    dataParcel.WriteInt32(slotId);
    dataParcel.WriteString16(smscU16);
    dataParcel.WriteString16(pduU16);
    dataParcel.WriteUint32(status);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnAddSimMessage(dataParcel, replyParcel, option);

    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr");
        return;
    }
    interfaceManager->InitInterfaceManager();
    interfaceManager->AddSimMessage(smsc, pdu, status);

    std::shared_ptr<SmsMiscManager> smsMiscManager = std::make_shared<SmsMiscManager>(slotId);
    if (smsMiscManager == nullptr) {
        TELEPHONY_LOGE("smsMiscManager nullptr");
        return;
    }
    smsMiscManager->AddSimMessage(smsc, pdu, status);
}

void DelSimMessage(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }
    CdmaSmsMessage msg;
    std::string pdu(reinterpret_cast<const char *>(data), size);
    msg.CreateMessage(pdu);
    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    uint32_t index = static_cast<uint32_t>(size);
    dataParcel.WriteInt32(slotId);
    dataParcel.WriteUint32(index);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnDelSimMessage(dataParcel, replyParcel, option);

    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr");
        return;
    }
    interfaceManager->InitInterfaceManager();
    interfaceManager->DelSimMessage(index);

    std::shared_ptr<SmsMiscManager> smsMiscManager = std::make_shared<SmsMiscManager>(slotId);
    if (smsMiscManager == nullptr) {
        TELEPHONY_LOGE("smsMiscManager nullptr");
        return;
    }
    smsMiscManager->DelSimMessage(index);
}

void HasSmsCapability(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnHasSmsCapability(dataParcel, replyParcel, option);

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr error");
        return;
    }
    interfaceManager->InitInterfaceManager();
    interfaceManager->HasSmsCapability();
}

void DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    OnRemoteRequest(data, size);
    GetAllSimMessages(data, size);
    AddSimMessage(data, size);
    DelSimMessage(data, size);
    HasSmsCapability(data, size);
    DelayedSingleton<SmsService>::DestroyInstance();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
