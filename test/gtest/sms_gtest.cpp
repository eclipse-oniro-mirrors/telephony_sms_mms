/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#define private public
#define protected public

#include "cdma_sms_message.h"
#include "cdma_sms_receive_handler.h"
#include "cdma_sms_transport_message.h"
#include "core_service_client.h"
#include "delivery_short_message_callback_stub.h"
#include "gtest/gtest.h"
#include "i_sms_service_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "radio_event.h"
#include "send_short_message_callback_stub.h"
#include "sms_broadcast_subscriber_gtest.h"
#include "sms_delivery_callback_gtest.h"
#include "sms_mms_gtest.h"
#include "sms_mms_test_helper.h"
#include "sms_send_callback_gtest.h"
#include "sms_service.h"
#include "sms_service_manager_client.h"
#include "string_utils.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "telephony_types.h"

namespace OHOS {
namespace Telephony {
namespace {
sptr<ISmsServiceInterface> g_telephonyService = nullptr;
} // namespace
using namespace testing::ext;

class SmsGtest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    static sptr<ISmsServiceInterface> GetProxy();
    static bool HasSimCard(int32_t slotId)
    {
        bool hasSimCard = false;
        if (CoreServiceClient::GetInstance().GetProxy() == nullptr) {
            return hasSimCard;
        }
        CoreServiceClient::GetInstance().HasSimCard(slotId, hasSimCard);
        return hasSimCard;
    }
};

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void SmsGtest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void SmsGtest::SetUp() {}

void SmsGtest::TearDown() {}

auto smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
const int32_t DEFAULT_SIM_SLOT_ID_1 = 1;
const std::string DES_ADDR = "10086";
const std::string TEXT_SMS_CONTENT = "hello world";
const uint8_t DATA_SMS[] = "hello world";
const uint16_t SMS_PORT = 100;
const uint16_t MESSAGE_TYPE = 4;
const uint16_t SMS_PDU_LEN = 36;
const int32_t WAIT_TIME_LONG_SECOND = 180;
const int32_t WAIT_TIME_SHORT_SECOND = 30;

void SmsGtest::SetUpTestCase()
{
    TELEPHONY_LOGI("SetUpTestCase slotId%{public}d", DEFAULT_SIM_SLOT_ID_1);
    g_telephonyService = GetProxy();
    if (g_telephonyService == nullptr) {
        return;
    }
    Singleton<SmsServiceManagerClient>::GetInstance().ResetSmsServiceProxy();
    Singleton<SmsServiceManagerClient>::GetInstance().InitSmsServiceProxy();
}

sptr<ISmsServiceInterface> SmsGtest::GetProxy()
{
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        return nullptr;
    }
    sptr<IRemoteObject> remote = systemAbilityMgr->CheckSystemAbility(TELEPHONY_SMS_MMS_SYS_ABILITY_ID);
    if (remote) {
        sptr<ISmsServiceInterface> smsService = iface_cast<ISmsServiceInterface>(remote);
        return smsService;
    }
    return nullptr;
}

#ifndef TEL_TEST_UNSUPPORT
/**
 * @tc.number   Telephony_SmsMmsGtest_GetProxy_0001
 * @tc.name     Get SmsMms service
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetProxy_0001, Function | MediumTest | Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetProxy_0001 -->");
    g_telephonyService = SmsGtest::GetProxy();
    TELEPHONY_LOGI("TelSMSMMSTest::GetProxy_0001 -->finished");
    EXPECT_FALSE(g_telephonyService == nullptr);
}

void SetDefaultSmsSlotIdTestFuc(SmsMmsTestHelper &helper)
{
    int32_t result = Singleton<SmsServiceManagerClient>::GetInstance().SetDefaultSmsSlotId(helper.slotId);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

void SetDefaultSmsSlotIdTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    SetDefaultSmsSlotIdTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetDefaultSmsSlotId_0001
 * @tc.name     Set Default Sms SlotId slotId is valid
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SetDefaultSmsSlotId_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(SetDefaultSmsSlotIdTestFucWithToken, std::ref(helper))) {
        TELEPHONY_LOGI("SetDefaultSmsSlotIdTestFucWithToken out of time");
        ASSERT_TRUE(helper.Run(SetDefaultSmsSlotIdTestFucWithToken, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetDefaultSmsSlotId_0002
 * @tc.name     Set Default Sms SlotId slotId is valid
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SetDefaultSmsSlotId_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(SetDefaultSmsSlotIdTestFucWithToken, std::ref(helper))) {
        TELEPHONY_LOGI("SetDefaultSmsSlotIdTestFucWithToken out of time");
        ASSERT_TRUE(helper.Run(SetDefaultSmsSlotIdTestFucWithToken, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0002 -->finished");
    EXPECT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetDefaultSmsSlotId_0003
 * @tc.name     Set Default Sms SlotId slotId is valid
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SetDefaultSmsSlotId_0003, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0003 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(SetDefaultSmsSlotIdTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("SetDefaultSmsSlotIdTestFuc out of time");
        ASSERT_TRUE(helper.Run(SetDefaultSmsSlotIdTestFuc, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SetDefaultSmsSlotId_0003 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

void GetDefaultSmsSlotIdTestFuc(SmsMmsTestHelper &helper)
{
    int32_t slotId = Singleton<SmsServiceManagerClient>::GetInstance().GetDefaultSmsSlotId();
    helper.SetIntResult(slotId);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetDefaultSmsSlotId_0001
 * @tc.name     Get Default Sms SlotId
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetDefaultSmsSlotId_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSlotId_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    if (!helper.Run(GetDefaultSmsSlotIdTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("GetDefaultSmsSlotIdTestFuc out of time");
        ASSERT_FALSE(helper.Run(GetDefaultSmsSlotIdTestFuc, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSlotId_0001 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

void GetDefaultSmsSimIdTestFuc(SmsMmsTestHelper &helper)
{
    int32_t simId = DEFAULT_SIM_SLOT_ID_REMOVE;
    Singleton<SmsServiceManagerClient>::GetInstance().GetDefaultSmsSimId(simId);
    helper.SetIntResult(simId);
    helper.NotifyAll();
}

/**
 * @tc.number   GetDefaultSmsSimId_0001
 * @tc.name     Get Default Sms SimId
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetDefaultSmsSimId_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSimId_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    if (!helper.Run(GetDefaultSmsSimIdTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("GetDefaultSmsSimIdTestFuc out of time");
        ASSERT_FALSE(helper.Run(GetDefaultSmsSimIdTestFuc, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSimId_0001 -->finished");
    ASSERT_GT(helper.GetIntResult(), 0);
}

/**
 * @tc.number   GetDefaultSmsSimId_0002
 * @tc.name     Get Default Sms SimId
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetDefaultSmsSimId_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSimId_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    if (!helper.Run(GetDefaultSmsSimIdTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("GetDefaultSmsSimIdTestFuc out of time");
        ASSERT_FALSE(helper.Run(GetDefaultSmsSimIdTestFuc, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetDefaultSmsSimId_0002 -->finished");
    ASSERT_GT(helper.GetIntResult(), 0);
}

void SetSmscAddrTestFuc(SmsMmsTestHelper &helper)
{
    // invalid slotID scenario, a invalid smsc addr is OKAY
    std::string scAddr("1234");
    int32_t result = Singleton<SmsServiceManagerClient>::GetInstance().SetScAddress(
        helper.slotId, StringUtils::ToUtf16(scAddr));
    helper.SetIntResult(result);
    helper.NotifyAll();
}

void SetSmscAddrTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    SetSmscAddrTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetSmscAddr_0001
 * @tc.name     Set smsc addr slotId is invalid
 * @tc.desc     Function test
 * @tc.require: issueI5JI0H
 */
HWTEST_F(SmsGtest, SetSmscAddr_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetSmscAddr_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_REMOVE;
    if (!helper.Run(SetSmscAddrTestFucWithToken, std::ref(helper))) {
        TELEPHONY_LOGI("SetSmscAddrTestFucWithToken out of time");
        ASSERT_TRUE(helper.Run(SetSmscAddrTestFucWithToken, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SetSmscAddr_0001 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetSmscAddr_0002
 * @tc.name     Set smsc addr slotId is invalid
 * @tc.desc     Function test
 * @tc.require: issueI5JI0H
 */
HWTEST_F(SmsGtest, SetSmscAddr_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetSmscAddr_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_REMOVE;
    if (!helper.Run(SetSmscAddrTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("SetSmscAddrTestFuc out of time");
        ASSERT_TRUE(helper.Run(SetSmscAddrTestFuc, std::ref(helper)));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SetSmscAddr_0002 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

void DelAllSimMessagesTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<ShortMessage> message;
    Singleton<SmsServiceManagerClient>::GetInstance().GetAllSimMessages(helper.slotId, message);
    for (auto msg : message) {
        TELEPHONY_LOGI("DelAllSimMessagesTestFuc,index:%{public}d", msg.GetIndexOnSim());
        Singleton<SmsServiceManagerClient>::GetInstance().DelSimMessage(helper.slotId, msg.GetIndexOnSim());
    }
    helper.SetBoolResult(message.size() >= 0);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DelAllSimMessages_0001
 * @tc.name     Delete All Sim Messages
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, DelAllSimMessages_0001, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::DelAllSimMessages_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(DelAllSimMessagesTestFuc, std::ref(helper), WAIT_TIME_LONG_SECOND)) {
        TELEPHONY_LOGI("DelAllSimMessagesTestFuc out of time");
        ASSERT_TRUE(helper.Run(DelAllSimMessagesTestFuc, std::ref(helper), WAIT_TIME_LONG_SECOND));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::DelAllSimMessages_0001 -->finished");
    EXPECT_TRUE(helper.GetBoolResult());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DelAllSimMessages_0002
 * @tc.name     Delete All Sim Messages
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, DelAllSimMessages_0002, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::DelAllSimMessages_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(DelAllSimMessagesTestFuc, std::ref(helper), WAIT_TIME_LONG_SECOND)) {
        TELEPHONY_LOGI("DelAllSimMessagesTestFuc out of time");
        ASSERT_TRUE(helper.Run(DelAllSimMessagesTestFuc, std::ref(helper), WAIT_TIME_LONG_SECOND));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::DelAllSimMessages_0002 -->finished");
    EXPECT_TRUE(helper.GetBoolResult());
}

void AddSimMessageTestFuc(SmsMmsTestHelper &helper)
{
    std::u16string smscData(u"");
    std::u16string pduData(u"01000B818176251308F4000007E8B0BCFD76E701");
    uint32_t status = 3;
    int32_t result = Singleton<SmsServiceManagerClient>::GetInstance().AddSimMessage(
        helper.slotId, smscData, pduData, static_cast<ISmsServiceInterface::SimMessageStatus>(status));
    helper.SetIntResult(result);
    helper.NotifyAll();
}

void AddSimMessageTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    AddSimMessageTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_AddSimMessage_0001
 * @tc.name     Add Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, AddSimMessage_0001, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;

    helper.Run(AddSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0001 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_AddSimMessage_0002
 * @tc.name     Add Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, AddSimMessage_0002, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;

    helper.Run(AddSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0002 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_AddSimMessage_0003
 * @tc.name     Add Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, AddSimMessage_0003, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0003 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;

    helper.Run(AddSimMessageTestFuc, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::AddSimMessage_0003 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

void GetAllSimMessagesTestFuc(SmsMmsTestHelper &helper)
{
    std::vector<ShortMessage> message;
    Singleton<SmsServiceManagerClient>::GetInstance().GetAllSimMessages(helper.slotId, message);
    bool empty = message.empty();
    helper.SetBoolResult(empty);
    helper.NotifyAll();
}

void GetAllSimMessagesTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    GetAllSimMessagesTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetAllSimMessages_0001
 * @tc.name     Get All Sim Messages
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetAllSimMessages_0001, TestSize.Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetAllSimMessages_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(GetAllSimMessagesTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::GetAllSimMessages_0001 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetAllSimMessages_0002
 * @tc.name     Get All Sim Messages
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetAllSimMessages_0002, TestSize.Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetAllSimMessages_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    helper.Run(GetAllSimMessagesTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::GetAllSimMessages_0002 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

void UpdateSimMessageTestFuc(SmsMmsTestHelper &helper)
{
    uint32_t msgIndex = 0;
    std::u16string smscData(u"");
    std::u16string pduData(u"01000B818176251308F4000007E8B0BCFD76E701");
    uint32_t status = 3;

    int32_t result = Singleton<SmsServiceManagerClient>::GetInstance().UpdateSimMessage(
        helper.slotId, msgIndex, static_cast<ISmsServiceInterface::SimMessageStatus>(status), pduData, smscData);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

void UpdateSimMessageTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    UpdateSimMessageTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_UpdateSimMessage_0001
 * @tc.name     Update Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, UpdateSimMessage_0001, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(UpdateSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0001 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_UpdateSimMessage_0002
 * @tc.name     Update Sim Message
 * @tc.desc     Function test
 * @tc.require: issueI5K12U
 */
HWTEST_F(SmsGtest, UpdateSimMessage_0002, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    helper.Run(UpdateSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0002 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_UpdateSimMessage_0003
 * @tc.name     Update Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, UpdateSimMessage_0003, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0003 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(UpdateSimMessageTestFuc, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::UpdateSimMessage_0003 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

void DelSimMessageTestFuc(SmsMmsTestHelper &helper)
{
    uint32_t msgIndex = 1;
    int32_t result = Singleton<SmsServiceManagerClient>::GetInstance().DelSimMessage(helper.slotId, msgIndex);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

void DelSimMessageTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    DelSimMessageTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DelSimMessage_0001
 * @tc.name     Del Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, DelSimMessage_0001, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(DelSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0001 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DelSimMessage_0002
 * @tc.name     Del Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, DelSimMessage_0002, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    helper.Run(DelSimMessageTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0002 -->finished");
    ASSERT_GE(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DelSimMessage_0003
 * @tc.name     Del Sim Message
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, DelSimMessage_0003, TestSize.Level0)
{
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0003 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(DelSimMessageTestFuc, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::DelSimMessage_0003 -->finished");
    EXPECT_NE(helper.GetIntResult(), 0);
}

void SetImsSmsConfigTestFuc(SmsMmsTestHelper &helper)
{
    Singleton<SmsServiceManagerClient>::GetInstance().SetImsSmsConfig(helper.slotId, 1);
    bool isSupported = false;
    Singleton<SmsServiceManagerClient>::GetInstance().IsImsSmsSupported(helper.slotId, isSupported);
    helper.SetBoolResult(isSupported);
    helper.NotifyAll();
}

void SetImsSmsConfigTestFucWithToken(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    SetImsSmsConfigTestFuc(helper);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetImsSmsConfig_0001
 * @tc.name     Enable IMS SMS
 * @tc.desc     Function test
 * @tc.require: issueI5K12U
 */
HWTEST_F(SmsGtest, SetImsSmsConfig_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(SetImsSmsConfigTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0001 -->finished");
    ASSERT_TRUE(helper.GetBoolResult());
}

void SetImsSmsConfigTestFuc2(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    bool isSupported = false;
    Singleton<SmsServiceManagerClient>::GetInstance().SetImsSmsConfig(helper.slotId, 0);
    Singleton<SmsServiceManagerClient>::GetInstance().IsImsSmsSupported(helper.slotId, isSupported);
    helper.SetBoolResult(isSupported);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetImsSmsConfig_0002
 * @tc.name     Disable Ims Sms
 * @tc.desc     Function test
 * @tc.require: issueI5K12U
 */
HWTEST_F(SmsGtest, SetImsSmsConfig_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(SetImsSmsConfigTestFuc2, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0002 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetImsSmsConfig_0003
 * @tc.name     Enable IMS SMS
 * @tc.desc     Function test
 * @tc.require: issueI5K12U
 */
HWTEST_F(SmsGtest, SetImsSmsConfig_0003, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0003 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    helper.Run(SetImsSmsConfigTestFucWithToken, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0003 -->finished");
    ASSERT_TRUE(helper.GetBoolResult());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetImsSmsConfig_0004
 * @tc.name     Disable Ims Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SetImsSmsConfig_0004, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0004 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    helper.Run(SetImsSmsConfigTestFuc2, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0004 -->finished");
    EXPECT_FALSE(helper.GetBoolResult());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SetImsSmsConfig_0005
 * @tc.name     Enable IMS SMS
 * @tc.desc     Function test
 * @tc.require: issueI5K12U
 */
HWTEST_F(SmsGtest, SetImsSmsConfig_0005, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0005 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    helper.Run(SetImsSmsConfigTestFuc, std::ref(helper));
    TELEPHONY_LOGI("TelSMSMMSTest::SetImsSmsConfig_0005 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

void SendDataMessageTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::string dest = DES_ADDR;
    std::u16string simcardNumber;
    if (!CoreServiceClient::GetInstance().GetSimTelephoneNumber(helper.slotId, simcardNumber) &&
        !simcardNumber.empty()) {
        dest = StringUtils::ToUtf8(simcardNumber);
    }

    std::string sca("");
    OHOS::sptr<SmsSendCallbackGTest> sendCallBackPtr(new SmsSendCallbackGTest(helper));
    OHOS::sptr<SmsDeliveryCallbackGTest> deliveryCallBackPtr(new SmsDeliveryCallbackGTest(helper));
    uint16_t port = SMS_PORT;
    if (sendCallBackPtr == nullptr) {
        TELEPHONY_LOGI("sendCallBackPtr is nullptr");
        helper.NotifyAll();
    }

    if (deliveryCallBackPtr == nullptr) {
        TELEPHONY_LOGI("deliveryCallBackPtr is nullptr");
        helper.NotifyAll();
    }
    sendCallBackPtr->HasDeliveryCallBack(true);
    Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(helper.slotId, StringUtils::ToUtf16(dest),
        StringUtils::ToUtf16(sca), port, DATA_SMS, (sizeof(DATA_SMS) / sizeof(DATA_SMS[0]) - 1), sendCallBackPtr,
        deliveryCallBackPtr);
}

void SendDataMessageTestFuc2(SmsMmsTestHelper &helper)
{
    std::u16string simcardNumber;
    std::string dest = DES_ADDR;
    if (!CoreServiceClient::GetInstance().GetSimTelephoneNumber(helper.slotId, simcardNumber) &&
        !simcardNumber.empty()) {
        dest = StringUtils::ToUtf8(simcardNumber);
    }

    std::string sca("");
    OHOS::sptr<SmsSendCallbackGTest> sendCallBackPtr(new SmsSendCallbackGTest(helper));
    OHOS::sptr<SmsDeliveryCallbackGTest> deliveryCallBackPtr(new SmsDeliveryCallbackGTest(helper));
    uint16_t port = SMS_PORT;
    if (sendCallBackPtr == nullptr) {
        TELEPHONY_LOGI("sendCallBackPtr is nullptr");
        helper.NotifyAll();
    }

    if (deliveryCallBackPtr == nullptr) {
        TELEPHONY_LOGI("deliveryCallBackPtr is nullptr");
        helper.NotifyAll();
    }
    sendCallBackPtr->HasDeliveryCallBack(false);
    Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(helper.slotId, StringUtils::ToUtf16(dest),
        StringUtils::ToUtf16(sca), port, DATA_SMS, (sizeof(DATA_SMS) / sizeof(DATA_SMS[0]) - 1), sendCallBackPtr,
        deliveryCallBackPtr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendDataMessage_0001
 * @tc.name     Send Data Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendDataMessage_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0001 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendDataMessageTestFuc, helper, WAIT_TIME_SHORT_SECOND)) {
        TELEPHONY_LOGI("SendDataMessageTestFuc out of time");
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0001 -->finished");
    ASSERT_TRUE(helper.GetSendSmsIntResult() == 0 || helper.GetDeliverySmsIntResult() == 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendDataMessage_0002
 * @tc.name     Send Data Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendDataMessage_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0002 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID_1;
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendDataMessageTestFuc, helper, WAIT_TIME_SHORT_SECOND)) {
        TELEPHONY_LOGI("SendDataMessageTestFuc out of time");
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0002 -->finished");
    ASSERT_TRUE(helper.GetSendSmsIntResult() == 0 || helper.GetDeliverySmsIntResult() == 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendDataMessage_0003
 * @tc.name     Send Data Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendDataMessage_0003, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0003 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendDataMessageTestFuc2, helper)) {
        TELEPHONY_LOGI("SendDataMessageTestFuc out of time");
        ASSERT_TRUE(helper.Run(SendDataMessageTestFuc2, helper));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendDataMessage_0003 -->finished");
    EXPECT_NE(helper.GetSendSmsIntResult(), 0);
}

void SendTextMessageTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::string dest = DES_ADDR;
    std::u16string simcardNumber;
    if (!CoreServiceClient::GetInstance().GetSimTelephoneNumber(helper.slotId, simcardNumber) &&
        !simcardNumber.empty()) {
        dest = StringUtils::ToUtf8(simcardNumber);
    }

    std::string sca("");
    OHOS::sptr<SmsDeliveryCallbackGTest> deliveryCallBackPtr(new SmsDeliveryCallbackGTest(helper));
    OHOS::sptr<SmsSendCallbackGTest> sendCallBackPtr(new SmsSendCallbackGTest(helper));
    std::string text = TEXT_SMS_CONTENT;
    if (sendCallBackPtr == nullptr) {
        TELEPHONY_LOGI("sendCallBackPtr is nullptr");
        helper.NotifyAll();
        return;
    }

    if (deliveryCallBackPtr == nullptr) {
        TELEPHONY_LOGI("deliveryCallBackPtr is nullptr");
        helper.NotifyAll();
        return;
    }
    sendCallBackPtr->HasDeliveryCallBack(true);
    Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(helper.slotId, StringUtils::ToUtf16(dest),
        StringUtils::ToUtf16(sca), StringUtils::ToUtf16(text), sendCallBackPtr, deliveryCallBackPtr);
}

void SendTextMessageTestFuc2(SmsMmsTestHelper &helper)
{
    std::u16string simcardNumber;
    std::string dest = DES_ADDR;
    if (!CoreServiceClient::GetInstance().GetSimTelephoneNumber(helper.slotId, simcardNumber) &&
        !simcardNumber.empty()) {
        dest = StringUtils::ToUtf8(simcardNumber);
    }

    std::string sca("");
    OHOS::sptr<SmsDeliveryCallbackGTest> deliveryCallBackPtr(new SmsDeliveryCallbackGTest(helper));
    OHOS::sptr<SmsSendCallbackGTest> sendCallBackPtr(new SmsSendCallbackGTest(helper));
    std::string text = TEXT_SMS_CONTENT;
    if (sendCallBackPtr == nullptr) {
        TELEPHONY_LOGI("sendCallBackPtr is nullptr");
        helper.NotifyAll();
        return;
    }

    if (deliveryCallBackPtr == nullptr) {
        TELEPHONY_LOGI("deliveryCallBackPtr is nullptr");
        helper.NotifyAll();
        return;
    }
    sendCallBackPtr->HasDeliveryCallBack(false);
    Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(helper.slotId, StringUtils::ToUtf16(dest),
        StringUtils::ToUtf16(sca), StringUtils::ToUtf16(text), sendCallBackPtr, deliveryCallBackPtr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendTextMessage_0001
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendTextMessage_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0001 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendTextMessageTestFuc, helper, WAIT_TIME_SHORT_SECOND)) {
        TELEPHONY_LOGI("SendTextMessageTestFuc out of time");
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0001 -->finished");
    ASSERT_TRUE(helper.GetSendSmsIntResult() == 0 || helper.GetDeliverySmsIntResult() == 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendTextMessage_0002
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendTextMessage_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0002 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID_1;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendTextMessageTestFuc, helper, WAIT_TIME_SHORT_SECOND)) {
        TELEPHONY_LOGI("SendTextMessageTestFuc out of time");
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0001 -->finished");
    ASSERT_TRUE(helper.GetSendSmsIntResult() == 0 || helper.GetDeliverySmsIntResult() == 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendTextMessage_0003
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SendTextMessage_0003, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0003 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(SendTextMessageTestFuc2, helper)) {
        TELEPHONY_LOGI("SendTextMessageTestFuc out of time");
        ASSERT_TRUE(helper.Run(SendTextMessageTestFuc2, helper));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::SendTextMessage_0003 -->finished");
    EXPECT_NE(helper.GetSendSmsIntResult(), 0);
}

void ReceiveSmsTestFunc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    auto message = std::make_shared<SmsMessageInfo>();
    message->indicationType = MESSAGE_TYPE;
    message->size = SMS_PDU_LEN;
    message->pdu =
        StringUtils::HexToByteVector("0891683110808805F0040D91686106571209F800003210921134922307D3F69C5A9ED301");

    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_GSM_SMS, message);
    smsReceiveHandler->ProcessEvent(event);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Receive_SMS_0001
 * @tc.name     Receive a normal Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Receive_SMS_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Receive_SMS_0001 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<SmsBroadcastSubscriberGtest> subscriberTest =
        std::make_shared<SmsBroadcastSubscriberGtest>(subscriberInfo, helper);
    ASSERT_NE(subscriberTest, nullptr);
    bool subscribeResult = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriberTest);
    TELEPHONY_LOGI("subscribeResult is : %{public}d", subscribeResult);

    EXPECT_TRUE(!helper.Run(ReceiveSmsTestFunc, helper) || helper.GetBoolResult());
    TELEPHONY_LOGI("TelSMSMMSTest::Receive_SMS_0001 -->finished");
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Receive_SMS_0002
 * @tc.name     Receive a normal Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Receive_SMS_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Receive_SMS_0002 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID_1;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    std::shared_ptr<SmsBroadcastSubscriberGtest> subscriberTest =
        std::make_shared<SmsBroadcastSubscriberGtest>(subscriberInfo, helper);
    ASSERT_NE(subscriberTest, nullptr);
    bool subscribeResult = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriberTest);
    TELEPHONY_LOGI("subscribeResult is : %{public}d", subscribeResult);

    EXPECT_TRUE(!helper.Run(ReceiveSmsTestFunc, helper) || helper.GetBoolResult());
    TELEPHONY_LOGI("TelSMSMMSTest::Receive_SMS_0002 -->finished");
}

void GetSmsSegmentsInfoTestFuc(SmsMmsTestHelper &helper)
{
    std::u16string message = u"";
    bool force7BitCode = false;
    ISmsServiceInterface::SmsSegmentsInfo result;
    Singleton<SmsServiceManagerClient>::GetInstance().GetSmsSegmentsInfo(
        helper.slotId, message, force7BitCode, result);
    bool isSupported = false;
    Singleton<SmsServiceManagerClient>::GetInstance().IsImsSmsSupported(helper.slotId, isSupported);
    helper.SetBoolResult(isSupported);
    helper.NotifyAll();
}

void GetSmsSegmentsInfoTestFuc2(SmsMmsTestHelper &helper)
{
    std::u16string message = u"message";
    bool force7BitCode = true;
    ISmsServiceInterface::SmsSegmentsInfo result;
    Singleton<SmsServiceManagerClient>::GetInstance().GetSmsSegmentsInfo(
        helper.slotId, message, force7BitCode, result);
    bool isSupported = false;
    Singleton<SmsServiceManagerClient>::GetInstance().IsImsSmsSupported(helper.slotId, isSupported);
    helper.SetBoolResult(isSupported);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetSmsSegmentsInfo_0001
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetSmsSegmentsInfo_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0001 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(GetSmsSegmentsInfoTestFuc, helper)) {
        TELEPHONY_LOGI("GetSmsSegmentsInfoTestFuc out of time");
        ASSERT_TRUE(helper.Run(GetSmsSegmentsInfoTestFuc, helper));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0001 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetSmsSegmentsInfo_0002
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetSmsSegmentsInfo_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0002 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(GetSmsSegmentsInfoTestFuc2, helper)) {
        TELEPHONY_LOGI("GetSmsSegmentsInfoTestFuc2 out of time");
        ASSERT_TRUE(helper.Run(GetSmsSegmentsInfoTestFuc2, helper));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0002 -->finished");
    EXPECT_GE(helper.GetBoolResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetSmsSegmentsInfo_0003
 * @tc.name     Send Text Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, GetSmsSegmentsInfo_0003, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0003 -->");
    int32_t slotId = DEFAULT_SIM_SLOT_ID_1;
    ASSERT_TRUE((SmsGtest::HasSimCard(slotId)));
    SmsMmsTestHelper helper;
    helper.slotId = slotId;
    if (!helper.Run(GetSmsSegmentsInfoTestFuc, helper)) {
        TELEPHONY_LOGI("GetSmsSegmentsInfoTestFuc3 out of time");
        ASSERT_TRUE(helper.Run(GetSmsSegmentsInfoTestFuc, helper));
    }
    TELEPHONY_LOGI("TelSMSMMSTest::GetSmsSegmentsInfo_0003 -->finished");
    EXPECT_FALSE(helper.GetBoolResult());
}

void Create7bitSubmitSmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector("21010B818176251308F4000002C130");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Create7bitSubmitSms_0001
 * @tc.name     Create 7bit Submit Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Create7bitSubmitSms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitSubmitSms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(Create7bitSubmitSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("Create7bitSubmitSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(Create7bitSubmitSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitSubmitSms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Create7bitSubmitSms_0002
 * @tc.name     Create 7bit Submit Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Create7bitSubmitSms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitSubmitSms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(Create7bitSubmitSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("Create7bitSubmitSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(Create7bitSubmitSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitSubmitSms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void CreateUcs2SubmitSmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector("21020B818176251308F40008046D4B8BD5");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateUcs2SubmitSms_0001
 * @tc.name     Create Ucs2 Submit Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateUcs2SubmitSms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2SubmitSms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(CreateUcs2SubmitSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateUcs2SubmitSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateUcs2SubmitSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2SubmitSms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateUcs2SubmitSms_0002
 * @tc.name     Create Ucs2 Submit Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateUcs2SubmitSms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2SubmitSms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(CreateUcs2SubmitSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateUcs2SubmitSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateUcs2SubmitSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2SubmitSms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void Create7bitDeliverySmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu =
        StringUtils::HexToByteVector("0891683108200075F4240D91688129562983F600001240800102142302C130");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Create7bitDeliverySms_0001
 * @tc.name     Create 7bit Delivery Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Create7bitDeliverySms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitDeliverySms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(Create7bitDeliverySmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("Create7bitDeliverySmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(Create7bitDeliverySmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitDeliverySms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Create7bitDeliverySms_0002
 * @tc.name     Create 7bit Delivery Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Create7bitDeliverySms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitDeliverySms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(Create7bitDeliverySmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("Create7bitDeliverySmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(Create7bitDeliverySmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::Create7bitDeliverySms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void CreateUcs2DeliverySmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu =
        StringUtils::HexToByteVector("0891683110206005F0240DA1688176251308F4000832500381459323044F60597D");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateUcs2DeliverySms_0001
 * @tc.name     Create Ucs2 Delivery Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateUcs2DeliverySms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2DeliverySms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(CreateUcs2DeliverySmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateUcs2DeliverySmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateUcs2DeliverySmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2DeliverySms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateUcs2DeliverySms_0002
 * @tc.name     Create Ucs2 Delivery Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateUcs2DeliverySms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2DeliverySms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(CreateUcs2DeliverySmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateUcs2DeliverySmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateUcs2DeliverySmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateUcs2DeliverySms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void CreateStatusReportSmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu =
        StringUtils::HexToByteVector("0891683110808805F006510B818176251308F4325013113382233250131143802300");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateStatusReportSms_0001
 * @tc.name     Create Status Report Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateStatusReportSms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateStatusReportSms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(CreateStatusReportSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateStatusReportSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateStatusReportSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateStatusReportSms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateStatusReportSms_0002
 * @tc.name     Create Status Report Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateStatusReportSms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateStatusReportSms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(CreateStatusReportSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateStatusReportSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateStatusReportSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateStatusReportSms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void CreateMultiPageSmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(
        "0891683110205005F06005A00110F00008325052214182238C050003D3030200310030002E0063006E002F007100410053004B00380020"
        "FF0C4EE5514D6B216708521D6263966476F851738D397528540E5F7154CD60A876846B635E384F7F752830024E2D56FD8054901A004100"
        "5000507545723D65B04F539A8CFF0C70B951FB0020002000680074007400700073003A002F002F0075002E00310030003000310030002"
        "E");

    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateMultiPageSms_0001
 * @tc.name     Create MultiPage Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateMultiPageSms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateMultiPageSms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(CreateMultiPageSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateMultiPageSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateMultiPageSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateMultiPageSms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateMultiPageSms_0002
 * @tc.name     Create MultiPage Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateMultiPageSms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateMultiPageSms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(CreateMultiPageSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateMultiPageSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateMultiPageSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateMultiPageSms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

void CreateWapPushSmsTestFuc(SmsMmsTestHelper &helper)
{
    AccessMmsToken token;
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector("21010B818176251308F4000002C130");
    ShortMessage *message = new ShortMessage();
    if (message == nullptr) {
        helper.SetIntResult(1);
        helper.NotifyAll();
        return;
    }
    int result = ShortMessage::CreateMessage(pdu, u"3gpp", *message);
    helper.SetIntResult(result);
    helper.NotifyAll();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateWapPushSms_0001
 * @tc.name     Create WapPush Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateWapPushSms_0001, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateWapPushSms_0001 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID;
    if (!helper.Run(CreateWapPushSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateWapPushSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateWapPushSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateWapPushSms_0001 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CreateWapPushSms_0002
 * @tc.name     Create WapPush Sms
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, CreateWapPushSms_0002, Function | MediumTest | Level2)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CreateWapPushSms_0002 -->");
    ASSERT_TRUE(SmsGtest::HasSimCard(DEFAULT_SIM_SLOT_ID_1));
    SmsMmsTestHelper helper;
    helper.slotId = DEFAULT_SIM_SLOT_ID_1;
    if (!helper.Run(CreateWapPushSmsTestFuc, std::ref(helper))) {
        TELEPHONY_LOGI("CreateWapPushSmsTestFuc out of time");
        ASSERT_TRUE(helper.Run(CreateWapPushSmsTestFuc, std::ref(helper)));
        return;
    }
    TELEPHONY_LOGI("TelSMSMMSTest::CreateWapPushSms_0002 -->finished");
    ASSERT_EQ(helper.GetIntResult(), 0);
}

HWTEST_F(SmsGtest, Sms_TestDump_0002, TestSize.Level0)
{
    std::string originatingAddress = "";
    bool result = DelayedSingleton<SmsPersistHelper>::GetInstance()->QueryBlockPhoneNumber(originatingAddress);
    ASSERT_FALSE(result);
}

/**
 * @tc.number   Sms_TestDump_0100
 * @tc.name    TestDump
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, Sms_TestDump_0001, TestSize.Level0)
{
    std::vector<std::u16string> emptyArgs = {};
    std::vector<std::u16string> args = { u"test", u"test1" };
    EXPECT_GE(DelayedSingleton<SmsService>::GetInstance()->Dump(-1, args), 0);
    EXPECT_GE(DelayedSingleton<SmsService>::GetInstance()->Dump(0, emptyArgs), 0);
    EXPECT_GE(DelayedSingleton<SmsService>::GetInstance()->Dump(0, args), 0);
}

/**
 * @tc.number   Telephony_SmsGtest_SmsPersistHelper_QueryOneSessionByPhoneNum
 * @tc.name     Test SmsService
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SmsPersistHelper_QueryOneSessionByPhoneNum, Function | MediumTest | Level1)
{
    auto smsPersistHelper = DelayedSingleton<SmsPersistHelper>::GetInstance();
    DataShare::DataSharePredicates predicates;
    std::string phoneNum = "";
    std::string telephone = "13866666666";
    uint16_t sessionId = 0;
    uint16_t messageCount = 0;
    EXPECT_FALSE(smsPersistHelper->QueryOneSessionByPhoneNum(predicates, sessionId, messageCount, phoneNum));
    AccessMmsToken token;
    EXPECT_TRUE(smsPersistHelper->QueryOneSessionByPhoneNum(predicates, sessionId, messageCount, phoneNum));
    phoneNum = "13866666666";
    EXPECT_FALSE(smsPersistHelper->QueryOneSessionByPhoneNum(predicates, sessionId, messageCount, phoneNum));
    smsPersistHelper->IsSameFormatPhoneNumber(phoneNum, telephone,
        i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::NATIONAL);
    smsPersistHelper->IsSameFormatPhoneNumber(phoneNum, telephone,
        i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::E164);
}

/**
 * @tc.number   Telephony_SmsGtest_SmsService_InsertSession
 * @tc.name     Test SmsService
 * @tc.desc     Function test
 */
HWTEST_F(SmsGtest, SmsService_InsertSession, Function | MediumTest | Level1)
{
    auto smsService = DelayedSingleton<SmsService>::GetInstance();
    bool isNewSession = 0;
    uint16_t messageCount = 0;
    const std::string number = "123";
    const std::string text = "qwe";
    EXPECT_FALSE(smsService->InsertSession(isNewSession, messageCount, number, text));
    isNewSession = 1;
    EXPECT_FALSE(smsService->InsertSession(isNewSession, messageCount, number, text));
}

#else // TEL_TEST_UNSUPPORT
#endif // TEL_TEST_UNSUPPORT
} // namespace Telephony
} // namespace OHOS