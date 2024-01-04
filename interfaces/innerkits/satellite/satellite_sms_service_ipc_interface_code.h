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

#ifndef SATELLITE_SMS_SERVICE_INTERFACE_CODE_H
#define SATELLITE_SMS_SERVICE_INTERFACE_CODE_H

namespace OHOS {
namespace Telephony {
enum class SatelliteSmsServiceInterfaceCode {
    REGISTER_SMS_NOTIFY,
    UNREGISTER_SMS_NOTIFY,
    SEND_SMS,
    SEND_SMS_MORE_MODE,
    SEND_SMS_ACK,
};
} // namespace Telephony
} // namespace OHOS
#endif // SATELLITE_SMS_SERVICE_INTERFACE_CODE_H
