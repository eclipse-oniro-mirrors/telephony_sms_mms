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

#ifndef SMS_RECEIVE_HANDLER_H
#define SMS_RECEIVE_HANDLER_H

#include <queue>
#include <vector>

#include "tel_ril_sms_parcel.h"
#include "sms_base_message.h"
#include "sms_receive_indexer.h"
#include "sms_receive_reliability_handler.h"
#include "sms_wap_push_handler.h"
#include "tel_event_handler.h"
#include "datashare_helper.h"
#include "system_ability_definition.h"
#ifdef BASE_POWER_IMPROVEMENT_FEATURE
#include "sms_state_handler.h"
#endif
#ifdef ABILITY_POWER_SUPPORT
#include "power_mgr_client.h"
#include "power_mgr_errors.h"
#endif

namespace OHOS {
namespace Telephony {
class SmsReceiveHandler : public TelEventHandler {
public:
    explicit SmsReceiveHandler(int32_t slotId);
    virtual ~SmsReceiveHandler();
    virtual void ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event) override;
    bool IsDataShareReady();
    void ApplyRunningLock();
    void ReduceRunningLock();
    void ReleaseRunningLock();

protected:
    virtual int32_t HandleSmsByType(const std::shared_ptr<SmsBaseMessage> smsBaseMessage) = 0;
    virtual int32_t HandleAck(const std::shared_ptr<SmsBaseMessage> smsBaseMessage) = 0;
    virtual void HandleRemainDataShare(const std::shared_ptr<SmsBaseMessage> smsBaseMessage) = 0;
    virtual bool ReplySmsToSmsc(int result) = 0;
    virtual void HandleMessageQueue();
    virtual void HandleReconnectEvent();
    virtual std::shared_ptr<SmsBaseMessage> TransformMessageInfo(const std::shared_ptr<SmsMessageInfo> info) = 0;
    void CombineMessagePart(const std::shared_ptr<SmsReceiveIndexer> &smsIndexer);
    bool IsRepeatedMessagePart(const std::shared_ptr<SmsReceiveIndexer> &smsIndexer);
    bool AddMsgToDB(const std::shared_ptr<SmsReceiveIndexer> indexer);

private:
    SmsReceiveHandler(const SmsReceiveHandler &) = delete;
    SmsReceiveHandler(const SmsReceiveHandler &&) = delete;
    SmsReceiveHandler &operator=(const SmsReceiveHandler &) = delete;
    SmsReceiveHandler &operator=(const SmsReceiveHandler &&) = delete;
    void HandleReceivedSms(const std::shared_ptr<SmsBaseMessage> smsBaseMessage);
    void HandleReceivedSmsWithoutDataShare(const std::shared_ptr<SmsBaseMessage> smsBaseMessage);
    bool CombineMultiPageMessage(const std::shared_ptr<SmsReceiveIndexer> &indexer,
        std::shared_ptr<std::vector<std::string>> pdus,
        std::shared_ptr<SmsReceiveReliabilityHandler> reliabilityHandler);
    void UpdateMultiPageMessage(
        const std::shared_ptr<SmsReceiveIndexer> &indexer, std::shared_ptr<std::vector<std::string>> pdus);
    void CreateRunningLockInner();
    void HandleRunningLockTimeoutEvent(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleSmsEvent(const AppExecFwk::InnerEvent::Pointer &event);

#ifdef BASE_POWER_IMPROVEMENT_FEATURE
    void HandleSmsReply();
#endif
protected:
    int32_t slotId_ = -1;

private:
    static const uint32_t RUNNING_LOCK_TIMEOUT_EVENT_ID = 100000;
    static const uint32_t DELAY_RELEASE_RUNNING_LOCK_EVENT_ID = 100001;
    static const uint32_t RETRY_CONNECT_DATASHARE_EVENT_ID = 100002;

    static const int64_t RUNNING_LOCK_DEFAULT_TIMEOUT_MS = 60 * 1000; // 60s
    static const int64_t DELAY_RELEASE_RUNNING_LOCK_TIMEOUT_MS = 5 * 1000; // 5s
    static const int64_t DELAY_REDUCE_RUNNING_LOCK_TIMEOUT_MS = 4 * 1000; // 8s
    static const int64_t DELAY_REDUCE_RUNNING_LOCK_SMS_QUEUE_TIMEOUT_MS = 20 * 1000; // 20s
    static const int64_t DELAY_RETRY_CONNECT_DATASHARE_MS = 5 * 1000; // 5s
    static const uint8_t RECONNECT_MAX_COUNT = 20;
    static const int32_t E_OK = 0;
    
    std::mutex queueMutex_;
    std::mutex datashareMutex_;
    std::unique_ptr<SmsWapPushHandler> smsWapPushHandler_;
#ifdef ABILITY_POWER_SUPPORT
    std::shared_ptr<PowerMgr::RunningLock> smsRunningLock_;
#endif
    std::atomic_uint smsRunningLockCount_;
    std::atomic_int smsLockSerialNum_;
    std::mutex mutexRunningLock_;
    bool alreadySendEvent_ = false;
    uint8_t reconnectDataShareCount_ = 0;
    std::queue<std::shared_ptr<SmsBaseMessage>> smsBaseMessageQueue_{};
};
} // namespace Telephony
} // namespace OHOS
#endif