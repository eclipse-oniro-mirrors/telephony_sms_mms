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
 
#ifndef TELEPHONY_EXT_WRAPPER_H
#define TELEPHONY_EXT_WRAPPER_H
 
#include "nocopyable.h"
#include "singleton.h"
 
namespace OHOS {
namespace Telephony {
class TelephonyExtWrapper final {
DECLARE_DELAYED_REF_SINGLETON(TelephonyExtWrapper);
 
public:
    DISALLOW_COPY_AND_MOVE(TelephonyExtWrapper);
    void InitTelephonyExtWrapper();

    /* add for etws */
    typedef bool (*IS_CUST_ETWS_MSG)(uint16_t channle);
    typedef void (*GET_CUST_ETWS_TYPE)(uint16_t channle, uint16_t &type);
    typedef uint32_t (*GET_CBS_PDU_LENGTH)(const std::vector<unsigned char> &pdu);

    /* add for booster service notify */
    typedef void (*NOTIFY_BOOSTER_SERVICE_MSG)(int32_t slotId, int32_t serviceType, int32_t action);
 
    /* add for etws */
    IS_CUST_ETWS_MSG isCustEtwsMessage_ = nullptr;
    GET_CUST_ETWS_TYPE getCustEtwsType_ = nullptr;
    GET_CBS_PDU_LENGTH getCbsPduLength_ = nullptr;

    /* add for booster service notify */
    NOTIFY_BOOSTER_SERVICE_MSG notifyBoosterServiceMsg_ = nullptr;
    void NotifyBoosterServiceMsg(int32_t slotId, int32_t serviceType, int32_t action);
 
private:
    void* telephonyExtWrapperHandle_ = nullptr;
};
 
#define TELEPHONY_EXT_WRAPPER ::OHOS::DelayedRefSingleton<TelephonyExtWrapper>::GetInstance()
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_EXT_WRAPPER_H