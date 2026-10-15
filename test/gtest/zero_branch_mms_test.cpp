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

#include "gtest/gtest.h"
#include "mms_body.h"
#include "mms_body_part.h"
#include "mms_content_type.h"
#include "mms_encode_buffer.h"
#include "mms_header.h"
#include "mms_msg.h"
#include "mms_network_manager.h"
#include "mms_persist_helper.h"
#include "sms_mms_gtest.h"
#include "sms_service.h"
#include "sms_wap_push_buffer.h"
#include "sms_wap_push_content_type.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

namespace {
const uint32_t TRANSACTION_ID_LENGTH = 3;
const uint32_t PDU_BUFFE_LENGTH = 126;
const uint32_t PDU_BUFFE_LENGTH_ONE = 128;
const uint32_t PDU_BUFFE_LENGTH_TWO = 127;
const uint32_t PDU_BUFFE_LENGTH_THREE = 31;
const uint32_t PDU_BUFFE_LENGTH_FOUR = 129;
const uint32_t PDU_BUFFE_LENGTH_FIVE = 139;
const uint32_t PDU_BUFFE_LENGTH_SIX = 134;
const uint32_t PDU_BUFFE_LENGTH_SEVEN = 152;
const uint32_t PDU_BUFFE_LENGTH_EIGHT = 153;
const uint32_t PDU_BUFFE_LENGTH_NINE = 138;
const uint32_t PDU_BUFFE_LENGTH_TEN = 133;
const uint32_t PDU_BUFFE_LENGTH_ELEVEN = 151;
const uint32_t PDU_BUFFE_LENGTH_TWELVE = 154;
const uint32_t PDU_BUFFE_LENGTH_THIRTEEN = 131;
const uint32_t PDU_BUFFE_LENGTH_FOURTEEN = 137;
const uint8_t MIN_TOKEN = 30;
const uint8_t MAX_TOKEN = 127;
const uint8_t FIELD_ID = 2;
const int32_t INVALID_SLOTID = 2;
const int32_t VALUE_LENGTH = 2;
static constexpr uint32_t MAX_MMS_MSG_PART_LEN = 10 * 1024 * 1024;
const uint32_t CODE_BUFFER_MAX_SIZE = 10 * 1024 * 1024;
const std::string METHOD_POST = "POST";
const std::string METHOD_GET = "GET";
const std::string TEST_DATA = "test data test data";
const std::string TEST_PATH = "testPath";
const std::string TEST_URLS = "url1, url2, url3,";
const size_t URL_COUNT = 3;
constexpr uint32_t SPLIT_PDU_LENGTH = 195 * 1024;
const size_t SPLIT_PDU_COUNT = 3;
const size_t BUF_LEN = 10;
const int VALUE_4 = 4;
const int VALUE_9 = 9;
const int VALUE_16 = 0x10;
const int VALUE_31 = 31;
const int VALUE_32 = 32;
const int VALUE_AF = 0xAF;
constexpr const char *TEST_STR = "str";
constexpr const char *INPUT_STR = "00";
constexpr const char *SRC_STR = "123";
constexpr const char *BEGIN_STR = "01";
constexpr const char *END_STR = "FF";
const char PDU_COUNT = 10;
const char INPUT_INTEGER = 128;
const std::string VNET_MMSC = "http://mmsc.vnet.mobi";
} // namespace

class BranchMmsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};
void BranchMmsTest::SetUpTestCase() {}

constexpr uint32_t EVENT_RELEASE_DATA_SHARE_HELPER = 10000;
void BranchMmsTest::TearDownTestCase()
{
    DelayedSingleton<SmsPersistHelper>::GetInstance()->RemoveEvent(EVENT_RELEASE_DATA_SHARE_HELPER);
}

void BranchMmsTest::SetUp() {}

void BranchMmsTest::TearDown() {}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsHeader_0001
 * @tc.name     Test MmsHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsHeader_0001, Function | MediumTest | Level1)
{
    auto mmsHeader = std::make_shared<MmsHeader>();
    MmsEncodeString encodeString;
    MmsDecodeBuffer decodeBuffer;
    MmsEncodeBuffer encodeBuffer;
    std::string encodeAddress = SRC_STR;
    uint8_t value = 0;
    MmsAddress address(encodeAddress);
    std::vector<MmsAddress> addressValue;
    addressValue.push_back(address);
    mmsHeader->octetValueMap_.insert(pair<uint8_t, uint8_t>(1, 1));
    mmsHeader->longValueMap_.insert(pair<uint8_t, int64_t>(1, 1));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(1, "1"));
    mmsHeader->encodeStringsMap_.insert(pair<uint8_t, MmsEncodeString>(1, encodeString));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(1, addressValue));
    mmsHeader->DumpMmsHeader();
    EXPECT_TRUE(mmsHeader->DecodeMmsHeader(decodeBuffer));
    EXPECT_FALSE(mmsHeader->EncodeMmsHeader(encodeBuffer));
    EXPECT_TRUE(mmsHeader->GetOctetValue(1, value));
    EXPECT_FALSE(mmsHeader->SetOctetValue(1, 0));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_MESSAGE_TYPE, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_PRIORITY, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_RESPONSE_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_SENDER_VISIBILITY, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_RETRIEVE_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_READ_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_REPLY_CHARGING, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_MESSAGE_TYPE, MMS_MESSAGE_TYPE));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_PRIORITY, MMS_PRIORITY));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_RESPONSE_STATUS, MMS_RESPONSE_STATUS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_SENDER_VISIBILITY, MMS_SENDER_VISIBILITY));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_STATUS, MMS_STATUS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_RETRIEVE_STATUS, MMS_RETRIEVE_STATUS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_READ_STATUS, MMS_READ_STATUS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_REPLY_CHARGING, MMS_REPLY_CHARGING));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_MM_STATE, MMS_MM_STATE));
    EXPECT_TRUE(mmsHeader->GetHeaderAllAddressValue(1, addressValue));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsHeader_0002
 * @tc.name     Test MmsHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsHeader_0002, Function | MediumTest | Level1)
{
    auto mmsHeader = std::make_shared<MmsHeader>();
    int64_t value = 0;
    std::string valueStr = "";
    std::string encodeAddress = SRC_STR;
    MmsAddress address(encodeAddress);
    std::vector<MmsAddress> addressValue;
    addressValue.push_back(address);
    MmsEncodeString encodeString;
    mmsHeader->longValueMap_.insert(pair<uint8_t, int64_t>(1, 1));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(1, "1"));
    mmsHeader->encodeStringsMap_.insert(pair<uint8_t, MmsEncodeString>(1, encodeString));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_STORE_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_CONTENT_CLASS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_MESSAGE_CLASS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_CANCEL_STATUS, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    mmsHeader->mmsHaderCateg_.mmsFieldDesMap_.insert(
        pair<uint8_t, MmsFieldValueType>(MMS_ADAPTATION_ALLOWED, MmsFieldValueType::MMS_FIELD_OCTET_TYPE));
    EXPECT_TRUE(mmsHeader->SetOctetValue(MMS_ADAPTATION_ALLOWED, MMS_BCC));
    EXPECT_TRUE(mmsHeader->GetLongValue(1, value));
    EXPECT_TRUE(mmsHeader->GetTextValue(1, valueStr));
    EXPECT_TRUE(mmsHeader->GetEncodeStringValue(1, encodeString));
    EXPECT_FALSE(mmsHeader->SetTextValue(MMS_CANCEL_STATUS, ""));
    EXPECT_FALSE(mmsHeader->SetTextValue(MMS_CANCEL_STATUS, SRC_STR));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_MM_FLAGS, MMS_MM_FLAGS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_CONTENT_CLASS, MMS_CONTENT_CLASS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_MESSAGE_CLASS, MMS_MESSAGE_CLASS));
    EXPECT_FALSE(mmsHeader->SetOctetValue(MMS_CANCEL_STATUS, MMS_CANCEL_STATUS));
    EXPECT_FALSE(mmsHeader->SetLongValue(MMS_MESSAGE_CLASS, 0));
    EXPECT_FALSE(mmsHeader->SetEncodeStringValue(1, 1, valueStr));
    EXPECT_FALSE(mmsHeader->SetEncodeStringValue(MMS_STORE_STATUS, 1, encodeAddress));
    EXPECT_FALSE(mmsHeader->AddHeaderAddressValue(1, address));
    EXPECT_TRUE(mmsHeader->GetStringValue(1, encodeAddress));
    EXPECT_FALSE(mmsHeader->GetStringValue(FIELD_ID, encodeAddress));
    EXPECT_TRUE(mmsHeader->GetSmilFileName(valueStr));
    EXPECT_FALSE(mmsHeader->IsHaveBody());
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsHeader_0003
 * @tc.name     Test MmsHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsHeader_0003, Function | MediumTest | Level1)
{
    auto mmsHeader = std::make_shared<MmsHeader>();
    MmsDecodeBuffer decodeBuffer;
    MmsEncodeBuffer buff;
    std::string encodeAddress = SRC_STR;
    MmsAddress address(encodeAddress);
    std::vector<MmsAddress> addressValue;
    addressValue.push_back(address);
    int32_t data = 0;
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_BCC, addressValue));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_FROM, addressValue));
    EXPECT_NE(mmsHeader->MakeTransactionId(TRANSACTION_ID_LENGTH), "");
    EXPECT_FALSE(mmsHeader->DecodeMmsMsgType(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeMmsMsgType(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldAddressModelValue(1, decodeBuffer, data));
    EXPECT_TRUE(mmsHeader->DecodeFieldAddressModelValue(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldOctetValue(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldLongValue(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldIntegerValue(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldTextStringValue(1, decodeBuffer, data));
    EXPECT_TRUE(mmsHeader->DecodeFieldTextStringValue(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldEncodedStringValue(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldEncodedStringValue(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFromValue(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeMmsMsgUnKnownField(decodeBuffer));
    EXPECT_FALSE(mmsHeader->DecodeMmsContentType(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldDate(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldDate(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldPreviouslySentDate(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldPreviouslySentDate(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldMBox(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldMBox(MMS_BCC, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldMMFlag(1, decodeBuffer, data));
    EXPECT_FALSE(mmsHeader->DecodeFieldMMFlag(MMS_BCC, decodeBuffer, data));
    EXPECT_TRUE(mmsHeader->EncodeFieldFromValue(buff, addressValue));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_CLASS, "personal"));
    EXPECT_TRUE(mmsHeader->EcondeFieldMessageClassValue(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_CLASS, "advertisement"));
    EXPECT_TRUE(mmsHeader->EcondeFieldMessageClassValue(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_CLASS, "informational"));
    EXPECT_TRUE(mmsHeader->EcondeFieldMessageClassValue(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_CLASS, "auto"));
    EXPECT_TRUE(mmsHeader->EcondeFieldMessageClassValue(buff));
    mmsHeader->octetValueMap_.insert(pair<uint8_t, uint8_t>(MMS_MESSAGE_CLASS, 1));
    EXPECT_TRUE(mmsHeader->EcondeFieldMessageClassValue(buff));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsHeader_0004
 * @tc.name     Test MmsHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsHeader_0004, Function | MediumTest | Level1)
{
    auto mmsHeader = std::make_shared<MmsHeader>();
    MmsEncodeBuffer buff;
    std::string encodeAddress = SRC_STR;
    MmsAddress address(encodeAddress);
    std::vector<MmsAddress> addrs;
    mmsHeader->longValueMap_.insert(pair<uint8_t, int64_t>(MMS_EXPIRY, 1));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_TRANSACTION_ID, "auto"));
    EXPECT_TRUE(mmsHeader->EncodeCommontFieldValue(buff));
    EXPECT_FALSE(mmsHeader->EncodeMultipleAddressValue(buff, 1, addrs));
    addrs.push_back(address);
    EXPECT_TRUE(mmsHeader->EncodeMultipleAddressValue(buff, 1, addrs));
    EXPECT_TRUE(mmsHeader->EncodeMmsSendReq(buff));
    EXPECT_FALSE(mmsHeader->EncodeMmsSendConf(buff));
    EXPECT_FALSE(mmsHeader->EncodeMmsNotificationInd(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_CONTENT_LOCATION, "auto"));
    EXPECT_FALSE(mmsHeader->EncodeMmsNotificationInd(buff));
    EXPECT_FALSE(mmsHeader->EnocdeMmsNotifyRespInd(buff));
    EXPECT_FALSE(mmsHeader->EnocdeMmsRetrieveConf(buff));
    mmsHeader->longValueMap_.insert(pair<uint8_t, int64_t>(MMS_DATE, 1));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_CC, addrs));
    EXPECT_TRUE(mmsHeader->EnocdeMmsRetrieveConf(buff));
    EXPECT_FALSE(mmsHeader->EnocdeMmsDeliveryInd(buff));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_TO, addrs));
    EXPECT_TRUE(mmsHeader->EnocdeMmsRetrieveConf(buff));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_FROM, addrs));
    EXPECT_TRUE(mmsHeader->EnocdeMmsRetrieveConf(buff));
    EXPECT_FALSE(mmsHeader->EnocdeMmsDeliveryInd(buff));
    EXPECT_FALSE(mmsHeader->EncodeMmsReadRecInd(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_ID, "auto"));
    EXPECT_FALSE(mmsHeader->EnocdeMmsDeliveryInd(buff));
    mmsHeader->longValueMap_.clear();
    EXPECT_FALSE(mmsHeader->EnocdeMmsDeliveryInd(buff));
    mmsHeader->addressMap_.clear();
    mmsHeader->textValueMap_.clear();
    EXPECT_FALSE(mmsHeader->EncodeMmsReadRecInd(buff));
    EXPECT_FALSE(mmsHeader->EncodeMmsReadOrigInd(buff));
    mmsHeader->textValueMap_.insert(pair<uint8_t, std::string>(MMS_MESSAGE_ID, "auto"));
    EXPECT_FALSE(mmsHeader->EncodeMmsReadOrigInd(buff));
    mmsHeader->addressMap_.insert(pair<uint8_t, std::vector<MmsAddress>>(MMS_TO, addrs));
    EXPECT_FALSE(mmsHeader->EncodeMmsReadOrigInd(buff));
    mmsHeader->octetValueMap_.insert(pair<uint8_t, uint8_t>(MMS_READ_STATUS, 1));
    EXPECT_FALSE(mmsHeader->EncodeMmsReadOrigInd(buff));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_DRM_CONTENT, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_BCC, 1));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsHeader_0005
 * @tc.name     Test MmsHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsHeader_0005, Function | MediumTest | Level1)
{
    auto mmsHeader = std::make_shared<MmsHeader>();
    MmsEncodeBuffer buff;
    MmsEncodeString value;
    EXPECT_FALSE(mmsHeader->EncodeTextStringValue(buff, 1, ""));
    EXPECT_FALSE(mmsHeader->EncodeEncodeStringValue(buff, 1, value));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_SEND_REQ));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_SEND_CONF));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_NOTIFICATION_IND));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_NOTIFYRESP_IND));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_RETRIEVE_CONF));
    EXPECT_TRUE(mmsHeader->IsHaveTransactionId(MMS_MSGTYPE_ACKNOWLEDGE_IND));
    EXPECT_TRUE(mmsHeader->CheckResponseStatus(static_cast<uint8_t>(MmsResponseStatus::MMS_OK)));
    EXPECT_TRUE(mmsHeader->CheckResponseStatus(static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_UNSUPPORTED_MESSAGE)));
    EXPECT_TRUE(mmsHeader->CheckResponseStatus(static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_PERMANENT_FAILURE)));
    EXPECT_TRUE(
        mmsHeader->CheckResponseStatus(static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_PERMANENT_LACK_OF_PREPAID)));
    EXPECT_TRUE(mmsHeader->CheckRetrieveStatus(static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_OK)));
    EXPECT_TRUE(mmsHeader->CheckRetrieveStatus(
        static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_PERMANENT_FAILURE)));
    EXPECT_TRUE(mmsHeader->CheckRetrieveStatus(
        static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_PERMANENT_CONTENT_UNSUPPORTED)));
    EXPECT_TRUE(mmsHeader->CheckRetrieveStatus(
        static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_TRANSIENT_FAILURE)));
    EXPECT_TRUE(mmsHeader->CheckRetrieveStatus(
        static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_TRANSIENT_NETWORK_PROBLEM)));
    EXPECT_TRUE(mmsHeader->CheckStoreStatus(static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_SUCCESS)));
    EXPECT_TRUE(
        mmsHeader->CheckStoreStatus(static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_TRANSIENT_FAILURE)));
    EXPECT_TRUE(mmsHeader->CheckStoreStatus(
        static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_TRANSIENT_NETWORK_PROBLEM)));
    EXPECT_TRUE(
        mmsHeader->CheckStoreStatus(static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_PERMANENT_FAILURE)));
    EXPECT_TRUE(
        mmsHeader->CheckStoreStatus(static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_PERMANENT_MMBOX_FULL)));
    EXPECT_TRUE(mmsHeader->CheckBooleanValue(MMS_DELIVERY_REPORT, static_cast<uint8_t>(MmsBoolType::MMS_YES)));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_READ_REPORT, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_REPORT_ALLOWED, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_STORE, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_STORED, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_TOTALS, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_QUOTAS, 0));
    EXPECT_FALSE(mmsHeader->CheckBooleanValue(MMS_DISTRIBUTION_INDICATOR, 0));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushBuffer_0001
 * @tc.name     Test SmsWapPushBuffer
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushBuffer_0001, Function | MediumTest | Level1)
{
    auto smsWapPushBuffer = std::make_shared<SmsWapPushBuffer>();
    uint8_t oneByte = 0;
    uint8_t sValueLength = 0;
    uint32_t valueLength = 0;
    uint32_t length = 1;
    std::string str = "qwe";
    EXPECT_FALSE(smsWapPushBuffer->GetOneByte(oneByte));
    EXPECT_FALSE(smsWapPushBuffer->IncreasePointer(1));
    EXPECT_FALSE(smsWapPushBuffer->DecreasePointer(1));
    EXPECT_FALSE(smsWapPushBuffer->DecodeShortLength(sValueLength));
    EXPECT_FALSE(smsWapPushBuffer->DecodeValueLengthReturnLen(valueLength, length));
    EXPECT_FALSE(smsWapPushBuffer->DecodeUintvar(valueLength, length));
    EXPECT_FALSE(smsWapPushBuffer->DecodeValueLength(valueLength));
    EXPECT_FALSE(smsWapPushBuffer->DecodeText(str, length));
    EXPECT_FALSE(smsWapPushBuffer->DecodeShortInteger(oneByte));
    smsWapPushBuffer->totolLength_ = 1;
    smsWapPushBuffer->curPosition_ = 1;
    EXPECT_FALSE(smsWapPushBuffer->IncreasePointer(1));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_ = std::make_unique<char[]>(1);
    smsWapPushBuffer->pduBuffer_[0] = 0;
    EXPECT_TRUE(smsWapPushBuffer->DecodeShortLength(sValueLength));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = 0;
    EXPECT_TRUE(smsWapPushBuffer->DecodeValueLengthReturnLen(valueLength, length));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_TRUE(smsWapPushBuffer->DecodeIsString());
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_ONE;
    EXPECT_FALSE(smsWapPushBuffer->DecodeUintvar(valueLength, length));
    smsWapPushBuffer->curPosition_ = 0;
    EXPECT_TRUE(smsWapPushBuffer->DecodeShortInteger(oneByte));
    smsWapPushBuffer->curPosition_ = 0;
    EXPECT_FALSE(smsWapPushBuffer->DecodeValueLength(valueLength));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_THREE;
    EXPECT_TRUE(smsWapPushBuffer->DecodeIsValueLength());
    EXPECT_FALSE(smsWapPushBuffer->DecodeValueLength(valueLength));
    smsWapPushBuffer->curPosition_ = 0;
    EXPECT_FALSE(smsWapPushBuffer->DecodeTokenText(str, length));
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_TRUE(smsWapPushBuffer->DecodeText(str, length));
    EXPECT_FALSE(smsWapPushBuffer->CharIsToken(MIN_TOKEN));
    EXPECT_FALSE(smsWapPushBuffer->CharIsToken(MAX_TOKEN));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH;
    EXPECT_TRUE(smsWapPushBuffer->DecodeTokenText(str, length));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushBuffer_0002
 * @tc.name     Test SmsWapPushBuffer
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushBuffer_0002, Function | MediumTest | Level1)
{
    auto smsWapPushBuffer = std::make_shared<SmsWapPushBuffer>();
    uint64_t iInteger = 0;
    std::string str = "qwe";
    bool isNoValue = true;
    uint32_t len = 1;
    smsWapPushBuffer->pduBuffer_ = std::make_unique<char[]>(1);
    EXPECT_FALSE(smsWapPushBuffer->DecodeLongInteger(iInteger));
    EXPECT_FALSE(smsWapPushBuffer->DecodeTextValue(str, isNoValue));
    EXPECT_FALSE(smsWapPushBuffer->DecodeNoValue(isNoValue));
    EXPECT_TRUE(smsWapPushBuffer->ReadDataBuffer(1, 1) == nullptr);
    smsWapPushBuffer->totolLength_ = 1;
    smsWapPushBuffer->pduBuffer_[0] = 1;
    EXPECT_FALSE(smsWapPushBuffer->DecodeLongInteger(iInteger));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_THREE;
    EXPECT_FALSE(smsWapPushBuffer->DecodeLongInteger(iInteger));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = 1;
    EXPECT_FALSE(smsWapPushBuffer->DecodeTextValue(str, isNoValue));
    EXPECT_TRUE(smsWapPushBuffer->ReadDataBuffer(1, 1) == nullptr);
    EXPECT_FALSE(smsWapPushBuffer->WriteRawStringBuffer(str));
    EXPECT_FALSE(smsWapPushBuffer->WriteDataBuffer(nullptr, 1));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = 0;
    EXPECT_FALSE(smsWapPushBuffer->DecodeQuotedText(str, len));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_TRUE(smsWapPushBuffer->DecodeQuotedText(str, len));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_ONE;
    EXPECT_TRUE(smsWapPushBuffer->DecodeInteger(iInteger));
    smsWapPushBuffer->curPosition_ = 0;
    smsWapPushBuffer->pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_FALSE(smsWapPushBuffer->DecodeInteger(iInteger));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushHandler_0001
 * @tc.name     Test SmsWapPushHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushHandler_0001, Function | MediumTest | Level1)
{
    auto smsWapPushHandler = std::make_shared<SmsWapPushHandler>(INVALID_SLOTID);
    SmsWapPushBuffer decodeBuffer;
    std::string wapPdu = "";
    EXPECT_FALSE(smsWapPushHandler->DecodeWapPushPduData(decodeBuffer, 1, 1));
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    EXPECT_FALSE(smsWapPushHandler->DecodeWapPushPdu(indexer, wapPdu));
    EXPECT_FALSE(smsWapPushHandler->DeocdeCheckIsBlock(wapPdu));
    wapPdu = "qwer";
    EXPECT_FALSE(smsWapPushHandler->DecodeWapPushPdu(indexer, wapPdu));
    EXPECT_FALSE(smsWapPushHandler->DecodePushType(decodeBuffer));
    EXPECT_FALSE(smsWapPushHandler->DeocdeCheckIsBlock(wapPdu));
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplication(decodeBuffer, 1));
    smsWapPushHandler->DeleteWapPush(indexer);
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
    auto dataBuf = std::make_unique<char[]>(BUF_LEN);
    decodeBuffer.WriteDataBuffer(std::move(dataBuf), BUF_LEN);
    decodeBuffer.totolLength_ = 1;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplication(decodeBuffer, 1));
    decodeBuffer.totolLength_ = 0;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplication(decodeBuffer, 0));
    EXPECT_TRUE(smsWapPushHandler->SendWapPushMessageBroadcast(indexer));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushHandler_0002
 * @tc.name     Test SmsWapPushHandler
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushHandler_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::SmsWapPushHandler_0002 -->");
    auto smsWapPushHandler = std::make_shared<SmsWapPushHandler>(INVALID_SLOTID);
    SmsWapPushBuffer decodeBuffer;
    auto dataBuf = std::make_unique<char[]>(BUF_LEN);
    decodeBuffer.WriteDataBuffer(std::move(dataBuf), BUF_LEN);
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplication(decodeBuffer, 1));

    std::string appId = TEST_PATH;
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_9;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplicationField(decodeBuffer, appId));
    appId = TEST_PATH;
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_AF;
    EXPECT_TRUE(smsWapPushHandler->DecodeXWapApplicationField(decodeBuffer, appId));

    appId = TEST_PATH;
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_AF;
    EXPECT_TRUE(smsWapPushHandler->DecodeXWapApplicationValue(decodeBuffer, appId));
    appId = TEST_PATH;
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_9;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapApplicationValue(decodeBuffer, appId));

    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_4;
    EXPECT_TRUE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_16;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_31;
    EXPECT_TRUE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
    decodeBuffer.curPosition_ = VALUE_9;
    decodeBuffer.pduBuffer_[VALUE_9] = VALUE_31;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = VALUE_32;
    EXPECT_FALSE(smsWapPushHandler->DecodeXWapAbandonHeaderValue(decodeBuffer));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushContentType_0001
 * @tc.name     Test SmsWapPushContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushContentType_0001, Function | MediumTest | Level1)
{
    auto smsWapPushContentType = std::make_shared<SmsWapPushContentType>();
    SmsWapPushBuffer decodeBuffer;
    int32_t contentLength = 1;
    decodeBuffer.totolLength_ = 1;
    decodeBuffer.pduBuffer_ = std::make_unique<char[]>(1);
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_ONE;
    EXPECT_FALSE(smsWapPushContentType->DecodeCTGeneralForm(decodeBuffer, contentLength));
    EXPECT_TRUE(smsWapPushContentType->DecodeContentType(decodeBuffer, contentLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_TRUE(smsWapPushContentType->DecodeContentType(decodeBuffer, contentLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = 0;
    EXPECT_FALSE(smsWapPushContentType->DecodeContentType(decodeBuffer, contentLength));
    EXPECT_FALSE(smsWapPushContentType->DecodeCTGeneralForm(decodeBuffer, contentLength));
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_THREE;
    decodeBuffer.curPosition_ = 1;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    EXPECT_FALSE(smsWapPushContentType->DecodeCTGeneralForm(decodeBuffer, contentLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_THREE;
    EXPECT_FALSE(smsWapPushContentType->DecodeCTGeneralForm(decodeBuffer, contentLength));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushContentType_0002
 * @tc.name     Test SmsWapPushContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushContentType_0002, Function | MediumTest | Level1)
{
    auto smsWapPushContentType = std::make_shared<SmsWapPushContentType>();
    SmsWapPushBuffer decodeBuffer;
    decodeBuffer.totolLength_ = 1;
    decodeBuffer.pduBuffer_ = std::make_unique<char[]>(1);
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_FOUR;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_FIVE;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_SIX;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_SEVEN;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_EIGHT;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_NINE;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_TEN;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_ELEVEN;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_TWELVE;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SmsWapPushContentType_0003
 * @tc.name     Test SmsWapPushContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsWapPushContentType_0003, Function | MediumTest | Level1)
{
    auto smsWapPushContentType = std::make_shared<SmsWapPushContentType>();
    SmsWapPushBuffer decodeBuffer;
    int32_t valueLength = 1;
    string strCharSet = "US-ASCII";
    string result = "*/*";
    uint32_t charSet = 1;
    decodeBuffer.totolLength_ = 1;
    decodeBuffer.pduBuffer_ = std::make_unique<char[]>(1);
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_THIRTEEN;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_FOURTEEN;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, 1));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = 1;
    EXPECT_FALSE(smsWapPushContentType->DecodeParameter(decodeBuffer, VALUE_LENGTH));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH;
    EXPECT_FALSE(smsWapPushContentType->DecodeCharsetField(decodeBuffer, valueLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = 0;
    EXPECT_FALSE(smsWapPushContentType->DecodeCharsetField(decodeBuffer, valueLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_FALSE(smsWapPushContentType->DecodeCharsetField(decodeBuffer, valueLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_TWO;
    EXPECT_TRUE(smsWapPushContentType->DecodeTypeField(decodeBuffer, valueLength));
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.pduBuffer_[0] = PDU_BUFFE_LENGTH_ONE;
    EXPECT_TRUE(smsWapPushContentType->DecodeTypeField(decodeBuffer, valueLength));
    EXPECT_TRUE(smsWapPushContentType->GetCharSetIntFromString(charSet, strCharSet));
    EXPECT_EQ(smsWapPushContentType->GetContentTypeFromInt(0), result);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsBodyPart_0001
 * @tc.name     Test MmsBodyPart
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsBodyPart_0001, TestSize.Level1)
{
    MmsBodyPart mmsBodyPart;
    MmsBodyPart testBodyPart;
    mmsBodyPart = testBodyPart;
    mmsBodyPart.AssignBodyPart(testBodyPart);
    MmsDecodeBuffer decoderBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    ASSERT_FALSE(mmsBodyPart.DecodePart(decoderBuffer, 3));
    decoderBuffer.pduBuffer_[0] = 0;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsBodyPart.DecodePart(decoderBuffer, 3));
    decoderBuffer.pduBuffer_[0] = 0;
    decoderBuffer.pduBuffer_[1] = 0;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 2;
    ASSERT_FALSE(mmsBodyPart.DecodePart(decoderBuffer, 3));
    decoderBuffer.pduBuffer_[0] = 0;
    decoderBuffer.pduBuffer_[1] = 0;
    decoderBuffer.pduBuffer_[2] = 0x70;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 3;
    ASSERT_FALSE(mmsBodyPart.DecodePart(decoderBuffer, 3));
    decoderBuffer.pduBuffer_[0] = 0x7F;
    decoderBuffer.pduBuffer_[1] = 0;
    decoderBuffer.pduBuffer_[2] = 0x70;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 10;
    ASSERT_FALSE(mmsBodyPart.DecodePart(decoderBuffer, 3));
    ASSERT_TRUE(mmsBodyPart.DecodePartHeader(decoderBuffer, 0));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    int32_t testLen = 3;
    ASSERT_FALSE(mmsBodyPart.DecodePartHeader(decoderBuffer, testLen));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    decoderBuffer.pduBuffer_[0] = 0x80;
    ASSERT_FALSE(mmsBodyPart.DecodePartHeader(decoderBuffer, testLen));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    decoderBuffer.pduBuffer_[0] = 0x20;
    ASSERT_FALSE(mmsBodyPart.DecodePartHeader(decoderBuffer, testLen));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    decoderBuffer.pduBuffer_[0] = 0x5;
    mmsBodyPart.DumpMmsBodyPart();
    ASSERT_FALSE(mmsBodyPart.DecodePartHeader(decoderBuffer, testLen));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsBodyPart_0002
 * @tc.name     Test MmsBodyPart
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsBodyPart_0002, TestSize.Level0)
{
    MmsBodyPart mmsBodyPart;

    MmsDecodeBuffer decoderBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    ASSERT_FALSE(mmsBodyPart.DecodePartBody(decoderBuffer, 3));
    decoderBuffer.curPosition_ = 0;
    int32_t offset = 2;
    decoderBuffer.totolLength_ = MAX_MMS_MSG_PART_LEN + offset;
    offset = 1;
    ASSERT_FALSE(mmsBodyPart.DecodePartBody(decoderBuffer, MAX_MMS_MSG_PART_LEN + offset));
    MmsAttachment mmsAttachment;
    ASSERT_FALSE(mmsBodyPart.SetAttachment(mmsAttachment));
    MmsEncodeBuffer encodeBuffer;
    ASSERT_FALSE(mmsBodyPart.EncodeMmsBodyPart(encodeBuffer));
    mmsBodyPart.DecodeSetFileName(3);
    ASSERT_FALSE(mmsBodyPart.WriteBodyFromFile(""));
    ASSERT_FALSE(mmsBodyPart.WriteBodyFromAttachmentBuffer(mmsAttachment));
    mmsAttachment.strFileName_ = "test";
    ASSERT_FALSE(mmsBodyPart.WriteBodyFromAttachmentBuffer(mmsAttachment));
    MmsBodyPart bodyPart;
    mmsBodyPart.bodyLen_ = 1;
    bodyPart = mmsBodyPart;
    uint32_t len = 0;
    ASSERT_TRUE(bodyPart.ReadBodyPartBuffer(len) == nullptr);
    mmsBodyPart.bodyLen_ = MAX_MMS_MSG_PART_LEN + 1;
    bodyPart = mmsBodyPart;
    len = 0;
    ASSERT_TRUE(bodyPart.ReadBodyPartBuffer(len) == nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsContentType_0001
 * @tc.name     Test MmsContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsContentType_0001, TestSize.Level0)
{
    MmsContentType mmsContentType;
    MmsContentType mmsTContentType;
    mmsTContentType.contentType_ = "lll";
    mmsContentType = mmsTContentType;
    MmsDecodeBuffer decoderBuffer;
    int32_t testPduBuffer = 0x8f;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    int32_t length;
    ASSERT_TRUE(mmsContentType.DecodeMmsContentType(decoderBuffer, length));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    testPduBuffer = 0x2f;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    ASSERT_FALSE(mmsContentType.DecodeMmsCTGeneralForm(decoderBuffer, length));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    ASSERT_FALSE(mmsContentType.DecodeMmsCTGeneralForm(decoderBuffer, length));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    testPduBuffer = 0x8f;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    ASSERT_FALSE(mmsContentType.DecodeMmsCTGeneralForm(decoderBuffer, length));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    decoderBuffer.pduBuffer_[0] = 0;
    ASSERT_FALSE(mmsContentType.DecodeMmsCTGeneralForm(decoderBuffer, length));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsContentType_0002
 * @tc.name     Test MmsContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsContentType_0002, TestSize.Level0)
{
    MmsContentType mmsContentType;
    MmsDecodeBuffer decoderBuffer;
    ASSERT_TRUE(mmsContentType.DecodeParameter(decoderBuffer, 0));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    ASSERT_FALSE(mmsContentType.DecodeParameter(decoderBuffer, 1));
    int32_t testPduBuffer = 129;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeParameter(decoderBuffer, 1));
    testPduBuffer = 154;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeParameter(decoderBuffer, 1));
    testPduBuffer = 137;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeParameter(decoderBuffer, 1));
    decoderBuffer.pduBuffer_[0] = 0;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeParameter(decoderBuffer, 4));
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    int32_t len;
    ASSERT_FALSE(mmsContentType.DecodeTextField(decoderBuffer, 0, len));
    ASSERT_FALSE(mmsContentType.DecodeCharsetField(decoderBuffer, len));
    testPduBuffer = 100;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeCharsetField(decoderBuffer, len));
    testPduBuffer = 30;
    decoderBuffer.pduBuffer_[0] = testPduBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_FALSE(mmsContentType.DecodeCharsetField(decoderBuffer, len));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsContentType_0003
 * @tc.name     Test MmsContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsContentType_0003, TestSize.Level0)
{
    MmsEncodeBuffer mmsEncodeBuffer;
    MmsContentType mmsContentType;
    MmsDecodeBuffer decoderBuffer;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 0;
    int32_t valueLen = 0;
    ASSERT_FALSE(mmsContentType.DecodeTypeField(decoderBuffer, valueLen));
    decoderBuffer.pduBuffer_[0] = 0x8f;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_TRUE(mmsContentType.DecodeTypeField(decoderBuffer, valueLen));
    decoderBuffer.pduBuffer_[0] = 0;
    decoderBuffer.curPosition_ = 0;
    decoderBuffer.totolLength_ = 1;
    ASSERT_TRUE(mmsContentType.DecodeTypeField(decoderBuffer, valueLen));
    mmsContentType.msgContentParm_.textMap_[152] = "";
    int32_t offset = 1;
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE + offset;
    mmsContentType.EncodeTextField(mmsEncodeBuffer);
    mmsContentType.msgContentParm_.textMap_[152] = "";
    mmsEncodeBuffer.curPosition_ = 0;
    ASSERT_TRUE(mmsContentType.EncodeTextField(mmsEncodeBuffer));
    ASSERT_TRUE(mmsContentType.EncodeCharsetField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.charset_ = offset;
    ASSERT_TRUE(mmsContentType.EncodeCharsetField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.charset_ = offset;
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE + offset;
    mmsContentType.EncodeCharsetField(mmsEncodeBuffer);
    mmsContentType.msgContentParm_.charset_ = offset;
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE - offset;
    mmsContentType.EncodeCharsetField(mmsEncodeBuffer);
    mmsContentType.msgContentParm_.charset_ = offset;
    mmsEncodeBuffer.curPosition_ = 0;
    ASSERT_TRUE(mmsContentType.EncodeCharsetField(mmsEncodeBuffer));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsContentType_0004
 * @tc.name     Test MmsContentType
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsContentType_0004, TestSize.Level0)
{
    MmsContentType mmsContentType;
    MmsEncodeBuffer mmsEncodeBuffer;
    ASSERT_TRUE(mmsContentType.EncodeTypeField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.type_ = "1";
    int32_t offset = 1;
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE + offset;
    ASSERT_FALSE(mmsContentType.EncodeTypeField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.type_ = "1";
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE - offset;
    ASSERT_FALSE(mmsContentType.EncodeTypeField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.type_ = "1";
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE - offset;
    ASSERT_FALSE(mmsContentType.EncodeTypeField(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.textMap_[152] = "";
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE + offset;
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentParam(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.textMap_[152] = "";
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE - offset;
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentParam(mmsEncodeBuffer));
    mmsContentType.msgContentParm_.textMap_[152] = "";
    offset = 2;
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE - offset;
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentParam(mmsEncodeBuffer));
    mmsContentType.contentType_ = "";
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentType(mmsEncodeBuffer));
    mmsContentType.contentType_ = "*/*";
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentType(mmsEncodeBuffer));
    mmsEncodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE + offset;
    ASSERT_FALSE(mmsContentType.EncodeMmsBodyPartContentType(mmsEncodeBuffer));
}

/**

 * @tc.number   Telephony_SmsMmsGtest_MmsBody_0001
 * @tc.name     Test MmsBody
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsMmsGtest_MmsBody_0001, Function | MediumTest | Level1)
{
    MmsBody mmsBody;
    MmsDecodeBuffer mmsDecodeBuffer;
    MmsHeader mmsHeader;
    MmsEncodeBuffer mmsEncodeBuffer;
    MmsBodyPart mmsBodyPart;
    mmsDecodeBuffer.curPosition_ = 0;
    mmsDecodeBuffer.totolLength_ = PDU_COUNT;
    MmsBodyPart part;
    mmsBody.mmsBodyParts_.push_back(part);
    EXPECT_TRUE(mmsBody.DecodeMultipart(mmsDecodeBuffer));
    EXPECT_FALSE(mmsBody.DecodeMmsBody(mmsDecodeBuffer, mmsHeader));
    EXPECT_FALSE(mmsBody.EncodeMmsBody(mmsEncodeBuffer));
    EXPECT_FALSE(mmsBody.EncodeMmsHeaderContentType(mmsHeader, mmsEncodeBuffer));
    EXPECT_TRUE(mmsBody.IsContentLocationPartExist(""));
    EXPECT_TRUE(mmsBody.IsContentIdPartExist(""));
    mmsBody.DumpMmsBody();
    mmsBody.GetBodyPartCount();
    std::vector<MmsBodyPart> parts;
    mmsBody.GetMmsBodyPart(parts);

    mmsDecodeBuffer.curPosition_ = PDU_COUNT;
    mmsDecodeBuffer.totolLength_ = 0;
    EXPECT_FALSE(mmsBody.DecodeMultipart(mmsDecodeBuffer));
    EXPECT_FALSE(mmsBody.DecodeMmsBody(mmsDecodeBuffer, mmsHeader));
    EXPECT_FALSE(mmsBody.EncodeMmsBody(mmsEncodeBuffer));
    EXPECT_FALSE(mmsBody.EncodeMmsHeaderContentType(mmsHeader, mmsEncodeBuffer));
    EXPECT_TRUE(mmsBody.IsContentLocationPartExist(""));
    EXPECT_TRUE(mmsBody.IsContentIdPartExist(""));

    EXPECT_TRUE(mmsBody.IsBodyPartExist(mmsBodyPart));
    EXPECT_TRUE(mmsBody.AddMmsBodyPart(mmsBodyPart));
    mmsBodyPart.isSmilFile_ = true;
    EXPECT_TRUE(mmsBody.AddMmsBodyPart(mmsBodyPart));
    mmsBodyPart.isSmilFile_ = false;
    mmsBody.bHaveSmilPart_ = true;
    EXPECT_TRUE(mmsBody.AddMmsBodyPart(mmsBodyPart));
    mmsBodyPart.isSmilFile_ = true;
    mmsBodyPart.SetSmilFile(false);
    mmsBodyPart.IsSmilFile();
    EXPECT_TRUE(mmsBody.AddMmsBodyPart(mmsBodyPart));
}

/**

 * @tc.number   Telephony_SmsMmsGtest_MmsBody_0002
 * @tc.name     Test MmsBody
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsMmsGtest_MmsBody_0002, Function | MediumTest | Level1)
{
    MmsBody mmsBody;
    MmsDecodeBuffer mmsDecodeBuffer;
    MmsHeader mmsHeader;
    MmsEncodeBuffer mmsEncodeBuffer;
    MmsBodyPart mmsBodyPart;
    mmsDecodeBuffer.curPosition_ = 0;
    mmsDecodeBuffer.totolLength_ = PDU_COUNT;
    MmsBodyPart part;
    std::string strCt = INPUT_STR;
    std::string getCt;
    part.SetContentType(strCt);
    part.GetContentType(getCt);
    EXPECT_STREQ(strCt.c_str(), getCt.c_str());
    part.SetContentId(strCt);
    part.GetContentId(getCt);
    EXPECT_STREQ(strCt.c_str(), getCt.c_str());
    part.SetContentLocation(strCt);
    part.GetContentLocation(getCt);
    EXPECT_STREQ(strCt.c_str(), getCt.c_str());
    part.SetContentDisposition(strCt);
    part.GetContentDisposition(getCt);
    EXPECT_STREQ(strCt.c_str(), getCt.c_str());
    part.SetFileName(strCt);
    EXPECT_STREQ(strCt.c_str(), part.GetPartFileName().c_str());
    part.GetContentType();
    part.GetPartHeader();
    EXPECT_STREQ(strCt.c_str(), getCt.c_str());
    mmsBody.mmsBodyParts_.push_back(part);
    EXPECT_TRUE(mmsBody.AddMmsBodyPart(mmsBodyPart));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsMsg_0001
 * @tc.name     Test MmsMsg
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsMmsGtest_MmsMsg_0001, Function | MediumTest | Level1)
{
    MmsMsg mmsMsg;
    uint32_t outLen;
    std::vector<MmsAddress> toAddrs = {};
    MmsAttachment mmsAttachment;
    std::vector<MmsAttachment> attachments = {};
    EXPECT_FALSE(mmsMsg.DecodeMsg(""));
    EXPECT_FALSE(mmsMsg.DecodeMsg(nullptr, 0));
    mmsMsg.EncodeMsg(outLen);
    MmsAddress address(INPUT_STR);
    mmsMsg.SetMmsFrom(address);
    mmsMsg.GetMmsFrom();
    EXPECT_FALSE(mmsMsg.SetMmsTo(toAddrs));
    std::string value = INPUT_STR;
    mmsMsg.SetHeaderStringValue(0, value);
    mmsMsg.GetHeaderStringValue(0);
    mmsMsg.GetHeaderContentTypeStart();
    EXPECT_FALSE(mmsMsg.AddAttachment(mmsAttachment));
    EXPECT_FALSE(mmsMsg.GetAllAttachment(attachments));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsMsg_0002
 * @tc.name     Test MmsMsg
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SmsMmsGtest_MmsMsg_0002, Function | MediumTest | Level1)
{
    MmsMsg mmsMsg;
    std::vector<MmsAddress> toAddrs = {};
    MmsAttachment mmsAttachment;
    std::vector<MmsAttachment> attachments = {};
    std::unique_ptr<char[]> inBuff = std::make_unique<char[]>(2);
    inBuff[0] = 1;
    inBuff[1] = 1;
    EXPECT_FALSE(mmsMsg.DecodeMsg(std::move(inBuff), 2));

    mmsMsg.SetMmsVersion(1);
    mmsMsg.GetMmsVersion();
    mmsMsg.SetMmsMessageType(1);
    mmsMsg.GetMmsMessageType();
    mmsMsg.SetMmsTransactionId("1");
    mmsMsg.GetMmsTransactionId();
    mmsMsg.SetMmsDate(1);
    mmsMsg.GetMmsDate();
    EXPECT_FALSE(mmsMsg.SetMmsTo(toAddrs));
    mmsMsg.GetMmsTo(toAddrs);
    mmsMsg.SetHeaderOctetValue(0, 0);
    mmsMsg.GetHeaderOctetValue(0);
    mmsMsg.SetHeaderIntegerValue(0, 0);
    mmsMsg.GetHeaderIntegerValue(0);
    mmsMsg.SetHeaderLongValue(0, 0);
    mmsMsg.GetHeaderLongValue(0);

    mmsMsg.SetHeaderEncodedStringValue(0, INPUT_STR, 0);
    MmsAddress address(INPUT_STR);
    mmsMsg.AddHeaderAddressValue(0, address);
    std::vector<MmsAddress> addressValue;
    mmsMsg.GetHeaderAllAddressValue(0, addressValue);
    mmsMsg.SetHeaderContentType(INPUT_STR);
    mmsMsg.GetHeaderContentType();
    mmsMsg.GetHeaderContentTypeStart();
    EXPECT_FALSE(mmsMsg.AddAttachment(mmsAttachment));
    EXPECT_FALSE(mmsMsg.GetAllAttachment(attachments));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsBodyPartHeader_0001
 * @tc.name     Test MmsBodyPartHeader
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsBodyPartHeader_0001, Function | MediumTest | Level1)
{
    auto mmsBodyPartHeader = std::make_shared<MmsBodyPartHeader>();
    MmsDecodeBuffer decodeBuffer;
    uint32_t len = 1;
    decodeBuffer.curPosition_ = 0;
    decodeBuffer.totolLength_ = 0;
    EXPECT_FALSE(mmsBodyPartHeader->DecodeContentDisposition(decodeBuffer, len));
    EXPECT_FALSE(mmsBodyPartHeader->DecodeContentLocation(decodeBuffer, len));
    EXPECT_FALSE(mmsBodyPartHeader->DecodeContentId(decodeBuffer, len));

    uint32_t dispLen = 1;
    uint32_t beginPos = 1;
    EXPECT_TRUE(mmsBodyPartHeader->DecodeDispositionParameter(decodeBuffer, dispLen, beginPos));
    EXPECT_FALSE(mmsBodyPartHeader->DecodeWellKnownHeader(decodeBuffer, len));
    EXPECT_FALSE(mmsBodyPartHeader->DecodeApplicationHeader(decodeBuffer, len));

    std::string str = TEST_STR;
    mmsBodyPartHeader->SetContentId(str);
    EXPECT_TRUE(mmsBodyPartHeader->GetContentId(str));
    mmsBodyPartHeader->SetContentTransferEncoding(str);
    EXPECT_TRUE(mmsBodyPartHeader->GetContentTransferEncoding(str));
    mmsBodyPartHeader->SetContentLocation(str);
    EXPECT_TRUE(mmsBodyPartHeader->GetContentDisposition(str));

    MmsEncodeBuffer encodeBuffer;
    mmsBodyPartHeader->strContentTransferEncoding_ = "";
    EXPECT_TRUE(mmsBodyPartHeader->EncodeContentTransferEncoding(encodeBuffer));
    EXPECT_TRUE(mmsBodyPartHeader->EncodeContentLocation(encodeBuffer));

    mmsBodyPartHeader->strContentTransferEncoding_ = str;
    EXPECT_TRUE(mmsBodyPartHeader->EncodeContentLocation(encodeBuffer));
    EXPECT_TRUE(mmsBodyPartHeader->EncodeContentId(encodeBuffer));
    EXPECT_TRUE(mmsBodyPartHeader->EncodeContentDisposition(encodeBuffer));

    encodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE;
    mmsBodyPartHeader->strContentLocation_ = SRC_STR;
    EXPECT_FALSE(mmsBodyPartHeader->EncodeMmsBodyPartHeader(encodeBuffer));
    mmsBodyPartHeader->strContentID_ = SRC_STR;
    encodeBuffer.curPosition_ = CODE_BUFFER_MAX_SIZE;
    auto otherBodyPartHeader = std::make_shared<MmsBodyPartHeader>();
    *otherBodyPartHeader = *mmsBodyPartHeader;
    otherBodyPartHeader->DumpBodyPartHeader();
    EXPECT_FALSE(mmsBodyPartHeader->EncodeContentLocation(encodeBuffer));
    EXPECT_FALSE(mmsBodyPartHeader->EncodeContentId(encodeBuffer));
    EXPECT_FALSE(mmsBodyPartHeader->EncodeMmsBodyPartHeader(encodeBuffer));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsBuffer_0002
 * @tc.name     Test MmsBuffer
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsBuffer_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsBuffer_0002 -->");
    MmsBuffer mmsBuffer;
    uint32_t len = 1;
    bool retBool;
    mmsBuffer.ReadDataBuffer(len);
    mmsBuffer.ReadDataBuffer(len, MAX_MMS_MSG_PART_LEN + 1);
    mmsBuffer.ReadDataBuffer(MAX_MMS_MSG_PART_LEN, len);
    retBool = mmsBuffer.WriteDataBuffer(nullptr, 0);
    retBool = mmsBuffer.WriteDataBuffer(std::make_unique<char[]>(len), MAX_MMS_MSG_PART_LEN + 1);
    retBool = mmsBuffer.WriteDataBuffer(std::make_unique<char[]>(len), 0);
    EXPECT_FALSE(retBool);
    retBool = mmsBuffer.WriteDataBuffer(std::make_unique<char[]>(len), len);
    EXPECT_TRUE(retBool);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsConnCallback_0001
 * @tc.name     Test MmsConnCallback
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsConnCallback_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsConnCallback_0001 -->");
    MmsConnCallbackStub connCallback;
    sptr<NetManagerStandard::NetHandle> nullHandle;
    sptr<NetManagerStandard::NetAllCapabilities> nullCap;
    sptr<NetManagerStandard::NetLinkInfo> nullInfo;
    connCallback.SyncNetworkResponse(true);
    EXPECT_EQ(connCallback.NetAvailable(nullHandle), TELEPHONY_ERR_FAIL);
    EXPECT_EQ(connCallback.NetCapabilitiesChange(nullHandle, nullCap), ERR_NONE);
    EXPECT_EQ(connCallback.NetConnectionPropertiesChange(nullHandle, nullInfo), ERR_NONE);
    EXPECT_EQ(connCallback.NetLost(nullHandle), ERR_NONE);
    EXPECT_EQ(connCallback.NetUnavailable(), ERR_NONE);
    EXPECT_EQ(connCallback.NetBlockStatusChange(nullHandle, false), ERR_NONE);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsNetworkManager_0001
 * @tc.name     Test MmsNetworkManager
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsNetworkManager_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsNetworkManager_0001 -->");
    MmsNetworkManager mmsNetworkMgr;
    EXPECT_GE(mmsNetworkMgr.AcquireNetwork(0, 0), 0);
    mmsNetworkMgr.ReleaseNetwork(0, 0, 0);
    EXPECT_NE(mmsNetworkMgr.GetOrCreateHttpClient(0), nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsNetworkClient_0001
 * @tc.name     Test MmsNetworkClient
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsNetworkClient_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsNetworkClient_0001 -->");
    AccessMmsToken token;
    MmsNetworkClient client(0);
    client.GetIfaceName();
    std::string storeDirName;
    client.Execute(METHOD_POST, "", storeDirName, "ua", "uaprof");
    client.Execute(METHOD_GET, "", storeDirName, "ua", "uaprof");
    client.HttpRequest(METHOD_POST, "", TEST_DATA, "ua", "uaprof");
    client.DeleteMmsPdu("");

    MmsNetworkClient clientSlot1(1);
    clientSlot1.Execute(METHOD_POST, "", storeDirName, "ua", "uaprof");
    clientSlot1.HttpRequest(METHOD_POST, "", storeDirName, "ua", "uaprof");
    client.responseData_ = "";

    std::string strBuf = TEST_DATA;
    EXPECT_EQ(client.GetMmsPduFromFile(storeDirName, strBuf), false);
    EXPECT_EQ(client.GetMmsPduFromFile(TEST_PATH, strBuf), false);

    std::unique_ptr<char[]> bufp = std::make_unique<char[]>(BUF_LEN);
    EXPECT_EQ(client.WriteBufferToFile(nullptr, BUF_LEN, storeDirName), false);
    EXPECT_EQ(client.WriteBufferToFile(bufp, BUF_LEN, storeDirName), false);
    EXPECT_EQ(client.WriteBufferToFile(bufp, BUF_LEN, storeDirName), false);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsPersistHelper_0001
 * @tc.name     Test MmsPersistHelper
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsPersistHelper_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::PersistHelper_0001 -->");
    MmsPersistHelper persistHelper;

    std::string urlStr = TEST_URLS;
    std::vector<std::string> urls = persistHelper.SplitUrl(urlStr);
    EXPECT_EQ(urls.size(), URL_COUNT);

    std::string *pduData = new (std::nothrow) std::string(SPLIT_PDU_COUNT * SPLIT_PDU_LENGTH, 'a');
    std::vector<std::string> pdus = persistHelper.SplitPdu(*pduData);
    EXPECT_EQ(pdus.size(), SPLIT_PDU_COUNT * 2);
    delete pduData;

    persistHelper.SetMmsPdu(TEST_DATA);
    persistHelper.GetMmsPdu(TEST_PATH);
    persistHelper.DeleteMmsPdu(TEST_PATH);
    std::string dbUrl;
    EXPECT_FALSE(persistHelper.InsertMmsPdu(TEST_DATA, dbUrl));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsPersistHelper_0002
 * @tc.name     Test MmsPersistHelper
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsPersistHelper_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::PersistHelper_0002 -->");
    AccessMmsToken token;
    MmsPersistHelper persistHelper;

    persistHelper.SetMmsPdu(TEST_DATA);
    persistHelper.GetMmsPdu(TEST_PATH);
    persistHelper.DeleteMmsPdu(TEST_PATH);
    std::string dbUrl;
    EXPECT_FALSE(persistHelper.InsertMmsPdu(TEST_DATA, dbUrl));
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsPersistHelper_0003
 * @tc.name     Test MmsPersistHelper
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsPersistHelper_0003, Function | MediumTest | Level1)
{
    AccessMmsToken token;
    auto mmsPersistHelper = DelayedSingleton<MmsPersistHelper>::GetInstance();
    std::string dbUrl = "1,";
    mmsPersistHelper->DeleteMmsPdu(dbUrl);
    std::string mmsPdu = "text";
    mmsPersistHelper->InsertMmsPdu(mmsPdu, dbUrl);
    mmsPersistHelper->GetMmsPdu(dbUrl);
    EXPECT_TRUE(mmsPersistHelper != nullptr);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsEncodeBuffer_0001
 * @tc.name     Test MmsEncodeBuffer
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsEncodeBuffer_0001, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsEncodeBuffer_0001 -->");
    MmsEncodeBuffer mmsEncodeBuffer;
    bool retBool;
    retBool = mmsEncodeBuffer.EncodeShortLength(1);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeShortLength(31);
    EXPECT_FALSE(retBool);

    retBool = mmsEncodeBuffer.EncodeShortInteger(1);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeShortLength(31);
    EXPECT_FALSE(retBool);
    mmsEncodeBuffer.curPosition_ = MAX_MMS_MSG_PART_LEN + 1;
    retBool = mmsEncodeBuffer.EncodeShortLength(1);
    EXPECT_FALSE(retBool);

    mmsEncodeBuffer.curPosition_ = 0;
    retBool = mmsEncodeBuffer.EncodeOctet(1);
    EXPECT_TRUE(retBool);
    mmsEncodeBuffer.curPosition_ = MAX_MMS_MSG_PART_LEN + 1;
    retBool = mmsEncodeBuffer.EncodeShortLength(1);
    EXPECT_FALSE(retBool);

    mmsEncodeBuffer.curPosition_ = 0;
    retBool = mmsEncodeBuffer.EncodeValueLength(1);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeValueLength(31);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeValueLength(31);
    mmsEncodeBuffer.curPosition_ = MAX_MMS_MSG_PART_LEN + 1;
    EXPECT_TRUE(retBool);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_MmsEncodeBuffer_0002
 * @tc.name     Test MmsEncodeBuffer
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, MmsEncodeBuffer_0002, Function | MediumTest | Level1)
{
    TELEPHONY_LOGI("TelSMSMMSTest::MmsEncodeBuffer_0002 -->");
    MmsEncodeBuffer mmsEncodeBuffer;
    bool retBool;
    mmsEncodeBuffer.curPosition_ = 0;
    retBool = mmsEncodeBuffer.EncodeInteger(1);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeInteger(INPUT_INTEGER);
    EXPECT_TRUE(retBool);

    retBool = mmsEncodeBuffer.EncodeLongInteger(0);
    EXPECT_TRUE(retBool);
    retBool = mmsEncodeBuffer.EncodeLongInteger(1);
    EXPECT_TRUE(retBool);

    retBool = mmsEncodeBuffer.EncodeQuotedText("");
    EXPECT_TRUE(retBool);
    mmsEncodeBuffer.curPosition_ = MAX_MMS_MSG_PART_LEN + 1;
    retBool = mmsEncodeBuffer.EncodeQuotedText(BEGIN_STR);
    EXPECT_FALSE(retBool);
    mmsEncodeBuffer.curPosition_ = 0;
    retBool = mmsEncodeBuffer.EncodeQuotedText(END_STR);
    EXPECT_TRUE(retBool);

    retBool = mmsEncodeBuffer.EncodeTokenText("");
    EXPECT_TRUE(retBool);
    mmsEncodeBuffer.curPosition_ = MAX_MMS_MSG_PART_LEN + 1;
    retBool = mmsEncodeBuffer.EncodeTokenText(BEGIN_STR);
    EXPECT_FALSE(retBool);
    mmsEncodeBuffer.curPosition_ = 0;
    retBool = mmsEncodeBuffer.EncodeTokenText(END_STR);
    EXPECT_TRUE(retBool);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendMms_0001
 * @tc.name     Test SendMms_0001
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SendMms_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsService> smsService = std::make_shared<SmsService>();
    int32_t slotId = 0;
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int64_t time = 0;
    int32_t notAccess = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    AccessMmsToken token;
    int32_t noMmsc = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    int32_t noData = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    EXPECT_GE(notAccess, 0);
    EXPECT_GE(noMmsc, 0);
    EXPECT_GE(noData, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_SendMms_0002
 * @tc.name     Test SendMms_0002
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, SendMms_0002, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsService> smsService = std::make_shared<SmsService>();
    int32_t slotId = 0;
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int64_t time = 0;
    system::SetParameter("persist.edm.mms_disable", "true");
    int32_t notAccess = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    AccessMmsToken token;
    int32_t noMmsc = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    int32_t noData = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time);
    system::SetParameter("persist.edm.mms_disable", "false");
    EXPECT_GE(notAccess, 0);
    EXPECT_GE(noMmsc, 0);
    EXPECT_GE(noData, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DownloadMms_0001
 * @tc.name     Test DownloadMms_0001
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, DownloadMms_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsService> smsService = std::make_shared<SmsService>();
    int32_t slotId = 0;
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int64_t time = 0;
    int32_t notAccess = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    AccessMmsToken token;
    int32_t noMmsc = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    int32_t noData = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    notAccess = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    noMmsc = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    noData = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    EXPECT_GE(notAccess, 0);
    EXPECT_GE(noMmsc, 0);
    EXPECT_GE(noData, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_DownloadMms_0002
 * @tc.name     Test DownloadMms_0002
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, DownloadMms_0002, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsService> smsService = std::make_shared<SmsService>();
    int32_t slotId = 0;
    std::u16string mmsc = u"";
    std::u16string data = u"";
    std::u16string ua = u"";
    std::u16string uaprof = u"";
    int64_t time = 0;
    system::SetParameter("persist.edm.mms_disable", "true");
    int32_t notAccess = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    AccessMmsToken token;
    int32_t noMmsc = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    int32_t noData = smsService->DownloadMms(slotId, mmsc, data, ua, uaprof);
    notAccess = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    noMmsc = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    mmsc = StringUtils::ToUtf16(VNET_MMSC);
    noData = smsService->SendMms(slotId, mmsc, data, ua, uaprof, time, true);
    system::SetParameter("persist.edm.mms_disable", "false");
    EXPECT_GE(notAccess, 0);
    EXPECT_GE(noMmsc, 0);
    EXPECT_GE(noData, 0);
}

/**
 * @tc.number   Telephony_SmsMmsGtest_ServiceAfterSendMmsComplete_0001
 * @tc.name     Test ServiceAfterSendMmsComplete_0001
 * @tc.desc     Function test
 */
HWTEST_F(BranchMmsTest, ServiceAfterSendMmsComplete_0001, Function | MediumTest | Level1)
{
    std::shared_ptr<SmsService> smsService = std::make_shared<SmsService>();
    int32_t slotId = 0;
    int64_t time = 0;
    DataShare::DataShareValuesBucket sessionBucket;
    int32_t dataBaseId = 0;
    std::string sendStatus = "";
    smsService->ServiceAfterSendMmsComplete(slotId, time, dataBaseId, sessionBucket, sendStatus);
    dataBaseId = 1;
    smsService->ServiceAfterSendMmsComplete(slotId, time, dataBaseId, sessionBucket, sendStatus);
    EXPECT_TRUE(smsService != nullptr);
}

HWTEST_F(BranchMmsTest, GetPortFromURLTest001, TestSize.Level2)
{
    std::string port = StringUtils::GetPortFromURL("https://www.example.com?data_string");
    EXPECT_STREQ(port.c_str(), "");
}

HWTEST_F(BranchMmsTest, GetPortFromURLTest002, TestSize.Level2)
{
    std::string port = StringUtils::GetPortFromURL("https://www.example.com:9984?data_string");
    EXPECT_STREQ(port.c_str(), "9984");
}

HWTEST_F(BranchMmsTest, GetPortFromURLTest003, TestSize.Level2)
{
    std::string port = StringUtils::GetPortFromURL("www.example.com:9984?data_string");
    EXPECT_STREQ(port.c_str(), "9984");
}

HWTEST_F(BranchMmsTest, GetPortFromURLTest004, TestSize.Level2)
{
    std::string port = StringUtils::GetPortFromURL("www.example.com?data_string");
    EXPECT_STREQ(port.c_str(), "");
}

HWTEST_F(BranchMmsTest, GetHostnameFromURL01, TestSize.Level2)
{
    std::string hostname = StringUtils::GetHostnameFromURL("https:////www.example.com?data_string");
    EXPECT_STREQ(hostname.c_str(), "www.example.com");
}

HWTEST_F(BranchMmsTest, GetHostnameFromURL02, TestSize.Level2)
{
    std::string hostname = StringUtils::GetHostnameFromURL(R"(https:/\\\\\\///\\/www.example.com?data_string)");
    EXPECT_STREQ(hostname.c_str(), "www.example.com");
}

HWTEST_F(BranchMmsTest, GetHostnameFromURL03, TestSize.Level2)
{
    std::string hostname = StringUtils::GetHostnameFromURL("");
    EXPECT_STREQ(hostname.c_str(), "");
}

} // namespace Telephony
} // namespace OHOS
