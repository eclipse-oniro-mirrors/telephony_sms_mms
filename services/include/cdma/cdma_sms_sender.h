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

#ifndef CDMA_SMS_SENDER_H
#define CDMA_SMS_SENDER_H

#include <memory>

#include "sms_sender.h"
#include "sms_receive_indexer.h"

namespace OHOS {
namespace Telephony {
class CdmaSmsSender : public SmsSender {
public:
    CdmaSmsSender(const std::shared_ptr<AppExecFwk::EventRunner> &runner, int32_t slotId,
        std::function<void(std::shared_ptr<SmsSendIndexer>)> sendRetryFun);
    ~CdmaSmsSender() override;
    void TextBasedSmsDelivery(const std::string &desAddr, const std::string &scAddr, const std::string &text,
        const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliveryCallback) override;
    void DataBasedSmsDelivery(const std::string &desAddr, const std::string &scAddr, int32_t port,
        const uint8_t *data, uint32_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliveryCallback) override;
    void SendSmsToRil(const std::shared_ptr<SmsSendIndexer> &smsIndexer) override;
    void Init() override;
    void ReceiveStatusReport(const std::shared_ptr<SmsReceiveIndexer> &smsIndexer);
    void ResendTextDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer) override;
    void ResendDataDelivery(const std::shared_ptr<SmsSendIndexer> &smsIndexer) override;

protected:
    void StatusReportAnalysis(const AppExecFwk::InnerEvent::Pointer &event) override;

private:
};
} // namespace Telephony
} // namespace OHOS
#endif