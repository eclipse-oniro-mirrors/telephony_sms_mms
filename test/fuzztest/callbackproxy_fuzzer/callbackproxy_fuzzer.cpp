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

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include "addsmstoken_fuzzer.h"
#include "ims_sms_callback_proxy.h"
#include "sms_delivery_short_message_proxy.h"
#include "sms_send_short_message_proxy.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr uint32_t SMS_SEND_RESULT_MOD = 4;
constexpr int32_t SLOT_COUNT = 2;
constexpr int32_t IMS_CONFIG_MOD = 2;

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
        std::memcpy(&value, data, copyLen);
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
        std::memcpy(&value, data, copyLen);
    }
    return value;
}

void FillSendSmsResultInfo(SendSmsResultInfo &result, const uint8_t *data, size_t size)
{
    result.msgRef = static_cast<int32_t>(MakeUint32(data, size));
    result.pdu = MakeString(data, size);
    result.errCode = MakeInt32(data, size);
    result.flag = MakeInt64(data, size);
}

void FillRadioResponseInfo(RadioResponseInfo &info, const uint8_t *data, size_t size)
{
    info.flag = MakeInt32(data, size);
    info.serial = MakeInt32(data, size);
    info.error = static_cast<ErrType>(MakeInt32(data, size));
    info.type = static_cast<ResponseTypes>(MakeInt32(data, size));
}

void FuzzSmsCallbackProxy(const uint8_t *data, size_t size)
{
    sptr<IRemoteObject> remote = nullptr;
    SmsSendShortMessageProxy sendProxy(remote);
    SmsDeliveryShortMessageProxy deliveryProxy(remote);
    auto result = static_cast<ISendShortMessageCallback::SmsSendResult>(MakeUint32(data, size) % SMS_SEND_RESULT_MOD);

    sendProxy.OnSmsSendResult(result);
    deliveryProxy.OnSmsDeliveryResult(MakeString16(data, size));
}

void FuzzImsCallbackProxy(const uint8_t *data, size_t size)
{
    sptr<IRemoteObject> remote = nullptr;
    ImsSmsCallbackProxy imsProxy(remote);
    int32_t slotId = static_cast<int32_t>(MakeUint32(data, size) % SLOT_COUNT);
    SendSmsResultInfo smsResult;
    RadioResponseInfo radioInfo;
    FillSendSmsResultInfo(smsResult, data, size);
    FillRadioResponseInfo(radioInfo, data, size);

    imsProxy.ImsSendMessageResponse(slotId, smsResult);
    imsProxy.ImsSendMessageResponse(slotId, radioInfo);
    imsProxy.ImsSetSmsConfigResponse(slotId, radioInfo);
    imsProxy.ImsGetSmsConfigResponse(slotId, MakeInt32(data, size) % IMS_CONFIG_MOD);
    imsProxy.ImsGetSmsConfigResponse(slotId, radioInfo);
}
} // namespace

void DoCallbackProxyFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzSmsCallbackProxy(data, size);
    FuzzImsCallbackProxy(data, size);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoCallbackProxyFuzzTest(data, size);
    return 0;
}
