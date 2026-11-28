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

#define private public
#define protected public

#include "cdma_sms_message.h"
#include "cdma_sms_teleservice_message.h"
#include "cdma_sms_transport_message.h"
#include "core_service_client.h"
#include "delivery_short_message_callback_stub.h"
#include "gtest/gtest.h"
#include "i_sms_service_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "radio_event.h"
#include "sms_mms_gtest.h"
#include "sms_mms_test_helper.h"
#include "sms_service.h"
#include "sms_service_manager_client.h"
#include "string_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
sptr<ISmsServiceInterface> g_telephonyService = nullptr;
} // namespace
using namespace testing::ext;

class CdmaSmsGtest : public testing::Test {
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
void CdmaSmsGtest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void CdmaSmsGtest::SetUp() {}

void CdmaSmsGtest::TearDown()
{
    DelayedSingleton<SmsPersistHelper>::DestroyInstance();
}

const int32_t DEFAULT_SIM_SLOT_ID_1 = 1;
const uint8_t HEX_CHAR_LEN = 2;
const uint8_t UNICODE_CHAR_LEN = 2;
const uint8_t VALUE_INDEX = 2;
const char *CDMA_PDU = "01010101";
const char *OTHER_CDMA_PDU = "111111";

void CdmaSmsGtest::SetUpTestCase()
{
    TELEPHONY_LOGI("SetUpTestCase slotId%{public}d", DEFAULT_SIM_SLOT_ID_1);
    g_telephonyService = GetProxy();
    if (g_telephonyService == nullptr) {
        return;
    }
    Singleton<SmsServiceManagerClient>::GetInstance().ResetSmsServiceProxy();
    Singleton<SmsServiceManagerClient>::GetInstance().InitSmsServiceProxy();
}

sptr<ISmsServiceInterface> CdmaSmsGtest::GetProxy()
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
 * @tc.number   Telephony_SmsMmsGtest_CdmaSmsMessage_0001
 * @tc.name     Test CdmaSmsMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsMessage_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsMessage_0001 -->");
    CdmaSmsMessage cdmaSmsMessage;
    std::string dest = "dest";
    std::string sc = "sc";
    std::string text = "text";
    int32_t port = 10;
    uint8_t *data;
    uint32_t dataLen = 10;
    std::string pdu = "01000B818176251308F4000007E8B0BCFD76E701";
    bool bStatusReport = false;
    DataCodingScheme codingScheme = DATA_CODING_7BIT;
    cdmaSmsMessage.CreateSubmitTransMsg(dest, sc, text, bStatusReport, codingScheme);
    cdmaSmsMessage.CreateSubmitTransMsg(dest, sc, port, data, dataLen, bStatusReport);
    cdmaSmsMessage.GreateTransMsg();
    cdmaSmsMessage.CovertEncodingType(codingScheme);
    cdmaSmsMessage.CreateMessage(pdu);
    cdmaSmsMessage.PduAnalysis(pdu);
    CdmaP2PMsg p2pMsg;
    cdmaSmsMessage.AnalysisP2pMsg(p2pMsg);
    cdmaSmsMessage.AnalsisDeliverMwi(p2pMsg);
    bool ret = cdmaSmsMessage.PduAnalysis("");
    EXPECT_EQ(false, ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CdmaSmsReceiveHandler_0001
 * @tc.name     Test CdmaSmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsReceiveHandler_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsReceiveHandler_0001 -->");
    auto smsReceiveManager = std::make_shared<SmsReceiveManager>(DEFAULT_SIM_SLOT_ID);
    EXPECT_TRUE(smsReceiveManager != nullptr);
    smsReceiveManager->Init();
    EXPECT_TRUE(smsReceiveManager->cdmaSmsReceiveHandler_ != nullptr);
    int32_t retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(nullptr);
    EXPECT_EQ(AckIncomeCause::SMS_ACK_UNKNOWN_ERROR, retInt);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CdmaSmsReceiveHandler_0002
 * @tc.name     Test CdmaSmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsReceiveHandler_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsReceiveHandler_0002 -->");
    auto smsReceiveManager = std::make_shared<SmsReceiveManager>(DEFAULT_SIM_SLOT_ID);
    EXPECT_TRUE(smsReceiveManager != nullptr);
    smsReceiveManager->Init();
    EXPECT_TRUE(smsReceiveManager->cdmaSmsReceiveHandler_ != nullptr);
    auto retMsg = std::make_shared<CdmaSmsMessage>();
    retMsg = CdmaSmsMessage::CreateMessage(CDMA_PDU);
    EXPECT_EQ(retMsg, nullptr);
    auto cdmaMsg = std::make_shared<CdmaSmsMessage>();
    cdmaMsg->transMsg_ = std::make_unique<struct CdmaTransportMsg>();
    cdmaMsg->transMsg_->type = CdmaTransportMsgType::BROADCAST;
    int32_t retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(cdmaMsg);
    EXPECT_EQ(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
    cdmaMsg->transMsg_->type = CdmaTransportMsgType::P2P;
    retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(cdmaMsg);
    EXPECT_EQ(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
    cdmaMsg->transMsg_->type = CdmaTransportMsgType::ACK;
    retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(cdmaMsg);
    EXPECT_EQ(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
    cdmaMsg->transMsg_->type = CdmaTransportMsgType::RESERVED;
    retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsByType(cdmaMsg);
    EXPECT_EQ(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CdmaSmsReceiveHandler_0003
 * @tc.name     Test CdmaSmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsReceiveHandler_0003, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsReceiveHandler_0003 -->");
    auto smsReceiveManager = std::make_shared<SmsReceiveManager>(DEFAULT_SIM_SLOT_ID);
    EXPECT_TRUE(smsReceiveManager != nullptr);
    smsReceiveManager->Init();
    EXPECT_TRUE(smsReceiveManager->cdmaSmsReceiveHandler_ != nullptr);
    int32_t retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsOtherSvcid(nullptr);
    EXPECT_NE(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
    auto retMsg = CdmaSmsMessage::CreateMessage(OTHER_CDMA_PDU);
    EXPECT_EQ(retMsg, nullptr);

    auto cdmaMsg = std::make_shared<CdmaSmsMessage>();
    smsReceiveManager->cdmaSmsReceiveHandler_->ReplySmsToSmsc(1);
    retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsOtherSvcid(cdmaMsg);
    EXPECT_NE(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
    cdmaMsg->smsConcat_ = std::make_shared<SmsConcat>();
    cdmaMsg->smsConcat_->is8Bits = true;
    cdmaMsg->smsConcat_->msgRef = 1;
    cdmaMsg->smsConcat_->seqNum = 1;
    cdmaMsg->smsConcat_->totalSeg = 1;
    retInt = smsReceiveManager->cdmaSmsReceiveHandler_->HandleSmsOtherSvcid(cdmaMsg);
    EXPECT_NE(AckIncomeCause::SMS_ACK_RESULT_OK, retInt);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_CdmaSmsReceiveHandler_0004
 * @tc.name     Test CdmaSmsReceiveHandler
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsReceiveHandler_0004, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsReceiveHandler_0004 -->");
    auto smsReceiveManager = std::make_shared<SmsReceiveManager>(DEFAULT_SIM_SLOT_ID);
    EXPECT_TRUE(smsReceiveManager != nullptr);
    smsReceiveManager->Init();
    EXPECT_TRUE(smsReceiveManager->cdmaSmsReceiveHandler_ != nullptr);
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = nullptr;
    auto smsSender = std::make_shared<CdmaSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    smsReceiveManager->cdmaSmsReceiveHandler_->SetCdmaSender(smsSender);
    auto info = std::make_shared<SmsMessageInfo>();
    auto ret = smsReceiveManager->cdmaSmsReceiveHandler_->TransformMessageInfo(info);
    EXPECT_EQ(ret, nullptr);
    smsReceiveManager->cdmaSmsReceiveHandler_->SendCBBroadcast(nullptr);
    auto cdmaMsg = std::make_shared<CdmaSmsMessage>();
    EXPECT_EQ(smsReceiveManager->cdmaSmsReceiveHandler_->SendCBBroadcast(cdmaMsg), true);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_SmsPduBuffer_0001
 * @tc.name     Test SmsPduBuffer
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, SmsPduBuffer_0001, Function | MediumTest | Level1)
{
    auto buffer = std::make_shared<SmsPduBuffer>();
    EXPECT_TRUE(buffer->IsEmpty());
    EXPECT_EQ(buffer->GetIndex(), 0);
    EXPECT_FALSE(buffer->SetIndex(1));
    EXPECT_EQ(buffer->MoveForward(), 0);
    EXPECT_EQ(buffer->SkipBits(), 0);

    std::string pduHex = "00000210020000021002";
    std::string pdu = StringUtils::HexToString(pduHex);
    auto rBuffer = std::make_shared<SmsReadBuffer>(pdu);
    EXPECT_FALSE(rBuffer->IsEmpty());
    rBuffer->SetIndex(0);
    uint8_t v = 0;
    uint16_t v2 = 0;
    EXPECT_TRUE(rBuffer->ReadByte(v));
    EXPECT_TRUE(rBuffer->ReadWord(v2));
    EXPECT_TRUE(rBuffer->ReadBits(v));

    auto wBuffer = std::make_shared<SmsWriteBuffer>();
    EXPECT_FALSE(wBuffer->IsEmpty());
    wBuffer->SetIndex(0);
    v = 1;
    v2 = 0x1234;
    EXPECT_TRUE(wBuffer->WriteByte(v));
    EXPECT_TRUE(wBuffer->WriteWord(v2));
    EXPECT_TRUE(wBuffer->InsertByte(v, 1));
    EXPECT_TRUE(wBuffer->WriteBits(v));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsTransportMessage_0001
 * @tc.name     Test CdmaSmsTransportMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsTransportMessage_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsTransportMessage_0001 -->");
    CdmaSmsMessage cdmaSmsMessage;
    std::string dest = "dest";
    std::string sc = "sc";
    std::string text = "text";
    bool bStatusReport = false;
    DataCodingScheme codingScheme = DATA_CODING_7BIT;
    std::unique_ptr<CdmaTransportMsg> transMsg =
        cdmaSmsMessage.CreateSubmitTransMsg(dest, sc, text, bStatusReport, codingScheme);

    std::unique_ptr<CdmaSmsTransportMessage> transportMessage =
        CdmaSmsTransportMessage::CreateTransportMessage(*transMsg.get());
    SmsWriteBuffer pduBuffer;
    EXPECT_NE(transportMessage, nullptr);
    EXPECT_NE(transportMessage->IsEmpty(), true);
    EXPECT_EQ(transportMessage->Encode(pduBuffer), true);
    std::unique_ptr<std::vector<uint8_t>> pdu = pduBuffer.GetPduBuffer();
    EXPECT_NE(pdu, nullptr);
    EXPECT_GT(pdu->size(), static_cast<uint32_t>(0));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsTransportMessage_0002
 * @tc.name     Test CdmaSmsTransportMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsTransportMessage_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::CdmaSmsTransportMessage_0002 -->");
    std::string pduHex = "0000021002040702C48D159E268406010408260003200640011910D61C58F265CD9F469D5AF66DDDBF871E5CFA75E"
                         "DDFC79F400801000A0140";
    std::string pdu = StringUtils::HexToString(pduHex);
    SmsReadBuffer pduBuffer(pdu);
    CdmaTransportMsg msg;
    memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    std::unique_ptr<CdmaSmsTransportMessage> transportMessage =
        CdmaSmsTransportMessage::CreateTransportMessage(msg, pduBuffer);
    EXPECT_NE(transportMessage, nullptr);
    EXPECT_NE(transportMessage->IsEmpty(), true);
    EXPECT_EQ(transportMessage->Decode(pduBuffer), true);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsTransportMessage_0003
 * @tc.name     Test CdmaSmsTransportMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsTransportMessage_0003, Function | MediumTest | Level1)
{
    // BROADCAST DELIVER CMASDATA
    uint8_t cmasTestBearerData[] = { 0x00, 0x03, 0x1C, 0x78, 0x00, 0x01, 0x59, 0x02, 0xB8, 0x00, 0x02, 0x10, 0xAA,
        0x68, 0xD3, 0xCD, 0x06, 0x9E, 0x68, 0x30, 0xA0, 0xE9, 0x97, 0x9F, 0x44, 0x1B, 0xF3, 0x20, 0xE9, 0xA3, 0x2A,
        0x08, 0x7B, 0xF6, 0xED, 0xCB, 0xCB, 0x1E, 0x9C, 0x3B, 0x10, 0x4D, 0xDF, 0x8B, 0x4E, 0xCC, 0xA8, 0x20, 0xEC,
        0xCB, 0xCB, 0xA2, 0x0A, 0x7E, 0x79, 0xF4, 0xCB, 0xB5, 0x72, 0x0A, 0x9A, 0x34, 0xF3, 0x41, 0xA7, 0x9A, 0x0D,
        0xFB, 0xB6, 0x79, 0x41, 0x85, 0x07, 0x4C, 0xBC, 0xFA, 0x2E, 0x00, 0x08, 0x20, 0x58, 0x38, 0x88, 0x80, 0x10,
        0x54, 0x06, 0x38, 0x20, 0x60, 0x30, 0xA8, 0x81, 0x90, 0x20, 0x08 };

    std::stringstream ss;
    ss.clear();
    ss << static_cast<uint8_t>(CdmaTransportMsgType::BROADCAST);
    ss << static_cast<uint8_t>(CdmaSmsParameterRecord::SERVICE_CATEGORY);
    ss << static_cast<uint8_t>(0x02);
    ss << static_cast<uint8_t>(static_cast<uint16_t>(SmsServiceCtg::CMAS_TEST) >> 8);
    ss << static_cast<uint8_t>(static_cast<uint16_t>(SmsServiceCtg::CMAS_TEST) & 0xff);
    ss << static_cast<uint8_t>(CdmaSmsParameterRecord::BEARER_DATA);
    ss << static_cast<uint8_t>(sizeof(cmasTestBearerData));
    for (uint8_t i = 0; i < sizeof(cmasTestBearerData); i++) {
        ss << cmasTestBearerData[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    CdmaTransportMsg msg;
    memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    std::unique_ptr<CdmaSmsTransportMessage> message = CdmaSmsTransportMessage::CreateTransportMessage(msg, rBuffer);
    EXPECT_TRUE(message->Decode(rBuffer));
    EXPECT_EQ(msg.data.broadcast.telesvcMsg.data.deliver.msgId.msgId, 0xc780);
    EXPECT_EQ(msg.data.broadcast.serviceCtg, static_cast<uint16_t>(SmsServiceCtg::CMAS_TEST));
    EXPECT_EQ(msg.data.broadcast.telesvcMsg.data.deliver.cmasData.dataLen, 74);
    EXPECT_EQ(msg.data.broadcast.telesvcMsg.data.deliver.cmasData.urgency, SmsCmaeUrgency::EXPECTED);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsTransportMessage_0004
 * @tc.name     Test CdmaSmsTransportMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsTransportMessage_0004, Function | MediumTest | Level1)
{
    // BROADCAST DELIVER USERDATA(BCCBB)
    std::string pduHex = "0101020004081300031008d00106102c2870e1420801c00c01c0";
    std::string pdu = StringUtils::HexToString(pduHex);
    SmsReadBuffer pduBuffer(pdu);
    CdmaTransportMsg msg;
    memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    std::unique_ptr<CdmaSmsTransportMessage> transportMessage =
        CdmaSmsTransportMessage::CreateTransportMessage(msg, pduBuffer);
    EXPECT_NE(transportMessage, nullptr);
    EXPECT_FALSE(transportMessage->IsEmpty());
    EXPECT_TRUE(transportMessage->Decode(pduBuffer));
}

string GetUserDataString(SmsTeleSvcUserData userData)
{
    std::stringstream ssUserData;
    if (userData.encodeType == SmsEncodingType::UNICODE) {
        char unicodeChar[UNICODE_CHAR_LEN + 1];
        unicodeChar[UNICODE_CHAR_LEN] = '\0';
        for (uint8_t i = 0; i < userData.userData.length; i += UNICODE_CHAR_LEN) {
            ssUserData << "\\u";
            snprintf_s(unicodeChar, sizeof(unicodeChar), sizeof(unicodeChar) - 1, "%02x",
                static_cast<uint8_t>(userData.userData.data[i]));
            ssUserData << unicodeChar;
            snprintf_s(unicodeChar, sizeof(unicodeChar), sizeof(unicodeChar) - 1, "%02x",
                static_cast<uint8_t>(userData.userData.data[i + 1]));
            ssUserData << unicodeChar;
        }
    } else {
        for (uint8_t i = 0; i < userData.userData.length; i++) {
            ssUserData << static_cast<uint8_t>(userData.userData.data[i]);
        }
    }
    return ssUserData.str();
}

void CheckDeliverUserData(string pdu, string userData)
{
    std::stringstream ss;
    ss << static_cast<uint8_t>(CdmaSmsParameterRecord::BEARER_DATA);
    ss << static_cast<uint8_t>(pdu.size() / HEX_CHAR_LEN);
    ss << StringUtils::HexToString(pdu);

    CdmaTeleserviceMsg v;
    memset_s(&v, sizeof(CdmaTeleserviceMsg), 0x00, sizeof(CdmaTeleserviceMsg));
    SmsReadBuffer rBuffer(ss.str());
    rBuffer.SetIndex(VALUE_INDEX);
    auto message = std::make_shared<CdmaSmsBearerData>(v, rBuffer);
    rBuffer.SetIndex(0);
    EXPECT_TRUE(message->Decode(rBuffer));
    EXPECT_STREQ(GetUserDataString(v.data.deliver.userData).c_str(), userData.c_str());
}

void CheckSubmitUserData(string pdu, string userData)
{
    std::stringstream ss;
    ss << static_cast<uint8_t>(CdmaSmsParameterRecord::BEARER_DATA);
    ss << static_cast<uint8_t>(pdu.size() / HEX_CHAR_LEN);
    ss << StringUtils::HexToString(pdu);

    CdmaTeleserviceMsg v;
    memset_s(&v, sizeof(CdmaTeleserviceMsg), 0x00, sizeof(CdmaTeleserviceMsg));
    SmsReadBuffer rBuffer(ss.str());
    rBuffer.SetIndex(VALUE_INDEX);
    auto message = std::make_shared<CdmaSmsBearerData>(v, rBuffer);
    rBuffer.SetIndex(0);
    EXPECT_TRUE(message->Decode(rBuffer));
    EXPECT_STREQ(GetUserDataString(v.data.submit.userData).c_str(), userData.c_str());

    auto message1 = std::make_shared<CdmaSmsBearerData>(v);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ssEncode;
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ssEncode << (*buffer)[i];
    }

    CdmaTeleserviceMsg v2;
    memset_s(&v2, sizeof(CdmaTeleserviceMsg), 0x00, sizeof(CdmaTeleserviceMsg));
    SmsReadBuffer rBuffer2(ssEncode.str());
    rBuffer2.SetIndex(VALUE_INDEX);
    auto message2 = std::make_shared<CdmaSmsBearerData>(v2, rBuffer2);
    rBuffer2.SetIndex(0);
    EXPECT_TRUE(message2->Decode(rBuffer2));
    EXPECT_STREQ(GetUserDataString(v2.data.submit.userData).c_str(), userData.c_str());
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsBearerData_0001
 * @tc.name     Test CdmaSmsBearerData
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsBearerData_0001, Function | MediumTest | Level1)
{
    // gsm 7bit
    CheckDeliverUserData("00031040900112488ea794e074d69e1b7392c270326cde9e98", "Test standard SMS");
    // ascii 7bit
    CheckDeliverUserData("0003100160010610262d5ab500", "bjjj");
    // ia5
    CheckDeliverUserData("00031002100109184539b4d052ebb3d0", "SMS Rulz");
    // unicode
    CheckDeliverUserData("000310021001062012716B2C380801000A0140", "\\u4e2d\\u6587");

    // gsm 7bit
    CheckSubmitUserData("00032006400112488ea794e074d69e1b7392c270326cde9e98", "Test standard SMS");
    // ascii 7bit
    CheckSubmitUserData("0003200640010610262d5ab500", "bjjj");
    // ia5
    CheckSubmitUserData("00032006400109184539b4d052ebb3d0", "SMS Rulz");
    // unicode
    CheckSubmitUserData("000320064001062012716B2C380801000A0140", "\\u4e2d\\u6587");
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsBearerData_0002
 * @tc.name     Test CdmaSmsBearerData
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsBearerData_0002, Function | MediumTest | Level1)
{
    std::string dataStr = "0003200010010410168d20020105030608120111015905019206069706180000000801c00901800a01e00b"
                          "01030c01c00d01070e05039acc13880f018011020566";
    std::stringstream ss;
    ss.clear();
    ss << static_cast<uint8_t>(CdmaSmsParameterRecord::BEARER_DATA);
    ss << static_cast<uint8_t>(dataStr.size() / HEX_CHAR_LEN);
    ss << StringUtils::HexToString(dataStr);

    CdmaTeleserviceMsg v;
    memset_s(&v, sizeof(CdmaTeleserviceMsg), 0x00, sizeof(CdmaTeleserviceMsg));
    SmsReadBuffer rBuffer(ss.str());
    rBuffer.SetIndex(VALUE_INDEX);
    auto message = std::make_shared<CdmaSmsBearerData>(v, rBuffer);
    rBuffer.SetIndex(0);
    EXPECT_TRUE(message->Decode(rBuffer));
    EXPECT_EQ(v.type, TeleserviceMsgType::SUBMIT);
    EXPECT_EQ(v.data.submit.msgId.msgId, 1);
    EXPECT_EQ(v.data.submit.priority, SmsPriorityIndicator::EMERGENCY);
    EXPECT_EQ(v.data.submit.privacy, SmsPrivacyIndicator::CONFIDENTIAL);
    EXPECT_EQ(v.data.submit.callbackNumber.addrLen, 7);
    char number[] = "3598271";
    for (uint8_t i = 0; i < sizeof(number); i++) {
        EXPECT_EQ(v.data.submit.callbackNumber.szData[i], number[i]);
    }
    EXPECT_EQ(v.data.submit.depositId, 1382);
    EXPECT_EQ(v.data.submit.language, SmsLanguageType::HEBREW);
    EXPECT_EQ(v.data.submit.alertPriority, SmsAlertPriority::HIGH);
    EXPECT_EQ(v.data.submit.deferValPeriod.time.absTime.year, 97);
    EXPECT_EQ(v.data.submit.deferValPeriod.time.absTime.month, 6);
    EXPECT_EQ(v.data.submit.deferValPeriod.time.absTime.day, 18);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSender_0001
 * @tc.name     Test CdmaSmsSender
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSender_0001, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = [](std::shared_ptr<SmsSendIndexer> indexer) {};
    std::shared_ptr<CdmaSmsSender> smsSender = std::make_shared<CdmaSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    int i = 0;
    std::vector<struct SplitInfo> cellsInfos;
    SplitInfo info;
    for (uint8_t j = 0; j < MAX_USER_DATA_LEN + 2; j++) {
        info.encodeData.push_back(j);
    }
    cellsInfos.push_back(info);
    GsmSmsMessage gsmSmsMessage;
    std::shared_ptr<struct SmsTpdu> tpdu = std::make_shared<struct SmsTpdu>();
    std::shared_ptr<uint8_t> unSentCellCount = std::make_shared<uint8_t>(0);
    std::shared_ptr<bool> hasCellFailed = std::make_shared<bool>(false);
    smsSender->SendSmsForEveryIndexer(i, cellsInfos, "", "", tpdu, gsmSmsMessage, unSentCellCount,
        hasCellFailed, DATA_CODING_7BIT, 0, nullptr, nullptr, 0, false);
    
    info.encodeData.clear();
    for (uint8_t j = 0; j < MAX_USER_DATA_LEN; j++) {
        info.encodeData.push_back(j);
    }
    cellsInfos.push_back(info);
    i = 1;
    smsSender->SendSmsForEveryIndexer(i, cellsInfos, "", "", tpdu, gsmSmsMessage, unSentCellCount,
        hasCellFailed, DATA_CODING_7BIT, 0, nullptr, nullptr, 0, false);
    
    cellsInfos.push_back(info);
    cellsInfos.push_back(info);
    i = 1;
    tpdu->data.submit.bStatusReport = true;
    smsSender->SendSmsForEveryIndexer(i, cellsInfos, "", "", tpdu, gsmSmsMessage, unSentCellCount,
        hasCellFailed, DATA_CODING_7BIT, 0, nullptr, nullptr, 0, false);
    EXPECT_FALSE(tpdu->data.submit.bStatusReport);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSender_0002
 * @tc.name     Test CdmaSmsSender
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSender_0002, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = [](std::shared_ptr<SmsSendIndexer> indexer) {};
    std::shared_ptr<CdmaSmsSender> smsSender = std::make_shared<CdmaSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    std::shared_ptr<SmsSendIndexer> indexer = std::make_shared<SmsSendIndexer>("des", "src", "text", nullptr, nullptr);
    std::shared_ptr<struct EncodeInfo> encodeInfo = std::make_shared<struct EncodeInfo>();
    smsSender->SetSendIndexerInfo(indexer, nullptr, 0);
    smsSender->SetSendIndexerInfo(nullptr, encodeInfo, 0);
    std::unique_ptr<CdmaTransportMsg> transMsg = std::make_unique<CdmaTransportMsg>();
    smsSender->SetPduSeqInfo(indexer, 2, transMsg, 0, 0);
    smsSender->isImsNetDomain_ = true;
    smsSender->imsSmsCfg_ = true;
    std::vector<struct SplitInfo> cellsInfos;
    SplitInfo info;
    for (uint8_t j = 0; j < MAX_USER_DATA_LEN; j++) {
        info.encodeData.push_back(j);
    }
    smsSender->DataBasedSmsDelivery("des", "src", 0, info.encodeData.data(), info.encodeData.size(), nullptr, nullptr);
    smsSender->imsSmsCfg_ = false;
    smsSender->DataBasedSmsDelivery("des", "src", 0, info.encodeData.data(), info.encodeData.size(), nullptr, nullptr);
    std::unique_ptr<CdmaTransportMsg> transMsg2 = std::make_unique<CdmaTransportMsg>();
    smsSender->EncodeMsgData(std::move(transMsg2), indexer, 0, nullptr);
    for (uint8_t j = 0; j < MAX_SEGMENT_NUM + 2; j++) {
        cellsInfos.push_back(info);
    }
    std::shared_ptr<struct SmsTpdu> tpdu = std::make_shared<struct SmsTpdu>();
    std::shared_ptr<uint8_t> unSentCellCount = std::make_shared<uint8_t>(0);
    std::shared_ptr<bool> hasCellFailed = std::make_shared<bool>(false);
    EXPECT_TRUE(smsSender->TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, nullptr, unSentCellCount,
        hasCellFailed, nullptr));
    EXPECT_TRUE(smsSender->TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, tpdu, unSentCellCount,
        hasCellFailed, nullptr));
    cellsInfos.clear();
    EXPECT_TRUE(smsSender->TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, tpdu, nullptr,
        hasCellFailed, nullptr));
    EXPECT_TRUE(smsSender->TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, tpdu, unSentCellCount,
        nullptr, nullptr));
    EXPECT_FALSE(smsSender->TpduNullOrSmsPageOverNormalOrSmsEncodeFail(cellsInfos, tpdu, unSentCellCount,
        hasCellFailed, nullptr));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSender_0003
 * @tc.name     Test CdmaSmsSender
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSender_0003, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = [](std::shared_ptr<SmsSendIndexer> indexer) {};
    std::shared_ptr<CdmaSmsSender> smsSender = std::make_shared<CdmaSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    std::shared_ptr<SmsReceiveIndexer> statusInfo = std::make_shared<SmsReceiveIndexer>();
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(0, statusInfo);
    smsSender->StatusReportAnalysis(event);
    std::vector<uint8_t> pdu =
        { 1, 1, 2, 0, 4, 8, 19, 0, 3, 16, 8, 208, 1, 6, 16, 44, 40, 112, 225, 66, 8, 1, 192, 12, 1, 192 };
    statusInfo->SetPdu(pdu);
    event = AppExecFwk::InnerEvent::Get(0, statusInfo);
    std::shared_ptr<SmsSendIndexer> indexer = std::make_shared<SmsSendIndexer>("des", "src", "text", nullptr, nullptr);
    auto message = CdmaSmsMessage::CreateMessage(StringUtils::StringToHex(pdu));
    ASSERT_NE(message, nullptr);
    indexer->SetMsgRefId(message->GetMsgRef() + 1);
    smsSender->reportList_.push_back(indexer);
    smsSender->StatusReportAnalysis(event);
    indexer->SetMsgRefId(message->GetMsgRef());
    sptr<DeliveryShortMessageCallbackStub> callback = new DeliveryShortMessageCallbackStub();
    indexer->SetDeliveryCallback(callback);
    smsSender->StatusReportAnalysis(event);
    smsSender->StatusReportSetImsSms(event);
    std::shared_ptr<RadioResponseInfo> imsResponseInfo = std::make_shared<RadioResponseInfo>();
    imsResponseInfo->error = ErrType::NONE;
    event = AppExecFwk::InnerEvent::Get(0, imsResponseInfo);
    smsSender->StatusReportSetImsSms(event);
    imsResponseInfo->error = ErrType::ERR_GENERIC_FAILURE;
    event = AppExecFwk::InnerEvent::Get(0, imsResponseInfo);
    smsSender->imsSmsCfg_ = CdmaSmsSender::IMS_SMS_ENABLE;
    smsSender->StatusReportSetImsSms(event);
    EXPECT_EQ(smsSender->imsSmsCfg_, CdmaSmsSender::IMS_SMS_DISABLE);
    std::shared_ptr<int32_t> imsSmsInfo = std::make_shared<int32_t>(CdmaSmsSender::IMS_SMS_ENABLE);
    event = AppExecFwk::InnerEvent::Get(0, imsSmsInfo);
    smsSender->StatusReportGetImsSms(event);
    EXPECT_EQ(smsSender->imsSmsCfg_, CdmaSmsSender::IMS_SMS_ENABLE);
    CdmaTransportMsg msg;
    EXPECT_EQ(smsSender->EncodeMsg(msg), nullptr);
    indexer->SetIsConcat(true);
    std::unique_ptr<CdmaTransportMsg> transMsg = std::make_unique<CdmaTransportMsg>();
    SmsConcat concat;
    concat.is8Bits = false;
    indexer->SetSmsConcat(concat);
    smsSender->SetConcact(indexer, transMsg);
    concat.is8Bits = true;
    indexer->SetSmsConcat(concat);
    smsSender->SetConcact(indexer, transMsg);
    EXPECT_EQ(transMsg->data.p2p.teleserviceId, static_cast<uint16_t>(SmsTransTelsvcId::WEMT));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsTeleserviceMessage_0003
 * @tc.name     Test CdmaSmsTeleserviceMessage
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsTeleserviceMessage_0003, Function | MediumTest | Level1)
{
    SmsWriteBuffer wPdu;
    wPdu.data_ = nullptr;
    SmsReadBuffer rPdu("");
    rPdu.data_ = nullptr;
    CdmaSmsTeleserviceMessage msg;
    EXPECT_FALSE(msg.Encode(wPdu));
    EXPECT_FALSE(msg.Decode(rPdu));
    EXPECT_EQ(msg.GetMessageType(rPdu), CdmaSmsTeleserviceMessage::RESERVED);
    TeleserviceSubmit submit;
    CdmaSmsSubmitMessage submitMsg(submit, rPdu);
    TeleserviceDeliver deliver;
    CdmaSmsDeliverMessage deliverMsg(deliver, rPdu, false);
    TeleserviceDeliverAck deliverAck;
    CdmaSmsDeliveryAck ack(deliverAck, rPdu);
    EXPECT_NE(ack.type_, CdmaSmsTeleserviceMessage::DELIVERY_ACK);
    TeleserviceUserAck userAck;
    CdmaSmsUserAck smsUserAck(userAck, rPdu);
    EXPECT_NE(smsUserAck.type_, CdmaSmsTeleserviceMessage::USER_ACK);
    TeleserviceReadAck readAck;
    CdmaSmsReadAck smsReadAck(readAck, rPdu);
    EXPECT_NE(smsReadAck.type_, CdmaSmsTeleserviceMessage::READ_ACK);
    SmsReadBuffer rPdu2("001020304050");
    CdmaSmsDeliveryAck ack2(deliverAck, rPdu2);
    EXPECT_EQ(ack2.type_, CdmaSmsTeleserviceMessage::DELIVERY_ACK);
    SmsReadBuffer rPdu3("001020304050");
    CdmaSmsUserAck smsUserAck2(userAck, rPdu3);
    EXPECT_EQ(smsUserAck2.type_, CdmaSmsTeleserviceMessage::USER_ACK);
    SmsReadBuffer rPdu4("001020304050");
    CdmaSmsReadAck smsReadAck2(readAck, rPdu4);
    EXPECT_EQ(smsReadAck2.type_, CdmaSmsTeleserviceMessage::READ_ACK);
    SmsReadBuffer rPdu5("1");
    EXPECT_EQ(msg.GetMessageType(rPdu5), CdmaSmsTeleserviceMessage::RESERVED);
    SmsReadBuffer rPdu6("10");
    EXPECT_EQ(msg.GetMessageType(rPdu6), CdmaSmsTeleserviceMessage::RESERVED);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0001
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0001, Function | MediumTest | Level1)
{
    uint8_t data;
    CdmaSmsBaseParameter param(0, data);
    SmsReadBuffer rPdu("1");
    EXPECT_FALSE(param.Decode(rPdu));
    SmsReadBuffer rPdu2("11");
    EXPECT_FALSE(param.Decode(rPdu2));
    SmsTimeAbs timeAbs;
    uint8_t id = static_cast<uint8_t>('0');
    CdmaSmsAbsoluteTime time(id, timeAbs);
    SmsReadBuffer rPdu3("00");
    EXPECT_FALSE(time.Decode(rPdu3));
    SmsReadBuffer rPdu4("001");
    EXPECT_FALSE(time.Decode(rPdu4));
    SmsReadBuffer rPdu5("0011");
    EXPECT_FALSE(time.Decode(rPdu5));
    SmsReadBuffer rPdu6("00111");
    EXPECT_FALSE(time.Decode(rPdu6));
    SmsReadBuffer rPdu7("001111");
    EXPECT_FALSE(time.Decode(rPdu7));
    SmsReadBuffer rPdu8("0011111");
    EXPECT_FALSE(time.Decode(rPdu8));
    SmsReadBuffer rPdu9("00111111");
    EXPECT_TRUE(time.Decode(rPdu9));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0002
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0002, Function | MediumTest | Level1)
{
    SmsTimeAbs timeAbs;
    uint8_t id = static_cast<uint8_t>('0');
    CdmaSmsAbsoluteTime time(id, timeAbs);
    SmsWriteBuffer wPdu;
    wPdu.length_ = 0;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.length_ = 1;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 4;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 5;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 6;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 7;
    EXPECT_FALSE(time.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 8;
    EXPECT_TRUE(time.Encode(wPdu));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0003
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0003, Function | MediumTest | Level1)
{
    uint8_t id = static_cast<uint8_t>('0');
    SmsTeleSvcMsgId value;
    CdmaSmsMessageId message(value, CdmaSmsTeleserviceMessage::DELIVER);
    message.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(message.Decode(rPdu));
    SmsReadBuffer rPdu1("001");
    EXPECT_FALSE(message.Decode(rPdu1));
    SmsReadBuffer rPdu2("0011");
    EXPECT_FALSE(message.Decode(rPdu2));
    SmsReadBuffer rPdu4("001111");
    EXPECT_TRUE(message.Decode(rPdu4));

    SmsWriteBuffer wPdu;
    wPdu.length_ = 0;
    EXPECT_FALSE(message.Encode(wPdu));
    wPdu.length_ = 1;
    EXPECT_FALSE(message.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(message.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_FALSE(message.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 4;
    EXPECT_FALSE(message.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 5;
    EXPECT_TRUE(message.Encode(wPdu));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0004
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0004, Function | MediumTest | Level1)
{
    uint8_t id = static_cast<uint8_t>('0');
    SmsPrivacyIndicator value;
    CdmaSmsPrivacyInd parameter(value);
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));
    SmsReadBuffer rPdu1("001");
    EXPECT_TRUE(parameter.Decode(rPdu1));

    SmsWriteBuffer wPdu;
    wPdu.length_ = 0;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.length_ = 1;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_TRUE(parameter.Encode(wPdu));

    SmsReplyOption op;
    CdmaSmsReplyOption option(op);
    option.id_ = id;
    SmsReadBuffer rPdu2("00");
    EXPECT_FALSE(option.Decode(rPdu2));

    wPdu.index_ = 0;
    wPdu.length_ = 0;
    EXPECT_FALSE(option.Encode(wPdu));
    wPdu.length_ = 1;
    EXPECT_FALSE(option.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(option.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_TRUE(option.Encode(wPdu));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0005
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0005, Function | MediumTest | Level1)
{
    uint8_t id = static_cast<uint8_t>('0');
    SmsTeleSvcUserData value;
    bool headerInd = false;
    CdmaSmsUserData parameter(value, headerInd);
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));
    SmsReadBuffer rPdu1("00" + std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::EPM) << 3)));
    EXPECT_FALSE(parameter.Decode(rPdu1));
    SmsReadBuffer rPdu2("00" +
        std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::GSMDCS) << 3)) + "1");
    EXPECT_FALSE(parameter.Decode(rPdu2));
    SmsReadBuffer rPdu3("00" +
        std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::UNICODE) << 3)) + "1");
    EXPECT_FALSE(parameter.Decode(rPdu3));
    SmsReadBuffer rPdu4("00" +
        std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::GSMDCS) << 3)) + "11");
    EXPECT_FALSE(parameter.Decode(rPdu4));
    SmsReadBuffer rPdu5("00" +
        std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::LATIN) << 3)) + "1");
    EXPECT_FALSE(parameter.Decode(rPdu5));

    SmsReadBuffer rPdu6("");
    parameter.headerInd_ = true;
    EXPECT_EQ(parameter.DecodeHeader7Bit(rPdu6), 0);
    SmsReadBuffer rPdu7("0");
    EXPECT_EQ(parameter.DecodeHeader7Bit(rPdu7), 0);
    SmsReadBuffer rPdu8("01");
    EXPECT_EQ(parameter.DecodeHeader7Bit(rPdu8), 0);
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0006
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0006, Function | MediumTest | Level1)
{
    SmsTeleSvcCmasData value;
    CdmaSmsCmasData parameter(value);
    uint8_t id = static_cast<uint8_t>('0');
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));
    SmsReadBuffer rPdu1("00" +
        std::string(1, static_cast<char>(static_cast<uint8_t>(SmsEncodingType::EUCKR) << 3)));
    EXPECT_FALSE(parameter.Decode(rPdu1));
    SmsReadBuffer rPdu2("00" +
        std::string(1, static_cast<char>((static_cast<uint8_t>(SmsEncodingType::OCTET) << 3) + 1)));
    EXPECT_FALSE(parameter.Decode(rPdu2));
    SmsReadBuffer rPdu3("00" +
        std::string(1, static_cast<char>((static_cast<uint8_t>(SmsEncodingType::OCTET) << 3) + 1)) + "1");
    EXPECT_FALSE(parameter.Decode(rPdu3));
    SmsReadBuffer rPdu4("00" +
        std::string(1, static_cast<char>((static_cast<uint8_t>(SmsEncodingType::OCTET) << 3) + 1)) + "11");
    EXPECT_FALSE(parameter.Decode(rPdu4));
    SmsReadBuffer rPdu5(std::string(1, static_cast<char>(0)) + "1");
    EXPECT_FALSE(parameter.DecodeData(rPdu5));
    SmsReadBuffer rPdu6(std::string(1, static_cast<char>(1)) + "1");
    EXPECT_FALSE(parameter.DecodeData(rPdu6));
    SmsReadBuffer rPdu7(std::string(1, static_cast<char>(1)) + "11");
    EXPECT_FALSE(parameter.DecodeData(rPdu7));
    SmsReadBuffer rPdu8(std::string(1, static_cast<char>(1)) + "111");
    EXPECT_FALSE(parameter.DecodeData(rPdu8));
    SmsReadBuffer rPdu9(std::string(1, static_cast<char>(2)) + "1");
    EXPECT_FALSE(parameter.DecodeData(rPdu9));
    SmsReadBuffer rPdu10(std::string(1, static_cast<char>(2)) + "11");
    EXPECT_FALSE(parameter.DecodeData(rPdu10));
    SmsReadBuffer rPdu11(std::string(1, static_cast<char>(2)) + "111");
    EXPECT_FALSE(parameter.DecodeData(rPdu11));
    SmsReadBuffer rPdu12(std::string(1, static_cast<char>(2)) + "1111");
    EXPECT_FALSE(parameter.DecodeData(rPdu12));
    SmsReadBuffer rPdu13(std::string(1, static_cast<char>(2)) + "11111");
    EXPECT_FALSE(parameter.DecodeData(rPdu13));
    SmsReadBuffer rPdu14(std::string(1, static_cast<char>(2)) + "111111");
    EXPECT_FALSE(parameter.DecodeData(rPdu14));
    SmsReadBuffer rPdu15(std::string(1, static_cast<char>(2)) + "1111111");
    EXPECT_FALSE(parameter.DecodeData(rPdu15));
    SmsReadBuffer rPdu16(std::string(1, static_cast<char>(2)) + "11111111");
    EXPECT_FALSE(parameter.DecodeData(rPdu16));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0007
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0007, Function | MediumTest | Level1)
{
    uint8_t id = static_cast<uint8_t>('0');
    SmsDisplayMode mode;
    CdmaSmsDisplayMode parameter(mode);
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));

    SmsStatusCode code;
    CdmaSmsMessageStatus status(code);
    status.id_ = id;
    SmsReadBuffer rPdu1("00");
    EXPECT_FALSE(status.Decode(rPdu1));

    uint32_t value = 0;
    CdmaSmsNumberMessages messages(value);
    messages.id_ = id;
    SmsReadBuffer rPdu2("00");
    EXPECT_FALSE(messages.Decode(rPdu2));

    SmsAlertPriority p = SmsAlertPriority::HIGH;
    CdmaSmsAlertPriority priority(p);
    priority.id_ = id;
    SmsReadBuffer rPdu3("00");
    EXPECT_FALSE(priority.Decode(rPdu3));

    SmsLanguageType l = SmsLanguageType::CHINESE;
    CdmaSmsLanguageInd ind(l);
    ind.id_ = id;
    SmsReadBuffer rPdu4("00");
    EXPECT_FALSE(ind.Decode(rPdu4));

    SmsTeleSvcAddr addr;
    CdmaSmsCallbackNumber number(addr);
    number.id_ = id;
    SmsReadBuffer rPdu5("00");
    EXPECT_FALSE(number.Decode(rPdu5));
    SmsReadBuffer rPdu6("00" + std::string(1, static_cast<char>(0b1 << 7)));
    EXPECT_FALSE(number.Decode(rPdu6));
    SmsReadBuffer rPdu7("00" + std::string(1, static_cast<char>(0b1 << 7)) + "11");
    EXPECT_FALSE(number.Decode(rPdu7));
    SmsReadBuffer rPdu8("0011");
    EXPECT_FALSE(number.Decode(rPdu8));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0008
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0008, Function | MediumTest | Level1)
{
    SmsEnhancedVmn value;
    CdmaSmsEnhancedVmn parameter(value);
    uint8_t id = static_cast<uint8_t>('0');
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));
    SmsReadBuffer rPdu1("001");
    EXPECT_FALSE(parameter.Decode(rPdu1));
    SmsReadBuffer rPdu2("0011");
    EXPECT_FALSE(parameter.Decode(rPdu2));
    SmsReadBuffer rPdu3("00111");
    EXPECT_FALSE(parameter.Decode(rPdu3));
    SmsReadBuffer rPdu4("001111");
    EXPECT_FALSE(parameter.Decode(rPdu4));
    SmsReadBuffer rPdu5("0011111");
    EXPECT_FALSE(parameter.Decode(rPdu5));
    SmsReadBuffer rPdu6("00111111");
    EXPECT_FALSE(parameter.Decode(rPdu6));
    SmsReadBuffer rPdu7("001111111");
    EXPECT_FALSE(parameter.Decode(rPdu7));
    SmsReadBuffer rPdu8("0011111111");
    EXPECT_FALSE(parameter.Decode(rPdu8));
    SmsReadBuffer rPdu9("00111111111");
    EXPECT_FALSE(parameter.Decode(rPdu9));
    SmsReadBuffer rPdu10("001111111111");
    EXPECT_FALSE(parameter.Decode(rPdu10));
    SmsReadBuffer rPdu11("0011111111111");
    EXPECT_FALSE(parameter.Decode(rPdu11));
    SmsReadBuffer rPdu12("00111111111111");
    EXPECT_FALSE(parameter.Decode(rPdu12));
    SmsReadBuffer rPdu13("001111111111111");
    EXPECT_FALSE(parameter.Decode(rPdu13));
    SmsReadBuffer rPdu14("0011111111111111");
    EXPECT_FALSE(parameter.Decode(rPdu14));

    SmsReadBuffer rPdu15("");
    EXPECT_FALSE(parameter.DecodeCallingPartyNumber(rPdu15));
    SmsReadBuffer rPdu16("0");
    EXPECT_FALSE(parameter.DecodeCallingPartyNumber(rPdu16));
    SmsReadBuffer rPdu17("01");
    EXPECT_FALSE(parameter.DecodeCallingPartyNumber(rPdu17));
    SmsReadBuffer rPdu18("011");
    EXPECT_FALSE(parameter.DecodeCallingPartyNumber(rPdu18));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsSubParameter_0009
 * @tc.name     Test CdmaSmsSubParameter
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsSubParameter_0009, Function | MediumTest | Level1)
{
    SmsEnhancedVmnAck value;
    CdmaSmsEnhancedVmnAck parameter(value);
    uint8_t id = static_cast<uint8_t>('0');
    parameter.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(parameter.Decode(rPdu));
    SmsReadBuffer rPdu1("001");
    EXPECT_FALSE(parameter.Decode(rPdu1));
    SmsReadBuffer rPdu2("0011");
    EXPECT_FALSE(parameter.Decode(rPdu2));
    SmsReadBuffer rPdu3("00111");
    EXPECT_FALSE(parameter.Decode(rPdu3));
    SmsReadBuffer rPdu4("001111");
    EXPECT_FALSE(parameter.Decode(rPdu4));
    SmsReadBuffer rPdu5("0011111");
    EXPECT_FALSE(parameter.Decode(rPdu5));
    SmsReadBuffer rPdu6("00111111");
    EXPECT_FALSE(parameter.Decode(rPdu6));
    SmsReadBuffer rPdu7("001111111");
    EXPECT_FALSE(parameter.Decode(rPdu7));
    SmsReadBuffer rPdu8("0011111111");
    EXPECT_FALSE(parameter.Decode(rPdu8));
    SmsReadBuffer rPdu9("00111111111");
    EXPECT_FALSE(parameter.Decode(rPdu9));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsParameterRecord_0001
 * @tc.name     Test CdmaSmsParameterRecord
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsParameterRecord_0001, Function | MediumTest | Level1)
{
    uint16_t value = 0;
    CdmaSmsTeleserviceId parameter(value);
    SmsWriteBuffer wPdu;
    wPdu.length_ = 0;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.length_ = 1;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_FALSE(parameter.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 4;
    EXPECT_TRUE(parameter.Encode(wPdu));

    CdmaSmsServiceCategory cat(value);
    wPdu.length_ = 0;
    EXPECT_FALSE(cat.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 1;
    EXPECT_FALSE(cat.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(cat.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_FALSE(cat.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 4;
    EXPECT_TRUE(cat.Encode(wPdu));

    uint8_t r = 0;
    CdmaSmsBearerReply reply(r);
    wPdu.length_ = 0;
    EXPECT_FALSE(reply.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 1;
    EXPECT_FALSE(reply.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 2;
    EXPECT_FALSE(reply.Encode(wPdu));
    wPdu.index_ = 0;
    wPdu.length_ = 3;
    EXPECT_TRUE(reply.Encode(wPdu));
    reply.id_ = static_cast<uint8_t>('0');
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(reply.Decode(rPdu));
}

/**
 * @tc.number   Telephony_CdmaSmsGtest_CdmaSmsParameterRecord_0002
 * @tc.name     Test CdmaSmsParameterRecord
 * @tc.desc     Function test
 */
HWTEST_F(CdmaSmsGtest, CdmaSmsParameterRecord_0002, Function | MediumTest | Level1)
{
    TransportCauseCode value;
    CdmaSmsCauseCodes codes(value);
    SmsWriteBuffer wPdu;
    wPdu.length_ = 0;
    EXPECT_FALSE(codes.Encode(wPdu));
    uint8_t id = static_cast<uint8_t>('0');
    codes.id_ = id;
    SmsReadBuffer rPdu("00");
    EXPECT_FALSE(codes.Decode(rPdu));
    SmsReadBuffer rPdu1("001");
    EXPECT_TRUE(codes.Decode(rPdu1));

    TransportAddr addr;
    CdmaSmsAddressParameter parameter(addr, CdmaSmsParameterRecord::ORG_ADDRESS);
    EXPECT_FALSE(parameter.Encode(wPdu));
    parameter.id_ = id;
    parameter.isInvalid_ = false;
    SmsReadBuffer rPdu2("00");
    EXPECT_FALSE(parameter.Decode(rPdu2));

    TransportSubAddr subAddr;
    CdmaSmsSubaddress subAddress(subAddr, CdmaSmsParameterRecord::ORG_SUB_ADDRESS);
    EXPECT_FALSE(subAddress.Encode(wPdu));
    subAddress.id_ = id;
    subAddress.isInvalid_ = false;
    SmsReadBuffer rPdu3("00");
    EXPECT_FALSE(subAddress.Decode(rPdu3));
    SmsReadBuffer rPdu4("001");
    EXPECT_FALSE(subAddress.Decode(rPdu4));
}
#endif // TEL_TEST_UNSUPPORT
} // namespace Telephony
} // namespace OHOS