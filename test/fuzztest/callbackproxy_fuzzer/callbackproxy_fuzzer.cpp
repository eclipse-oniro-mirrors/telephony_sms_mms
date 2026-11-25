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

#include "ims_sms_callback_proxy.h"
#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "sms_delivery_short_message_proxy.h"
#include "sms_fuzzer_reply_remote.h"
#include "sms_fuzzer_utils.h"
#include "sms_send_short_message_proxy.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace SmsFuzz;

namespace {
constexpr uint32_t MAX_SEND_RESULT = 4;
constexpr uint32_t MAX_IMS_CONFIG = 2;
constexpr uint32_t GROUP_SMS_CALLBACK = 0;
constexpr uint32_t GROUP_IMS_CALLBACK = 1;
constexpr uint32_t GROUP_COUNT = GROUP_IMS_CALLBACK + 1;

void FillSendSmsResultInfo(SendSmsResultInfo &result, FuzzedDataProvider &fdp)
{
    result.msgRef = fdp.ConsumeIntegral<int32_t>();
    result.pdu = ConsumeString(fdp);
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
        fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_SEND_RESULT));
    sendProxy.OnSmsSendResult(result);
    deliveryProxy.OnSmsDeliveryResult(ConsumeText(fdp));
}

void FuzzImsCallbackProxy(const sptr<IRemoteObject> &remote, FuzzedDataProvider &fdp)
{
    ImsSmsCallbackProxy imsProxy(remote);
    int32_t slotId = ConsumeSlotId(fdp);
    SendSmsResultInfo smsResult;
    RadioResponseInfo radioInfo;
    FillSendSmsResultInfo(smsResult, fdp);
    FillRadioResponseInfo(radioInfo, fdp);

    imsProxy.ImsSendMessageResponse(slotId, smsResult);
    imsProxy.ImsSendMessageResponse(slotId, radioInfo);
    imsProxy.ImsSetSmsConfigResponse(slotId, radioInfo);
    imsProxy.ImsGetSmsConfigResponse(slotId,
        static_cast<int32_t>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_IMS_CONFIG)));
    imsProxy.ImsGetSmsConfigResponse(slotId, radioInfo);
}
} // namespace

void DoCallbackProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    sptr<IRemoteObject> remote = new SmsServiceReplyRemote(fdp);
    if (fdp.ConsumeIntegralInRange<uint32_t>(GROUP_SMS_CALLBACK, GROUP_COUNT - 1) == GROUP_SMS_CALLBACK) {
        FuzzSmsCallbackProxy(remote, fdp);
        return;
    }
    FuzzImsCallbackProxy(remote, fdp);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DoCallbackProxyFuzzTest(data, size);
    return 0;
}
