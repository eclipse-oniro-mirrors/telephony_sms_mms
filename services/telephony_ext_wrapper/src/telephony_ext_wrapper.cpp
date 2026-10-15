/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include <dlfcn.h>
#include "telephony_ext_wrapper.h"
#include "telephony_log_wrapper.h"
 
namespace OHOS {
namespace Telephony {
namespace {
const std::string TELEPHONY_EXT_WRAPPER_PATH = "libtelephony_ext_service.z.so";
} // namespace
 
TelephonyExtWrapper::TelephonyExtWrapper()
{
    TELEPHONY_LOGD("TelephonyExtWrapper::TelephonyExtWrapper() create");
}
TelephonyExtWrapper::~TelephonyExtWrapper()
{
    TELEPHONY_LOGD("TelephonyExtWrapper::~TelephonyExtWrapper() start");
    if (telephonyExtWrapperHandle_ != nullptr) {
        dlclose(telephonyExtWrapperHandle_);
        telephonyExtWrapperHandle_ = nullptr;
    }
}
void TelephonyExtWrapper::InitTelephonyExtWrapper()
{
    telephonyExtWrapperHandle_ = dlopen(TELEPHONY_EXT_WRAPPER_PATH.c_str(), RTLD_NOW);
    if (telephonyExtWrapperHandle_ == nullptr) {
        TELEPHONY_LOGE("libtelephony_ext_service.z.so was not loaded, error: %{public}s", dlerror());
        return;
    }
    notifyBoosterServiceMsg_ =
        (NOTIFY_BOOSTER_SERVICE_MSG)dlsym(telephonyExtWrapperHandle_, "NotifyBoosterServiceMsg");
    if (notifyBoosterServiceMsg_ == nullptr) {
        TELEPHONY_LOGE("telephony ext wrapper symbol NotifyBoosterServiceMsg failed, error: %{public}s", dlerror());
    }
    isCustEtwsMessage_ = (IS_CUST_ETWS_MSG)dlsym(telephonyExtWrapperHandle_, "IsCustEtwsMessage");
    getCustEtwsType_ = (GET_CUST_ETWS_TYPE)dlsym(telephonyExtWrapperHandle_, "GetCustEtwsType");
    getCbsPduLength_ = (GET_CBS_PDU_LENGTH)dlsym(telephonyExtWrapperHandle_, "getCbsPduLength");
    if (isCustEtwsMessage_ == nullptr || getCustEtwsType_ == nullptr || getCbsPduLength_ == nullptr) {
        TELEPHONY_LOGE("[ETWS]telephony ext wrapper symbol failed, error: %{public}s", dlerror());
        return;
    }
    TELEPHONY_LOGI("telephony ext wrapper init success");
}

void TelephonyExtWrapper::NotifyBoosterServiceMsg(int32_t slotId, int32_t serviceType, int32_t action)
{
    if (notifyBoosterServiceMsg_ != nullptr) {
        notifyBoosterServiceMsg_(slotId, serviceType, action);
        return;
    }
    TELEPHONY_LOGE("NotifyBoosterServiceMsg is not supported");
}
} // namespace Telephony
} // namespace OHOS