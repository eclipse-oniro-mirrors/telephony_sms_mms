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

#include "sendmessagedata_fuzzer.h"

#define private public
#include "addsmstoken_fuzzer.h"
#include "core_manager_inner.h"
#include "delivery_short_message_callback_stub.h"
#include "send_short_message_callback_stub.h"
#include "sms_service.h"

using namespace OHOS::Telephony;
namespace OHOS {
static bool g_isInited = false;
constexpr int32_t SLOT_NUM = 2;
static int32_t MAX_PORT = 65535;
constexpr int32_t SLEEP_TIME_SECONDS = 2;

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

void SendSmsDataRequest(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    auto desAddrU16 = Str8ToStr16("123456");
    auto scAddrU16 = Str8ToStr16("123456");
    uint16_t port = static_cast<uint16_t>(size % MAX_PORT);

    std::unique_ptr<SendShortMessageCallbackStub> sendCallback = std::make_unique<SendShortMessageCallbackStub>();
    std::unique_ptr<DeliveryShortMessageCallbackStub> deliveryCallback =
        std::make_unique<DeliveryShortMessageCallbackStub>();

    dataParcel.WriteInt32(slotId);
    dataParcel.WriteString16(desAddrU16);
    dataParcel.WriteString16(scAddrU16);
    dataParcel.WriteInt16(port);
    if (sendCallback != nullptr) {
        dataParcel.WriteRemoteObject(sendCallback.release()->AsObject().GetRefPtr());
    }
    if (deliveryCallback != nullptr) {
        dataParcel.WriteRemoteObject(deliveryCallback.release()->AsObject().GetRefPtr());
    }
    dataParcel.WriteInt16(size);
    dataParcel.WriteRawData(data, size);
    dataParcel.RewindRead(0);

    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsDataRequest(dataParcel, replyParcel, option);
}

void GetAllSimMessages(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnGetAllSimMessages(dataParcel, replyParcel, option);
    return;
}

void DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    SendSmsDataRequest(data, size);
    GetAllSimMessages(data, size);
    DelayedSingleton<ImsSmsClient>::GetInstance()->UnInit();
    DelayedSingleton<ImsSmsClient>::DestroyInstance();
    sleep(SLEEP_TIME_SECONDS);
    DelayedSingleton<SmsService>::DestroyInstance();
}
}  // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
