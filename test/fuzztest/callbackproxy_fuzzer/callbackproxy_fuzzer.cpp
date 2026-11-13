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

#include "callbackproxy_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "ims_sms_callback_proxy.h"
#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "sms_delivery_short_message_proxy.h"
#include "sms_send_short_message_proxy.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr uint8_t ASCII_PRINTABLE_BASE = 0x20;
constexpr uint8_t ASCII_PRINTABLE_RANGE = 0x5F;
constexpr int32_t MAX_SLOT_ID = 2;
constexpr int32_t INVALID_SLOT_ID = -1;
constexpr uint32_t SMS_SEND_RESULT_MOD = 4;
constexpr uint32_t IMS_CONFIG_MOD = 2;
constexpr uint32_t GROUP_COUNT = 2;

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

class CallbackRemote final : public IRemoteObject {
public:
    CallbackRemote(FuzzedDataProvider &fdp) : IRemoteObject(u""), fdp_(fdp) {}

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
        reply.WriteInt32(fdp_.ConsumeIntegral<int32_t>());
        return fdp_.ConsumeIntegral<int32_t>();
    }

private:
    FuzzedDataProvider &fdp_;
};

void FillSendSmsResultInfo(SendSmsResultInfo &result, FuzzedDataProvider &fdp)
{
    result.msgRef = fdp.ConsumeIntegral<int32_t>();
    result.pdu = MakeString(fdp);
    result.errCode = fdp.ConsumeIntegral<int32_t>();
    result.flag = fdp.ConsumeIntegral<int64_t>();
}

void FillRadioResponseInfo(RadioResponseInfo &info, FuzzedDataProvider &fdp)
{
    info.flag = fdp.ConsumeIntegral<int32_t>();
    info.serial = fdp.ConsumeIntegral<int32_t>();
    info.error = static_cast<ErrType>(fdp.ConsumeIntegral<int32_t>());
    info.type = static_cast<ResponseTypes>(fdp.ConsumeIntegral<int32_t>());
}

void FuzzSmsCallbackProxy(const sptr<IRemoteObject> &remote, FuzzedDataProvider &fdp)
{
    SmsSendShortMessageProxy sendProxy(remote);
    SmsDeliveryShortMessageProxy deliveryProxy(remote);
    auto result = static_cast<ISendShortMessageCallback::SmsSendResult>(
        fdp.ConsumeIntegralInRange<uint32_t>(0, SMS_SEND_RESULT_MOD));
    sendProxy.OnSmsSendResult(result);
    deliveryProxy.OnSmsDeliveryResult(MakeText(fdp));
}

void FuzzImsCallbackProxy(const sptr<IRemoteObject> &remote, FuzzedDataProvider &fdp)
{
    ImsSmsCallbackProxy imsProxy(remote);
    int32_t slotId = fdp.ConsumeIntegralInRange<int32_t>(INVALID_SLOT_ID, MAX_SLOT_ID);
    SendSmsResultInfo smsResult;
    RadioResponseInfo radioInfo;
    FillSendSmsResultInfo(smsResult, fdp);
    FillRadioResponseInfo(radioInfo, fdp);

    imsProxy.ImsSendMessageResponse(slotId, smsResult);
    imsProxy.ImsSendMessageResponse(slotId, radioInfo);
    imsProxy.ImsSetSmsConfigResponse(slotId, radioInfo);
    imsProxy.ImsGetSmsConfigResponse(slotId,
        static_cast<int32_t>(fdp.ConsumeIntegralInRange<uint32_t>(0, IMS_CONFIG_MOD)));
    imsProxy.ImsGetSmsConfigResponse(slotId, radioInfo);
}
} // namespace

void DoCallbackProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remote = new CallbackRemote(fdp);
    if (fdp.ConsumeIntegralInRange<uint32_t>(0, GROUP_COUNT - 1) == 0) {
        FuzzSmsCallbackProxy(remote, fdp);
        return;
    }
    FuzzImsCallbackProxy(remote, fdp);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoCallbackProxyFuzzTest(data, size);
    return 0;
}
