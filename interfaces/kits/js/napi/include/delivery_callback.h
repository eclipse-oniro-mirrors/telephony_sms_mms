/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef DELIVERY_CALL_BACK_H
#define DELIVERY_CALL_BACK_H
#include "delivery_short_message_callback_stub.h"
#include "napi/native_api.h"

namespace OHOS {
namespace TelephonyNapi {
class DeliveryCallback : public DeliveryShortMessageCallbackStub {
public:
    DeliveryCallback(bool hasCallback, napi_env env, napi_value thisVar, napi_ref callbackRef);
    ~DeliveryCallback();
    int32_t OnSmsDeliveryResult(const std::u16string pdu) override;

private:
    bool hasCallback_;
    napi_env env_;
    napi_value thisVar_;
    napi_ref callbackRef_;
};
} // namespace TelephonyNapi
} // namespace OHOS

#endif // DELIVERY_CALL_BACK_H