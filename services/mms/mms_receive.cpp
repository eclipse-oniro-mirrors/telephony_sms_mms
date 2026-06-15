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

#include "mms_receive.h"

#include "data_request.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
std::string DOWNLOAD_METHOD = "GET";

MmsReceive::MmsReceive(int32_t slotId) : DataRequest(slotId)
{
    slotId_ = slotId;
    CreateRequestId();
}

MmsReceive::~MmsReceive() {}

int32_t MmsReceive::ExecuteDownloadMms(
    const std::string &contentUrl, std::string &pduDir, const std::string &ua, const std::string &uaprof)
{
    auto mmsNetworkMgr = std::make_shared<MmsNetworkManager>();
    if (mmsNetworkMgr == nullptr) {
        TELEPHONY_LOGE("mmsNetworkMgr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }

    std::unique_lock<std::mutex> lock(downloadMmsMutex_);
    /*
     * SSRF攻击，需要借助服务端转发，mmsclient是彩信的SDK，应用如果想要发起SSRF攻击，访问某个file://，
     * 可以完全不用sdk，直接发包。在客户端进行拦截和校验url，完全没有必要。SSRF防御是要在服务端做防御的。
     * 客户端防御没有任何意义。如果应用搭建了一个file服务器想要访问。sdk不应进行限制。
     */
    return ExecuteMms(DOWNLOAD_METHOD, mmsNetworkMgr, contentUrl, pduDir, ua, uaprof);
}
} // namespace Telephony
} // namespace OHOS