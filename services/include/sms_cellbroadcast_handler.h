/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef SMS_CELLBROADCAST_HANDLER_H
#define SMS_CELLBROADCAST_HANDLER_H
#include <memory>
#include "phone_manager.h"
#include "event_handler.h"
#include "event_runner.h"
namespace OHOS {
namespace SMS {
class SmsCellbroadcastHandler : public AppExecFwk::EventHandler {
public:
    SmsCellbroadcastHandler() = default;
    ~SmsCellbroadcastHandler() = default;
    virtual void ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event) override;

protected:
private:
    int32_t subId_;
};
} // namespace SMS
} // namespace OHOS
#endif