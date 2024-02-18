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

#include "data_request.h"

#include "core_manager_inner.h"
#include "net_conn_constants.h"
#include "network_search_types.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace NetManagerStandard;
constexpr static const int32_t WAIT_TIME_SECOND = 30;

DataRequest::DataRequest(int32_t slotId) : slotId_(slotId) {}

DataRequest::~DataRequest() {}

int32_t DataRequest::HttpRequest(int32_t slotId, const std::string &method, std::shared_ptr<MmsNetworkManager> netMgr,
    const std::string &contentUrl, std::string &pduDir)
{
    if (netMgr == nullptr) {
        TELEPHONY_LOGE("netMgr is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<MmsNetworkClient> netClient = netMgr->GetOrCreateHttpClient(slotId);
    if (netClient == nullptr) {
        TELEPHONY_LOGE("netClient is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return netClient->Execute(method, contentUrl, pduDir);
}

int32_t DataRequest::ExecuteMms(const std::string &method, std::shared_ptr<MmsNetworkManager> mmsNetworkMgr,
    const std::string &contentUrl, std::string &pduDir)
{
    std::unique_lock<std::mutex> lck(ctx_);
    if (mmsNetworkMgr->AcquireNetwork(slotId_, GetRequestId()) != NETMANAGER_SUCCESS) {
        TELEPHONY_LOGE("acquire network fail");
        return TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR;
    }

    networkReady_ = false;
    while (!networkReady_) {
        TELEPHONY_LOGI("wait(), networkReady = false");
        if (cv_.wait_for(lck, std::chrono::seconds(WAIT_TIME_SECOND)) == std::cv_status::timeout) {
            break;
        }
    }

    if (!networkReady_) {
        TELEPHONY_LOGE("acquire network fail");
        mmsNetworkMgr->ReleaseNetwork(GetRequestId(), false);
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }

    int32_t executeResult = HttpRequest(slotId_, method, mmsNetworkMgr, contentUrl, pduDir);
    mmsNetworkMgr->ReleaseNetwork(GetRequestId(), false);
    return executeResult;
}

uint8_t DataRequest::GetRequestId()
{
    return requestId_;
}

void DataRequest::CreateRequestId()
{
    requestId_++;
}
} // namespace Telephony
} // namespace OHOS