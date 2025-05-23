/*
 * Copyright (C)2023 Huawei Device Co., Ltd.
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

#include "cb_start_ability.h"
#include "os_account_manager.h"
#include "ability_manager_client.h"
#include "int_wrapper.h"
#include "string_wrapper.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "want.h"

namespace OHOS {
namespace Telephony {
const std::string BUNDLE_NAME = "";
constexpr const char *ABILITY_NAME = "AlertService";
static constexpr int32_t USER_ID = 100;

CbStartAbility::~CbStartAbility() {}

CbStartAbility::CbStartAbility() {}

void CbStartAbility::StartAbility(AAFwk::Want &want)
{
    std::vector<int> activatedOsAccountIds;
    AccountSA::OsAccountManager::QueryActiveOsAccountIds(activatedOsAccountIds);
    TELEPHONY_LOGI("start cellbroadcast ability");
    want.SetElementName("", BUNDLE_NAME, ABILITY_NAME);
    if (!activatedOsAccountIds.empty()) {
        TELEPHONY_LOGI("the foreground OS account local ID: %{public}d", activatedOsAccountIds[0]);
        ErrCode code = AAFwk::AbilityManagerClient::GetInstance()->StartExtensionAbility(want, nullptr,
        activatedOsAccountIds[0]);
        TELEPHONY_LOGI("start ability code:%{public}d", code);
    } else {
        TELEPHONY_LOGI("the activatedOsAccountId is empty, input default userid: 100");
        ErrCode code = AAFwk::AbilityManagerClient::GetInstance()->StartExtensionAbility(want, nullptr, USER_ID);
        TELEPHONY_LOGI("start ability code:%{public}d", code);
    }
}
} // namespace Telephony
} // namespace OHOS
