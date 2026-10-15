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

#include "mms_network_manager.h"

#include "core_manager_inner.h"
#include "net_conn_client.h"
#include "net_specifier.h"
#include "telephony_ext_wrapper.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace NetManagerStandard;
static constexpr uint32_t NET_REGISTER_TIMEOUT_MS = 20000;
constexpr const char *SIMID_IDENT_PREFIX = "simId";
/* booster service notify content */
static constexpr int32_t BOOSTER_MSG_SERVICE_TYPE_MMS = 1;
static constexpr int32_t BOOSTER_MSG_ACTION_START = 1;
static constexpr int32_t BOOSTER_MSG_ACTION_STOP = 0;
static constexpr int32_t BOOSTER_MSG_NOTIFY_SLOT_ID = 3;

static void NotifyBoosterMsgForMms(int32_t slotId, int32_t action)
{
    if (slotId != BOOSTER_MSG_NOTIFY_SLOT_ID) {
        return;
    }
    TELEPHONY_EXT_WRAPPER.NotifyBoosterServiceMsg(slotId, BOOSTER_MSG_SERVICE_TYPE_MMS, action);
}

int32_t MmsNetworkManager::AcquireNetwork(int32_t slotId, uint8_t requestId)
{
    NotifyBoosterMsgForMms(slotId, BOOSTER_MSG_ACTION_START);
    NetSpecifier netSpecifier;
    NetAllCapabilities netAllCapabilities;
    netAllCapabilities.netCaps_.insert(NetCap::NET_CAPABILITY_MMS);
    netAllCapabilities.bearerTypes_.insert(NetBearType::BEARER_CELLULAR);

    int32_t simId = CoreManagerInner::GetInstance().GetSimId(slotId);
    TELEPHONY_LOGI("requestId = %{public}d, slot = %{public}d, simId = %{public}d", requestId, slotId, simId);
    netSpecifier.ident_ = SIMID_IDENT_PREFIX + std::to_string(simId);
    netSpecifier.netCapabilities_ = netAllCapabilities;

    sptr<NetSpecifier> specifier = new (std::nothrow) NetSpecifier(netSpecifier);
    if (specifier == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callback_ = new (std::nothrow) MmsConnCallbackStub();
    if (callback_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }

    int32_t result =
        NetConnClient::GetInstance().RegisterNetConnCallback(specifier, callback_, NET_REGISTER_TIMEOUT_MS);
    TELEPHONY_LOGI("acquire network result = %{public}d", result);
    return result;
}

void MmsNetworkManager::ReleaseNetwork(int32_t slotId, uint8_t requestId, bool shouldDelayRelease)
{
    TELEPHONY_LOGI("requestId:%{public}d,shouldDelayRelease:%{public}d", requestId, shouldDelayRelease);
    NotifyBoosterMsgForMms(slotId, BOOSTER_MSG_ACTION_STOP);
    if (callback_ != nullptr) {
        TELEPHONY_LOGI("UnregisterNetConnCallback");
        NetConnClient ::GetInstance().UnregisterNetConnCallback(callback_);
        callback_ = nullptr;
    }
}

std::shared_ptr<MmsNetworkClient> MmsNetworkManager::GetOrCreateHttpClient(int32_t slotId)
{
    if (mmsNetworkClient_ == nullptr) {
        TELEPHONY_LOGE("mmsNetworkClient_ nullptr");
        mmsNetworkClient_ = std::make_shared<OHOS::Telephony::MmsNetworkClient>(slotId);
    }
    return mmsNetworkClient_;
}
} // namespace Telephony
} // namespace OHOS
