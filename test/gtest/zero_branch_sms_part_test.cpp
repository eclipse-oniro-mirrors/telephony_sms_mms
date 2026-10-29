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

#include <gmock/gmock.h>
#include <random>
#include "cdma_sms_message.h"
#include "delivery_short_message_callback_stub.h"
#include "gtest/gtest.h"
#include "gsm_sms_tpdu_decode.h"
#include "gsm_sms_param_decode.h"
#include "gsm_sms_tpdu_encode.h"
#include "gsm_user_data_pdu.h"
#include "mock/mock_data_share_helper.h"
#include "mock/mock_data_share_result_set.h"
#include "radio_event.h"
#include "send_short_message_callback_stub.h"
#include "sms_broadcast_subscriber_receiver.h"
#include "sms_hisysevent.h"
#include "sms_misc_manager.h"
#include "sms_mms_gtest.h"
#include "sms_network_policy_manager.h"
#include "sms_service.h"
#include "sms_persist_helper.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

namespace {
const std::string TEST_SOURCE_DATA = "srcData";
const std::string SMS_ADDR_ONE = "";
const std::string SMS_ADDR_TWO = " ";
const std::string SMS_ADDR_THREE = " 123 ";
const std::string SMS_ADDR_FOUR = "1 23";
const std::string SMS_ADDR_FIVE = " a 123 ";
const std::string SEND_MMS_FILE_URL = "1,2,";
const std::string VNET_MMSC = "http://mmsc.vnet.mobi";
static constexpr uint8_t MAX_GSM_7BIT_DATA_LEN = 160;
static constexpr uint8_t MAX_SMSC_LEN = 20;
} // namespace

class BranchSmsPartTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};
void BranchSmsPartTest::SetUpTestCase() {}

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void BranchSmsPartTest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void BranchSmsPartTest::SetUp() {}

void BranchSmsPartTest::TearDown() {}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceStub_0001
 * @tc.name     Test SmsInterfaceStub
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceStub_0001, Function | MediumTest | Level1)
{
    AccessMmsToken token;
    int32_t slotId = 0;
    MessageParcel dataParcel;
    dataParcel.WriteInt32(slotId);
    dataParcel.WriteBuffer(TEST_SOURCE_DATA.c_str(), TEST_SOURCE_DATA.size());
    dataParcel.RewindRead(0);
    MessageParcel replyParcel;
    uint32_t code = 0;
    MessageOption option(MessageOption::TF_SYNC);
    DelayedSingleton<SmsService>::GetInstance()->OnRemoteRequest(code, dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsTextRequest(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsTextWithoutSaveRequest(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsDataRequest(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmscAddr(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnAddSimMessage(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnUpdateSimMessage(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetAllSimMessages(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnSplitMessage(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnCreateMessage(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetBase64Encode(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetBase64Decode(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetEncodeStringFunc(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmsSegmentsInfo(dataParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmsShortCodeType(dataParcel, replyParcel, option);

    MessageParcel sendTextParcel;
    sendTextParcel.WriteInt32(0);
    sendTextParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    sendTextParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    sendTextParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    sendTextParcel.WriteString(TEST_SOURCE_DATA);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsTextRequest(sendTextParcel, replyParcel, option);
    sendTextParcel.WriteInt32(-1);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsTextRequest(sendTextParcel, replyParcel, option);
    int32_t result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel sendDataParcel;
    dataParcel.WriteInt32(0);
    dataParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    dataParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    dataParcel.WriteInt16(0);
    dataParcel.WriteInt16(TEST_SOURCE_DATA.size());
    dataParcel.WriteRawData(TEST_SOURCE_DATA.c_str(), TEST_SOURCE_DATA.size());
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsDataRequest(dataParcel, replyParcel, option);
    sendTextParcel.WriteInt32(-1);
    DelayedSingleton<SmsService>::GetInstance()->OnSendSmsDataRequest(dataParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, -1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceStub_0002
 * @tc.name     Test SmsInterfaceStub
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceStub_0002, Function | MediumTest | Level1)
{
    std::u16string desAddr = Str8ToStr16(SMS_ADDR_ONE);
    DelayedSingleton<SmsService>::GetInstance()->RemoveSpacesInDesAddr(desAddr);
    EXPECT_GE(desAddr.size(), 0);
    desAddr = Str8ToStr16(SMS_ADDR_TWO);
    DelayedSingleton<SmsService>::GetInstance()->RemoveSpacesInDesAddr(desAddr);
    EXPECT_GE(desAddr.size(), 0);
    desAddr = Str8ToStr16(SMS_ADDR_THREE);
    DelayedSingleton<SmsService>::GetInstance()->RemoveSpacesInDesAddr(desAddr);
    EXPECT_GE(desAddr.size(), 0);
    desAddr = Str8ToStr16(SMS_ADDR_FOUR);
    DelayedSingleton<SmsService>::GetInstance()->RemoveSpacesInDesAddr(desAddr);
    EXPECT_GE(desAddr.size(), 0);
    desAddr = Str8ToStr16(SMS_ADDR_FIVE);
    DelayedSingleton<SmsService>::GetInstance()->RemoveSpacesInDesAddr(desAddr);
    EXPECT_GE(desAddr.size(), 0);

    MessageParcel setSmscParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    setSmscParcel.WriteInt32(0);
    setSmscParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    DelayedSingleton<SmsService>::GetInstance()->OnSetSmscAddr(setSmscParcel, replyParcel, option);
    MessageParcel getSmscParcel;
    getSmscParcel.WriteInt32(0);
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmscAddr(getSmscParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetAllSimMessages(getSmscParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnSetDefaultSmsSlotId(getSmscParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetDefaultSmsSlotId(getSmscParcel, replyParcel, option);
    int32_t result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel addSimMsgParcel;
    addSimMsgParcel.WriteInt32(0);
    addSimMsgParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    addSimMsgParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    addSimMsgParcel.WriteUint32(0);
    DelayedSingleton<SmsService>::GetInstance()->OnAddSimMessage(addSimMsgParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel delSimMsgParcel;
    delSimMsgParcel.WriteInt32(0);
    delSimMsgParcel.WriteUint32(0);
    DelayedSingleton<SmsService>::GetInstance()->OnDelSimMessage(delSimMsgParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceStub_0003
 * @tc.name     Test SmsInterfaceStub
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceStub_0003, Function | MediumTest | Level1)
{
    MessageParcel simMsgParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    simMsgParcel.WriteInt32(0);
    simMsgParcel.WriteUint32(0);
    simMsgParcel.WriteUint32(0);
    simMsgParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    simMsgParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    DelayedSingleton<SmsService>::GetInstance()->OnUpdateSimMessage(simMsgParcel, replyParcel, option);
    int32_t result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel SetImsSmsParcel;
    SetImsSmsParcel.WriteInt32(0);
    SetImsSmsParcel.WriteInt32(false);
    DelayedSingleton<SmsService>::GetInstance()->OnSetImsSmsConfig(SetImsSmsParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel splitMsgParcel;
    splitMsgParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    DelayedSingleton<SmsService>::GetInstance()->OnSplitMessage(splitMsgParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetBase64Encode(splitMsgParcel, replyParcel, option);
    DelayedSingleton<SmsService>::GetInstance()->OnGetBase64Decode(splitMsgParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel createMsgParcel;
    createMsgParcel.WriteString(TEST_SOURCE_DATA);
    createMsgParcel.WriteString(TEST_SOURCE_DATA);
    DelayedSingleton<SmsService>::GetInstance()->OnCreateMessage(createMsgParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel segmentParcel;
    segmentParcel.WriteInt32(0);
    segmentParcel.WriteString16(Str8ToStr16(TEST_SOURCE_DATA));
    segmentParcel.WriteBool(false);
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmsSegmentsInfo(segmentParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);

    MessageParcel formatParcel;
    DelayedSingleton<SmsService>::GetInstance()->OnGetImsShortMessageFormat(formatParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    DelayedSingleton<SmsService>::GetInstance()->OnGetDefaultSmsSimId(formatParcel, replyParcel, option);
    result = replyParcel.ReadInt32();
    EXPECT_GE(result, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceStub_0004
 * @tc.name     Test SmsInterfaceStub_GetSmsShortCodeType
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceStub_GetSmsShortCodeType, TestSize.Level0)
{
    MessageParcel ShortCodeParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    ShortCodeParcel.WriteInt32(0);
    ShortCodeParcel.WriteString("");
    DelayedSingleton<SmsService>::GetInstance()->OnGetSmsShortCodeType(ShortCodeParcel, replyParcel, option);
    int32_t result = replyParcel.ReadInt32();
    EXPECT_EQ(result, -1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceStub_0004
 * @tc.name     Test SmsInterfaceStub
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceStub_0004, Function | MediumTest | Level1)
{
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);
    MessageParcel cbConfigListMessage;
    cbConfigListMessage.WriteInt32(1);
    cbConfigListMessage.WriteInt32(257);
    replyParcel.WriteInt32(111);
    DelayedSingleton<SmsService>::GetInstance()->OnSetCBConfigList(cbConfigListMessage, replyParcel, option);
    EXPECT_GE(replyParcel.ReadInt32(), 111);
}
/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceManager_0001
 * @tc.name     Test SmsInterfaceManager
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceManager_0001, Function | MediumTest | Level1)
{
    auto interfaceManager = std::make_shared<SmsInterfaceManager>(0);
    ASSERT_NE(interfaceManager, nullptr);
    interfaceManager->HasSmsCapability();
    std::string dataStr = TEST_SOURCE_DATA;
    auto callStub = iface_cast<ISendShortMessageCallback>(new SendShortMessageCallbackStub());
    auto deliveryStub = iface_cast<IDeliveryShortMessageCallback>(new DeliveryShortMessageCallbackStub());
    std::string emptyStr = "";
    int32_t dataBaseId = 0;
    interfaceManager->TextBasedSmsDelivery(dataStr, dataStr, dataStr, callStub, deliveryStub, dataBaseId);
    interfaceManager->TextBasedSmsDelivery(dataStr, emptyStr, dataStr, callStub, deliveryStub, dataBaseId);
    interfaceManager->TextBasedSmsDelivery(dataStr, dataStr, emptyStr, callStub, deliveryStub, dataBaseId);
    int32_t result = interfaceManager->TextBasedSmsDelivery(dataStr, emptyStr, emptyStr,
        callStub, deliveryStub, dataBaseId);
    EXPECT_GE(result, 0);

    const uint8_t *data = reinterpret_cast<const uint8_t *>(dataStr.c_str());
    interfaceManager->DataBasedSmsDelivery(dataStr, dataStr, 0, data, dataStr.size(), callStub, deliveryStub);
    interfaceManager->DataBasedSmsDelivery(dataStr, emptyStr, 0, data, dataStr.size(), callStub, deliveryStub);
    interfaceManager->DataBasedSmsDelivery(dataStr, dataStr, 0, nullptr, 0, callStub, deliveryStub);
    interfaceManager->smsSendManager_ = nullptr;
    interfaceManager->TextBasedSmsDelivery(dataStr, dataStr, dataStr, callStub, deliveryStub, dataBaseId);
    result = interfaceManager->DataBasedSmsDelivery(dataStr, dataStr, 0, data, dataStr.size(), callStub, deliveryStub);
    EXPECT_GE(result, 0);

    interfaceManager->AddSimMessage(dataStr, dataStr, ISmsServiceInterface::SIM_MESSAGE_STATUS_UNREAD);
    interfaceManager->UpdateSimMessage(0, ISmsServiceInterface::SIM_MESSAGE_STATUS_UNREAD, dataStr, dataStr);
    std::vector<ShortMessage> message;
    interfaceManager->GetAllSimMessages(message);
    result = interfaceManager->SetSmscAddr(dataStr);
    EXPECT_GE(result, 0);

    std::u16string smsc;
    interfaceManager->GetSmscAddr(smsc);
    interfaceManager->SetCBConfig(false, 0, 1, 0);
    std::vector<int32_t> messageIds1 = {4352, 4355, 4388, 4390};
    interfaceManager->SetCBConfigList(messageIds1, 1);
    std::vector<int32_t> messageIds2 = {4352, 4388, 4390};
    interfaceManager->SetCBConfigList(messageIds2, 1);
    result = interfaceManager->SetImsSmsConfig(0, false);
    EXPECT_GE(result, 0);

    interfaceManager->SetDefaultSmsSlotId(0);
    interfaceManager->GetDefaultSmsSlotId();
    int32_t simId;
    interfaceManager->GetDefaultSmsSimId(simId);
    std::vector<std::u16string> splitMessage;
    result = interfaceManager->SplitMessage(dataStr, splitMessage);
    EXPECT_GE(result, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceManager_0002
 * @tc.name     Test SmsInterfaceManager
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceManager_0002, Function | MediumTest | Level1)
{
    auto interfaceManager = std::make_shared<SmsInterfaceManager>(0);
    ASSERT_NE(interfaceManager, nullptr);
    LengthInfo outInfo;
    std::string dataStr = TEST_SOURCE_DATA;
    interfaceManager->GetSmsSegmentsInfo(dataStr, false, outInfo);
    bool isSupported = false;
    interfaceManager->IsImsSmsSupported(0, isSupported);
    std::u16string format;
    interfaceManager->GetImsShortMessageFormat(format);
    int32_t result = interfaceManager->HasSmsCapability();
    EXPECT_GE(result, 0);

    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    
    interfaceManager->smsMiscManager_ = nullptr;
    interfaceManager->AddSimMessage(dataStr, dataStr, ISmsServiceInterface::SIM_MESSAGE_STATUS_UNREAD);
    interfaceManager->UpdateSimMessage(0, ISmsServiceInterface::SIM_MESSAGE_STATUS_UNREAD, dataStr, dataStr);
    std::vector<ShortMessage> message;
    interfaceManager->GetAllSimMessages(message);
    interfaceManager->SetSmscAddr(dataStr);
    std::u16string smsc;
    result = interfaceManager->GetSmscAddr(mmsc);
    EXPECT_GE(result, 0);

    interfaceManager->SetCBConfig(false, 0, 1, 0);
    std::vector<int32_t> messageIds1 = {4352, 4355, 4388, 4390};
    interfaceManager->SetCBConfigList(messageIds1, 1);
    interfaceManager->SetImsSmsConfig(0, false);
    interfaceManager->SetDefaultSmsSlotId(0);
    interfaceManager->GetDefaultSmsSlotId();
    int32_t simId;
    result = interfaceManager->GetDefaultSmsSimId(simId);
    EXPECT_GE(result, 0);

    std::vector<std::u16string> splitMessage;
    interfaceManager->SplitMessage(dataStr, splitMessage);
    interfaceManager->GetSmsSegmentsInfo(dataStr, false, outInfo);
    result = interfaceManager->IsImsSmsSupported(0, isSupported);
    EXPECT_GE(result, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceManager_0003
 * @tc.name     Test SmsInterfaceManager
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceManager_0003, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
#ifdef SMS_SUPPORT_MMS
    interfaceManager->mmsSendManager_ = nullptr;
    std::u16string mmsc(StringUtils::ToUtf16(VNET_MMSC));
    std::u16string data(StringUtils::ToUtf16(SEND_MMS_FILE_URL));
    std::u16string ua = u"112233";
    std::u16string uaprof = u"112233";
    int32_t sendMmsRet = interfaceManager->SendMms(mmsc, data, ua, uaprof);
    interfaceManager->mmsReceiverManager_ = nullptr;
    int32_t downloadMmsRet = interfaceManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_GE(sendMmsRet, 0);
    EXPECT_GE(downloadMmsRet, 0);
#endif

    if (interfaceManager->smsSendManager_ == nullptr) {
        interfaceManager->smsSendManager_ = std::make_unique<SmsSendManager>(0);
    }
    interfaceManager->smsSendManager_->Init();
    interfaceManager->smsSendManager_->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_GSM;
    std::string specialText =
        "ฏ็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็็";
    std::vector<std::u16string> splitMessage;
    interfaceManager->SplitMessage(specialText, splitMessage);
    size_t segCount = 2;
    size_t maxSegLen = 67;
    ASSERT_EQ(splitMessage.size(), segCount);
    EXPECT_EQ(splitMessage[0].size(), maxSegLen);

    std::string normalText =
        "这是一个普通的长短信，由数字0123456789、小写字母abcdefghijklmnopqrstuvwxyz、大写字母ABCDEFGHIJKLMNOPQRSTUVWXYZ和汉字组成";
    splitMessage.clear();
    interfaceManager->SplitMessage(normalText, splitMessage);
    ASSERT_EQ(splitMessage.size(), segCount);
    EXPECT_EQ(splitMessage[0].size(), maxSegLen);

    std::string randomText;
    size_t randomLength = 500;
    randomText.reserve(randomLength);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 0xFF);
    for (size_t i = 0; i < randomLength; i++) {
        randomText += static_cast<char>(dis(gen));
    }
    splitMessage.clear();
    interfaceManager->SplitMessage(randomText, splitMessage);
    EXPECT_GE(splitMessage.size(), 1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceManager_0004
 * @tc.name     Test SmsInterfaceManager
 * @tc.desc     Function test
 */
#ifdef SMS_SUPPORT_MMS
HWTEST_F(BranchSmsPartTest, SmsInterfaceManager_0004, Function | MediumTest | Level1)
{
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int32_t slotId = 0;
    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);

    interfaceManager->SendMms(mmsc, data, ua, uaprof);
    result = interfaceManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_GE(result, 0);
    interfaceManager->GetImsShortMessageFormat(format);
    interfaceManager->mmsSendManager_ = nullptr;
    interfaceManager->SendMms(mmsc, data, ua, uaprof);
    interfaceManager->mmsReceiverManager_ = nullptr;
    result = interfaceManager->DownloadMms(mmsc, data, ua, uaprof);
    EXPECT_GE(result, 0);
}
#endif

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsInterfaceManager_0005
 * @tc.name     Test SmsInterfaceManager_GetSmsShortCodeType
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsInterfaceManager_GetSmsShortCodeType, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::string desAddr = "12345";
    int32_t smsShortCodeType = -1;
    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    interfaceManager->smsSendManager_ = nullptr;
    int32_t result = interfaceManager->GetSmsShortCodeType(slotId, desAddr, smsShortCodeType);
    EXPECT_EQ(result, TELEPHONY_ERR_LOCAL_PTR_NULL);
    interfaceManager->smsSendManager_ = std::make_unique<SmsSendManager>(slotId);
    interfaceManager->smsSendManager_->smsShortCodeMatcher_ = std::make_shared<SmsShortCodeMatcher>();
    result = interfaceManager->GetSmsShortCodeType(slotId, desAddr, smsShortCodeType);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(smsShortCodeType, 0);
    desAddr = "10660";
    result = interfaceManager->GetSmsShortCodeType(slotId, desAddr, smsShortCodeType);
    EXPECT_EQ(result, TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(smsShortCodeType, 1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsEmailMessage_0001
 * @tc.name     Test SmsEmailMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsEmailMessage_0001, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    string pdu = ("07963258974152f204038105f300008022110232438a28e6a71b40c587eb7"
                  "076d9357eb7412f7d793a07ddeb6278794d07bde8d5391b246e93d3");
    EXPECT_TRUE(gsmSmsMessage->CreateMessage(pdu) != nullptr);
    auto message = gsmSmsMessage->CreateMessage(pdu);
    EXPECT_EQ(message->GetEmailMessageBody(), "zeSs wubpekt /tUsl bmdi");
    EXPECT_EQ(message->GetVisibleMessageBody(), "zeSs wubpekt /tUsl bmdi");
    EXPECT_EQ(message->GetEmailAddress(), "fOn@Txauple.com");
    EXPECT_EQ(message->GetVisibleOriginatingAddress(), "fOn@Txauple.com");
    EXPECT_TRUE(message->IsEmail());

    pdu = ("07913244457812f204038105f400007120103215358a29d6e71b503667db7076d935"
           "7eb741af0d0a442fcfe8c13639bfe16d289bdee6b5f1813629");
    EXPECT_TRUE(gsmSmsMessage->CreateMessage(pdu) != nullptr);
    message = gsmSmsMessage->CreateMessage(pdu);
    EXPECT_EQ(message->GetEmailMessageBody(), "{ te3tAmdy[^~\\] }");
    EXPECT_EQ(message->GetVisibleMessageBody(), "{ te3tAmdy[^~\\] }");
    EXPECT_EQ(message->GetEmailAddress(), "VOo@efYmple.com");
    EXPECT_EQ(message->GetVisibleOriginatingAddress(), "VOo@efYmple.com");
    EXPECT_TRUE(message->IsEmail());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsEmailMessage_0002
 * @tc.name     Test SmsEmailMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsEmailMessage_0002, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    string pdu = ("07963258974152f204038105f300008022110232438a28e6a71b40c587eb"
                  "7076d9357eb7412f7d793a07ddeb6278794d07bde8d5391b246e93d3");
    EXPECT_TRUE(gsmSmsMessage->CreateMessage(pdu) != nullptr);
    auto message = gsmSmsMessage->CreateMessage(pdu);
    EXPECT_TRUE(message != nullptr);

    EXPECT_TRUE(message->IsEmailAddress("\"DAS\" <NAME@sadds.com>"));
    EXPECT_TRUE(message->IsEmailAddress("DAS <NAME@sadds.com>"));
    EXPECT_FALSE(message->IsEmailAddress(""));
    EXPECT_FALSE(message->IsEmailAddress("\"DAS\" 45654654564"));
    EXPECT_FALSE(message->IsEmailAddress("DAS 13254654654"));
    EXPECT_TRUE(message->IsEmailAddress("阿松大@163.com"));
    EXPECT_TRUE(message->IsEmailAddress("aaa@aa.bb.163.com"));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsEmailMessage_0003
 * @tc.name     Test SmsEmailMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsEmailMessage_0003, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    string pdu = ("07963258974152f2040D91687103920059F100008022110232438a28e6a71b40c587e"
           "b7076d9357eb7412f7d793a07ddeb6278794d07bde8d5391b246e93d3");
    EXPECT_TRUE(gsmSmsMessage->CreateMessage(pdu) != nullptr);
    auto message = gsmSmsMessage->CreateMessage(pdu);
    EXPECT_FALSE(message->IsEmail());

    pdu = ("07963258974152f204038105f300008022110232438a10e6a71b40c587eb7076d9357eb743");
    EXPECT_TRUE(gsmSmsMessage->CreateMessage(pdu) != nullptr);
    message = gsmSmsMessage->CreateMessage(pdu);
    EXPECT_FALSE(message->IsEmail());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsStateObserver_0001
 * @tc.name     Test SmsStateObserver_0001
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsStateObserver_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsStateObserver> smsStateObserver = std::make_shared<SmsStateObserver>();
    smsStateObserver->smsSubscriber_ = nullptr;
    smsStateObserver->StopEventSubscriber();
    std::shared_ptr<CommonEventSubscribeInfo> subscribeInfo = std::make_shared<CommonEventSubscribeInfo>();
    std::shared_ptr<SmsStateEventSubscriber> smsStateEventSubscriber =
        std::make_shared<SmsStateEventSubscriber>(*subscribeInfo, *smsStateObserver);
    EventFwk::CommonEventData eventData = EventFwk::CommonEventData();
    AAFwk::Want want = AAFwk::Want();
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_EMERGENCY_CB_RECEIVE_COMPLETED);
    eventData.SetWant(want);
    smsStateEventSubscriber->OnReceiveEvent(eventData);
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    eventData.SetWant(want);
    smsStateEventSubscriber->OnReceiveEvent(eventData);
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED);
    eventData.SetWant(want);
    smsStateEventSubscriber->OnReceiveEvent(eventData);
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_LOW);
    eventData.SetWant(want);
    smsStateEventSubscriber->OnReceiveEvent(eventData);
#ifdef BASE_POWER_IMPROVEMENT_FEATURE
    want.SetAction(EXIT_STR_TELEPHONY_NOTIFY);
    eventData.SetWant(want);
    eventData.SetCode(6);
    smsStateEventSubscriber->InitEventMap();
    smsStateEventSubscriber->OnReceiveEvent(eventData);
#endif
    EXPECT_TRUE(smsStateObserver != nullptr);
    EXPECT_TRUE(subscribeInfo != nullptr);
    EXPECT_TRUE(smsStateEventSubscriber != nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamCodec_0014
 * @tc.name     Test GsmSmsParamCodec DecodeSmscPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamCodec_0014, Function | MediumTest | Level1) {
    /**
    * test supporting smsAddress.ton is unknown
    */
    auto gsmSmsParamCodec = std::make_shared<GsmSmsParamCodec>();
    EXPECT_NE(gsmSmsParamCodec, nullptr);
    AddressNumber smsAddress;

    unsigned char encodeData[] = { 0x2, 0x81, 0x2A, 0xB1 };
    unsigned char *pSMSC = encodeData;
    EXPECT_GE(gsmSmsParamCodec->DecodeSmscPdu(pSMSC, 5, smsAddress), 0);
    std::string address(smsAddress.address);
    EXPECT_EQ(address, "*21#");
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamCodec_0015
 * @tc.name     Test GsmSmsParamCodec DecodeDcsPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamCodec_0015, Function | MediumTest | Level1) {
    /**
    * test DecodeDcsPdu
    */
    auto gsmSmsParamCodec = std::make_shared<GsmSmsParamCodec>();
    EXPECT_NE(gsmSmsParamCodec, nullptr);

    auto decodeBuffer = std::make_shared<SmsReadBuffer>("0011000D91685150800576F70001C404D4F29C0E");
    SmsDcs *smsDcs = new SmsDcs();
    smsDcs->codingGroup = OHOS::Telephony::PduSchemeGroup::CODING_GENERAL_GROUP;
    smsDcs->bCompressed = false;
    smsDcs->codingScheme = OHOS::Telephony::DataCodingScheme::DATA_CODING_UCS2;
    smsDcs->msgClass = OHOS::Telephony::SmsMessageClass::SMS_INSTANT_MESSAGE;

    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer, smsDcs), 1);
    auto decodeBuffer1 = std::make_shared<SmsReadBuffer>("16D131D98C56B3DD7039584C36A3D56C375C0E169301");
    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer1, smsDcs), 1);
    auto decodeBuffer2 = std::make_shared<SmsReadBuffer>("4011000D91685150800576F70001C404D4F29C0E");
    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer2, smsDcs), 1);
    auto decodeBuffer3 = std::make_shared<SmsReadBuffer>("C011000D91685150800576F70001C404D4F29C0E");
    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer3, smsDcs), 1);
    auto decodeBuffer4 = std::make_shared<SmsReadBuffer>("D011000D91685150800576F70001C404D4F29C0E");
    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer4, smsDcs), 1);
    auto decodeBuffer5 = std::make_shared<SmsReadBuffer>("B011000D91685150800576F70001C404D4F29C0E");
    EXPECT_EQ(gsmSmsParamCodec->DecodeDcsPdu(*decodeBuffer5, smsDcs), 1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0003
 * @tc.name     Test GsmSmsParamDecode DecodeDcsGeneralGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0003, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsGeneralGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsGeneralGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->msgClass, OHOS::Telephony::SMS_CLASS_UNKNOWN);
    dcs = 0x15;
    gsmSmsParamDecode->DecodeDcsGeneralGroupPdu(dcs, smsDcs1);
    EXPECT_NE(smsDcs1->msgClass, OHOS::Telephony::SMS_CLASS_UNKNOWN);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0004
 * @tc.name     Test GsmSmsParamDecode DecodeDcsClassGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0004, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsClassGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsClassGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::SMS_CLASS_GROUP);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0005
 * @tc.name     Test GsmSmsParamDecode DecodeDcsDeleteGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0005, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsDeleteGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsDeleteGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::CODING_DELETION_GROUP);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0006
 * @tc.name     Test GsmSmsParamDecode DecodeDcsDiscardGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0006, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsDiscardGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsDiscardGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::CODING_DISCARD_GROUP);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0007
 * @tc.name     Test GsmSmsParamDecode DecodeDcsStoreGsmGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0007, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsStoreGsmGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsStoreGsmGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::CODING_STORE_GROUP);
    EXPECT_EQ(smsDcs1->codingScheme, OHOS::Telephony::DATA_CODING_7BIT);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0008
 * @tc.name     Test GsmSmsParamDecode DecodeDcsStoreUCS2GroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0008, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsStoreUCS2GroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsStoreUCS2GroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::CODING_STORE_GROUP);
    EXPECT_EQ(smsDcs1->codingScheme, OHOS::Telephony::DATA_CODING_UCS2);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0009
 * @tc.name     Test GsmSmsParamDecode DecodeDcsUnknownGroupPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0009, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    uint8_t dcs = 0x08;
    SmsDcs *smsDcs = nullptr;
    gsmSmsParamDecode->DecodeDcsUnknownGroupPdu(dcs, smsDcs);
    EXPECT_EQ(smsDcs, nullptr);
    SmsDcs *smsDcs1 = new SmsDcs();
    gsmSmsParamDecode->DecodeDcsUnknownGroupPdu(dcs, smsDcs1);
    EXPECT_EQ(smsDcs1->codingGroup, OHOS::Telephony::CODING_UNKNOWN_GROUP);
    EXPECT_EQ(smsDcs1->msgClass, OHOS::Telephony::SMS_CLASS_UNKNOWN);
}


/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0010
 * @tc.name     Test GsmSmsParamDecode DecodeTimePduPartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0010, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    auto decodeBuffer = std::make_shared<SmsReadBuffer>("");
    EXPECT_NE(decodeBuffer, nullptr);
    EXPECT_TRUE(decodeBuffer->IsEmpty());
    SmsTimeStamp *pTimeStamp = new SmsTimeStamp();
    pTimeStamp->format = SmsTimeFormat::SMS_TIME_ABSOLUTE;
    pTimeStamp->time.absolute.timeZone = -1;
    EXPECT_EQ(gsmSmsParamDecode->DecodeTimePduPartData(*decodeBuffer, pTimeStamp), false); // first branch PickOneByte

    auto decodeBuffer1 = std::make_shared<SmsReadBuffer>("00");
    EXPECT_NE(decodeBuffer1, nullptr);
    EXPECT_FALSE(decodeBuffer1->IsEmpty());
    decodeBuffer1->bitIndex_ = 1;
    EXPECT_EQ(gsmSmsParamDecode->DecodeTimePduPartData(*decodeBuffer1, pTimeStamp), false); // fifth branch ReadByte
    EXPECT_EQ(gsmSmsParamDecode->DecodeTimePduPartData(*decodeBuffer1, pTimeStamp), false); // third branch ReadByte
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamDecode_0011
 * @tc.name     Test GsmSmsParamDecode DecodeTimePduData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamDecode_0011, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    auto decodeBuffer = std::make_shared<SmsReadBuffer>("");
    EXPECT_NE(decodeBuffer, nullptr);
    EXPECT_FALSE(gsmSmsParamDecode->DecodeTimePduData(*decodeBuffer, nullptr)); // first branch nullptr

    SmsTimeStamp *pTimeStamp = new SmsTimeStamp();
    pTimeStamp->format = SmsTimeFormat::SMS_TIME_ABSOLUTE;
    pTimeStamp->time.absolute.timeZone = -1;
    EXPECT_FALSE(gsmSmsParamDecode->DecodeTimePduData(*decodeBuffer, pTimeStamp)); // second branch PickOneByte

    auto decodeBuffer1 = std::make_shared<SmsReadBuffer>("00");
    EXPECT_NE(decodeBuffer1, nullptr);
    EXPECT_TRUE(gsmSmsParamDecode->DecodeTimePduData(*decodeBuffer1, pTimeStamp));
    EXPECT_FALSE(gsmSmsParamDecode->DecodeTimePduData(*decodeBuffer1, pTimeStamp)); // second branch PickOneByte

    auto decodeBuffer2 = std::make_shared<SmsReadBuffer>("16D131D98C56B3DD7039584C36A3D56C375C0E169301");
    EXPECT_NE(decodeBuffer2, nullptr);
    EXPECT_TRUE(gsmSmsParamDecode->DecodeTimePduData(*decodeBuffer2, pTimeStamp));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DecodeAddressAlphaNum_0001
 * @tc.name     Test GsmSmsParamDecode DecodeAddressAlphaNum
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, DecodeAddressAlphaNum_0001, Function | MediumTest | Level1)
{
    auto gsmSmsParamDecode = std::make_shared<GsmSmsParamDecode>();
    EXPECT_NE(gsmSmsParamDecode, nullptr);
    auto buffer = std::make_shared<SmsReadBuffer>("00");
    AddressNumber *pAddress = new AddressNumber();
    uint8_t bcdLen = 1;
    uint8_t addrLen = 1;
    EXPECT_FALSE(gsmSmsParamDecode->DecodeAddressAlphaNum(*buffer, pAddress, bcdLen, addrLen)); // third branch

    std::string pdu = StringUtils::HexToString("");
    auto decodeBuffer1 = std::make_shared<SmsReadBuffer>(pdu);
    printf("decodeBuffer1 index_ = %d, length_ = %d\n", decodeBuffer1->index_, decodeBuffer1->length_);
    EXPECT_FALSE(gsmSmsParamDecode->DecodeAddressAlphaNum(*buffer, pAddress, bcdLen, addrLen)); // first branch
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0004
 * @tc.name     Test GsmSmsMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0004, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    unsigned char langId = 0;
    gsmSmsMessage->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_STATUS_REP;
    EXPECT_EQ(gsmSmsMessage->SetHeaderLang(1, DataCodingScheme::DATA_CODING_UCS2, langId), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0005
 * @tc.name     Test GsmSmsMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0005, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    SmsConcat concat;
    gsmSmsMessage->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_STATUS_REP;
    EXPECT_EQ(gsmSmsMessage->SetHeaderConcat(1, concat), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0006
 * @tc.name     Test GsmSmsMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0006, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    gsmSmsMessage->smsTpdu_ = nullptr;
    EXPECT_EQ(gsmSmsMessage->SetHeaderReply(1), 0);

    gsmSmsMessage->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    gsmSmsMessage->replyAddress_ = "+13588421254";
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_STATUS_REP;
    EXPECT_EQ(gsmSmsMessage->SetHeaderReply(1), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0007
 * @tc.name     Test GsmSmsMessage CreateDefaultSubmit
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0007, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    gsmSmsMessage->CreateDefaultSubmit(false, DataCodingScheme::DATA_CODING_7BIT);
    EXPECT_EQ(gsmSmsMessage->smsTpdu_->tpduType, OHOS::Telephony::SmsTpduType::SMS_TPDU_SUBMIT);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0008
 * @tc.name     Test GsmSmsMessage PduAnalysisMsg
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0008, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    gsmSmsMessage->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_STATUS_REP;
    // also test AnalysisMsgDeliver
    EXPECT_TRUE(gsmSmsMessage->PduAnalysisMsg());
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_SUBMIT;
    // alse test AnalysisMsgSubmit
    EXPECT_TRUE(gsmSmsMessage->PduAnalysisMsg());
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_DELIVER_REP;
    EXPECT_FALSE(gsmSmsMessage->PduAnalysisMsg());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0009
 * @tc.name     Test GsmSmsMessage ConvertUserData and ConvertUserPartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0009, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);

    gsmSmsMessage->smsTpdu_ = std::make_shared<struct SmsTpdu>();
    gsmSmsMessage->smsTpdu_->tpduType = OHOS::Telephony::SmsTpduType::SMS_TPDU_DELIVER;
    gsmSmsMessage->codingScheme_ = OHOS::Telephony::DataCodingScheme::DATA_CODING_7BIT;
    size_t  oldSize = gsmSmsMessage->visibleMessageBody_.size();
    gsmSmsMessage->smsUserData_.header[0].udh.singleShift.langId = 1;
    std::string text = "1234566";
    gsmSmsMessage->SetFullText(text);
    gsmSmsMessage->ConvertUserData();
    size_t  newSize = gsmSmsMessage->visibleMessageBody_.size();
    EXPECT_EQ(newSize, oldSize);

    gsmSmsMessage->codingScheme_ = OHOS::Telephony::DataCodingScheme::DATA_CODING_UCS2;
    oldSize = gsmSmsMessage->visibleMessageBody_.size();
    gsmSmsMessage->ConvertUserData();
    newSize = gsmSmsMessage->visibleMessageBody_.size();
    EXPECT_EQ(newSize, oldSize);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0010
 * @tc.name     Test GsmSmsMessage IsSpecialMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0010, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);
    gsmSmsMessage->protocolId_ = 0x40;
    EXPECT_TRUE(gsmSmsMessage->IsSpecialMessage());

    gsmSmsMessage->msgClass_ = OHOS::Telephony::SmsMessageClass::SMS_SIM_MESSAGE;
    gsmSmsMessage->protocolId_ = 0x7f;
    EXPECT_TRUE(gsmSmsMessage->IsSpecialMessage());

    gsmSmsMessage->protocolId_ = 0x7c;
    EXPECT_TRUE(gsmSmsMessage->IsSpecialMessage());

    gsmSmsMessage->protocolId_ = 0x41;
    gsmSmsMessage->bMwiSet_ = true;
    EXPECT_TRUE(gsmSmsMessage->IsSpecialMessage());

    gsmSmsMessage->bMwiClear_ = true;
    EXPECT_TRUE(gsmSmsMessage->IsSpecialMessage());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessage_0011
 * @tc.name     Test GsmSmsMessage DecodeMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsMessage_0011, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    EXPECT_TRUE(gsmSmsMessage != nullptr);

    std::string msgText("");
    // init destination array of pdu data
    uint8_t decodeData[(MAX_GSM_7BIT_DATA_LEN * OHOS::Telephony::MAX_SEGMENT_NUM) + 1];
    EXPECT_EQ(memset_s(decodeData, sizeof(decodeData), 0x00, sizeof(decodeData)), EOK);

    int encodeLen = 0;
    bool bAbnormal = false;
    unsigned char langId = OHOS::Telephony::MsgLanguageId::MSG_ID_RESERVED_LANG;
    OHOS::Telephony::DataCodingScheme codingType = OHOS::Telephony::DataCodingScheme::DATA_CODING_7BIT;

    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_EQ(encodeLen, 0);

    msgText = "123456";
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_GT(encodeLen, 0);

    codingType = OHOS::Telephony::DataCodingScheme::DATA_CODING_8BIT;
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_GT(encodeLen, 0);

    msgText.append(sizeof(decodeData), '1');
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_EQ(encodeLen, 0);

    msgText = "123456";
    codingType = OHOS::Telephony::DataCodingScheme::DATA_CODING_UCS2;
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_GT(encodeLen, 0);

    codingType = OHOS::Telephony::DataCodingScheme::DATA_CODING_AUTO;
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_GT(encodeLen, 0);

    codingType = OHOS::Telephony::DataCodingScheme::DATA_CODING_EUCKR;
    encodeLen = gsmSmsMessage->DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    EXPECT_GT(encodeLen, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamCodec_0016
 * @tc.name     Test GsmSmsParamCodec EncodeAddressPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamCodec_0016, Function | MediumTest | Level1)
{
    auto gsmSmsParamCodec = std::make_shared<GsmSmsParamCodec>();
    AddressNumber *pAddress = new AddressNumber();
    std::string paramStr;

    std::string smsc = "**21*1300555002012345";
    memcpy_s(&pAddress->address, sizeof(pAddress->address), smsc.data(), smsc.length());
    pAddress->address[smsc.length()] = '\0';
    pAddress->ton = TYPE_INTERNATIONAL;
    pAddress->npi = SMS_NPI_ISDN;
    EXPECT_FALSE(gsmSmsParamCodec->EncodeAddressPdu(pAddress, paramStr)); // second branch
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsParamCodec_0017
 * @tc.name     Test GsmSmsParamCodec EncodeSmscPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsParamCodec_0017, Function | MediumTest | Level1)
{
    /*
     *   test third branch
     */
    auto gsmSmsParamCodec = std::make_shared<GsmSmsParamCodec>();
    EXPECT_NE(gsmSmsParamCodec, nullptr);
    const std::string smsc = "+17005550020";
    AddressNumber *pAddress = new AddressNumber();
    uint8_t encodeSmscAddr[MAX_SMSC_LEN];
    memset_s(encodeSmscAddr, sizeof(encodeSmscAddr), 0x00, sizeof(encodeSmscAddr));
    std::string decodeAddr;
    memcpy_s(&pAddress->address, sizeof(pAddress->address), smsc.data(), smsc.length());
    pAddress->address[smsc.length()] = '\0';
    pAddress->ton = TYPE_INTERNATIONAL;
    pAddress->npi = SMS_NPI_ISDN;
    uint8_t encodeSmscLen = 0;
    encodeSmscLen = gsmSmsParamCodec->EncodeSmscPdu(pAddress, encodeSmscAddr, sizeof(encodeSmscAddr));
    EXPECT_GT(encodeSmscLen, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0005
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0005, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    encodeBuffer->data_[0] = 1;
    SmsSubmit *pSubmit = new SmsSubmit();
    /*
     * parameter check
     */
    auto encode = std::make_shared<GsmSmsTpduEncode>(nullptr, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_FALSE(encode->EncodeSubmitPdu(*encodeBuffer, pSubmit));
    auto encode1 = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, nullptr, tpduCodec);
    EXPECT_FALSE(encode1->EncodeSubmitPdu(*encodeBuffer, pSubmit));

    auto encode2 = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_TRUE(encode2->EncodeSubmitPdu(*encodeBuffer, pSubmit));
    /*
     * third branch
     */
    const std::string smsc = "+17005550020324556627";
    memcpy_s(&pSubmit->destAddress.address, sizeof(pSubmit->destAddress.address), smsc.data(), smsc.length());
    pSubmit->destAddress.address[smsc.length()] = '\0';
    EXPECT_FALSE(encode2->EncodeSubmitPdu(*encodeBuffer, pSubmit));

    delete pSubmit;
    pSubmit = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0006
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitPduPartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0006, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsSubmit *pSubmit = new SmsSubmit();
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    /*
     * parameter check
     */
    encodeBuffer->data_ = nullptr;
    EXPECT_FALSE(encode->EncodeSubmitPduPartData(*encodeBuffer, pSubmit));
    /*
     * third branch
     */
    encodeBuffer->index_ = 0;
    pSubmit->vpf = OHOS::Telephony::SmsVpf::SMS_VPF_ENHANCED;
    EXPECT_FALSE(encode->EncodeSubmitPdu(*encodeBuffer, pSubmit));

    encodeBuffer->data_ = nullptr;
    EXPECT_FALSE(encode->EncodeSubmitPdu(*encodeBuffer, pSubmit));

    delete pSubmit;
    pSubmit = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0007
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitTpduType
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0007, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsSubmit pSubmit;
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);

    /*
     * parameter check
     */
    pSubmit.bRejectDup = false;
    encodeBuffer->data_ = nullptr;
    uint16_t count = encodeBuffer->GetSize();
    encode->EncodeSubmitTpduType(*encodeBuffer, pSubmit);
    EXPECT_EQ(encodeBuffer->GetSize(), count);
    /*
     * third branch
     */
    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer2->data_[0] = 1;
    encodeBuffer2->bitIndex_ = 1;
    uint16_t count2 = encodeBuffer2->GetSize();
    encode->EncodeSubmitTpduType(*encodeBuffer2, pSubmit);
    EXPECT_EQ(encodeBuffer2->GetSize(), count2);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0008
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitTypePartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0008, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsSubmit pSubmit;
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    pSubmit.bStatusReport = false;
    pSubmit.bHeaderInd = false;
    pSubmit.bReplyPath = false;
    /*
     * SMS_VPF_ENHANCED
     */
    pSubmit.vpf = OHOS::Telephony::SmsVpf::SMS_VPF_ENHANCED;
    encodeBuffer->data_ = nullptr;
    uint16_t count1 = encodeBuffer->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer, pSubmit);
    EXPECT_EQ(encodeBuffer->GetSize(), count1);

    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer2->data_[0] = 1;
    encodeBuffer2->index_ = 1;
    uint16_t count2 = encodeBuffer2->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer2, pSubmit);
    EXPECT_EQ(encodeBuffer2->GetSize(), count2);
    /*
     * SMS_VPF_RELATIVE
     */
    pSubmit.vpf = OHOS::Telephony::SmsVpf::SMS_VPF_ENHANCED;
    auto encodeBuffer3 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer3->data_ = nullptr;
    uint16_t count3 = encodeBuffer3->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer3, pSubmit);
    EXPECT_EQ(encodeBuffer3->GetSize(), count3);

    auto encodeBuffer4 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer4->data_[0] = 1;
    encodeBuffer4->index_ = 1;
    uint16_t count4 = encodeBuffer4->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer4, pSubmit);
    EXPECT_EQ(encodeBuffer4->GetSize(), count4);
    /*
     * SMS_VPF_RELATIVE
     */
    pSubmit.vpf = OHOS::Telephony::SmsVpf::SMS_VPF_RELATIVE;
    auto encodeBuffer5 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer5->data_ = nullptr;
    uint16_t count5 = encodeBuffer5->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer5, pSubmit);
    EXPECT_EQ(encodeBuffer5->GetSize(), count5);

    auto encodeBuffer6 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer6->data_[0] = 1;
    encodeBuffer6->index_ = 1;
    uint16_t count6 = encodeBuffer6->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer6, pSubmit);
    EXPECT_EQ(encodeBuffer6->GetSize(), count6);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0009
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitTypePartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0009, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsSubmit pSubmit;
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    pSubmit.bStatusReport = false;
    pSubmit.bHeaderInd = false;
    pSubmit.bReplyPath = false;
    /*
     * SMS_VPF_ABSOLUTE
     */
    pSubmit.vpf = OHOS::Telephony::SmsVpf::SMS_VPF_ABSOLUTE;
    auto encodeBuffer7 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer7->data_ = nullptr;
    uint16_t count7 = encodeBuffer7->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer7, pSubmit);
    EXPECT_EQ(encodeBuffer7->GetSize(), count7);

    auto encodeBuffer8 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer8->data_[0] = 1;
    encodeBuffer8->index_ = 1;
    uint16_t count8 = encodeBuffer8->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer8, pSubmit);
    EXPECT_EQ(encodeBuffer8->GetSize(), count8);
    /*
     * SMS_VPF_NOT_PRESENT
     */
    pSubmit.vpf = OHOS::Telephony::SmsVpf::SMS_VPF_NOT_PRESENT;
    auto encodeBuffer9 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer9->data_ = nullptr;
    uint16_t count9 = encodeBuffer9->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer9, pSubmit);
    EXPECT_EQ(encodeBuffer9->GetSize(), count9);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0010
 * @tc.name     Test GsmSmsTpduCodec EncodeSubmitTypeData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0010, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsSubmit pSubmit;
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    /*
     * pSubmit->bStatusReport is true
     */
    pSubmit.bStatusReport = true;
    encode->EncodeSubmitTypePartData(*encodeBuffer, pSubmit);
    auto encodeBuffer1 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer1->data_[0] = 1;
    encodeBuffer1->index_ = 1;
    uint16_t count1 = encodeBuffer1->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer1, pSubmit);
    EXPECT_EQ(encodeBuffer1->GetSize(), count1);
    /*
     * pSubmit.bHeaderInd is true
     */
    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    pSubmit.bStatusReport = false;
    pSubmit.bHeaderInd = true;
    const std::string smsc = "17005550020";
    memcpy_s(&pSubmit.destAddress.address, sizeof(pSubmit.destAddress.address), smsc.data(), smsc.length());
    pSubmit.destAddress.address[smsc.length()] = '\0';
    encodeBuffer2->data_ = nullptr;
    uint16_t count2 = encodeBuffer2->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer2, pSubmit);
    EXPECT_EQ(encodeBuffer2->GetSize(), count2);

    auto encodeBuffer3 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer3->data_[0] = 1;
    encodeBuffer3->index_ = 1;
    uint16_t count3 = encodeBuffer3->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer3, pSubmit);
    EXPECT_EQ(encodeBuffer3->GetSize(), count3);
    /*
     * pSubmit.bReplyPath is true
     */
    pSubmit.bStatusReport = false;
    pSubmit.bHeaderInd = false;
    pSubmit.bReplyPath = true;
    auto encodeBuffer4 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer4->data_ = nullptr;
    uint16_t count4 = encodeBuffer4->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer4, pSubmit);
    EXPECT_EQ(encodeBuffer4->GetSize(), count4);

    auto encodeBuffer5 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer5->data_[0] = 1;
    encodeBuffer5->index_ = 1;
    uint16_t count5 = encodeBuffer5->GetSize();
    encode->EncodeSubmitTypePartData(*encodeBuffer5, pSubmit);
    EXPECT_EQ(encodeBuffer5->GetSize(), count5);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0011
 * @tc.name     Test GsmSmsTpduCodec EncodeDeliverPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0011, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsDeliver *pDeliver = new SmsDeliver();
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    /*
     * second branch
     */
    encodeBuffer->data_ = nullptr;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer, pDeliver));
    /*
     * third branch pDeliver->bMoreMsg is false
     */
    auto encodeBuffer1 = std::make_shared<SmsWriteBuffer>();
    pDeliver->bMoreMsg = false;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer1, pDeliver));

    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer2->data_[0] = 1;
    encodeBuffer2->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer2, pDeliver));
    /*
     * fourth branch pDeliver->bStatusReport is true
     */
    pDeliver->bMoreMsg = true;
    pDeliver->bStatusReport = true;
    auto encodeBuffer3 = std::make_shared<SmsWriteBuffer>();
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer3, pDeliver));

    auto encodeBuffer4 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer4->data_[0] = 1;
    encodeBuffer4->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer4, pDeliver));
    /*
     * fifth branch pDeliver->bHeaderInd is true
     */
    auto encodeBuffer5 = std::make_shared<SmsWriteBuffer>();
    pDeliver->bStatusReport = false;
    pDeliver->bHeaderInd = true;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer5, pDeliver));

    auto encodeBuffer6 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer6->data_[0] = 1;
    encodeBuffer6->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPdu(*encodeBuffer6, pDeliver));

    delete pDeliver;
    pDeliver = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0012
 * @tc.name     Test GsmSmsTpduCodec EncodeDeliverPartData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0012, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsDeliver *pDeliver = new SmsDeliver();
    /*
     * parameter check
     */
    auto encode0 = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, nullptr, tpduCodec);
    EXPECT_NE(encode0, nullptr);
    EXPECT_FALSE(encode0->EncodeDeliverPartData(*encodeBuffer, pDeliver));

    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    /*
     * pDeliver->bReplyPath is true
     */
    pDeliver->bReplyPath = true;
    encodeBuffer->data_ = nullptr;
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer, pDeliver));

    auto encodeBuffer1 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer1->data_[0] = 1;
    encodeBuffer1->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer, pDeliver));
    /*
     * third branch
     */
    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    pDeliver->bReplyPath = false;
    const std::string smsc = "17005550020111111111";
    memcpy_s(&pDeliver->originAddress.address, sizeof(pDeliver->originAddress.address), smsc.data(), smsc.length());
    pDeliver->originAddress.address[smsc.length()] = '\0';
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer2, pDeliver));
    /*
     * fourth branch
     */
    auto encodeBuffer3 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer3->data_[0] = 1;
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer3, pDeliver));

    delete pDeliver;
    pDeliver = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0013
 * @tc.name     Test GsmSmsTpduCodec EncodeDeliverData
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0013, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    SmsDeliver *pDeliver = new SmsDeliver();
    /*
     * parameter check
     */
    auto encode0 = std::make_shared<GsmSmsTpduEncode>(nullptr, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode0, nullptr);
    EXPECT_FALSE(encode0->EncodeDeliverData(*encodeBuffer, pDeliver));
    /*
     * second branch
     */
    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    EXPECT_NE(encode, nullptr);
    encodeBuffer->data_ = nullptr;
    EXPECT_FALSE(encode->EncodeDeliverData(*encodeBuffer, pDeliver));

    auto encodeBuffer1 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer1, pDeliver));
    /*
     * main branch
     */
    auto encodeBuffer2 = std::make_shared<SmsWriteBuffer>();
    encodeBuffer2->data_[0] = 1;
    encodeBuffer2->index_ = 1;
    EXPECT_FALSE(encode->EncodeDeliverPartData(*encodeBuffer2, pDeliver));

    delete pDeliver;
    pDeliver = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsBaseMessage_0001
 * @tc.name     Test SmsBaseMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsBaseMessage_0001, Function | MediumTest | Level1)
{
    auto smsBaseMessage = std::make_shared<GsmSmsMessage>();
    smsBaseMessage->ConvertMessageClass(SmsMessageClass::SMS_SIM_MESSAGE);
    smsBaseMessage->ConvertMessageClass(SmsMessageClass::SMS_INSTANT_MESSAGE);
    smsBaseMessage->ConvertMessageClass(SmsMessageClass::SMS_OPTIONAL_MESSAGE);
    smsBaseMessage->ConvertMessageClass(SmsMessageClass::SMS_FORWARD_MESSAGE);
    smsBaseMessage->ConvertMessageClass(SmsMessageClass::SMS_CLASS_UNKNOWN);
    SplitInfo split;
    DataCodingScheme codingType = DataCodingScheme::DATA_CODING_7BIT;
    smsBaseMessage->ConvertSpiltToUtf8(split, codingType);
    split.encodeData.push_back(1);
    codingType = DataCodingScheme::DATA_CODING_UCS2;
    smsBaseMessage->ConvertSpiltToUtf8(split, codingType);
    codingType = DataCodingScheme::DATA_CODING_AUTO;
    smsBaseMessage->ConvertSpiltToUtf8(split, codingType);
    smsBaseMessage->smsUserData_.headerCnt = 1;
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_CONCAT_8BIT;
    EXPECT_FALSE(smsBaseMessage->GetConcatMsg() == nullptr);
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_CONCAT_16BIT;
    EXPECT_FALSE(smsBaseMessage->GetConcatMsg() == nullptr);
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_APP_PORT_8BIT;
    EXPECT_FALSE(smsBaseMessage->GetPortAddress() == nullptr);
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_APP_PORT_16BIT;
    EXPECT_FALSE(smsBaseMessage->GetPortAddress() == nullptr);
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_SPECIAL_SMS;
    EXPECT_FALSE(smsBaseMessage->GetSpecialSmsInd() == nullptr);
    smsBaseMessage->smsUserData_.header[0].udhType = UserDataHeadType::UDH_APP_PORT_8BIT;
    EXPECT_FALSE(smsBaseMessage->IsWapPushMsg());
    MSG_LANGUAGE_ID_T langId = 1;
    codingType = DataCodingScheme::DATA_CODING_7BIT;
    EXPECT_GT(smsBaseMessage->GetMaxSegmentSize(codingType, 1, true, langId, 1), 0);
    EXPECT_GT(smsBaseMessage->GetSegmentSize(codingType, 1, true, langId), 0);
    codingType = DataCodingScheme::DATA_CODING_ASCII7BIT;
    EXPECT_GT(smsBaseMessage->GetMaxSegmentSize(codingType, 1, true, langId, 1), 0);
    EXPECT_GT(smsBaseMessage->GetSegmentSize(codingType, 1, true, langId), 0);
    codingType = DataCodingScheme::DATA_CODING_8BIT;
    EXPECT_GT(smsBaseMessage->GetMaxSegmentSize(codingType, 1, true, langId, 1), 0);
    EXPECT_GT(smsBaseMessage->GetSegmentSize(codingType, 1, true, langId), 0);
    codingType = DataCodingScheme::DATA_CODING_UCS2;
    EXPECT_GT(smsBaseMessage->GetMaxSegmentSize(codingType, 1, true, langId, 1), 0);
    EXPECT_GT(smsBaseMessage->GetSegmentSize(codingType, 1, true, langId), 0);
    std::string message = "";
    LengthInfo lenInfo;
    EXPECT_GE(smsBaseMessage->GetSmsSegmentsInfo(message, true, lenInfo), 0);
    message = "123";
    EXPECT_EQ(smsBaseMessage->GetSmsSegmentsInfo(message, true, lenInfo), TELEPHONY_ERR_SUCCESS);
    EXPECT_EQ(smsBaseMessage->GetSmsSegmentsInfo(message, false, lenInfo), TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsBaseMessage_0002
 * @tc.name     Test SmsBaseMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsBaseMessage_0002, Function | MediumTest | Level1)
{
    auto gsmSmsMessage = std::make_shared<GsmSmsMessage>();
    std::vector<struct SplitInfo> splitResult;
    DataCodingScheme codingType = DATA_CODING_AUTO;
    std::string text = {0xe4, 0xbd, 0xa0, 0xe4, 0xbd, 0xa0, 0xe4, 0xbd, 0xa0, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5,
        0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3,
        0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3, 0xf0, 0x9f, 0xa5, 0xb3};
    std::string desAddr = "";
    gsmSmsMessage->SplitMessage(splitResult, text, false, codingType, false, desAddr);
    std::vector<unsigned char> expect1 = {0x4f, 0x60, 0x4f, 0x60, 0x4f, 0x60, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73};
    std::vector<unsigned char> expect2 = {0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73,
        0xd8, 0x3e, 0xdd, 0x73, 0xd8, 0x3e, 0xdd, 0x73};
    EXPECT_TRUE(splitResult[0].encodeData == expect1);
    EXPECT_TRUE(splitResult[1].encodeData == expect2);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsPersistHelper_0002
 * @tc.name     Test SmsPersistHelper
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsPersistHelper_0002, Function | MediumTest | Level1)
{
    auto smsPersistHelper = DelayedSingleton<SmsPersistHelper>::GetInstance();
    smsPersistHelper->CreateSmsHelper();
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(0, 0);
    smsPersistHelper->ProcessEvent(event);
    uint32_t releaseDataShareHelperEventId = 10000;
    event = AppExecFwk::InnerEvent::Get(releaseDataShareHelperEventId, 0);
    smsPersistHelper->ProcessEvent(event);
    EXPECT_TRUE(smsPersistHelper->smsDataShareHelper_ == nullptr);
    smsPersistHelper->RemoveEvent(releaseDataShareHelperEventId);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsPersistHelper_0003
 * @tc.name     Test SmsPersistHelper
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsPersistHelper_0003, Function | MediumTest | Level1)
{
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(true));
    DataShare::DataShareValuesBucket values;
    int32_t dataBaseId;
    uint16_t idxId;
    DataShare::DataSharePredicates predicates;
    EXPECT_FALSE(DelayedSingleton<SmsPersistHelper>::GetInstance()->QuerySmsMmsForId(predicates, dataBaseId));
    std::vector<SmsReceiveIndexer> indexers;
    EXPECT_FALSE(DelayedSingleton<SmsPersistHelper>::GetInstance()->Query(predicates, indexers));
    EXPECT_FALSE(DelayedSingleton<SmsPersistHelper>::GetInstance()->QueryMaxGroupId(predicates, idxId));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveHandler_0001
 * @tc.name     Test SmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveHandler_0001, Function | MediumTest | Level1)
{
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    auto resultSet = std::make_shared<DataShareResultSetMock>();
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    indexer->msgCount_ = 1;
    auto pdus = std::make_shared<vector<string>>();
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Return(resultSet));
    EXPECT_CALL(*resultSet, GoToFirstRow())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GoToNextRow())
        .WillOnce(Return(0))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, GetColumnIndex(_, _))
        .WillRepeatedly(Invoke([](const std::string &columnName, int &columnIndex) -> int {
            if (columnName == SmsSubsection::SIZE) {
                columnIndex = 1;
            } else if (columnName == SmsSubsection::SUBSECTION_INDEX) {
                columnIndex = 2;
            }
            return 0;
        }));
    EXPECT_CALL(*resultSet, GetInt(_, _))
        .WillRepeatedly(Invoke([](int columnIndex, int &value) -> int {
            if (columnIndex == 1) {
                value = 1;
            } else if (columnIndex == 2) {
                value = 2;
            }
            return 0;
        }));
    EXPECT_CALL(*resultSet, GetString(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, Close())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(0));
    EXPECT_FALSE(smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveHandler_0002
 * @tc.name     Test SmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveHandler_0002, Function | MediumTest | Level1)
{
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    auto resultSet = std::make_shared<DataShareResultSetMock>();
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    indexer->msgCount_ = 4;
    auto pdus = std::make_shared<vector<string>>();
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Return(resultSet));
    EXPECT_CALL(*resultSet, GoToFirstRow())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GoToNextRow())
        .WillOnce(Return(0))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, GetColumnIndex(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GetInt(_, _))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, GetString(_, _))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, Close())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(0));
    EXPECT_FALSE(smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveHandler_0003
 * @tc.name     Test SmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveHandler_0003, Function | MediumTest | Level1)
{
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    auto resultSet = std::make_shared<DataShareResultSetMock>();
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    indexer->msgCount_ = 1;
    auto pdus = std::make_shared<vector<string>>();
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Return(resultSet));
    EXPECT_CALL(*resultSet, GoToFirstRow())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GoToNextRow())
        .WillOnce(Return(0))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, GetColumnIndex(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GetInt(_, _))
        .WillRepeatedly(Invoke([](int columnIndex, int &value) -> int {
            value = 1;
            return 0;
        }));
    EXPECT_CALL(*resultSet, GetString(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, Close())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(0));
    EXPECT_FALSE(smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler));
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveHandler_0004
 * @tc.name     Test SmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveHandler_0004, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    EXPECT_TRUE(smsReceiveHandler != nullptr);
    smsReceiveHandler->smsLockSerialNum_ = 0;
    smsReceiveHandler->smsRunningLockCount_ = 0;
#ifdef ABILITY_POWER_SUPPORT
    smsReceiveHandler->smsRunningLock_ = nullptr;
#endif
    AppExecFwk::InnerEvent::Pointer event =
        AppExecFwk::InnerEvent::Get(SmsReceiveHandler::RUNNING_LOCK_TIMEOUT_EVENT_ID, -1);
    smsReceiveHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(SmsReceiveHandler::RUNNING_LOCK_TIMEOUT_EVENT_ID, 0);
    smsReceiveHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(SmsReceiveHandler::DELAY_RELEASE_RUNNING_LOCK_EVENT_ID, 0);
    smsReceiveHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(SmsReceiveHandler::RETRY_CONNECT_DATASHARE_EVENT_ID, 0);
    smsReceiveHandler->ProcessEvent(event);
#ifdef BASE_POWER_IMPROVEMENT_FEATURE
    event = AppExecFwk::InnerEvent::Get(SMS_EVENT_NEW_SMS_REPLY, 0);
    auto smsStateHandler = DelayedSingleton<SmsStateHandler>::GetInstance();
    smsStateHandler->smsStateObserver_ = std::make_shared<SmsStateObserver>();
    smsReceiveHandler->ProcessEvent(event);
    smsStateHandler->smsStateObserver_ = nullptr;
#endif
    smsReceiveHandler->ReduceRunningLock();
    smsReceiveHandler->ReleaseRunningLock();
    smsReceiveHandler->ApplyRunningLock();
#ifdef ABILITY_POWER_SUPPORT
    EXPECT_EQ(smsReceiveHandler->smsRunningLockCount_, 1);
    smsReceiveHandler->ApplyRunningLock();
    EXPECT_EQ(smsReceiveHandler->smsRunningLockCount_, 2);
    smsReceiveHandler->ReduceRunningLock();
    EXPECT_EQ(smsReceiveHandler->smsRunningLockCount_, 1);
    smsReceiveHandler->ReduceRunningLock();
    EXPECT_EQ(smsReceiveHandler->smsRunningLockCount_, 0);
#endif
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveHandler_0005
 * @tc.name     Test SmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveHandler_0005, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsReceiveIndexer> indexer = nullptr;
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    smsReceiveHandler->UpdateMultiPageMessage(indexer, nullptr);
    EXPECT_FALSE(smsReceiveHandler->IsRepeatedMessagePart(indexer));
    indexer = std::make_shared<SmsReceiveIndexer>();
    smsReceiveHandler->UpdateMultiPageMessage(indexer, nullptr);
    indexer->msgCount_ = 2;
    smsReceiveHandler->CombineMessagePart(indexer);
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    auto resultSet = std::make_shared<DataShareResultSetMock>();
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    auto pdus = std::make_shared<vector<string>>();
    smsReceiveHandler->UpdateMultiPageMessage(indexer, pdus);
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Return(resultSet));
    EXPECT_CALL(*resultSet, GoToFirstRow())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GoToNextRow())
        .WillOnce(Return(0))
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(*resultSet, GetColumnIndex(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*resultSet, GetInt(_, _))
        .WillRepeatedly(Invoke([](int columnIndex, int &value) -> int {
            value = 1;
            return 0;
        }));
    EXPECT_CALL(*resultSet, GetString(_, _))
        .WillRepeatedly(Invoke([](int columnIndex, std::string &value) -> int {
            value = "test";
            return 0;
        }));
    EXPECT_CALL(*resultSet, Close())
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(true));
    EXPECT_TRUE(smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler));
    EXPECT_FALSE(smsReceiveHandler->IsRepeatedMessagePart(indexer));
    std::shared_ptr<GsmSmsMessage> message = std::make_shared<GsmSmsMessage>();
    smsReceiveHandler->HandleReceivedSmsWithoutDataShare(message);
    indexer->msgSeqId_ = 1;
    EXPECT_TRUE(smsReceiveHandler->IsRepeatedMessagePart(indexer));
    std::make_shared<SmsReceiveReliabilityHandler>(0)->SmsReceiveReliabilityProcessing();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveReliabilityHandler_0001
 * @tc.name     Test SmsReceiveReliabilityHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveReliabilityHandler_0001, Function | MediumTest | Level1)
{
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    auto pdus = std::make_shared<vector<string>>();
    indexer->originatingAddress_ = "10659401";
    reliabilityHandler->SendBroadcast(indexer, pdus);
    indexer->originatingAddress_ = "8610659401";
    reliabilityHandler->SendBroadcast(indexer, pdus);
    indexer->originatingAddress_ = "+8610659401";
    reliabilityHandler->SendBroadcast(indexer, pdus);
    indexer->originatingAddress_ = "+86106594011";
    reliabilityHandler->SendBroadcast(indexer, pdus);
    reliabilityHandler->HiSysEventCBResult(false);
    DelayedSingleton<SmsHiSysEvent>::GetInstance()->smsBroadcastStartTime_ = 0;
    reliabilityHandler->HiSysEventCBResult(true);
    EXPECT_NE(DelayedSingleton<SmsHiSysEvent>::GetInstance()->smsBroadcastStartTime_, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveReliabilityHandler_0002
 * @tc.name     Test SmsReceiveReliabilityHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveReliabilityHandler_0002, Function | MediumTest | Level1)
{
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    int32_t smsPagesCount = 0;
    std::vector<SmsReceiveIndexer> dbIndexers;
    SmsReceiveIndexer indexer;
    indexer.msgRefId_ = 1;
    indexer.msgSeqId_ = 1;
    std::vector<uint8_t> pdu = { 8, 145, 104, 49, 8, 32, 1, 5, 240, 68, 13, 145, 104, 145, 39, 32, 49, 100, 240,
        0, 0, 82, 64, 80, 34, 145, 17, 35, 160, 5, 0, 3, 1, 2, 1, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44,
        22, 139, 197, 98, 177, 152, 44, 54, 171, 209, 108, 55, 25, 142, 54, 163, 213, 108, 180, 90, 12, 55, 187,
        213, 104, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 76, 22, 155, 213, 104,
        182, 155, 12, 71, 155, 209, 106, 54, 90, 45, 134, 155, 221, 106, 180, 88, 44, 22, 139, 197, 98, 177, 88,
        44, 22, 139, 197, 98, 177, 88, 44, 38, 139, 205, 106, 52, 219, 77, 134, 163, 205, 104, 53, 27, 173, 22,
        195, 205, 110, 53, 90, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 147, 197, 102,
        53, 154, 237, 38, 195, 209, 102 };
    indexer.pdu_ = pdu;
    dbIndexers.push_back(indexer);
    indexer.msgSeqId_ = -1;
    dbIndexers.push_back(indexer);
    indexer.msgSeqId_ = MAX_SEGMENT_NUM + 1;
    dbIndexers.push_back(indexer);
    indexer.msgRefId_ = 2;
    dbIndexers.push_back(indexer);
    indexer.msgRefId_ = 1;
    indexer.pdu_ = {};
    dbIndexers.push_back(indexer);
    std::shared_ptr<std::vector<std::string>> userDataRaws = std::make_shared<std::vector<std::string>>();
    userDataRaws->assign(MAX_SEGMENT_NUM, "");
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, -1, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 100, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 1, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 2, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 0, userDataRaws);
    EXPECT_EQ(smsPagesCount, 1);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 0, userDataRaws);
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 0, userDataRaws);
    EXPECT_EQ(smsPagesCount, 2);
    reliabilityHandler->DeleteMessageFormDb(0, 1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveReliabilityHandler_0003
 * @tc.name     Test SmsReceiveReliabilityHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveReliabilityHandler_0003, Function | MediumTest | Level1)
{
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    std::vector<SmsReceiveIndexer> dbIndexers;
    SmsReceiveIndexer indexer;
    indexer.destPort_ = 2948;
    indexer.msgCount_ = 1;
    std::vector<uint8_t> pdu = { 8, 145, 104, 49, 8, 32, 1, 5, 240, 68, 13, 145, 104, 145, 39, 32, 49, 100, 240,
        0, 0, 82, 64, 80, 34, 145, 17, 35, 160, 5, 0, 3, 1, 2, 1, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44,
        22, 139, 197, 98, 177, 152, 44, 54, 171, 209, 108, 55, 25, 142, 54, 163, 213, 108, 180, 90, 12, 55, 187,
        213, 104, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 76, 22, 155, 213, 104,
        182, 155, 12, 71, 155, 209, 106, 54, 90, 45, 134, 155, 221, 106, 180, 88, 44, 22, 139, 197, 98, 177, 88,
        44, 22, 139, 197, 98, 177, 88, 44, 38, 139, 205, 106, 52, 219, 77, 134, 163, 205, 104, 53, 27, 173, 22,
        195, 205, 110, 53, 90, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 147, 197, 102,
        53, 154, 237, 38, 195, 209, 102 };
    indexer.pdu_ = pdu;
    dbIndexers.push_back(indexer);
    indexer.msgCount_ = 2;
    dbIndexers.push_back(indexer);
    reliabilityHandler->CheckUnReceiveWapPush(dbIndexers);
    EXPECT_EQ(dbIndexers.size(), 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsNetworkPolicyManager_0001
 * @tc.name     Test SmsNetworkPolicyManager
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsNetworkPolicyManager_0001, Function | MediumTest | Level1)
{
    SmsNetworkPolicyManager manager(0);
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(1234567);
    manager.ProcessEvent(event);
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Delete(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Release())
        .WillRepeatedly(Return(true));
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_FACTORY_RESET);
    manager.ProcessEvent(event);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
    event = nullptr;
    manager.ProcessEvent(event);
    std::function<void(bool isImsNetDomain, int32_t voiceServiceState)> callback = nullptr;
    EXPECT_EQ(manager.NetworkRegister(callback), std::nullopt);
    manager.NetworkUnregister(0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsReceiveHandler_0001
 * @tc.name     Test GsmSmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsReceiveHandler_0001, Function | MediumTest | Level1)
{
    std::vector<uint8_t> pdu = { 8, 145, 104, 49, 8, 32, 1, 5, 240, 68, 13, 145, 104, 145, 39, 32, 49, 100, 240,
        0, 0, 82, 64, 80, 34, 145, 17, 35, 160, 5, 0, 3, 1, 2, 1, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44,
        22, 139, 197, 98, 177, 152, 44, 54, 171, 209, 108, 55, 25, 142, 54, 163, 213, 108, 180, 90, 12, 55, 187,
        213, 104, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 76, 22, 155, 213, 104,
        182, 155, 12, 71, 155, 209, 106, 54, 90, 45, 134, 155, 221, 106, 180, 88, 44, 22, 139, 197, 98, 177, 88,
        44, 22, 139, 197, 98, 177, 88, 44, 38, 139, 205, 106, 52, 219, 77, 134, 163, 205, 104, 53, 27, 173, 22,
        195, 205, 110, 53, 90, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 147, 197, 102,
        53, 154, 237, 38, 195, 209, 102 };
    std::shared_ptr<SmsBaseMessage> baseMessage = GsmSmsMessage::CreateMessage(StringUtils::StringToHex(pdu));
    EXPECT_NE(baseMessage, nullptr);
    std::shared_ptr<GsmSmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    smsReceiveHandler->HandleReceivedSmsWithoutDataShare(baseMessage);
    smsReceiveHandler->HandleRemainDataShare(baseMessage);
    smsReceiveHandler->HandleSmsByType(baseMessage);
    smsReceiveHandler->HandleRemainDataShare(nullptr);
    EXPECT_NE(smsReceiveHandler->HandleAck(baseMessage), AckIncomeCause::SMS_ACK_UNKNOWN_ERROR);
    EXPECT_EQ(smsReceiveHandler->HandleAck(nullptr), AckIncomeCause::SMS_ACK_UNKNOWN_ERROR);
    std::shared_ptr<CdmaSmsMessage> message = std::make_shared<CdmaSmsMessage>();
    EXPECT_NE(message, nullptr);
    message->transMsg_ = std::make_unique<struct CdmaTransportMsg>();
    EXPECT_NE(message->transMsg_, nullptr);
    (void)memset_s(message->transMsg_.get(), sizeof(struct CdmaTransportMsg), 0x00, sizeof(struct CdmaTransportMsg));
    std::make_shared<CdmaSmsReceiveHandler>(0)->HandleRemainDataShare(message);
    EXPECT_NE(std::make_shared<CdmaSmsReceiveHandler>(0)->HandleAck(message), AckIncomeCause::SMS_ACK_UNKNOWN_ERROR);
    pdu = { 1, 1, 2, 0, 4, 8, 19, 0, 3, 16, 8, 208, 1, 6, 16, 44, 40, 112, 225, 66, 8, 1, 192, 12, 1, 192 };
    message = CdmaSmsMessage::CreateMessage(StringUtils::StringToHex(pdu));
    EXPECT_NE(message, nullptr);
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Insert(_, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*dataShareHelperMock, Delete(_, _))
        .WillOnce(Return(-1))
        .WillRepeatedly(Return(0));
    std::make_shared<CdmaSmsReceiveHandler>(0)->HandleRemainDataShare(message);
    std::make_shared<CdmaSmsReceiveHandler>(0)->HandleRemainDataShare(message);
    EXPECT_NE(std::make_shared<CdmaSmsReceiveHandler>(0)->HandleAck(message), AckIncomeCause::SMS_ACK_UNKNOWN_ERROR);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = nullptr;
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_0014
 * @tc.name     Test GsmSmsTpduCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmSmsTpduCodec_0014, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    SmsStatusReport pStatusRep;
    SmsSubmit pSmsSub;

    auto encode = std::make_shared<GsmSmsTpduEncode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    auto encodeBuffer = std::make_shared<SmsWriteBuffer>();
    encodeBuffer->data_[0] = 1;
    EXPECT_FALSE(encode->EncodeStatusReportPdu(*encodeBuffer, nullptr));
    EXPECT_FALSE(encode->EncodeStatusReportPdu(*encodeBuffer, &pStatusRep));
    pStatusRep.bMoreMsg = true;
    pStatusRep.bStatusReport = true;
    pStatusRep.bHeaderInd = true;
    pStatusRep.paramInd = 1;
    EXPECT_FALSE(encode->EncodeStatusReportPdu(*encodeBuffer, &pStatusRep));
    pStatusRep.paramInd = 2;
    EXPECT_FALSE(encode->EncodeStatusReportPdu(*encodeBuffer, &pStatusRep));
    pStatusRep.paramInd = 4;
    EXPECT_FALSE(encode->EncodeStatusReportPdu(*encodeBuffer, &pStatusRep));
    encode->paramCodec_ = nullptr;
    EXPECT_FALSE(encode->EncodeDeliverReportPartData(*encodeBuffer, nullptr));
    EXPECT_FALSE(encode->EncodeStatusReportPartData(*encodeBuffer, &pStatusRep));
    encode->uDataCodec_ = nullptr;
    EXPECT_FALSE(encode->EncodeDeliverReportPartData(*encodeBuffer, nullptr));
    EXPECT_FALSE(encode->EncodeStatusReportData(*encodeBuffer, &pStatusRep, 0));

    auto decode = std::make_shared<GsmSmsTpduDecode>(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    auto decodeBuffer = std::make_shared<SmsReadBuffer>("00");
    decodeBuffer->data_[0] = 1;
    EXPECT_FALSE(decode->DecodeSubmit(*decodeBuffer, &pSmsSub));
    EXPECT_FALSE(decode->DecodeSubmit(*decodeBuffer, nullptr));
    SmsDeliver pDeliver;
    EXPECT_FALSE(decode->DecodeDeliver(*decodeBuffer, &pDeliver));
    EXPECT_FALSE(decode->DecodeDeliver(*decodeBuffer, nullptr));
    EXPECT_FALSE(decode->DecodeStatusReport(*decodeBuffer, &pStatusRep));
    EXPECT_FALSE(decode->DecodeStatusReport(*decodeBuffer, nullptr));
    encode->EncodeStatusReportData(*encodeBuffer, &pStatusRep, 0);
    decode->DecodeStatusReportData(*decodeBuffer, &pStatusRep);
    decode->tpdu_ = nullptr;
    EXPECT_FALSE(decode->DecodeStatusReportData(*decodeBuffer, &pStatusRep));
    EXPECT_FALSE(decode->DecodeDeliver(*decodeBuffer, &pDeliver));
    EXPECT_FALSE(decode->DecodeSubmit(*decodeBuffer, &pSmsSub));
    decode->paramCodec_ = nullptr;
    EXPECT_FALSE(decode->DecodeSubmitPartData(*decodeBuffer, &pSmsSub));
    EXPECT_FALSE(decode->DecodeDeliver(*decodeBuffer, &pDeliver));
    EXPECT_FALSE(decode->DecodeStatusReport(*decodeBuffer, &pStatusRep));
    decode->uDataCodec_ = nullptr;
    EXPECT_FALSE(decode->DecodeSubmitPartData(*decodeBuffer, &pSmsSub));
    EXPECT_FALSE(decode->DecodeDeliverPartData(*decodeBuffer, nullptr));
    EXPECT_FALSE(decode->DecodeStatusReportData(*decodeBuffer, &pStatusRep));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataPdu_0001
 * @tc.name     Test GsmUserDataPdu
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, GsmUserDataPdu_0001, Function | MediumTest | Level1)
{
    SmsWriteBuffer wBuf;
    wBuf.data_ = nullptr;
    SmsReadBuffer rBuf("");
    rBuf.data_ = nullptr;
    GsmUserDataPdu gsmUserDataPdu;
    uint8_t udhl = 0;
    EXPECT_FALSE(gsmUserDataPdu.GetHeaderCnt(rBuf, nullptr, udhl, MAX_UD_HEADER_NUM));
    SmsUDH header;
    header.udhType = UDH_SINGLE_SHIFT;
    gsmUserDataPdu.EncodeHeader(wBuf, header);
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderPartData(rBuf, header, 0));
    header.udhType = UDH_LOCKING_SHIFT;
    gsmUserDataPdu.EncodeHeader(wBuf, header);
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderPartData(rBuf, header, 0));
    header.udhType = UDH_NONE;
    gsmUserDataPdu.EncodeHeaderAppPort8bit(wBuf, header);
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderPartData(rBuf, header, 0));
    header.udhType = UDH_CONCAT_8BIT;
    gsmUserDataPdu.EncodeHeaderAppPort16bit(wBuf, header);
    uint16_t len = 0;
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeader(rBuf, header, len));
    header.udhType = UDH_APP_PORT_8BIT;
    gsmUserDataPdu.EncodeHeaderReplyAddress(wBuf, header);
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeader(rBuf, header, len));
    gsmUserDataPdu.EncodeHeaderConcat8Bit(wBuf, header);
    gsmUserDataPdu.EncodeHeaderConcat16Bit(wBuf, header);
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderConcat8Bit(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderConcat16Bit(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderAppPort16Bit(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderSpecialSms(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderSingleShift(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderLockingShift(rBuf, header));
    EXPECT_FALSE(gsmUserDataPdu.DecodeHeaderDefaultCase(rBuf, header));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsBroadcastSubscriberReceiver_0001
 * @tc.name     Test SmsBroadcastSubscriberReceiver
 * @tc.desc     Function test
 */
HWTEST_F(BranchSmsPartTest, SmsBroadcastSubscriberReceiver_0001, Function | MediumTest | Level1)
{
    using namespace EventFwk;
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Delete(_, _))
        .WillRepeatedly(Return(0));
    MatchingSkills smsSkills;
    smsSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    CommonEventSubscribeInfo smsSubscriberInfo(smsSkills);
    smsSubscriberInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    auto receiver = std::make_shared<SmsBroadcastSubscriberReceiver>(smsSubscriberInfo);
    Want want;
    CommonEventData data;
    std::string addr = "";
    want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_ADDRESS_KEY, addr);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_DATABASE_ID_KEY, 1);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_MSG_COUNT_KEY, 1);
    data.SetWant(want);
    receiver->OnReceiveEvent(data);

    addr = "addr";
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_ADDRESS_KEY, addr);
    data.SetWant(want);
    receiver->OnReceiveEvent(data);

    want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED);
    data.SetWant(want);
    receiver->OnReceiveEvent(data);

    want.SetAction("invalid action");
    data.SetWant(want);
    receiver->OnReceiveEvent(data);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = nullptr;

    SmsWapPushHandler wapPushHandler(0);
    EXPECT_FALSE(wapPushHandler.SendWapPushMessageBroadcast(nullptr));
}

namespace {
bool IsConcatIeAccepted(uint8_t totalSeg, uint8_t seqNum)
{
    std::shared_ptr<GsmSmsMessage> message = std::make_shared<GsmSmsMessage>();
    message->smsUserData_.headerCnt = 1;
    message->smsUserData_.header[0].udhType = UDH_CONCAT_8BIT;
    message->smsUserData_.header[0].udh.concat8bit.msgRef = 0x2A;
    message->smsUserData_.header[0].udh.concat8bit.totalSeg = totalSeg;
    message->smsUserData_.header[0].udh.concat8bit.seqNum = seqNum;
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    std::shared_ptr<SmsConcat> concat = smsReceiveHandler->GetValidConcatMsg(message);
    if (concat == nullptr) {
        return false;
    }
    return (concat->totalSeg == totalSeg) && (concat->seqNum == seqNum);
}
} // namespace

/**
 * @tc.number   Telephony_SmsMmsGtest_GetValidConcatMsg_0001
 * @tc.name     Test GetValidConcatMsg
 * @tc.desc     3GPP TS 23.040 9.2.3.24.1: a concat IE with total 0, seq 0 or seq > total is ignored
 */
HWTEST_F(BranchSmsPartTest, GetValidConcatMsg_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    EXPECT_EQ(smsReceiveHandler->GetValidConcatMsg(nullptr), nullptr);
    EXPECT_EQ(smsReceiveHandler->GetValidConcatMsg(std::make_shared<GsmSmsMessage>()), nullptr);
    EXPECT_FALSE(IsConcatIeAccepted(0, 0));
    EXPECT_FALSE(IsConcatIeAccepted(0, 1));
    EXPECT_FALSE(IsConcatIeAccepted(2, 0));
    EXPECT_FALSE(IsConcatIeAccepted(2, 3));
    EXPECT_FALSE(IsConcatIeAccepted(127, 128));
    EXPECT_FALSE(IsConcatIeAccepted(254, 255));
    EXPECT_TRUE(IsConcatIeAccepted(1, 1));
    EXPECT_TRUE(IsConcatIeAccepted(2, 1));
    EXPECT_TRUE(IsConcatIeAccepted(2, 2));
    EXPECT_TRUE(IsConcatIeAccepted(128, 128));
    EXPECT_TRUE(IsConcatIeAccepted(255, 255));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsReceiveHandler_0002
 * @tc.name     Test HandleNormalSmsByType with an invalid concat IE
 * @tc.desc     seq 3 > total 2: the message is stored and delivered as a single message, the subsection
 *              table is neither queried for repeated parts nor for combining, nothing is deleted
 */
HWTEST_F(BranchSmsPartTest, GsmSmsReceiveHandler_0002, Function | MediumTest | Level1)
{
    // SMS-DELIVER, 8-bit data, UDH 05 00 03 2A 02 03: concat IE with seq 3 > total 2, payload "hi"
    const std::string concatSeqOverTotalPdu = "00440B912143658709F1000452109121430000080500032A02036869";
    std::shared_ptr<SmsBaseMessage> baseMessage = GsmSmsMessage::CreateMessage(concatSeqOverTotalPdu);
    ASSERT_NE(baseMessage, nullptr);
    ASSERT_TRUE(baseMessage->IsConcatMsg());
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _)).Times(0);
    EXPECT_CALL(*dataShareHelperMock, Insert(_, _)).Times(1).WillOnce(Return(1));
    EXPECT_CALL(*dataShareHelperMock, Release()).WillRepeatedly(Return(true));
    std::shared_ptr<GsmSmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    EXPECT_EQ(smsReceiveHandler->HandleNormalSmsByType(baseMessage), AckIncomeCause::SMS_ACK_RESULT_OK);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

namespace {
constexpr const char *SENDER_A = "12345678901";
constexpr const char *SENDER_B = "10987654321";
// SMS-DELIVER, 8-bit data, UDH 05 00 03 2A 02 01 (ref 0x2A, total 2, seq 1), payload "hi"
constexpr const char *CONCAT_SEG1_PDU = "00440B912143658709F1000452109121430000080500032A02016869";

SmsReceiveIndexer MakeDbSegment(const std::string &address, uint16_t refId, uint16_t seqId, uint16_t msgCount)
{
    return SmsReceiveIndexer(StringUtils::HexToByteVector(CONCAT_SEG1_PDU), 0, -1, false, address, address, refId,
        seqId, msgCount, false, "");
}
} // namespace

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveReliabilityHandler_0004
 * @tc.name     Test GetSmsUserDataMultipage grouping key
 * @tc.desc     3GPP TS 23.040 9.2.3.24.1: segments are grouped by sender, reference number and total, a single
 *              message or a segment of another sender with the same reference number is not combined
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveReliabilityHandler_0004, Function | MediumTest | Level1)
{
    const uint16_t refId = 0;
    const uint16_t total = 2;
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    std::vector<SmsReceiveIndexer> dbIndexers;
    dbIndexers.push_back(MakeDbSegment(SENDER_A, refId, 2, total));
    dbIndexers.push_back(MakeDbSegment(SENDER_B, refId, 1, 1));
    dbIndexers.push_back(MakeDbSegment(SENDER_B, refId, 1, total));
    dbIndexers.push_back(MakeDbSegment(SENDER_A, refId, 1, static_cast<uint16_t>(total + 1)));
    auto pdus = std::make_shared<std::vector<std::string>>();
    int32_t smsPagesCount = 1;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, total, dbIndexers, 0, pdus);
    EXPECT_EQ(smsPagesCount, 1);
    EXPECT_EQ(dbIndexers.size(), 4);
    ASSERT_EQ(pdus->size(), total);
    EXPECT_TRUE(pdus->at(0).empty());
    EXPECT_FALSE(pdus->at(1).empty());
 
    dbIndexers.push_back(MakeDbSegment(SENDER_A, refId, 1, total));
    smsPagesCount = 1;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, total, dbIndexers, 0, pdus);
    EXPECT_EQ(smsPagesCount, total);
    EXPECT_EQ(dbIndexers.size(), 4);
    EXPECT_FALSE(pdus->at(0).empty());
    EXPECT_EQ(dbIndexers[1].GetOriginatingAddress(), SENDER_B);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveReliabilityHandler_0005
 * @tc.name     Test GetWapPushUserDataMultipage grouping key
 * @tc.desc     a WAP push segment of another sender with the same reference number is not combined
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveReliabilityHandler_0005, Function | MediumTest | Level1)
{
    const uint16_t refId = 1;
    const uint16_t total = 2;
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(0);
    std::vector<SmsReceiveIndexer> dbIndexers;
    dbIndexers.push_back(MakeDbSegment(SENDER_A, refId, 1, total));
    dbIndexers.push_back(MakeDbSegment(SENDER_B, refId, 2, total));
    auto userDataRaws = std::make_shared<std::vector<std::string>>();
    userDataRaws->assign(MAX_SEGMENT_NUM, "");
    int32_t smsPagesCount = 1;
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 0, userDataRaws);
    EXPECT_EQ(smsPagesCount, 1);
    EXPECT_EQ(dbIndexers.size(), total);
    EXPECT_TRUE(userDataRaws->at(1).empty());
 
    dbIndexers.push_back(MakeDbSegment(SENDER_A, refId, 2, total));
    reliabilityHandler->GetWapPushUserDataMultipage(smsPagesCount, dbIndexers, 0, userDataRaws);
    EXPECT_EQ(smsPagesCount, total);
    EXPECT_EQ(dbIndexers.size(), total);
    EXPECT_EQ(dbIndexers[1].GetOriginatingAddress(), SENDER_B);
}

namespace {
constexpr int32_t SECOND_SLOT_ID = 1;

bool HasSlotIdPredicate(const DataShare::DataSharePredicates &predicates, int32_t slotId)
{
    const size_t fieldAndValue = 2;
    for (const auto &item : predicates.GetOperationList()) {
        if (item.operation != DataShare::EQUAL_TO || item.singleParams.size() < fieldAndValue) {
            continue;
        }
        std::string field = item.GetSingle(0);
        std::string value = item.GetSingle(1);
        if (field == SmsSubsection::SLOT_ID && value == std::to_string(slotId)) {
            return true;
        }
    }
    return false;
}
} // namespace

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsReceiveSlotId_0001
 * @tc.name     Test slot id of subsection query and delete
 * @tc.desc     3GPP TS 23.040 9.2.3.24.1: segments received on different SIM slots belong to different receiving
 *              entities, combine / repeated check / delete after broadcast are restricted to the receiving slot
 */
HWTEST_F(BranchSmsPartTest, SmsReceiveSlotId_0001, Function | MediumTest | Level1)
{
    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    int queryWithSlot = 0;
    int deleteWithSlot = 0;
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _))
        .WillRepeatedly(Invoke([&queryWithSlot](Uri &uri, const DataShare::DataSharePredicates &predicates,
            std::vector<std::string> &columns, DatashareBusinessError *businessError)
            -> std::shared_ptr<DataShare::DataShareResultSet> {
            queryWithSlot += HasSlotIdPredicate(predicates, SECOND_SLOT_ID) ? 1 : 0;
            return nullptr;
        }));
    EXPECT_CALL(*dataShareHelperMock, Delete(_, _))
        .WillRepeatedly(Invoke([&deleteWithSlot](Uri &uri, const DataShare::DataSharePredicates &predicates) -> int {
            deleteWithSlot += HasSlotIdPredicate(predicates, SECOND_SLOT_ID) ? 1 : 0;
            return 0;
        }));
    EXPECT_CALL(*dataShareHelperMock, Release()).WillRepeatedly(Return(true));

    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(SECOND_SLOT_ID);
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(SECOND_SLOT_ID);
    auto indexer = std::make_shared<SmsReceiveIndexer>(MakeDbSegment(SENDER_A, 1, 1, 2));
    auto pdus = std::make_shared<vector<string>>();
    EXPECT_FALSE(smsReceiveHandler->IsRepeatedMessagePart(indexer));
    EXPECT_FALSE(smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler));
    EXPECT_EQ(queryWithSlot, 2);
    reliabilityHandler->DeleteMessageFormDb(1, 1, 2, SENDER_A);
    EXPECT_EQ(deleteWithSlot, 1);

    using namespace EventFwk;
    MatchingSkills smsSkills;
    smsSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SMS_RECEIVE_COMPLETED);
    CommonEventSubscribeInfo smsSubscriberInfo(smsSkills);
    auto receiver = std::make_shared<SmsBroadcastSubscriberReceiver>(smsSubscriberInfo);
    Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED);
    want.SetParam("slotId", static_cast<int>(SECOND_SLOT_ID));
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_ADDRESS_KEY, std::string(SENDER_A));
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_DATABASE_ID_KEY, 1);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_MSG_REF_ID_KEY, 1);
    want.SetParam(SmsBroadcastSubscriberReceiver::SMS_BROADCAST_MSG_COUNT_KEY, 2);
    CommonEventData data;
    data.SetWant(want);
    receiver->OnReceiveEvent(data);
    EXPECT_EQ(deleteWithSlot, 2);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsReceiveHandler_0004
 * @tc.name     Test HandleRemainDataShare with a Type 0 message and port 5499
 * @tc.desc     3GPP TS 23.040 9.2.3.9: a Type 0 short message queued while DataShare was not ready is not stored
 *              or shown, the same as HandleSmsByType; port 5499 is dropped as in HandleNormalSmsByType
 */
HWTEST_F(BranchSmsPartTest, GsmSmsReceiveHandler_0004, Function | MediumTest | Level1)
{
    // SMS-DELIVER, TP-PID 0x40 (Type 0), GSM 7 bit "hello"
    const std::string typeZeroPdu = "00040B912143658709F140005210912143000005E8329BFD06";
    // SMS-DELIVER, 8-bit data, UDH 06 05 04 157B 0000: destination port 5499, payload "hi"
    const std::string invalidPortPdu = "00440B912143658709F1000452109121430000" "09060504157B00006869";
    std::shared_ptr<SmsBaseMessage> typeZeroMessage = GsmSmsMessage::CreateMessage(typeZeroPdu);
    ASSERT_NE(typeZeroMessage, nullptr);
    ASSERT_TRUE(static_cast<GsmSmsMessage *>(typeZeroMessage.get())->IsSpecialMessage());
    std::shared_ptr<SmsBaseMessage> invalidPortMessage = GsmSmsMessage::CreateMessage(invalidPortPdu);
    ASSERT_NE(invalidPortMessage, nullptr);
    const uint16_t invalidPort = 5499;
    ASSERT_FALSE(static_cast<GsmSmsMessage *>(invalidPortMessage.get())->IsSpecialMessage());
    ASSERT_EQ(static_cast<GsmSmsMessage *>(invalidPortMessage.get())->GetDestPort(), invalidPort);

    auto dataShareHelperMock = std::make_shared<DataShareHelperMock>();
    DelayedSingleton<SmsPersistHelper>::GetInstance()->smsDataShareHelper_ = dataShareHelperMock;
    EXPECT_CALL(*dataShareHelperMock, Insert(_, _)).Times(0);
    EXPECT_CALL(*dataShareHelperMock, Query(_, _, _, _)).Times(0);
    EXPECT_CALL(*dataShareHelperMock, Delete(_, _)).Times(0);
    EXPECT_CALL(*dataShareHelperMock, Release()).WillRepeatedly(Return(true));
    std::shared_ptr<GsmSmsReceiveHandler> smsReceiveHandler = std::make_shared<GsmSmsReceiveHandler>(0);
    // Type 0 and port 5499: both return before any database access, neither is stored nor combined
    smsReceiveHandler->HandleRemainDataShare(typeZeroMessage);
    smsReceiveHandler->HandleRemainDataShare(invalidPortMessage);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->ReleaseDataShareHelper();
}
} // namespace Telephony
} // namespace OHOS
