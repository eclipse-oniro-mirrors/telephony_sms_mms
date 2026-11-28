/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "cdma_sms_common.h"
#include "cdma_sms_sub_parameter.h"
#include "data_request.h"
#include "gsm_cb_umts_codec.h"
#include "gsm_sms_sender.h"
#include "gtest/gtest.h"
#include "ims_reg_state_callback_stub.h"
#include "mms_apn_info.h"
#include "radio_event.h"
#include "sms_mms_gtest.h"
#include "sms_pdu_buffer.h"
#include "sms_receive_handler.h"
#include "sms_state_handler.h"
#include "sms_send_manager.h"
#include "sms_sender.h"
#include "sms_service.h"
#include "sms_service_manager_client.h"
#include "sms_state_handler.h"
#include "telephony_errors.h"

namespace OHOS {
namespace Telephony {
namespace {
static constexpr int TEST_MAX_UD_HEADER_NUM = 8;
static constexpr int TEST_MAX_USER_DATA_LEN = 170;
static constexpr uint8_t TEST_MAX_FIELD_LEN = 170;
static constexpr uint8_t MAX_FIELD_LEN = 255;
static constexpr int TEST_DATA_LEN = 100;
static constexpr int TEST_SEND_CONF_MAX_SIZE = 600;
const int32_t INVALID_SLOTID = 2;
const unsigned int SMS_REF_ID = 10;
const std::string BLOCK_NUMBER = "123";
const int16_t WAP_PUSH_PORT = 2948;
const int8_t TEXT_PORT_NUM = -1;
} // namespace
using namespace testing::ext;

class SmsServicesMmsGtest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void SmsServicesMmsGtest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void SmsServicesMmsGtest::SetUp() {}

void SmsServicesMmsGtest::TearDown()
{
    DelayedSingleton<SmsPersistHelper>::DestroyInstance();
}

void SmsServicesMmsGtest::SetUpTestCase() {}

HWTEST_F(SmsServicesMmsGtest, DataRequest_0001, Function | MediumTest | Level2)
{
    int32_t slotId = 0;
    auto dataRequest = std::make_shared<DataRequest>(slotId);
    std::string testStr = "";
    EXPECT_EQ(dataRequest->HttpRequest(slotId, testStr, nullptr, testStr, testStr, "ua", "uaprof"),
        TELEPHONY_ERR_LOCAL_PTR_NULL);
    EXPECT_EQ(dataRequest->ExecuteMms(testStr, nullptr, testStr, testStr, "ua", "uaprof"),
        TELEPHONY_ERR_LOCAL_PTR_NULL);
}

HWTEST_F(SmsServicesMmsGtest, MmsNetworkClient_0001, Function | MediumTest | Level2)
{
    int32_t slotId = 0;
    MmsNetworkClient client(0);
    std::string testStr = "testStr";
    auto mmsApnInfo = std::make_shared<MmsApnInfo>(slotId);
    mmsApnInfo->setMmscUrl("");
    EXPECT_EQ(client.PostUrl(testStr, testStr, "ua", "uaprof"), TELEPHONY_ERR_ARGUMENT_INVALID);

    client.responseData_ = std::string(TEST_SEND_CONF_MAX_SIZE, 'a');
    EXPECT_TRUE(client.CheckSendConf());

    client.responseData_ = "";
    EXPECT_TRUE(client.CheckSendConf());

    client.responseData_ = "responseData";
    EXPECT_TRUE(client.CheckSendConf());
}

HWTEST_F(SmsServicesMmsGtest, MmsConnCallbackStub_0001, Function | MediumTest | Level2)
{
    MmsConnCallbackStub connCallback;
    sptr<NetManagerStandard::NetHandle> netHandle = new NetManagerStandard::NetHandle;
    EXPECT_EQ(connCallback.NetAvailable(netHandle), ERR_NONE);
}

HWTEST_F(SmsServicesMmsGtest, CdmaSmsMessageId_0001, Function | MediumTest | Level2)
{
    auto initValue = static_cast<uint16_t>(0x0);
    auto testValue = static_cast<uint16_t>(0x10);
    SmsTeleSvcMsgId v1;
    memset_s(&v1, sizeof(SmsTeleSvcMsgId), 0x00, sizeof(SmsTeleSvcMsgId));
    v1.msgId = testValue;
    SmsTeleSvcMsgId v2;
    v2.msgId = initValue;
    uint8_t type = static_cast<uint8_t>(TeleserviceMsgType::SUBMIT);

    auto message1 = std::make_shared<CdmaSmsMessageId>(v1, type);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ss;
    ss.clear();
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ss << (*buffer)[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message2 = std::make_shared<CdmaSmsMessageId>(v2, type);

    rBuffer.bitIndex_ = 0;
    rBuffer.index_ = 0;
    rBuffer.length_ = 0;
    EXPECT_FALSE(message2->Decode(rBuffer));
}

HWTEST_F(SmsServicesMmsGtest, CdmaSmsAbsoluteTime_0001, Function | MediumTest | Level2)
{
    auto initValue = static_cast<unsigned char>(0);
    auto testValue = static_cast<unsigned char>(5);
    SmsTimeAbs v1;
    memset_s(&v1, sizeof(SmsTimeAbs), 0x00, sizeof(SmsTimeAbs));
    v1.month = testValue;
    SmsTimeAbs v2;
    v2.month = initValue;

    auto message1 = std::make_shared<CdmaSmsAbsoluteTime>(CdmaSmsSubParameter::VALIDITY_PERIOD_ABSOLUTE, v1);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ss;
    ss.clear();
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ss << (*buffer)[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message2 = std::make_shared<CdmaSmsAbsoluteTime>(CdmaSmsSubParameter::VALIDITY_PERIOD_ABSOLUTE, v2);

    rBuffer.bitIndex_ = 0;
    rBuffer.index_ = 0;
    rBuffer.length_ = 0;
    EXPECT_FALSE(message2->Decode(rBuffer));
}

HWTEST_F(SmsServicesMmsGtest, SmsTeleSvcUserData_0001, Function | MediumTest | Level2)
{
    auto initValue = SmsEncodingType::RESERVED;
    auto testValue = SmsEncodingType::ASCII_7BIT;
    SmsTeleSvcUserData v1;
    memset_s(&v1, sizeof(SmsTeleSvcUserData), 0x00, sizeof(SmsTeleSvcUserData));
    v1.encodeType = testValue;
    SmsTeleSvcUserData v2;
    v2.encodeType = initValue;
    bool headerInd = false;

    auto message1 = std::make_shared<CdmaSmsUserData>(v1, headerInd);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ss;
    ss.clear();
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ss << (*buffer)[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message2 = std::make_shared<CdmaSmsUserData>(v2, headerInd);
    EXPECT_TRUE(message2->Decode(rBuffer));
    EXPECT_EQ(v2.encodeType, testValue);

    message2->data_.userData.headerCnt = TEST_MAX_UD_HEADER_NUM;
    EXPECT_FALSE(message2->EncodeHeader7Bit(wBuffer));
    EXPECT_FALSE(message2->EncodeAscii7Bit(wBuffer));
    EXPECT_FALSE(message2->EncodeGsm7Bit(wBuffer));
    EXPECT_FALSE(message2->EncodeHeaderUnicode(wBuffer));
    EXPECT_FALSE(message2->EncodeUnicode(wBuffer));
}

HWTEST_F(SmsServicesMmsGtest, SmsTeleSvcUserData_0002, Function | MediumTest | Level2)
{
    auto initValue = SmsEncodingType::RESERVED;
    auto testValue = SmsEncodingType::ASCII_7BIT;
    SmsTeleSvcUserData v1;
    memset_s(&v1, sizeof(SmsTeleSvcUserData), 0x00, sizeof(SmsTeleSvcUserData));
    v1.encodeType = testValue;
    SmsTeleSvcUserData v2;
    v2.encodeType = initValue;
    bool headerInd = false;

    auto message1 = std::make_shared<CdmaSmsUserData>(v1, headerInd);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ss;
    ss.clear();
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ss << (*buffer)[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message2 = std::make_shared<CdmaSmsUserData>(v2, headerInd);
    EXPECT_TRUE(message2->Decode(rBuffer));
    EXPECT_EQ(v2.encodeType, testValue);

    message2->data_.userData.length = TEST_MAX_USER_DATA_LEN;
    EXPECT_FALSE(message2->Encode8BitData(wBuffer));

    rBuffer.bitIndex_ = 0;
    rBuffer.index_ = 0;
    rBuffer.length_ = 0;
    EXPECT_FALSE(message2->Decode(rBuffer));

    message2->headerInd_ = true;
    EXPECT_EQ(message2->DecodeHeader7Bit(rBuffer), 0);
    EXPECT_FALSE(message2->DecodeAscii7Bit(rBuffer, 0, BIT7));
    EXPECT_FALSE(message2->DecodeAscii7Bit(rBuffer, TEST_MAX_FIELD_LEN, 0));
    EXPECT_FALSE(message2->DecodeAscii7Bit(rBuffer, 1, 0));
}

HWTEST_F(SmsServicesMmsGtest, SmsTeleSvcUserData_0003, Function | MediumTest | Level2)
{
    auto initValue = SmsEncodingType::RESERVED;
    auto testValue = SmsEncodingType::ASCII_7BIT;
    SmsTeleSvcUserData v1;
    memset_s(&v1, sizeof(SmsTeleSvcUserData), 0x00, sizeof(SmsTeleSvcUserData));
    v1.encodeType = testValue;
    SmsTeleSvcUserData v2;
    v2.encodeType = initValue;
    bool headerInd = false;

    auto message1 = std::make_shared<CdmaSmsUserData>(v1, headerInd);
    SmsWriteBuffer wBuffer;
    EXPECT_TRUE(message1->Encode(wBuffer));
    auto buffer = wBuffer.GetPduBuffer();
    EXPECT_GT(buffer->size(), 0);
    std::stringstream ss;
    ss.clear();
    for (uint16_t i = 0; i < buffer->size(); i++) {
        ss << (*buffer)[i];
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message2 = std::make_shared<CdmaSmsUserData>(v2, headerInd);
    EXPECT_TRUE(message2->Decode(rBuffer));
    EXPECT_EQ(v2.encodeType, testValue);

    rBuffer.bitIndex_ = 0;
    rBuffer.index_ = 0;
    rBuffer.length_ = 0;
    EXPECT_FALSE(message2->DecodeGsm7Bit(rBuffer, 0, BIT7));
    EXPECT_FALSE(message2->DecodeGsm7Bit(rBuffer, MAX_FIELD_LEN, 0));

    message2->data_.userData.length = TEST_MAX_USER_DATA_LEN;
    EXPECT_FALSE(message2->Decode8BitData(rBuffer));
}

HWTEST_F(SmsServicesMmsGtest, CdmaSmsCmasData_0001, Function | MediumTest | Level2)
{
    SmsTeleSvcCmasData v1;
    std::stringstream ss;
    ss.clear();
    ss << static_cast<uint8_t>(CdmaSmsSubParameter::USER_DATA);
    uint8_t len = TEST_DATA_LEN;
    ss << static_cast<uint8_t>(len);
    for (uint8_t i = 0; i < len; i++) {
        ss << static_cast<uint8_t>(0x00);
    }
    SmsReadBuffer rBuffer(ss.str());
    auto message1 = std::make_shared<CdmaSmsCmasData>(v1);
    rBuffer.bitIndex_ = 0;
    rBuffer.index_ = 0;
    rBuffer.length_ = 0;
    EXPECT_FALSE(message1->DecodeData(rBuffer));
    EXPECT_FALSE(message1->DecodeType0Data(rBuffer));
    EXPECT_FALSE(message1->DecodeType1Data(rBuffer));
    EXPECT_FALSE(message1->DecodeType2Data(rBuffer));
    EXPECT_FALSE(message1->DecodeAbsTime(rBuffer));
}

HWTEST_F(SmsServicesMmsGtest, ImsRegStateCallbackStub_0001, Function | MediumTest | Level1)
{
    ImsRegStateCallbackStub stub(nullptr);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    EXPECT_EQ(stub.OnRemoteRequest(0, data, reply, option), TELEPHONY_ERR_DESCRIPTOR_MISMATCH);
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(0);
    EXPECT_EQ(stub.OnRemoteRequest(0, data, reply, option), TELEPHONY_SUCCESS);
}

HWTEST_F(SmsServicesMmsGtest, SmsStateHandler_0001, Function | MediumTest | Level1)
{
    SmsStateHandler handler;
    handler.smsStateObserver_ = nullptr;
    EXPECT_FALSE(handler.UnRegisterHandler());
    handler.UnInit();
}

HWTEST_F(SmsServicesMmsGtest, SmsSender_0001, Function | MediumTest | Level1)
{
    bool hasRetry = false;
    GsmSmsSender sender(0, [&hasRetry](std::shared_ptr<SmsSendIndexer>) { hasRetry = true; });
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(MSG_SMS_RETRY_DELIVERY);
    sender.ProcessEvent(event);
    EXPECT_TRUE(hasRetry);
    event = AppExecFwk::InnerEvent::Get(MSG_SMS_RETRY_DELIVERY, std::make_shared<SendSmsResultInfo>());
    EXPECT_EQ(sender.FindCacheMapAndTransform(event), nullptr);
    sender.sendCacheMap_[0] = nullptr;
    EXPECT_EQ(sender.FindCacheMapAndTransform(event), nullptr);
    event = AppExecFwk::InnerEvent::Get(MSG_SMS_RETRY_DELIVERY, std::make_shared<RadioResponseInfo>());
    EXPECT_EQ(sender.FindCacheMapAndTransform(event), nullptr);
    sender.sendCacheMap_.clear();
    EXPECT_EQ(sender.FindCacheMapAndTransform(event), nullptr);
}

HWTEST_F(SmsServicesMmsGtest, GsmCbUmtsCodec_0001, Function | MediumTest | Level1)
{
    GsmCbUmtsCodec codec(nullptr, nullptr, nullptr);
    EXPECT_FALSE(codec.Decode3gHeader());
    std::shared_ptr<GsmCbCodec::GsmCbMessageHeader> header = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    std::shared_ptr<GsmCbPduDecodeBuffer> buffer = std::make_shared<GsmCbPduDecodeBuffer>(0);
    buffer->totolLength_ = 0;
    std::shared_ptr<GsmCbCodec> cbCodec = std::make_shared<GsmCbCodec>();
    codec.cbHeader_ = header;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbHeader_ = nullptr;
    codec.cbPduBuffer_ = buffer;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbPduBuffer_ = nullptr;
    codec.cbCodec_ = cbCodec;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbHeader_ = header;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbPduBuffer_ = buffer;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbCodec_ = nullptr;
    EXPECT_FALSE(codec.Decode3gHeader());
    codec.cbCodec_ = cbCodec;
    codec.cbHeader_ = nullptr;
    EXPECT_FALSE(codec.Decode3gHeader());
}

HWTEST_F(SmsServicesMmsGtest, SmsReceiveHandlerDisable_0001, Function | MediumTest | Level1)
{
    system::SetParameter("persist.edm.sms_disable", "true");
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_GSM_SMS, 1);
    std::shared_ptr<SmsReceiveHandler> smsReceiveHandler = std::make_shared<CdmaSmsReceiveHandler>(INVALID_SLOTID);
    smsReceiveHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_CDMA_SMS, 1);
    smsReceiveHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_CONNECTED, 1);
    smsReceiveHandler->ProcessEvent(event);
    event = nullptr;
    smsReceiveHandler->ProcessEvent(event);
    std::shared_ptr<SmsReceiveIndexer> indexer = nullptr;
    std::shared_ptr<SmsBaseMessage> smsBaseMessage = nullptr;
    std::shared_ptr<vector<string>> pdus = nullptr;
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(INVALID_SLOTID);
    reliabilityHandler->DeleteMessageFormDb(SMS_REF_ID);
    smsReceiveHandler->CombineMessagePart(indexer);

    reliabilityHandler->CheckBlockedPhoneNumber(BLOCK_NUMBER);
    reliabilityHandler->DeleteAutoSmsFromDB(reliabilityHandler, indexer);
    reliabilityHandler->SendBroadcast(indexer, pdus);
    smsReceiveHandler->HandleReceivedSms(smsBaseMessage);
    indexer = std::make_shared<SmsReceiveIndexer>();
    smsReceiveHandler->CombineMessagePart(indexer);
    indexer->msgCount_ = 1;
    indexer->destPort_ = WAP_PUSH_PORT;
    smsReceiveHandler->CombineMessagePart(indexer);
    reliabilityHandler->SendBroadcast(indexer, pdus);
    pdus = std::make_shared<vector<string>>();
    string pud = "qwe";
    pdus->push_back(pud);
    reliabilityHandler->SendBroadcast(indexer, pdus);
    smsReceiveHandler->CombineMultiPageMessage(indexer, pdus, reliabilityHandler);
    smsReceiveHandler->UpdateMultiPageMessage(indexer, pdus);
    indexer->destPort_ = TEXT_PORT_NUM;
    reliabilityHandler->SendBroadcast(indexer, pdus);
    smsReceiveHandler->AddMsgToDB(indexer);
    smsReceiveHandler->IsRepeatedMessagePart(indexer);
    indexer = nullptr;
    EXPECT_FALSE(smsReceiveHandler->AddMsgToDB(indexer));
    system::SetParameter("persist.edm.sms_disable", "false");
}

HWTEST_F(SmsServicesMmsGtest, SmsServiceSendMessageDisable_0001, Function | MediumTest | Level1)
{
    int32_t slotId = 0;
    std::u16string desAddr = u"";
    sptr<ISendShortMessageCallback> sendCallback;
    sptr<IDeliveryShortMessageCallback> deliveryCallback;
    auto smsService = DelayedSingleton<SmsService>::GetInstance();
    AccessMmsToken token;
    system::SetParameter("persist.edm.sms_disable", "true");
    int32_t ret = smsService->SendMessage(slotId, desAddr, desAddr, desAddr, sendCallback, deliveryCallback, true);
    EXPECT_TRUE(ret == TELEPHONY_ERR_POLICY_DISABLED);
    ret = smsService->SendMessage(slotId, desAddr, desAddr, desAddr, sendCallback, deliveryCallback, false);
    EXPECT_TRUE(ret == TELEPHONY_ERR_POLICY_DISABLED);
    uint16_t port = 1;
    uint8_t *data = nullptr;
    ret = smsService->SendMessage(slotId, desAddr, desAddr, port, data, port, sendCallback, deliveryCallback);
    EXPECT_TRUE(ret == TELEPHONY_ERR_POLICY_DISABLED);
    std::string telephone = "13888888888";
    int32_t dataBaseId = 0;
    smsService->InsertSessionAndDetail(slotId, telephone, telephone, dataBaseId);
    smsService->InsertSessionAndDetail(slotId, "10000", "text", dataBaseId);
    smsService->InsertSessionAndDetail(slotId, "10000,10001", "text", dataBaseId);
    smsService->InsertSessionAndDetail(slotId, "11112123", "text", dataBaseId);
    smsService->InsertSessionAndDetail(slotId, "invalid_number", "text", dataBaseId);

    bool isSupported = false;
    slotId = 0;
    smsService->IsImsSmsSupported(slotId, isSupported);
    std::u16string format = u"";
    smsService->GetImsShortMessageFormat(format);
    smsService->HasSmsCapability();
    int32_t setSmscRes = 0;
    setSmscRes = smsService->SetSmscAddr(slotId, desAddr);
    desAddr = u" test";
    string sca = StringUtils::ToUtf8(desAddr);
    smsService->TrimSmscAddr(sca);
    desAddr = u"test ";
    sca = StringUtils::ToUtf8(desAddr);
    smsService->TrimSmscAddr(sca);
    int32_t smscRes = 0;

    smscRes = smsService->GetSmscAddr(slotId, desAddr);
    system::SetParameter("persist.edm.sms_disable", "false");
    EXPECT_GE(setSmscRes, TELEPHONY_ERR_SLOTID_INVALID);
    EXPECT_GE(smscRes, TELEPHONY_ERR_ARGUMENT_INVALID);
    EXPECT_TRUE(smsService != nullptr);
}
} // namespace Telephony
} // namespace OHOS