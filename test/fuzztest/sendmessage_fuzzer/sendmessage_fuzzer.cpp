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

#include "sendmessage_fuzzer.h"

#define private public

#include <cstdlib>
#include <unistd.h>
#include "addsmstoken_fuzzer.h"
#include "core_manager_inner.h"
#include "delivery_short_message_callback_stub.h"
#include "send_short_message_callback_stub.h"
#include "sms_service.h"

using namespace OHOS::Telephony;
namespace OHOS {
static bool g_isInited = false;
constexpr int32_t SLOT_NUM = 2;
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

void SendSmsTextRequest(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    std::string text(reinterpret_cast<const char *>(data), size);
    auto desAddrU16 = Str8ToStr16("123456");
    auto scAddrU16 = Str8ToStr16("123456");
    auto textU16 = Str8ToStr16(text);

    std::unique_ptr<SendShortMessageCallbackStub> sendCallback = std::make_unique<SendShortMessageCallbackStub>();
    std::unique_ptr<DeliveryShortMessageCallbackStub> deliveryCallback =
        std::make_unique<DeliveryShortMessageCallbackStub>();

    dataParcel.WriteInt32(slotId);
    dataParcel.WriteString16(desAddrU16);
    dataParcel.WriteString16(scAddrU16);
    dataParcel.WriteString16(textU16);
    if (sendCallback != nullptr) {
        dataParcel.WriteRemoteObject(sendCallback.release()->AsObject().GetRefPtr());
    }
    if (deliveryCallback != nullptr) {
        dataParcel.WriteRemoteObject(deliveryCallback.release()->AsObject().GetRefPtr());
    }
    dataParcel.RewindRead(0);

    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsTextRequest(dataParcel, replyParcel, option);
}

void GetDefaultSmsSlotId(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnGetDefaultSmsSlotId(dataParcel, replyParcel, option);
    return;
}

void SmsServiceInterfaceTest(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }
    int32_t id = 0;
    int32_t fd = static_cast<int32_t>(data[0]);
    auto service = DelayedSingleton<SmsService>::GetInstance();
    std::vector<std::u16string> args;
    service->Dump(-1, args);
    service->Dump(fd, args);
    std::string argsStr(reinterpret_cast<const char *>(data), size);
    std::u16string argsStrU16 = StringUtils::ToUtf16(argsStr);
    args.push_back(argsStrU16);
    service->Dump(fd, args);
    service->GetBindTime();

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    service->InsertSessionAndDetail(slotId, argsStr, argsStr, id);
    uint16_t num = static_cast<uint16_t>(size);
    service->QuerySessionByTelephone(argsStr, num, num);
    service->InsertSmsMmsInfo(slotId, num, argsStr, argsStr, id);
    bool value = slotId == 0 ? true : false;
    service->InsertSession(value, num, argsStr, argsStr);
    service->IsImsSmsSupported(slotId, value);
    service->GetImsShortMessageFormat(argsStrU16);
    service->HasSmsCapability();
    service->SetSmscAddr(slotId, argsStrU16);
    service->TrimSmscAddr(argsStr);
    service->GetSmscAddr(slotId, argsStrU16);
    uint32_t index = static_cast<uint32_t>(size);
    service->CheckSimMessageIndexValid(slotId, index);
    service->SetImsSmsConfig(slotId, value);
    service->SetDefaultSmsSlotId(slotId);
    service->GetDefaultSmsSlotId();
    int32_t simId;
    service->GetDefaultSmsSimId(simId);
    service->GetServiceRunningState();
    service->GetEndTime();
    service->ValidDestinationAddress(argsStr);
    service->GetSpendTime();
    service->GetBase64Encode(argsStr, argsStr);
    service->GetBase64Decode(argsStr, argsStr);
    service->GetEncodeStringFunc(argsStr, index, argsStr.size(), argsStr);
    int64_t timeStamp = 0;
    service->SendMms(slotId, argsStrU16, argsStrU16, argsStrU16, argsStrU16, timeStamp);
    service->DownloadMms(slotId, argsStrU16, argsStrU16, argsStrU16, argsStrU16);
}

void DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    SendSmsTextRequest(data, size);
    GetDefaultSmsSlotId(data, size);
    SmsServiceInterfaceTest(data, size);
    DelayedSingleton<ImsSmsClient>::GetInstance()->UnInit();
    DelayedSingleton<ImsSmsClient>::DestroyInstance();
    sleep(SLEEP_TIME_SECONDS);
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
