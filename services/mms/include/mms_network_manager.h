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

#ifndef MMS_NETWORK_MANAGER_H
#define MMS_NETWORK_MANAGER_H

#include "mms_conn_callback_stub.h"
#include "mms_network_client.h"

namespace OHOS {
namespace Telephony {
class MmsNetworkManager {
public:
    MmsNetworkManager() = default;
    virtual ~MmsNetworkManager() = default;
    int32_t AcquireNetwork(int32_t slotId, uint8_t requestId);
    std::shared_ptr<MmsNetworkClient> GetOrCreateHttpClient(int32_t slotId);
    void ReleaseNetwork(int32_t slotId, uint8_t requestId, bool shouldDelayRelease);

private:
    std::shared_ptr<MmsNetworkClient> mmsNetworkClient_ = nullptr;
    sptr<MmsConnCallbackStub> callback_;
};
} // namespace Telephony
} // namespace OHOS
#endif