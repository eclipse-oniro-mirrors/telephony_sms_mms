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