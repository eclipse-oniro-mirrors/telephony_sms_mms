/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "smsrecvitem_fuzzer.h"

#ifdef GTEST_API_
#define private public
#define protected public
#endif
#include <cstdlib>
#include <unistd.h>

#include "addsmstoken_fuzzer.h"
#include "cdma_sms_message.h"
#include "common_event.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "delivery_short_message_callback_stub.h"
#include "send_short_message_callback_stub.h"
#include "sms_broadcast_subscriber_receiver.h"
#include "sms_interface_manager.h"
#include "sms_receive_reliability_handler.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
using namespace EventFwk;
constexpr int32_t SLOT_NUM = 2;
bool g_flag = false;

void DoRecvItemsTest(const uint8_t *data, size_t size, std::shared_ptr<SmsReceiveManager> smsReceiveManager)
{
    const std::shared_ptr<GsmSmsMessage> smsGsmMessage = std::make_shared<GsmSmsMessage>();
    const std::shared_ptr<CdmaSmsMessage> smsCdmaMessage = std::make_shared<CdmaSmsMessage>();
    smsReceiveManager->gsmSmsReceiveHandler_->HandleReceivedSms(smsGsmMessage);
    smsReceiveManager->cdmaSmsReceiveHandler_->HandleReceivedSms(smsCdmaMessage);
    smsReceiveManager->gsmSmsReceiveHandler_->HandleSmsByType(smsGsmMessage);
    smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(smsCdmaMessage);

    std::int32_t result = static_cast<int32_t>(size);
    smsReceiveManager->gsmSmsReceiveHandler_->ReplySmsToSmsc(result);
    smsReceiveManager->cdmaSmsReceiveHandler_->ReplySmsToSmsc(result);

    const std::shared_ptr<SmsReceiveIndexer> smsReceiveIndexer = std::make_shared<SmsReceiveIndexer>();
    smsReceiveManager->gsmSmsReceiveHandler_->CombineMessagePart(smsReceiveIndexer);
    smsReceiveManager->cdmaSmsReceiveHandler_->CombineMessagePart(smsReceiveIndexer);

    smsReceiveManager->gsmSmsReceiveHandler_->IsRepeatedMessagePart(smsReceiveIndexer);
    smsReceiveManager->cdmaSmsReceiveHandler_->IsRepeatedMessagePart(smsReceiveIndexer);

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId);
    if (reliabilityHandler == nullptr) {
        return;
    }
    reliabilityHandler->DeleteMessageFormDb(smsReceiveIndexer->GetMsgRefId(), smsReceiveIndexer->GetDataBaseId(),
        smsReceiveIndexer->GetMsgCount(), smsReceiveIndexer->GetOriginatingAddress());

    std::vector<SmsReceiveIndexer> dbIndexers;
    std::string strData(reinterpret_cast<const char *>(data), size);
    auto indexer = SmsReceiveIndexer(StringUtils::HexToByteVector(strData), size, size, size % SLOT_NUM, strData,
        strData, size, size, size, size % SLOT_NUM, strData);

    dbIndexers.push_back(indexer);
    indexer = SmsReceiveIndexer(
        StringUtils::HexToByteVector(strData), size, size, size % SLOT_NUM, size % SLOT_NUM, strData, strData, strData);
    dbIndexers.push_back(indexer);
    reliabilityHandler->CheckUnReceiveWapPush(dbIndexers);

    std::shared_ptr<std::vector<std::string>> userDataRaws = std::make_shared<std::vector<std::string>>();
    userDataRaws->push_back(strData);

    int32_t pages = 0;
    reliabilityHandler->GetWapPushUserDataSinglePage(indexer, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(pages, dbIndexers, size, userDataRaws);
    reliabilityHandler->ReadyDecodeWapPushUserData(indexer, userDataRaws);
    reliabilityHandler->GetSmsUserDataMultipage(pages, indexer.GetMsgCount(), dbIndexers, size, userDataRaws);
    reliabilityHandler->ReadySendSmsBroadcast(indexer, userDataRaws);
    reliabilityHandler->DeleteMessageFormDb(size, size, indexer.GetMsgCount(), indexer.GetOriginatingAddress());
    reliabilityHandler->RemoveBlockedSms(dbIndexers);

    std::shared_ptr<SmsReceiveIndexer> indexerPtr =
        std::make_shared<SmsReceiveIndexer>(StringUtils::HexToByteVector(strData), size, size, size % SLOT_NUM, strData,
            strData, size, size, size, size % SLOT_NUM, strData);
    if (indexerPtr == nullptr) {
        return;
    }
    reliabilityHandler->SendBroadcast(indexerPtr, userDataRaws);
}

void DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    if (g_flag) {
        return;
    }
    g_flag = true;

    int32_t slotId = static_cast<int32_t>(size % SLOT_NUM);
    auto smsInterfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (smsInterfaceManager == nullptr) {
        return;
    }

    smsInterfaceManager->InitInterfaceManager();
    auto smsReceiveManager = std::make_shared<SmsReceiveManager>(slotId);
    if (smsReceiveManager == nullptr) {
        return;
    }
    smsReceiveManager->Init();
    if (smsReceiveManager->gsmSmsReceiveHandler_ == nullptr || smsReceiveManager->cdmaSmsReceiveHandler_ == nullptr) {
        return;
    }
    smsReceiveManager->gsmSmsReceiveHandler_->Init();
    smsReceiveManager->cdmaSmsReceiveHandler_->Init();

    std::int32_t eventId = static_cast<int32_t>(size);
    std::int64_t refId = static_cast<int64_t>(size);

    AppExecFwk::InnerEvent::Pointer response = AppExecFwk::InnerEvent::Get(eventId, refId);
    smsReceiveManager->gsmSmsReceiveHandler_->ProcessEvent(response);
    smsReceiveManager->cdmaSmsReceiveHandler_->ProcessEvent(response);

    MatchingSkills smsSkills;
    std::string strData(reinterpret_cast<const char *>(data), size);
    smsSkills.AddEvent(strData);
    CommonEventSubscribeInfo smsSubscriberInfo(smsSkills);
    std::string addr(reinterpret_cast<const char *>(data), size);
    auto smsReceiver = std::make_shared<SmsBroadcastSubscriberReceiver>(smsSubscriberInfo);
    CommonEventData comData;
    smsReceiver->OnReceiveEvent(comData);

    DoRecvItemsTest(data, size, smsReceiveManager);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    bool g_atexitRegistered = false;
    if (!g_atexitRegistered) {
        g_atexitRegistered = true;
        atexit([] { _exit(0); });
    }
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
