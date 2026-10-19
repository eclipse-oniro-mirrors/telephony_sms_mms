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

#include "gsm_cb_codec.h"
#include "gsm_cb_gsm_codec.h"
#include "gsm_cb_umts_codec.h"
#include "gtest/gtest.h"
#include "radio_event.h"
#include "sms_service.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

namespace {
const int32_t INVALID_SLOTID = 2;
const uint16_t PWS_FIRST_ID = 0x1100;
const std::string ETWS_PDU = "000B1100011165FA7D4E9BD564";
const std::string CMAS_PDU = "000D11120111E376784E9BDD60";
const std::string CBS_PDU = "00031112011163F19C36BBC11A";
const std::string CMAS_JP_PDU =
    "01A41F51101102EA3030A830EA30A230E130FC30EB914D4FE130C630B930C8000D000A3053308C306F8A669A137528306E30E130C330BB30FC"
    "30B8306730593002000D000AFF080032003000310033002F00310031002F003252EA300037002000310035003A00340034FF09000D000AFF08"
    "30A830EA30A25E02FF090000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    "0022";
} // namespace

class BranchCbTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};
void BranchCbTest::SetUpTestCase() {}

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void BranchCbTest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void BranchCbTest::SetUp() {}

void BranchCbTest::TearDown() {}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0001
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0001, Function | MediumTest | Level1)
{
    std::vector<unsigned char> pdu;
    unsigned char data = 1;
    pdu.push_back(data);
    auto cbCodec = std::make_shared<GsmCbCodec>();
    cbCodec->PduAnalysis(pdu);
    int8_t format = 1;
    EXPECT_TRUE(cbCodec->GetFormat(format));
    EXPECT_TRUE(cbCodec->GetPriority(format));
    EXPECT_NE(cbCodec->GsmCbCodec::ToString(), "");
    uint8_t gs = 1;
    EXPECT_TRUE(cbCodec->GetGeoScope(gs));
    uint16_t serial = 1;
    EXPECT_TRUE(cbCodec->GetSerialNum(serial));
    bool isUserAlert = false;
    EXPECT_TRUE(cbCodec->IsEtwsEmergencyUserAlert(isUserAlert));
    EXPECT_TRUE(cbCodec->IsEtwsPopupAlert(isUserAlert));
    EXPECT_TRUE(cbCodec->GetServiceCategory(serial));
    uint8_t cmasType;
    EXPECT_TRUE(cbCodec->GetCmasResponseType(cmasType));
    uint8_t cmasClass;
    EXPECT_TRUE(cbCodec->GetCmasMessageClass(cmasClass));
    EXPECT_TRUE(cbCodec->GetWarningType(serial));
    EXPECT_TRUE(cbCodec->GetMsgType(gs));
    EXPECT_TRUE(cbCodec->GetLangType(gs));
    EXPECT_TRUE(cbCodec->GetDcs(gs));
    int64_t recvTime = 0;
    EXPECT_TRUE(cbCodec->GetReceiveTime(recvTime));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0002
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0002, Function | MediumTest | Level1)
{
    std::vector<unsigned char> pdu;
    unsigned char data = 1;
    pdu.push_back(data);
    auto cbCodec = std::make_shared<GsmCbCodec>();
    cbCodec->PduAnalysis(pdu);
    uint8_t cmasType;
    cbCodec->GetCmasCategory(cmasType);
    std::string raw = "qwe";
    std::string message = "asd";
    cbCodec->ConvertToUTF8(raw, message);
    cbCodec->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbCodec->cbHeader_->dcs.codingScheme = DATA_CODING_7BIT;
    cbCodec->ConvertToUTF8(raw, message);
    cbCodec->cbHeader_->dcs.codingScheme = DATA_CODING_UCS2;
    cbCodec->ConvertToUTF8(raw, message);
    cbCodec->cbHeader_->dcs.codingScheme = DATA_CODING_AUTO;
    cbCodec->ConvertToUTF8(raw, message);
    cbCodec->cbHeader_->bEtwsMessage = true;
    cbCodec->cbHeader_->cbEtwsType = GsmCbCodec::ETWS_PRIMARY;
    cbCodec->ConvertToUTF8(raw, message);
    cbCodec->GetCbMessageRaw();
    int8_t format = 1;
    EXPECT_TRUE(cbCodec->GetPriority(format));
    cbCodec->cbHeader_->msgId = PWS_FIRST_ID;
    EXPECT_TRUE(cbCodec->GetPriority(format));
    EXPECT_FALSE(cbCodec->IsSinglePageMsg());
    std::string pdus = "123";
    EXPECT_TRUE(cbCodec->CreateCbMessage(pdu) == nullptr);
    EXPECT_FALSE(cbCodec->CreateCbMessage(pdus));
    EXPECT_TRUE(cbCodec->GetCbHeader() != nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0003
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0003, Function | MediumTest | Level1)
{
    auto smsCbMessage = std::make_shared<GsmCbCodec>();
    uint8_t severity = 1;
    smsCbMessage->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_OBSERVED_DEFUALT;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0004
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0004, Function | MediumTest | Level1)
{
    auto smsCbMessage = std::make_shared<GsmCbCodec>();
    uint8_t severity = 1;
    smsCbMessage->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::OPERATOR_ALERT_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasSeverity(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasUrgency(severity));
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_OBSERVED_DEFUALT;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_OBSERVED;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_OBSERVED_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXTREME_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::SEVERE_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::ALERT_LIKELY_SPANISH;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
    smsCbMessage->cbHeader_->msgId = GsmCbCodec::CmasMsgType::EXPECTED_LIKELY;
    EXPECT_TRUE(smsCbMessage->GetCmasCertainty(severity));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0005
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0005, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    ASSERT_NE(cbMsg, nullptr);
    ASSERT_NE(gsmMsg, nullptr);
    ASSERT_NE(umtsMsg, nullptr);
    EXPECT_FALSE(gsmMsg->Decode2gHeader());
    unsigned char data = 1;
    std::vector<unsigned char> pdu;
    pdu.push_back(data);
    cbMsg->PduAnalysis(pdu);
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    umtsMsg->Decode3gUCS2();
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::PRESIDENTIAL), GsmCbCodec::GSMCbMsgSubType::CMAS_PRESIDENTIAL);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::PRESIDENTIAL_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_PRESIDENTIAL);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::EXTREME_OBSERVED), GsmCbCodec::GSMCbMsgSubType::CMAS_EXTREME);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::EXTREME_OBSERVED_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_EXTREME);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::EXTREME_LIKELY), GsmCbCodec::GSMCbMsgSubType::CMAS_EXTREME);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::EXTREME_LIKELY_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_EXTREME);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::SEVERE_OBSERVED), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::SEVERE_OBSERVED_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::SEVERE_LIKELY), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::SEVERE_LIKELY_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::ALERT_OBSERVED_DEFUALT), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(cbMsg->CMASClass(GsmCbCodec::ALERT_OBSERVED_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodec_0006
 * @tc.name     Test GsmCbCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodec_0006, Function | MediumTest | Level1)
{
    auto smsCbMessage = std::make_shared<GsmCbCodec>();
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::ALERT_LIKELY), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::ALERT_LIKELY_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXPECTED_OBSERVED), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXPECTED_OBSERVED_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXPECTED_LIKELY_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXPECTED_LIKELY_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_SEVERE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::AMBER_ALERT), GsmCbCodec::GSMCbMsgSubType::CMAS_AMBER);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::AMBER_ALERT_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_AMBER);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::MONTHLY_ALERT), GsmCbCodec::GSMCbMsgSubType::CMAS_TEST);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::MONTHLY_ALERT_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_TEST);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXERCISE_ALERT), GsmCbCodec::GSMCbMsgSubType::CMAS_EXERCISE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::EXERCISE_ALERT_SPANISH), GsmCbCodec::GSMCbMsgSubType::CMAS_EXERCISE);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::OPERATOR_ALERT), GsmCbCodec::GSMCbMsgSubType::CMAS_OPERATOR_DEFINED);
    EXPECT_EQ(smsCbMessage->CMASClass(GsmCbCodec::OPERATOR_ALERT_SPANISH),
        GsmCbCodec::GSMCbMsgSubType::CMAS_OPERATOR_DEFINED);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbGsmCodec_0001
 * @tc.name     Test GsmCbGsmCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbGsmCodec_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(ETWS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(ETWS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++) {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    umtsMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->Decode2gHeader();
    gsmMsg->Decode2gHeaderEtws();
    EXPECT_TRUE(gsmMsg->Decode2gCbMsg());

    gsmMsg->Decode2gHeaderCommonCb();
    uint16_t dataLen = gsmMsg->cbPduBuffer_->GetSize() - gsmMsg->cbPduBuffer_->GetCurPosition();
    gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_TRUE(gsmMsg->DecodeEtwsMsg());

    umtsMsg->Decode3gHeader();
    umtsMsg->Decode3gHeaderPartData(0);
    umtsMsg->Decode3gCbMsg();
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbGsmCodec_0002
 * @tc.name     Test GsmCbGsmCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbGsmCodec_0002, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(CMAS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(CMAS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++) {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    umtsMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->Decode2gHeader();
    gsmMsg->Decode2gHeaderEtws();
    EXPECT_TRUE(gsmMsg->Decode2gCbMsg());

    gsmMsg->Decode2gHeaderCommonCb();
    uint16_t dataLen = gsmMsg->cbPduBuffer_->GetSize() - gsmMsg->cbPduBuffer_->GetCurPosition();
    gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_TRUE(gsmMsg->DecodeEtwsMsg());

    umtsMsg->Decode3gHeader();
    umtsMsg->Decode3gHeaderPartData(0);
    umtsMsg->Decode3gCbMsg();
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbGsmCodec_0003
 * @tc.name     Test GsmCbGsmCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbGsmCodec_0003, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(CBS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(CBS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++) {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    umtsMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->Decode2gHeader();
    gsmMsg->Decode2gHeaderEtws();
    EXPECT_TRUE(gsmMsg->Decode2gCbMsg());

    gsmMsg->Decode2gHeaderCommonCb();
    uint16_t dataLen = gsmMsg->cbPduBuffer_->GetSize() - gsmMsg->cbPduBuffer_->GetCurPosition();
    gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_TRUE(gsmMsg->DecodeEtwsMsg());

    umtsMsg->Decode3gHeader();
    umtsMsg->Decode3gHeaderPartData(0);
    umtsMsg->Decode3gCbMsg();
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbGsmCodec_0004
 * @tc.name     Test GsmCbGsmCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbGsmCodec_0004, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(CMAS_JP_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(CMAS_JP_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++) {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    umtsMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->Decode2gHeader();
    gsmMsg->Decode2gHeaderEtws();
    EXPECT_TRUE(gsmMsg->Decode2gCbMsg());

    gsmMsg->Decode2gHeaderCommonCb();
    uint16_t dataLen = gsmMsg->cbPduBuffer_->GetSize() - gsmMsg->cbPduBuffer_->GetCurPosition();
    gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_FALSE(gsmMsg->DecodeEtwsMsg());

    umtsMsg->Decode3gHeader();
    umtsMsg->Decode3gHeaderPartData(0);
    umtsMsg->Decode3gCbMsg();
    umtsMsg->Decode3g7Bit();
    EXPECT_TRUE(umtsMsg->Decode3gUCS2());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(umtsMsg->Decode3gCbMsg());

    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    EXPECT_TRUE(umtsMsg->Decode3gUCS2());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbGsmCodec_0005
 * @tc.name     Test GsmCbGsmCodec
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbGsmCodec_0005, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    unsigned char data = 1;
    std::vector<unsigned char> pdu;
    pdu.push_back(data);
    cbMsg->PduAnalysis(pdu);
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    auto umtsMsg = std::make_shared<GsmCbUmtsCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    ASSERT_NE(gsmMsg, nullptr);
    ASSERT_NE(umtsMsg, nullptr);
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_7BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_8BIT;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_UCS2;
    gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(umtsMsg->Decode3gCbMsg());
    cbMsg->cbHeader_->dcs.codingScheme = DataCodingScheme::DATA_CODING_ASCII7BIT;
    gsmMsg->Decode2gCbMsg();
    umtsMsg->Decode3gCbMsg();
    cbMsg->cbHeader_->totalPages = 1;
    umtsMsg->Decode3g7Bit();
    EXPECT_FALSE(umtsMsg->Decode3gUCS2());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmSmsCbHandler_0001
 * @tc.name     Test GsmSmsCbHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmSmsCbHandler_0001, Function | MediumTest | Level1)
{
    auto gsmSmsCbHandler = std::make_shared<GsmSmsCbHandler>(INVALID_SLOTID);
    auto cbMessage = std::make_shared<GsmCbCodec>();
    auto message = std::make_shared<CBConfigReportInfo>();
    AppExecFwk::InnerEvent::Pointer event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_CELL_BROADCAST, 1);
    gsmSmsCbHandler->ProcessEvent(event);
    event = AppExecFwk::InnerEvent::Get(RadioEvent::RADIO_GET_STATUS, 1);
    gsmSmsCbHandler->ProcessEvent(event);
    gsmSmsCbHandler->HandleCbMessage(message);
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMessage), 0);
    EXPECT_TRUE(gsmSmsCbHandler->FindCbMessage(cbMessage) == nullptr);
    EXPECT_FALSE(gsmSmsCbHandler->AddCbMessageToList(cbMessage));
    EXPECT_FALSE(gsmSmsCbHandler->SendCbMessageBroadcast(cbMessage));
    EXPECT_TRUE(gsmSmsCbHandler->CheckCbActive(cbMessage));
    cbMessage->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMessage), 1);
    cbMessage->cbHeader_->totalPages = 1;
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMessage), 0x01);
    EXPECT_FALSE(gsmSmsCbHandler->AddCbMessageToList(cbMessage));
    CbInfo cbInfo;
    gsmSmsCbHandler->cbMsgList_.push_back(cbInfo);
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMessage), 0x01);
    cbMessage = nullptr;
    message = nullptr;
    event = nullptr;
    gsmSmsCbHandler->ProcessEvent(event);
    SmsCbData::CbData sendData;
    gsmSmsCbHandler->GetCbData(cbMessage, sendData);
    EventFwk::Want want;
    gsmSmsCbHandler->PackageWantData(sendData, want);
    gsmSmsCbHandler->HandleCbMessage(message);
    gsmSmsCbHandler->SetWantData(want, cbMessage);
    EXPECT_FALSE(gsmSmsCbHandler->CheckCbActive(cbMessage));
    EXPECT_FALSE(gsmSmsCbHandler->AddCbMessageToList(cbMessage));
    EXPECT_FALSE(gsmSmsCbHandler->SendCbMessageBroadcast(cbMessage));
    EXPECT_TRUE(gsmSmsCbHandler->FindCbMessage(cbMessage) == nullptr);
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMessage), 0);
    gsmSmsCbHandler->cbMsgList_.clear();
    gsmSmsCbHandler->ClearExpiredMessage();
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecOperator_0001
 * @tc.name     Test GsmCbCodec::operator==
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecOperator_0001, Function | MediumTest | Level1)
{
    GsmCbCodec gsmCbCodec1;
    GsmCbCodec gsmCbCodec2;
    const std::vector<unsigned char> pdu = {0x01, 0x02};
    EXPECT_FALSE(gsmCbCodec1 == gsmCbCodec2);

    bool ret = gsmCbCodec2.ParamsCheck(pdu);
    EXPECT_FALSE(gsmCbCodec1 == gsmCbCodec2);

    gsmCbCodec2.cbHeader_ = nullptr;
    ret = gsmCbCodec1.ParamsCheck(pdu);
    EXPECT_FALSE(gsmCbCodec1 == gsmCbCodec2);

    ret = gsmCbCodec2.ParamsCheck(pdu);
    EXPECT_TRUE(gsmCbCodec1 == gsmCbCodec2);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_PickOneByte_0001
 * @tc.name     Test PickOneByte
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, PickOneByte_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    std::vector<unsigned char> pdu(GsmSmsCbHandler::MAX_CB_MSG_LEN + 1, 0x01);
    bool ret = cbMsg->PduAnalysis(pdu);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetPduData_0001
 * @tc.name     Test GetPduData
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GetPduData_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    std::vector<unsigned char> dataPdu = {0x01};
    cbMsg->GetPduData(dataPdu);
    EXPECT_TRUE(cbMsg->cbPduBuffer_ == nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_ConvertToUTF8_0001
 * @tc.name     Test ConvertToUTF8
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, ConvertToUTF8_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    const std::string raw = "raw";
    std::string message = "message";
    cbMsg->ConvertToUTF8(raw, message);
    EXPECT_TRUE(cbMsg->cbHeader_ == nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DecodeGeneralDcs_0001
 * @tc.name     Test DecodeGeneralDcs
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, DecodeGeneralDcs_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    const uint8_t dcsData = 0x04;
    GsmCbCodec::GsmCbMsgDcs dcs = {};
    uint8_t tmpScheme = (dcsData & 0x0C) >> 0x02;
    EXPECT_EQ(tmpScheme, 0x01);
    cbMsg->DecodeGeneralDcs(dcsData, dcs);
    EXPECT_EQ(dcs.codingScheme, DATA_CODING_8BIT);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DecodeCbMsgDCS_0001
 * @tc.name     Test DecodeCbMsgDCS
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, DecodeCbMsgDCS_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    const uint8_t dcsData = 0xE0;
    const unsigned short iosData = 1;
    GsmCbCodec::GsmCbMsgDcs dcs = {};
    cbMsg->DecodeCbMsgDCS(dcsData, iosData, dcs);
    EXPECT_EQ(dcs.codingGroup, GsmCbCodec::SMS_CBMSG_CODGRP_WAP);

    const uint8_t dcsData1 = 0xF0;
    cbMsg->DecodeCbMsgDCS(dcsData1, iosData, dcs);
    EXPECT_EQ(dcs.codingGroup, GsmCbCodec::SMS_CBMSG_CODGRP_CLASS_CODING);

    const uint8_t dcsData2 = 0xD0;
    cbMsg->DecodeCbMsgDCS(dcsData2, iosData, dcs);
    EXPECT_EQ(dcs.codingGroup, GsmCbCodec::SMS_CBMSG_CODGRP_GENERAL_DCS);
}

namespace {
constexpr size_t CB_PAGE_OCTETS = 82;
constexpr size_t CB_PAGE_SEPTETS = 93;
constexpr uint8_t CB_SEPTET_BITS = 7;
constexpr uint8_t CB_OCTET_BITS = 8;
constexpr uint8_t CB_GSM7_CR = 0x0D;

std::vector<unsigned char> PackCbSeptets(const std::vector<uint8_t> &septets)
{
    std::vector<unsigned char> out((septets.size() * CB_SEPTET_BITS + CB_OCTET_BITS - 1) / CB_OCTET_BITS, 0);
    for (size_t i = 0; i < septets.size(); i++) {
        for (size_t bit = 0; bit < CB_SEPTET_BITS; bit++) {
            size_t pos = i * CB_SEPTET_BITS + bit;
            if ((septets[i] >> bit) & 0x01) {
                out[pos / CB_OCTET_BITS] |= static_cast<unsigned char>(1 << (pos % CB_OCTET_BITS));
            }
        }
    }
    return out;
}

std::vector<unsigned char> BuildGsmCbPduWithContent(
    uint16_t serial, uint16_t msgId, uint8_t dcs, uint8_t pageParam, std::vector<unsigned char> content)
{
    std::vector<unsigned char> pdu = { static_cast<unsigned char>(serial >> CB_OCTET_BITS),
        static_cast<unsigned char>(serial), static_cast<unsigned char>(msgId >> CB_OCTET_BITS),
        static_cast<unsigned char>(msgId), dcs, pageParam };
    content.resize(CB_PAGE_OCTETS, 0x00);
    pdu.insert(pdu.end(), content.begin(), content.end());
    return pdu;
}

// GSM CBS page, 93 GSM 7bit characters padded with CR (3GPP TS 23.041 9.4.1.2.5)
std::vector<unsigned char> BuildGsmCbPdu(
    uint16_t serial, uint16_t msgId, uint8_t dcs, uint8_t pageParam, const std::string &text)
{
    std::vector<uint8_t> septets(text.begin(), text.end());
    septets.resize(CB_PAGE_SEPTETS, CB_GSM7_CR);
    return BuildGsmCbPduWithContent(serial, msgId, dcs, pageParam, PackCbSeptets(septets));
}

// UMTS CBS message, every page is 82 octets plus the CBS-Message-Information-Length octet (3GPP TS 23.041 9.4.2.2)
std::vector<unsigned char> BuildUmtsCbPdu(
    uint16_t msgId, uint8_t dcs, const std::vector<std::pair<std::vector<unsigned char>, uint8_t>> &pages)
{
    std::vector<unsigned char> pdu = { 0x01, static_cast<unsigned char>(msgId >> CB_OCTET_BITS),
        static_cast<unsigned char>(msgId), 0x40, 0x10, dcs, static_cast<unsigned char>(pages.size()) };
    for (auto &page : pages) {
        std::vector<unsigned char> content = page.first;
        content.resize(CB_PAGE_OCTETS, 0x00);
        pdu.insert(pdu.end(), content.begin(), content.end());
        pdu.push_back(page.second);
    }
    return pdu;
}

std::vector<unsigned char> Ucs2Of(const std::string &text)
{
    std::vector<unsigned char> out;
    for (char c : text) {
        out.push_back(0x00);
        out.push_back(static_cast<unsigned char>(c));
    }
    return out;
}

std::string BodyOf(const std::shared_ptr<GsmCbCodec> &cbMsg)
{
    std::string body;
    if (cbMsg != nullptr) {
        cbMsg->ConvertToUTF8(cbMsg->GetCbMessageRaw(), body);
    }
    return body;
}
} // namespace

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode3gUCS2PageOffset_0001
 * @tc.name     Test Decode3gUCS2 with 4 and 15 pages
 * @tc.desc     Function test, the length octet of page i is at 83 * i + 82 (3GPP TS 23.041 9.4.2.2.5)
 */
HWTEST_F(BranchCbTest, Decode3gUCS2PageOffset_0001, Function | MediumTest | Level1)
{
    const size_t maxPages = 15;
    for (size_t pageCount : { static_cast<size_t>(3), static_cast<size_t>(4), maxPages }) {
        std::vector<std::pair<std::vector<unsigned char>, uint8_t>> pages;
        std::string expected;
        for (size_t i = 0; i < pageCount; i++) {
            std::string text(CB_PAGE_OCTETS / 2, static_cast<char>('A' + i));
            pages.push_back({ Ucs2Of(text), static_cast<uint8_t>(CB_PAGE_OCTETS) });
            expected += text;
        }
        auto cbMsg = GsmCbCodec::CreateCbMessage(BuildUmtsCbPdu(0x0032, 0x48, pages));
        ASSERT_NE(cbMsg, nullptr);
        EXPECT_EQ(cbMsg->GetCbMessageRaw().size(), pageCount * CB_PAGE_OCTETS);
        EXPECT_EQ(BodyOf(cbMsg), expected);
    }
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gPageParameter_0001
 * @tc.name     Test GSM page parameter normalization
 * @tc.desc     Function test, page parameter 0000 is treated as 0001 0001 (3GPP TS 23.041 9.4.1.2.4)
 */
HWTEST_F(BranchCbTest, Decode2gPageParameter_0001, Function | MediumTest | Level1)
{
    const uint8_t pageParamMax = 0xFF;
    std::vector<std::pair<uint8_t, std::pair<uint8_t, uint8_t>>> cases = { { 0x00, { 1, 1 } }, { 0x10, { 1, 1 } },
        { 0x01, { 1, 1 } }, { 0x32, { 1, 1 } }, { 0x11, { 1, 1 } }, { 0x12, { 1, 2 } }, { 0x22, { 2, 2 } },
        { pageParamMax, { 15, 15 } } };
    for (auto &item : cases) {
        auto cbMsg = GsmCbCodec::CreateCbMessage(BuildGsmCbPdu(0x4010, 0x0032, 0x01, item.first, "Test"));
        ASSERT_NE(cbMsg, nullptr);
        EXPECT_EQ(cbMsg->GetCbHeader()->page, item.second.first);
        EXPECT_EQ(cbMsg->GetCbHeader()->totalPages, item.second.second);
    }

    auto gsmSmsCbHandler = std::make_shared<GsmSmsCbHandler>(INVALID_SLOTID);
    auto cbMsg = GsmCbCodec::CreateCbMessage(BuildGsmCbPdu(0x4010, 0x0032, 0x01, 0x00, "Test"));
    ASSERT_NE(cbMsg, nullptr);
    EXPECT_EQ(gsmSmsCbHandler->CheckCbMessage(cbMsg), cbMsg->GetCbHeader()->totalPages);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecToString_0001
 * @tc.name     Test GsmCbCodecToString
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecToString_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    std::string ret = cbMsg->ToString();
    EXPECT_EQ(ret, "GsmCbCodec Header nullptr");
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecGetBranch_0001
 * @tc.name     Test GsmCbCodecGetBranch
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecGetBranch_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    int8_t cbPriority = 1;
    bool ret = cbMsg->GetPriority(cbPriority);
    EXPECT_FALSE(ret);

    uint8_t geoScope = 1;
    ret = cbMsg->GetGeoScope(geoScope);
    EXPECT_FALSE(ret);

    uint16_t cbSerial = 1;
    ret = cbMsg->GetSerialNum(cbSerial);
    EXPECT_FALSE(ret);

    uint16_t cbCategoty = 1;
    ret = cbMsg->GetServiceCategory(cbCategoty);
    EXPECT_FALSE(ret);

    bool primary = true;
    ret = cbMsg->IsEtwsPrimary(primary);
    EXPECT_FALSE(ret);

    ret = cbMsg->IsEtwsMessage(primary);
    EXPECT_FALSE(ret);

    ret = cbMsg->IsCmasMessage(primary);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetWarningType_0001
 * @tc.name     Test GetWarningType
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GetWarningType_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    uint16_t type = 1;
    bool ret = cbMsg->GetWarningType(type);
    EXPECT_FALSE(ret);

    std::vector<unsigned char> pdu = {0x01};
    ret = cbMsg->ParamsCheck(pdu);
    ret = cbMsg->GetWarningType(type);
    EXPECT_TRUE(ret);

    type = -2; // OTHER_TYPE = -2
    ret = cbMsg->GetWarningType(type);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecIsEtws_0001
 * @tc.name     Test GsmCbCodecIsEtws
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecIsEtws_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    bool alert = true;
    bool ret = cbMsg->IsEtwsEmergencyUserAlert(alert);
    EXPECT_FALSE(ret);

    ret = cbMsg->IsEtwsPopupAlert(alert);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecGetCmas_0001
 * @tc.name     Test GsmCbCodecGetCmas
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecGetCmas_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    uint8_t severity = 1;
    bool ret = cbMsg->GetCmasSeverity(severity);
    EXPECT_FALSE(ret);

    uint8_t urgency = 1;
    ret = cbMsg->GetCmasUrgency(urgency);
    EXPECT_FALSE(ret);

    uint8_t certainty = 1;
    ret = cbMsg->GetCmasCertainty(certainty);
    EXPECT_FALSE(ret);

    uint8_t cmasClass = 1;
    ret = cbMsg->GetCmasMessageClass(cmasClass);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GsmCbCodecGetBranch_0002
 * @tc.name     Test GsmCbCodecGetBranch
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, GsmCbCodecGetBranch_0002, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    uint16_t msgId = 1;
    bool ret = cbMsg->GetMessageId(msgId);
    EXPECT_FALSE(ret);

    uint8_t msgType = 1;
    ret = cbMsg->GetMsgType(msgType);
    EXPECT_FALSE(ret);

    ret = cbMsg->GetLangType(msgType);
    EXPECT_FALSE(ret);

    ret = cbMsg->GetDcs(msgType);
    EXPECT_FALSE(ret);

    int64_t receiveTime = 1;
    ret = cbMsg->GetReceiveTime(receiveTime);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gHeaderBranch_0001
 * @tc.name     Test Decode2gHeaderBranch
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Decode2gHeaderBranch_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(ETWS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(ETWS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++)
    {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_;
    bool ret = gsmMsg->Decode2gHeader();
    EXPECT_FALSE(ret);
    ret = gsmMsg->Decode2gHeaderEtws();
    EXPECT_FALSE(ret);
    ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_FALSE(ret);

    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 1;
    EXPECT_FALSE(gsmMsg->cbHeader_->msgId >= PWS_FIRST_ID);
    ret = gsmMsg->Decode2gHeaderEtws();
    EXPECT_FALSE(ret);
    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 1;
    ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_FALSE(ret);

    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 2;
    ret = gsmMsg->Decode2gHeader();
    EXPECT_FALSE(ret);
    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 2;
    ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_FALSE(ret);

    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 3;
    ret = gsmMsg->Decode2gHeader();
    EXPECT_FALSE(ret);
    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 3;
    ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gHeaderCommonCb_0001
 * @tc.name     Test Decode2gHeaderCommonCb
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Decode2gHeaderCommonCb_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(ETWS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(ETWS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++)
    {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    ASSERT_NE(gsmMsg, nullptr);
    gsmMsg->cbPduBuffer_ = nullptr;
    bool ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_FALSE(ret);

    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->cbHeader_->totalPages = 18; // MAX_PAGE_NUM
    ret = gsmMsg->Decode2gHeaderCommonCb();
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gCbMsg_0001
 * @tc.name     Test Decode2gCbMsg
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Decode2gCbMsg_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(ETWS_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(ETWS_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++)
    {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    gsmMsg->cbHeader_->dcs.codingScheme = DATA_CODING_8BIT;
    gsmMsg->cbPduBuffer_->curPosition_ = gsmMsg->cbPduBuffer_->totolLength_ - 1;
    bool ret = gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(ret);

    gsmMsg->cbPduBuffer_->curPosition_ = 0;
    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(ret);

    gsmMsg->cbHeader_->dcs.iso639Lang[0] = 0;
    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(ret);

    gsmMsg->cbHeader_->dcs.iso639Lang[0] = 1;
    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(ret);

    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(ret);

    gsmMsg->cbPduBuffer_->pduBuffer_ = nullptr;
    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_FALSE(ret);

    gsmMsg->cbHeader_->dcs.codingScheme = DATA_CODING_AUTO;
    ret = gsmMsg->Decode2gCbMsg();
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gCbMsg7bit_0001
 * @tc.name     Test Decode2gCbMsg7bit
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Decode2gCbMsg7bit_0001, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(0);
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    ASSERT_NE(gsmMsg, nullptr);
    uint16_t dataLen = 5;
    bool ret = gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_Decode2gCbMsg7bit_0002
 * @tc.name     Test Decode2gCbMsg7bit
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Decode2gCbMsg7bit_0002, Function | MediumTest | Level1)
{
    auto cbMsg = std::make_shared<GsmCbCodec>();
    ASSERT_NE(cbMsg, nullptr);
    cbMsg->CreateCbMessage(CMAS_JP_PDU);
    std::vector<unsigned char> pdu = StringUtils::HexToByteVector(CMAS_JP_PDU);
    cbMsg->cbPduBuffer_ = std::make_shared<GsmCbPduDecodeBuffer>(pdu.size());
    cbMsg->cbHeader_ = std::make_shared<GsmCbCodec::GsmCbMessageHeader>();
    for (size_t index = 0; index < pdu.size() && index < cbMsg->cbPduBuffer_->GetSize(); index++)
    {
        cbMsg->cbPduBuffer_->pduBuffer_[index] = static_cast<char>(pdu[index]);
    }
    auto gsmMsg = std::make_shared<GsmCbGsmCodec>(cbMsg->cbHeader_, cbMsg->cbPduBuffer_, cbMsg);
    ASSERT_NE(gsmMsg, nullptr);
    gsmMsg->cbPduBuffer_ = cbMsg->cbPduBuffer_;
    gsmMsg->cbHeader_->dcs.iso639Lang[0] = 1;
    uint16_t dataLen = 5;
    bool ret = gsmMsg->Decode2gCbMsg7bit(dataLen);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_misc_manager_CloseCBRange_0001
 * @tc.name     Test CloseCBRange
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, misc_manager_CloseCBRange_0001, Function | MediumTest | Level1)
{
    auto smsMiscManager = std::make_shared<SmsMiscManager>(INVALID_SLOTID);
    ASSERT_NE(smsMiscManager, nullptr);
    uint32_t fromMsgId = 2;
    uint32_t toMsgId = 2;
    SmsMiscManager::gsmCBRangeInfo rangeInfo(2, 2);
    smsMiscManager->rangeList_ = {rangeInfo};
    bool ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    toMsgId = 3;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    toMsgId = 1;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    fromMsgId = 1;
    toMsgId = 2;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    toMsgId = 3;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    fromMsgId = 3;
    toMsgId = 2;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    smsMiscManager->rangeList_ = {rangeInfo};
    toMsgId = 1;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    SmsMiscManager::gsmCBRangeInfo rangeInfo1(2, 3);
    smsMiscManager->rangeList_ = {rangeInfo1};
    toMsgId = 3;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);

    SmsMiscManager::gsmCBRangeInfo rangeInfo2(2, 1);
    smsMiscManager->rangeList_ = {rangeInfo2};
    fromMsgId = 1;
    toMsgId = 1;
    ret = smsMiscManager->CloseCBRange(fromMsgId, toMsgId);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_misc_manager_SplitMidValue_0001
 * @tc.name     Test SplitMidValue
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, misc_manager_SplitMidValue_0001, Function | MediumTest | Level1)
{
    auto smsMiscManager = std::make_shared<SmsMiscManager>(INVALID_SLOTID);
    ASSERT_NE(smsMiscManager, nullptr);
    std::string value = "";
    std::string start = "";
    std::string end = "";
    const std::string delimiter = ":";
    bool ret = smsMiscManager->SplitMidValue(value, start, end, delimiter);
    EXPECT_FALSE(ret);

    value = ":value";
    ret = smsMiscManager->SplitMidValue(value, start, end, delimiter);
    EXPECT_FALSE(ret);

    value = "value:";
    ret = smsMiscManager->SplitMidValue(value, start, end, delimiter);
    EXPECT_FALSE(ret);

    value = "value";
    ret = smsMiscManager->SplitMidValue(value, start, end, delimiter);
    EXPECT_TRUE(ret);

    value = "smsMisc:value";
    ret = smsMiscManager->SplitMidValue(value, start, end, delimiter);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_misc_DelSimMessage_0001
 * @tc.name     Test DelSimMessage
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, misc_manager_DelSimMessage_0001, Function | MediumTest | Level1)
{
    auto smsMiscManager = std::make_shared<SmsMiscManager>(INVALID_SLOTID);
    ASSERT_NE(smsMiscManager, nullptr);
    uint32_t msgIndex = 1;
    int32_t ret = smsMiscManager->DelSimMessage(msgIndex);
    EXPECT_EQ(ret, TELEPHONY_ERR_SLOTID_INVALID);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_misc_SetSmscAddr_0001
 * @tc.name     Test SetSmscAddr
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, misc_manager_SetSmscAddr_0001, Function | MediumTest | Level1)
{
    auto smsMiscManager = std::make_shared<SmsMiscManager>(INVALID_SLOTID);
    ASSERT_NE(smsMiscManager, nullptr);
    const std::string scAddr = "";
    int32_t ret = smsMiscManager->SetSmscAddr(scAddr);
    EXPECT_EQ(ret, TELEPHONY_ERR_RIL_CMD_FAIL);
    std::u16string smscAddress = u"";
    ret = smsMiscManager->GetSmscAddr(smscAddress);
    EXPECT_EQ(ret, TELEPHONY_ERR_SUCCESS);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_GetSmsStateEventIntValue_0001
 * @tc.name     Test GetSmsStateEventIntValue
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, misc_GetSmsStateEventIntValue_0001, Function | MediumTest | Level1)
{
    SmsStateObserver observer;
    std::shared_ptr<CommonEventSubscribeInfo> subscribeInfo = std::make_shared<CommonEventSubscribeInfo>();
    std::shared_ptr<SmsStateEventSubscriber> smsStateEventSubscriber =
        std::make_shared<SmsStateEventSubscriber>(*subscribeInfo, observer);
    std::string event = "event";
    smsStateEventSubscriber->smsStateEvenMapIntValues_[event] = COMMON_EVENT_SMS_CB_RECEIVE_COMPLETED;
    auto ret = smsStateEventSubscriber->GetSmsStateEventIntValue(event);
    EXPECT_EQ(ret, COMMON_EVENT_SMS_CB_RECEIVE_COMPLETED);
}

/**
 * @tc.number   Telephony_Sms_OnAddSystemAbility_0001
 * @tc.name     Test OnAddSystemAbility
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Sms_OnAddSystemAbility_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsStateEventSubscriber> smsStateEventSubscriber = nullptr;
    std::shared_ptr<SmsStateObserver::SystemAbilityStatusChangeListener> sysAbilityStatus =
        std::make_shared<SmsStateObserver::SystemAbilityStatusChangeListener>(smsStateEventSubscriber);
    int32_t systemAbilityId = 1;
    const std::string deviceId = "123";
    sysAbilityStatus->OnAddSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(systemAbilityId != COMMON_EVENT_SERVICE_ID);

    systemAbilityId = COMMON_EVENT_SERVICE_ID;
    sysAbilityStatus->OnAddSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(sysAbilityStatus->sub_ == nullptr);
}

/**
 * @tc.number   Telephony_Sms_OnRemoveSystemAbility_0001
 * @tc.name     Test OnRemoveSystemAbility
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Sms_OnRemoveSystemAbility_0001, Function | MediumTest | Level1)
{
    SmsStateObserver observer;
    std::shared_ptr<CommonEventSubscribeInfo> subscribeInfo = std::make_shared<CommonEventSubscribeInfo>();
    std::shared_ptr<SmsStateEventSubscriber> smsStateEventSubscriber = nullptr;
    std::shared_ptr<SmsStateObserver::SystemAbilityStatusChangeListener> sysAbilityStatus =
        std::make_shared<SmsStateObserver::SystemAbilityStatusChangeListener>(smsStateEventSubscriber);
    int32_t systemAbilityId = 1;
    const std::string deviceId = "123";
    sysAbilityStatus->OnRemoveSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(systemAbilityId != COMMON_EVENT_SERVICE_ID);

    systemAbilityId = COMMON_EVENT_SERVICE_ID;
    sysAbilityStatus->OnRemoveSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(systemAbilityId == COMMON_EVENT_SERVICE_ID);

    sysAbilityStatus->OnRemoveSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(sysAbilityStatus->sub_ == nullptr);

    smsStateEventSubscriber = std::make_shared<SmsStateEventSubscriber>(*subscribeInfo, observer);
    sysAbilityStatus =
        std::make_shared<SmsStateObserver::SystemAbilityStatusChangeListener>(smsStateEventSubscriber);
    sysAbilityStatus->OnRemoveSystemAbility(systemAbilityId, deviceId);
    EXPECT_TRUE(sysAbilityStatus->sub_ != nullptr);
}

/**
 * @tc.number   Telephony_Sms_GetSmsUserDataMultipage_0001
 * @tc.name     Test GetSmsUserDataMultipage
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Sms_GetSmsUserDataMultipage_0001, Function | MediumTest | Level1)
{
    auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(INVALID_SLOTID);
    int32_t smsPagesCount = 1;
    SmsReceiveIndexer smsReceiveIndexer;
    smsReceiveIndexer.msgSeqId_ = 0;
    smsReceiveIndexer.msgRefId_ = 1;
    std::vector<SmsReceiveIndexer> dbIndexers = {smsReceiveIndexer, smsReceiveIndexer};
    int32_t position = -1;
    std::vector<std::string> initialData = {"User1", "User2"};
    std::shared_ptr<std::vector<std::string>> userDataRaws = std::make_shared<std::vector<std::string>>(initialData);
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, MAX_SEGMENT_NUM, dbIndexers, position, userDataRaws);
    EXPECT_TRUE(position < 0);

    position = 2;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, MAX_SEGMENT_NUM, dbIndexers, position, userDataRaws);
    EXPECT_TRUE(position >= static_cast<int32_t>(dbIndexers.size()));

    position = 0;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, dbIndexers[position].GetMsgCount(), dbIndexers,
        position, userDataRaws);
    EXPECT_TRUE(dbIndexers[position].GetMsgSeqId() < 1);

    dbIndexers[position].msgSeqId_ = MAX_SEGMENT_NUM + 1;
    reliabilityHandler->HiSysEventCBResult(true);
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, dbIndexers[position].GetMsgCount(), dbIndexers,
        position, userDataRaws);
    EXPECT_TRUE(dbIndexers[position].GetMsgSeqId() > MAX_SEGMENT_NUM);

    dbIndexers[position].msgSeqId_ = 1;
    dbIndexers[position + 1].msgSeqId_ = 0;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, dbIndexers[position].GetMsgCount(), dbIndexers,
        position, userDataRaws);
    EXPECT_TRUE(dbIndexers[position + 1].GetMsgSeqId() < 1);

    dbIndexers.push_back(smsReceiveIndexer);
    dbIndexers[position + 1].msgSeqId_ = MAX_SEGMENT_NUM + 1;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, dbIndexers[position].GetMsgCount(), dbIndexers,
        position, userDataRaws);
    EXPECT_TRUE(dbIndexers[position + 1].GetMsgSeqId() > MAX_SEGMENT_NUM);

    dbIndexers.push_back(smsReceiveIndexer);
    dbIndexers[position + 1].msgSeqId_ = 1;
    reliabilityHandler->GetSmsUserDataMultipage(smsPagesCount, dbIndexers[position].GetMsgCount(), dbIndexers,
        position, userDataRaws);
    EXPECT_EQ(smsPagesCount, 2);
}

/**
 * @tc.number   Telephony_Sms_SendCacheMapLimitCheck_0001
 * @tc.name     Test SendCacheMapLimitCheck
 * @tc.desc     Function test
 */
HWTEST_F(BranchCbTest, Sms_SendCacheMapLimitCheck_0001, Function | MediumTest | Level1)
{
    std::function<void(std::shared_ptr<SmsSendIndexer>)> fun = nullptr;
    std::shared_ptr<SmsSender> smsSender = std::make_shared<CdmaSmsSender>(INVALID_SLOTID, fun);
    const sptr<ISendShortMessageCallback> sendCallback = nullptr;
    bool ret = smsSender->SendCacheMapLimitCheck(sendCallback);
    EXPECT_FALSE(ret);
    for (size_t i = 0; i < 30; i++)
    {
        smsSender->sendCacheMap_[i] = nullptr;
    }
    ret = smsSender->SendCacheMapLimitCheck(sendCallback);
    EXPECT_TRUE(ret);
}
} // namespace Telephony
} // namespace OHOS
