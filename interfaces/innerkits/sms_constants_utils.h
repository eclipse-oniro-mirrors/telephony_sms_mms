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

#ifndef SMS_CONTANTS_UTILS_H
#define SMS_CONTANTS_UTILS_H

#include <cstdint>

namespace OHOS {
namespace Telephony {
static constexpr const uint32_t MAX_MMS_ATTACHMENT_LEN = 10 * 1024 * 1024;
static constexpr const uint32_t MMS_PDU_MAX_SIZE = 10 * 1024 * 1024;
} // namespace Telephony
} // namespace OHOS
#endif // SMS_CONTANTS_UTILS_H
