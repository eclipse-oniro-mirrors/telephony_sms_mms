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

#include "gtest/gtest.h"
#define private public
#define protected public
#include "cb_start_ability.h"
#include "delivery_short_message_callback_stub.h"
#include "gsm_sms_param_codec.h"
#include "gsm_pdu_hex_value.h"
#include "gsm_sms_common_utils.h"
#include "gsm_sms_message.h"
#include "gsm_sms_receive_handler.h"
#include "gsm_sms_sender.h"
#include "gsm_sms_tpdu_decode.h"
#include "gsm_user_data_decode.h"
#include "gsm_user_data_encode.h"
#include "short_message.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

static constexpr uint8_t MAX_GSM_7BIT_DATA_LEN = 160;
constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
class SmsGsmTest : public testing::Test {
public:
    static void SetUpTestCase()
    {}
    static void TearDownTestCase()
    {
        DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
    }
    void SetUp()
    {}
    void TearDown()
    {}
};

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_001
 * @tc.name     SmsGsmTest_001
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_001, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_0D;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_002
 * @tc.name     SmsGsmTest_002
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_002, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_003
 * @tc.name     SmsGsmTest_003
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_003, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    strBuff += HEX_VALUE_04;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_004
 * @tc.name     SmsGsmTest_004
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_004, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    strBuff += HEX_VALUE_D0;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}


/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_005
 * @tc.name     SmsGsmTest_005
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_005, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();
    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    strBuff += HEX_VALUE_D0;
    strBuff += HEX_VALUE_D0;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_006
 * @tc.name     SmsGsmTest_006
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_006, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    strBuff += HEX_VALUE_D0;
    strBuff += HEX_VALUE_11;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_FALSE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_007
 * @tc.name     SmsGsmTest_007
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_007, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsParamCodec> codec = std::make_shared<GsmSmsParamCodec>();

    string strBuff;
    int32_t setType = 0;
    int32_t indType = 0;
    strBuff += HEX_VALUE_04;
    strBuff += HEX_VALUE_D0;
    strBuff += HEX_VALUE_10;
    strBuff += HEX_VALUE_10;
    SmsReadBuffer smsReadBuff(strBuff);
    EXPECT_TRUE(codec->CheckVoicemail(smsReadBuff, &setType, &indType));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_008
 * @tc.name     SmsGsmTest_008
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_008, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    SmsWriteBuffer buffer;
    EXPECT_FALSE(utils->Pack7bitChar(buffer, nullptr, 0, 0));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_009
 * @tc.name     SmsGsmTest_009
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_009, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    std::vector<uint8_t> vectData;
    uint8_t fillBits = 1;

    SmsWriteBuffer buffer;
    EXPECT_FALSE(utils->Pack7bitChar(buffer, static_cast<const uint8_t*>(&vectData[0]), 0, fillBits));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_010
 * @tc.name     SmsGsmTest_010
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_010, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    std::vector<uint8_t> vectData;
    uint8_t fillBits = 1;
    vectData.push_back(1);
    vectData.push_back(2);
    vectData.push_back(3);
    vectData.push_back(4);
    SmsWriteBuffer buffer;
    EXPECT_TRUE(utils->Pack7bitChar(buffer, static_cast<const uint8_t*>(&vectData[0]), vectData.size(), fillBits));

    vectData.clear();
    vectData.assign(MAX_GSM_7BIT_DATA_LEN + 1, 1); // first branch
    EXPECT_FALSE(utils->Pack7bitChar(buffer, static_cast<const uint8_t*>(&vectData[0]), vectData.size(), fillBits));
    vectData.clear();
    EXPECT_TRUE(utils->Pack7bitChar(buffer, static_cast<const uint8_t*>(&vectData[0]), vectData.size(), fillBits));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_011
 * @tc.name     SmsGsmTest_011
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_011, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    std::string strBuffer;
    uint8_t fillBits = 0;
    uint8_t dstIdx = 0;

    SmsReadBuffer buffer(strBuffer);
    EXPECT_FALSE(utils->Unpack7bitChar(buffer, strBuffer.size(), fillBits,  nullptr, 0, dstIdx));
    fillBits = 8;
    EXPECT_FALSE(utils->Unpack7bitChar(buffer, strBuffer.size(), fillBits,  nullptr, 0, dstIdx));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_012
 * @tc.name     SmsGsmTest_012
 * @tc.desc     Function test
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_012, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    std::string strBuffer;
    std::vector<uint8_t> vectData;
    uint8_t fillBits = 1;
    uint8_t dstIdx = 0;

    SmsReadBuffer buffer(strBuffer);
    EXPECT_FALSE(utils->Unpack7bitChar(buffer, strBuffer.size(), fillBits,
        static_cast<uint8_t*>(&vectData[0]), vectData.size(), dstIdx));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsGsmTest_013
 * @tc.name     SmsGsmTest_013
 * @tc.desc     Function test Unpack7bitCharForMiddlePart
 */
HWTEST_F(SmsGsmTest, SmsGsmTest_013, Function | MediumTest | Level1)
{
    std::shared_ptr<GsmSmsCommonUtils> utils = std::make_shared<GsmSmsCommonUtils>();
    std::string dataStr("1141515");
    std::vector<uint8_t> vectData;

    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(dataStr.c_str());
    EXPECT_FALSE(utils->Unpack7bitCharForMiddlePart(nullptr, 0, static_cast<uint8_t*>(&vectData[0])));
    EXPECT_FALSE(utils->Unpack7bitCharForMiddlePart(buffer, dataStr.size(), nullptr));
    EXPECT_FALSE(utils->Unpack7bitCharForMiddlePart(buffer, 0, static_cast<uint8_t*>(&vectData[0])));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsTpduCodec_001
 * @tc.name     GsmSmsTpduCodec_001
 * @tc.desc     Function test decode
 */
HWTEST_F(SmsGsmTest, GsmSmsTpduCodec_001, Function | MediumTest | Level1)
{
    auto tpduCodec = std::make_shared<GsmSmsTpduCodec>();
    GsmSmsTpduDecode decode(tpduCodec->uDataCodec_, tpduCodec->paramCodec_, tpduCodec);
    SmsReadBuffer buffer("");
    SmsSubmit submit;
    EXPECT_FALSE(decode.DecodeSubmitPartData(buffer, &submit));
    SmsReadBuffer buffer0(std::string(2, 0x80));
    EXPECT_FALSE(decode.DecodeSubmitPartData(buffer0, &submit));
    SmsDeliver deliver;
    SmsReadBuffer buffer1(std::string(2, 0x80));
    decode.DecodeDeliver(buffer1, &deliver);
    EXPECT_TRUE(deliver.bReplyPath);
    SmsStatusReport statusRep;
    SmsReadBuffer buffer2("0000000");
    EXPECT_FALSE(decode.DecodeStatusReportPartData(buffer, &statusRep));
    EXPECT_FALSE(decode.DecodeStatusReportPartData(buffer2, &statusRep));
    SmsReadBuffer buffer3("00000000");
    EXPECT_TRUE(decode.DecodeStatusReportPartData(buffer3, &statusRep));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecode_001
 * @tc.name     GsmUserDataDecode_001
 * @tc.desc     Function test decode
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecode_001, Function | MediumTest | Level1)
{
    GsmUserDataDecode decode(nullptr);
    GsmUserDataDecode decode2(std::make_shared<GsmUserDataPdu>());
    SmsReadBuffer buffer("");
    SmsUDPackage userData;
    SmsTpud tpud;
    uint8_t udl = 1;
    uint8_t fillBits = 0;
    EXPECT_FALSE(decode.DecodeGsmHeadPdu(buffer, false, nullptr, &tpud, udl, fillBits));
    EXPECT_FALSE(decode.DecodeGsmHeadPdu(buffer, false, &userData, &tpud, udl, fillBits));
    SmsReadBuffer buffer1(std::string(2, 161));
    EXPECT_FALSE(decode.DecodeGsmHeadPdu(buffer1, false, &userData, &tpud, udl, fillBits));

    EXPECT_FALSE(decode.DecodeGsmHeadPduPartData(buffer, false, &userData, udl, fillBits));
    EXPECT_FALSE(decode2.DecodeGsmHeadPduPartData(buffer, true, &userData, udl, fillBits));

    EXPECT_FALSE(decode.DecodeGsmBodyPdu(buffer, false, nullptr, &tpud, udl, fillBits));

    EXPECT_FALSE(decode2.Decode8bitPduPartData(buffer, true, &userData, &tpud, udl, fillBits));

    EXPECT_FALSE(decode.Decode8bitPdu(buffer, false, nullptr, &tpud));
    EXPECT_FALSE(decode.Decode8bitPdu(buffer, false, &userData, &tpud));
    SmsReadBuffer buffer2(std::string(2, 141));
    EXPECT_FALSE(decode.Decode8bitPdu(buffer2, false, &userData, &tpud));

    EXPECT_FALSE(decode.DecodeUcs2Pdu(buffer, false, nullptr, &tpud));
    SmsReadBuffer buffer3(std::string(2, 141));
    EXPECT_FALSE(decode.DecodeUcs2Pdu(buffer3, false, &userData, &tpud));

    EXPECT_FALSE(decode.DecodeUcs2PduPartData(buffer, false, &userData, udl, fillBits));
    EXPECT_FALSE(decode2.DecodeUcs2PduPartData(buffer, true, &userData, udl, fillBits));
    userData.length = 0;
    EXPECT_TRUE(decode2.DecodeUcs2PduPartData(buffer, false, &userData, udl, fillBits));
    EXPECT_TRUE(decode2.Decode8bitPduPartData(buffer, false, &userData, &tpud, udl, fillBits));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecode_002
 * @tc.name     GsmUserDataDecode_002
 * @tc.desc     8bit TP-UD with UDH: body starts right after UDH (3GPP TS 23.040 9.2.3.24)
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecode_002, Function | MediumTest | Level1)
{
    auto udPdu = std::make_shared<GsmUserDataPdu>();
    // UDL=11, UDHL=6, IEI=05 IEDL=04 dest port 2948 origin port 9200, body "ABCD"
    std::vector<uint8_t> ud = { 0x0B, 0x06, 0x05, 0x04, 0x0B, 0x84, 0x23, 0xF0, 'A', 'B', 'C', 'D' };
    SmsReadBuffer buffer(std::string(ud.begin(), ud.end()));
    SmsUDPackage userData;
    SmsTpud tpud;
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer, true, DATA_CODING_8BIT, &userData, &tpud));
    EXPECT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_APP_PORT_16BIT);
    EXPECT_EQ(userData.header[0].udh.appPort16bit.destPort, 2948);
    ASSERT_EQ(userData.length, 4);
    EXPECT_EQ(std::string(userData.data, userData.length), "ABCD");
    ASSERT_EQ(tpud.udl, 4);
    EXPECT_EQ(std::string(tpud.ud, tpud.udl), "ABCD");
    EXPECT_EQ(buffer.GetIndex(), ud.size());

    SmsReadBuffer buffer1(std::string(ud.begin(), ud.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer1, true, DATA_CODING_8BIT, &userData));
    ASSERT_EQ(userData.length, 4);
    EXPECT_EQ(std::string(userData.data, userData.length), "ABCD");

    // one octet body after 8bit concat IE
    std::vector<uint8_t> ud2 = { 0x07, 0x05, 0x00, 0x03, 0x2A, 0x02, 0x01, 'Z' };
    SmsReadBuffer buffer2(std::string(ud2.begin(), ud2.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer2, true, DATA_CODING_8BIT, &userData));
    ASSERT_EQ(userData.length, 1);
    EXPECT_EQ(userData.data[0], 'Z');

    // empty body: UDL == UDHL + 1
    std::vector<uint8_t> ud3 = { 0x06, 0x05, 0x00, 0x03, 0x2A, 0x02, 0x01 };
    SmsReadBuffer buffer3(std::string(ud3.begin(), ud3.end()));
    EXPECT_TRUE(udPdu->DecodeUserDataPdu(buffer3, true, DATA_CODING_8BIT, &userData));
    EXPECT_EQ(userData.length, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecode_003
 * @tc.name     GsmUserDataDecode_003
 * @tc.desc     8bit TP-UD with UDH: max UDL 140 keeps the whole body, UDHL beyond the PDU fails
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecode_003, Function | MediumTest | Level1)
{
    auto udPdu = std::make_shared<GsmUserDataPdu>();
    const uint8_t maxUdl = 140;
    const uint8_t udhLen = 6;
    std::vector<uint8_t> ud = { maxUdl, 0x05, 0x00, 0x03, 0x01, 0x02, 0x01 };
    for (uint8_t i = 0; i < maxUdl - udhLen; i++) {
        ud.push_back(i);
    }
    SmsReadBuffer buffer(std::string(ud.begin(), ud.end()));
    SmsUDPackage userData;
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer, true, DATA_CODING_8BIT, &userData));
    ASSERT_EQ(userData.length, maxUdl - udhLen);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[0]), 0);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[maxUdl - udhLen - 1]), maxUdl - udhLen - 1);

    // UDHL=10 but only 7 octets follow it
    std::vector<uint8_t> ud2 = { 0x0B, 0x0A, 0x05, 0x04, 0x0B, 0x84, 0x23, 0xF0, 'A' };
    SmsReadBuffer buffer2(std::string(ud2.begin(), ud2.end()));
    EXPECT_FALSE(udPdu->DecodeUserDataPdu(buffer2, true, DATA_CODING_8BIT, &userData));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataEncode_001
 * @tc.name     GsmUserDataEncode_001
 * @tc.desc     Function test encode
 */
HWTEST_F(SmsGsmTest, GsmUserDataEncode_001, Function | MediumTest | Level1)
{
    GsmUserDataEncode encode(nullptr);
    SmsWriteBuffer buffer;
    SmsUDPackage userData;
    uint8_t fillBits = 0;
    EXPECT_FALSE(encode.EncodeGsmHeadPdu(buffer, &userData, fillBits));
    EXPECT_FALSE(encode.EncodeGsmHeadPdu(buffer, nullptr, fillBits));
    EXPECT_FALSE(encode.EncodeGsmBodyPdu(buffer, nullptr, fillBits));
    std::string destAddr = "";
    EXPECT_FALSE(encode.Encode8bitHeadPdu(buffer, &userData, destAddr));
    EXPECT_FALSE(encode.Encode8bitHeadPdu(buffer, nullptr, destAddr));
    buffer.data_ = nullptr;
    EXPECT_FALSE(encode.Encode8bitBodyPdu(buffer, &userData));
    EXPECT_FALSE(encode.Encode8bitBodyPdu(buffer, nullptr));
    EXPECT_FALSE(encode.EncodeUcs2HeadPdu(buffer, &userData));
    EXPECT_FALSE(encode.EncodeUcs2HeadPdu(buffer, nullptr));
    EXPECT_FALSE(encode.EncodeUcs2BodyPdu(buffer, nullptr));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsSender_001
 * @tc.name     GsmSmsSender_001
 * @tc.desc     Function test GsmSmsSender
 */
HWTEST_F(SmsGsmTest, GsmSmsSender_001, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = [](std::shared_ptr<SmsSendIndexer> indexer) {};
    std::shared_ptr<GsmSmsSender> smsSender = std::make_shared<GsmSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    std::shared_ptr<SmsMessageInfo> statusInfo = std::make_shared<SmsMessageInfo>();
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(0, statusInfo);
    smsSender->StatusReportAnalysis(event);
    std::vector<uint8_t> pdu = { 8, 145, 104, 49, 8, 32, 1, 5, 240, 68, 13, 145, 104, 145, 39, 32, 49, 100, 240,
        0, 0, 82, 64, 80, 34, 145, 17, 35, 160, 5, 0, 3, 1, 2, 1, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44,
        22, 139, 197, 98, 177, 152, 44, 54, 171, 209, 108, 55, 25, 142, 54, 163, 213, 108, 180, 90, 12, 55, 187,
        213, 104, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 76, 22, 155, 213, 104,
        182, 155, 12, 71, 155, 209, 106, 54, 90, 45, 134, 155, 221, 106, 180, 88, 44, 22, 139, 197, 98, 177, 88,
        44, 22, 139, 197, 98, 177, 88, 44, 38, 139, 205, 106, 52, 219, 77, 134, 163, 205, 104, 53, 27, 173, 22,
        195, 205, 110, 53, 90, 44, 22, 139, 197, 98, 177, 88, 44, 22, 139, 197, 98, 177, 88, 44, 22, 147, 197, 102,
        53, 154, 237, 38, 195, 209, 102 };
    statusInfo->pdu = pdu;
    event = AppExecFwk::InnerEvent::Get(0, statusInfo);
    std::shared_ptr<SmsSendIndexer> indexer = std::make_shared<SmsSendIndexer>("des", "src", "text", nullptr, nullptr);
    auto message = GsmSmsMessage::CreateMessage(StringUtils::StringToHex(pdu));
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
    smsSender->imsSmsCfg_ = GsmSmsSender::IMS_SMS_ENABLE;
    smsSender->StatusReportSetImsSms(event);
    EXPECT_EQ(smsSender->imsSmsCfg_, GsmSmsSender::IMS_SMS_DISABLE);
    std::shared_ptr<int32_t> imsSmsInfo = std::make_shared<int32_t>(GsmSmsSender::IMS_SMS_ENABLE);
    event = AppExecFwk::InnerEvent::Get(0, imsSmsInfo);
    smsSender->StatusReportGetImsSms(event);
    EXPECT_EQ(smsSender->imsSmsCfg_, GsmSmsSender::IMS_SMS_ENABLE);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsSender_002
 * @tc.name     GsmSmsSender_002
 * @tc.desc     Function test GsmSmsSender
 */
HWTEST_F(SmsGsmTest, GsmSmsSender_002, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = [](std::shared_ptr<SmsSendIndexer> indexer) {};
    std::shared_ptr<GsmSmsSender> smsSender = std::make_shared<GsmSmsSender>(DEFAULT_SIM_SLOT_ID, fun);
    GsmSimMessageParam smsData;
    std::shared_ptr<SmsSendIndexer> indexer = std::make_shared<SmsSendIndexer>("des", "src", "text", nullptr, nullptr);
    indexer->hasMore_ = true;
#ifdef SMS_MMS_SATELLITE
    smsSender->SendSatelliteSms(indexer, smsData);
#endif
    indexer->csResendCount_ = 1;
#ifdef SMS_MMS_SATELLITE
    smsSender->SendSatelliteSms(indexer, smsData);
#endif
    smsSender->ResendDataDelivery(indexer);
    EXPECT_NE(indexer, nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_ShortMessage_001
 * @tc.name     ShortMessage_001
 * @tc.desc     Function test ShortMessage
 */
HWTEST_F(SmsGsmTest, ShortMessage_001, Function | MediumTest | Level1)
{
    auto shortMessage = std::make_shared<ShortMessage>();
    Parcel parcel;
    EXPECT_TRUE(shortMessage->Marshalling(parcel));
    EXPECT_TRUE(shortMessage->ReadFromParcel(parcel));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessageSplitEscape_001
 * @tc.name     GsmSmsMessageSplitEscape_001
 * @tc.desc     7bit split never separates ESC from its extension code (3GPP TS 23.038 6.2.1.1)
 */
HWTEST_F(SmsGsmTest, GsmSmsMessageSplitEscape_001, Function | MediumTest | Level1)
{
    const uint8_t gsmEscape = 0x1B;
    const size_t multiSegSize = 153;
    GsmSmsMessage gsmSmsMessage;
    std::vector<struct SplitInfo> splits;
    DataCodingScheme coding = DATA_CODING_AUTO;
    std::string desAddr;
    // 152 x 'a' + EURO SIGN (ESC 0x65) + 10 x 'b': the escape sequence would straddle septet 153
    std::string text = std::string(152, 'a') + "\xE2\x82\xAC" + std::string(10, 'b');
    gsmSmsMessage.SplitMessage(splits, text, false, coding, false, desAddr);
    EXPECT_EQ(coding, DATA_CODING_7BIT);
    ASSERT_EQ(splits.size(), 2u);
    ASSERT_EQ(splits[0].encodeData.size(), multiSegSize - 1);
    EXPECT_NE(splits[0].encodeData.back(), gsmEscape);
    ASSERT_EQ(splits[1].encodeData.size(), 12u);
    EXPECT_EQ(splits[1].encodeData[0], gsmEscape);
    EXPECT_EQ(splits[0].text + splits[1].text, text);

    // 151 x 'a' + EURO SIGN: the escape sequence ends exactly at septet 153, nothing to move
    splits.clear();
    text = std::string(151, 'a') + "\xE2\x82\xAC" + std::string(10, 'b');
    gsmSmsMessage.SplitMessage(splits, text, false, coding, false, desAddr);
    ASSERT_EQ(splits.size(), 2u);
    EXPECT_EQ(splits[0].encodeData.size(), multiSegSize);
    EXPECT_EQ(splits[1].encodeData.size(), 10u);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecodeUdh_001
 * @tc.name     GsmUserDataDecodeUdh_001
 * @tc.desc     UDH is parsed up to UDHL whatever the number of IEs (3GPP TS 23.040 9.2.3.24)
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecodeUdh_001, Function | MediumTest | Level1)
{
    auto udPdu = std::make_shared<GsmUserDataPdu>();
    SmsUDPackage userData = {};
    // UDHL=0x13: 7 unsupported zero-length IEs (0x70..0x76), then concat IE ref 0x55 2/1; 7bit body "XY"
    std::vector<uint8_t> ud7bit = { 0x19, 0x13, 0x70, 0x00, 0x71, 0x00, 0x72, 0x00, 0x73, 0x00, 0x74, 0x00, 0x75,
        0x00, 0x76, 0x00, 0x00, 0x03, 0x55, 0x02, 0x01, 0xB0, 0x59 };
    SmsReadBuffer buffer(std::string(ud7bit.begin(), ud7bit.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer, true, DATA_CODING_7BIT, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_CONCAT_8BIT);
    EXPECT_EQ(userData.header[0].udh.concat8bit.msgRef, 0x55);
    EXPECT_EQ(userData.header[0].udh.concat8bit.totalSeg, 2);
    EXPECT_EQ(userData.header[0].udh.concat8bit.seqNum, 1);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(std::string(userData.data, userData.length), "XY");

    // same UDH, UCS2 body U+4F60
    std::vector<uint8_t> udUcs2 = { 0x16, 0x13, 0x70, 0x00, 0x71, 0x00, 0x72, 0x00, 0x73, 0x00, 0x74, 0x00, 0x75,
        0x00, 0x76, 0x00, 0x00, 0x03, 0x55, 0x02, 0x01, 0x4F, 0x60 };
    SmsReadBuffer buffer1(std::string(udUcs2.begin(), udUcs2.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer1, true, DATA_CODING_UCS2, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_CONCAT_8BIT);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[0]), 0x4F);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[1]), 0x60);

    // same UDH, 8bit: the concat IE behind the unsupported IEs is kept
    std::vector<uint8_t> ud8bit = udUcs2;
    SmsReadBuffer buffer2(std::string(ud8bit.begin(), ud8bit.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer2, true, DATA_CODING_8BIT, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_CONCAT_8BIT);

    // 8 supported concat IEs: MAX_UD_HEADER_NUM are kept and the 7bit body still starts after UDHL ("Hi")
    std::vector<uint8_t> udMany = { 0x31, 0x28, 0x00, 0x03, 0x10, 0x02, 0x01, 0x00, 0x03, 0x11, 0x02, 0x01, 0x00,
        0x03, 0x12, 0x02, 0x01, 0x00, 0x03, 0x13, 0x02, 0x01, 0x00, 0x03, 0x14, 0x02, 0x01, 0x00, 0x03, 0x15, 0x02,
        0x01, 0x00, 0x03, 0x16, 0x02, 0x01, 0x00, 0x03, 0x17, 0x02, 0x01, 0x90, 0x69 };
    SmsReadBuffer buffer3(std::string(udMany.begin(), udMany.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer3, true, DATA_CODING_7BIT, &userData));
    EXPECT_EQ(userData.headerCnt, MAX_UD_HEADER_NUM);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(std::string(userData.data, userData.length), "Hi");
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecodeUdh_002
 * @tc.name     GsmUserDataDecodeUdh_002
 * @tc.desc     Every IE is delimited by its IEDL, a malformed IE is skipped (3GPP TS 23.040 9.2.3.24)
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecodeUdh_002, Function | MediumTest | Level1)
{
    auto udPdu = std::make_shared<GsmUserDataPdu>();
    SmsUDPackage userData = {};
    // special SMS indication IE with IEDL=3 (defined as 2) is skipped, the concat IE behind it is kept, body "Hi"
    std::vector<uint8_t> ud1 = { 0x0F, 0x0A, 0x01, 0x03, 0x80, 0x01, 0x00, 0x00, 0x03, 0x01, 0x02, 0x01, 0x40, 0xA6,
        0x01 };
    SmsReadBuffer buffer1(std::string(ud1.begin(), ud1.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer1, true, DATA_CODING_7BIT, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_CONCAT_8BIT);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(std::string(userData.data, userData.length), "Hi");

    // concat IE with IEDL=4 (defined as 3) is skipped by IEDL, the 8bit port IE behind it is decoded
    std::vector<uint8_t> ud2 = { 0x0C, 0x0A, 0x00, 0x04, 0x01, 0x02, 0x01, 0xEE, 0x04, 0x02, 0x10, 0x20, 0x5A };
    SmsReadBuffer buffer2(std::string(ud2.begin(), ud2.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer2, true, DATA_CODING_8BIT, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_APP_PORT_8BIT);
    EXPECT_EQ(userData.header[0].udh.appPort8bit.destPort, 0x10);
    EXPECT_EQ(userData.header[0].udh.appPort8bit.originPort, 0x20);

    // IEDL beyond UDHL: the rest of the UDH is ignored, the UCS2 body is intact
    std::vector<uint8_t> ud3 = { 0x08, 0x05, 0x00, 0x09, 0x01, 0x02, 0x01, 0x4F, 0x60 };
    SmsReadBuffer buffer3(std::string(ud3.begin(), ud3.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer3, true, DATA_CODING_UCS2, &userData));
    EXPECT_EQ(userData.headerCnt, 0);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[0]), 0x4F);

    // UCS2: zero-length IE right before the end of UDH, the body starts at UDH start + UDHL
    std::vector<uint8_t> ud4 = { 0x0A, 0x07, 0x00, 0x03, 0x7A, 0x02, 0x01, 0x1A, 0x00, 0x4F, 0x60 };
    SmsReadBuffer buffer4(std::string(ud4.begin(), ud4.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer4, true, DATA_CODING_UCS2, &userData));
    EXPECT_EQ(userData.headerCnt, 1);
    ASSERT_EQ(userData.length, 2);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[0]), 0x4F);
    EXPECT_EQ(static_cast<uint8_t>(userData.data[1]), 0x60);

    // UDHL beyond the end of the PDU is a real format error
    std::vector<uint8_t> ud5 = { 0x0B, 0x0A, 0x05, 0x04, 0x0B, 0x84, 0x23, 0xF0, 'A' };
    SmsReadBuffer buffer5(std::string(ud5.begin(), ud5.end()));
    EXPECT_FALSE(udPdu->DecodeUserDataPdu(buffer5, true, DATA_CODING_8BIT, &userData));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataDecodeConcat16Bit_001
 * @tc.name     GsmUserDataDecodeConcat16Bit_001
 * @tc.desc     16 bit concat IE keeps both octets of the reference number (3GPP TS 23.040 9.2.3.24.8)
 */
HWTEST_F(SmsGsmTest, GsmUserDataDecodeConcat16Bit_001, Function | MediumTest | Level1)
{
    const uint16_t msgRef = 0x1234;
    const uint16_t msgRefUcs2 = 0xABCD;
    auto udPdu = std::make_shared<GsmUserDataPdu>();
    SmsUDPackage userData = {};
    // UDHL=6: IEI 0x08 IEDL=4 reference 0x12 0x34, 3 segments, segment 1; 7bit body "ABC"
    std::vector<uint8_t> ud7bit = { 0x0B, 0x06, 0x08, 0x04, 0x12, 0x34, 0x03, 0x01, 0x41, 0xE1, 0x10 };
    SmsReadBuffer buffer(std::string(ud7bit.begin(), ud7bit.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer, true, DATA_CODING_7BIT, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udhType, UDH_CONCAT_16BIT);
    EXPECT_EQ(userData.header[0].udh.concat16bit.msgRef, msgRef);
    EXPECT_EQ(userData.header[0].udh.concat16bit.totalSeg, 3);
    EXPECT_EQ(userData.header[0].udh.concat16bit.seqNum, 1);
    ASSERT_EQ(userData.length, 3);
    EXPECT_EQ(std::string(userData.data, userData.length), "ABC");

    // UCS2 body U+4F60, reference 0xAB 0xCD
    std::vector<uint8_t> udUcs2 = { 0x09, 0x06, 0x08, 0x04, 0xAB, 0xCD, 0x02, 0x02, 0x4F, 0x60 };
    SmsReadBuffer buffer1(std::string(udUcs2.begin(), udUcs2.end()));
    ASSERT_TRUE(udPdu->DecodeUserDataPdu(buffer1, true, DATA_CODING_UCS2, &userData));
    ASSERT_EQ(userData.headerCnt, 1);
    EXPECT_EQ(userData.header[0].udh.concat16bit.msgRef, msgRefUcs2);
    EXPECT_EQ(userData.header[0].udh.concat16bit.seqNum, 2);

    // SMS-DELIVER with the 7bit UD above: the full reference reaches the key used to merge the segments
    auto message = GsmSmsMessage::CreateMessage("0044048121430000425081214365000B0608041234030141E110");
    ASSERT_NE(message, nullptr);
    std::shared_ptr<SmsConcat> concat = message->GetConcatMsg();
    ASSERT_NE(concat, nullptr);
    EXPECT_FALSE(concat->is8Bits);
    EXPECT_EQ(concat->msgRef, msgRef);
    EXPECT_EQ(concat->totalSeg, 3);
    EXPECT_EQ(concat->seqNum, 1);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsMessageNationalShift_001
 * @tc.name     GsmSmsMessageNationalShift_001
 * @tc.desc     7bit decode selects shift tables from IEI 0x24 / 0x25 (3GPP TS 23.040 9.2.3.24.15/16)
 */
HWTEST_F(SmsGsmTest, GsmSmsMessageNationalShift_001, Function | MediumTest | Level1)
{
    // concat IE first, Turkish single shift IE second, septets 'a' ESC 0x73 'b'
    auto message = GsmSmsMessage::CreateMessage("0044048121430000425081214365000F0800032A020124010120BC998B01");
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetVisibleMessageBody(), "a\xC5\x9F" "b");
    // Turkish locking shift IE, septets 04 0B 1C
    message = GsmSmsMessage::CreateMessage("0044048121430000425081214365000803250101202C38");
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetVisibleMessageBody(), "\xE2\x82\xAC\xC4\x9E\xC5\x9E");
    // concat IE with reference 1 and no language IE keeps the default extension table: ESC 0x47 is a space
    message = GsmSmsMessage::CreateMessage("0044048121430000425081214365000B050003010201C29BA318");
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetVisibleMessageBody(), "a b");
    // locking shift for a language without locking table falls back to the default alphabet
    message = GsmSmsMessage::CreateMessage("0044048121430000425081214365000703250102080A01");
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetVisibleMessageBody(), "AB");

    GsmSmsMessage gsmSmsMessage;
    gsmSmsMessage.SetSmsCodingNationalType(SMS_CODING_NATIONAL_TYPE_TURKISH);
    std::vector<struct SplitInfo> splits;
    DataCodingScheme coding = DATA_CODING_AUTO;
    std::string desAddr;
    gsmSmsMessage.SplitMessage(splits, "a\xC5\x9F" "b", false, coding, false, desAddr);
    ASSERT_EQ(splits.size(), 1u);
    EXPECT_EQ(splits[0].langId, MSG_ID_TURKISH_LANG);
    EXPECT_EQ(splits[0].text, "a\xC5\x9F" "b");
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataPdu_001
 * @tc.name     GsmUserDataPdu_001
 * @tc.desc     Function test DecodeHeaderConcat16Bit with non-zero high byte of msgRef
 */
HWTEST_F(SmsGsmTest, GsmUserDataPdu_001, Function | MediumTest | Level1)
{
    auto userDataPdu = std::make_shared<GsmUserDataPdu>();
    std::string pduData;
    pduData.push_back(0x04);
    pduData.push_back(0x12);
    pduData.push_back(0x34);
    pduData.push_back(0x02);
    pduData.push_back(0x01);
    SmsReadBuffer buffer(pduData);
    struct SmsUDH header = {};
    EXPECT_TRUE(userDataPdu->DecodeHeaderConcat16Bit(buffer, header));
    EXPECT_EQ(header.udh.concat16bit.msgRef, 0x1234);
    EXPECT_EQ(header.udh.concat16bit.totalSeg, 0x02);
    EXPECT_EQ(header.udh.concat16bit.seqNum, 0x01);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmUserDataPdu_002
 * @tc.name     GsmUserDataPdu_002
 * @tc.desc     Function test DecodeHeaderConcat16Bit with zero high byte of msgRef
 */
HWTEST_F(SmsGsmTest, GsmUserDataPdu_002, Function | MediumTest | Level1)
{
    auto userDataPdu = std::make_shared<GsmUserDataPdu>();
    std::string pduData;
    pduData.push_back(0x04);
    pduData.push_back(0x00);
    pduData.push_back(0x56);
    pduData.push_back(0x03);
    pduData.push_back(0x02);
    SmsReadBuffer buffer(pduData);
    struct SmsUDH header = {};
    EXPECT_TRUE(userDataPdu->DecodeHeaderConcat16Bit(buffer, header));
    EXPECT_EQ(header.udh.concat16bit.msgRef, 0x0056);
    EXPECT_EQ(header.udh.concat16bit.totalSeg, 0x03);
    EXPECT_EQ(header.udh.concat16bit.seqNum, 0x02);
}
}
}