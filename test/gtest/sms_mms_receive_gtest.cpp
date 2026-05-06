/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "core_manager_inner.h"
#include "data_request.h"
#include "mms_apn_info.h"
#include "mms_receive.h"
#include "mms_receive_manager.h"
#include "mms_sender.h"
#include "mms_send_manager.h"
#include "mms_network_client.h"
#include "mock/mock_sim_manager.h"
#include "sms_persist_helper.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace {
constexpr const char *KEY_MMS_USER_AGENT_PROFILE_STRING = "mms_user_agent_profile_string";
} // namespace

class MmsReceiveGtest : public testing::Test {
public:
    MmsReceiveGtest()
    {
        mockSimManager = new MockSimManager();
        std::shared_ptr<MockSimManager> mockSimManagerPtr(mockSimManager);
        CoreManagerInner::GetInstance().simManager_ = mockSimManagerPtr;
    }
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    MockSimManager *mockSimManager = new MockSimManager();
};
void MmsReceiveGtest::SetUpTestCase() {}

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void MmsReceiveGtest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void MmsReceiveGtest::SetUp() {}

void MmsReceiveGtest::TearDown() {}
/**
 * @tc.number   Telephony_MmsReceiveManagerTest_0001
 * @tc.name     Test MmsReceiveManager
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsReceiveManagerTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsReceiverManager = std::make_shared<MmsReceiveManager>(slotId);
    ASSERT_NE(mmsReceiverManager, nullptr);
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int32_t stata = mmsReceiverManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_EQ(stata, TELEPHONY_ERR_LOCAL_PTR_NULL);

    mmsReceiverManager->Init();
    stata = mmsReceiverManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_EQ(stata, TELEPHONY_ERR_MMS_FAIL_HTTP_ERROR);
    mmsc = u"http://example.com";
    stata = mmsReceiverManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_EQ(stata, TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR);
}

/**
 * @tc.number   Telephony_MmsSendManagerTest_0001
 * @tc.name     Test MmsSendManager
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsSendManagerTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsSendManager = std::make_shared<MmsSendManager>(slotId);
    ASSERT_NE(nullptr, mmsSendManager);
    mmsSendManager->Init();
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int32_t stata = mmsSendManager->SendMms(mmsc, data, ua, uaprof);
    EXPECT_EQ(stata, TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR);
}

/**
 * @tc.number   Telephony_DataRequestTest_0001
 * @tc.name     Test DataRequest
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, DataRequestTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto dataRequest = std::make_shared<DataRequest>(slotId);
    ASSERT_NE(nullptr, dataRequest);
    std::string method;
    auto netMgr = std::make_shared<MmsNetworkManager>();
    ASSERT_NE(nullptr, netMgr);
    std::string contentUrl;
    std::string pduDir;
    int32_t stata = dataRequest->HttpRequest(slotId, method, netMgr, contentUrl, pduDir, "ua", "uaprof");
    EXPECT_NE(stata, TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR);
}

/**
 * @tc.number   Telephony_MmsApnInfoTest_0001
 * @tc.name     Test MmsApnInfo
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsApnInfoTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsApnInfo = std::make_shared<MmsApnInfo>(slotId);
    ASSERT_NE(nullptr, mmsApnInfo);
    std::string apn = "mms,mms,mms,mms";
    EXPECT_TRUE(mmsApnInfo->SplitAndMatchApnTypes(apn));
    apn = "";
    EXPECT_FALSE(mmsApnInfo->SplitAndMatchApnTypes(apn));
}

/**
 * @tc.number   Telephony_MmsNetworkClientTest_0002
 * @tc.name     Test MmsNetworkClient
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsNetworkClientTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsNetworkClient = std::make_shared<MmsNetworkClient>(slotId);
    ASSERT_NE(nullptr, mmsNetworkClient);
    std::string str;
    mmsNetworkClient->GetCoverUrl(str);
    EXPECT_EQ(str, "");
    str = "str";
    mmsNetworkClient->GetCoverUrl(str);
    EXPECT_EQ(str, "str");
}

/**
 * @tc.number   Telephony_MmsReceiveTest_0001
 * @tc.name     Test MmsReceive
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsMmsReceiveTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsReceive = std::make_shared<MmsReceive>(slotId);
    ASSERT_NE(nullptr, mmsReceive);
    std::string method;
    std::string url;
    std::string data;
    std::string uaprof;
    int32_t ret = mmsReceive->ExecuteDownloadMms(method, url, data, uaprof);
    EXPECT_EQ(ret, TELEPHONY_ERR_MMS_FAIL_HTTP_ERROR);
    method = "http://example.com";
    ret = mmsReceive->ExecuteDownloadMms(method, url, data, uaprof);
    EXPECT_EQ(ret, TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR);
}

/**
 * @tc.number   Telephony_MmsSenderTest_0001
 * @tc.name     Test MmsSender
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, MmsSenderTest_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    auto mmsSender = std::make_shared<MmsSender>(slotId);
    ASSERT_NE(nullptr, mmsSender);
    std::string method;
    std::string url;
    std::string data;
    std::string uaprof;
    int32_t ret = mmsSender->ExecuteSendMms(method, url, data, uaprof);
    EXPECT_EQ(ret, TELEPHONY_ERR_MMS_FAIL_DATA_NETWORK_ERROR);
}

/**
 * @tc.number   Telephony_GetDefaultMmsUserAgentProfileTest
 * @tc.name     Test GetMmsUserAgentProfile
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, GetDefaultMmsUserAgentProfileTest, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::string uaProfile = "testUrl";
    auto dataRequest = std::make_shared<DataRequest>(slotId);
    dataRequest->GetMmsUserAgentProfile(uaProfile);
    EXPECT_EQ(uaProfile, "testUrl");
}

/**
 * @tc.number   Telephony_GetCustomMmsUserAgentProfileTest
 * @tc.name     Test GetMmsUserAgentProfile
 * @tc.desc     Function test
 */
HWTEST_F(MmsReceiveGtest, GetCustomMmsUserAgentProfileTest, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::string uaProfile = "testUrl";
    auto dataRequest = std::make_shared<DataRequest>(slotId);
    EXPECT_CALL(*mockSimManager, GetOperatorConfigs(_, _)).Times(AtLeast(1))
        .WillOnce([](int32_t slotId, OperatorConfig &poc) {
            poc.stringValue[KEY_MMS_USER_AGENT_PROFILE_STRING] = "custUrl";
            return 0;
        });
    dataRequest->GetMmsUserAgentProfile(uaProfile);
    EXPECT_EQ(uaProfile, "custUrl");
}
} // namespace Telephony
} // namespace OHOS
