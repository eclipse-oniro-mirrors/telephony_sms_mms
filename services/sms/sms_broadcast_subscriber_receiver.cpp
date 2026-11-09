/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "sms_broadcast_subscriber_receiver.h"
#include "sms_persist_helper.h"
#include "sms_receive_reliability_handler.h"
#include "telephony_log_wrapper.h"
#include "want.h"
#include "event_handler.h"
#include "event_runner.h"

namespace OHOS {
namespace Telephony {
using namespace EventFwk;
// same key as used by SmsReceiveReliabilityHandler::PacketSmsData and SmsWapPushHandler::SendWapPushMessageBroadcast
static constexpr const char *SMS_BROADCAST_SLOT_ID_KEY = "slotId";
static constexpr int32_t SMS_BROADCAST_DEFAULT_SLOT_ID = 0;

SmsBroadcastSubscriberReceiver::SmsBroadcastSubscriberReceiver(
    const CommonEventSubscribeInfo &subscriberInfo) : CommonEventSubscriber(subscriberInfo) {}

void SmsBroadcastSubscriberReceiver::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    bool isSms = action == CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED;
    bool isWapPush = action == CommonEventSupport::COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED;
    if (isSms || isWapPush) {
        int refId = want.GetIntParam(SMS_BROADCAST_MSG_REF_ID_KEY, 0);
        int dataBaseId = want.GetIntParam(SMS_BROADCAST_DATABASE_ID_KEY, 0);
        int msgCount = want.GetIntParam(SMS_BROADCAST_MSG_COUNT_KEY, 0);
        int slotId = want.GetIntParam(SMS_BROADCAST_SLOT_ID_KEY, SMS_BROADCAST_DEFAULT_SLOT_ID);
        TELEPHONY_LOGI("OnReceiveEvent refId =%{public}d, dataBaseId =%{public}d, msgCount = %{public}d, "
            "slotId = %{public}d", refId, dataBaseId, msgCount, slotId);
        std::string address = want.GetStringParam(SMS_BROADCAST_ADDRESS_KEY);
        if (!address.empty() && isSms) {
            DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateContact(address);
        }
        std::make_shared<SmsReceiveReliabilityHandler>(slotId)->
            DeleteMessageFormDb(refId, dataBaseId, msgCount, address);
    }
}
} // namespace Telephony
} // namespace OHOS