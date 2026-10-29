/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#include "sms_base_message.h"

#include "securec.h"
#include "sms_mms_errors.h"
#include "sms_service_manager_client.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "text_coder.h"
#include "unicode/rbbi.h"
#include "unicode/unistr.h"
#include "unicode/ucnv.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint8_t PID_87 = 0xc0;
static constexpr uint8_t PID_7 = 0x40;
static constexpr uint8_t PID_10_LOW = 0x3f;
static constexpr int16_t WAP_PUSH_PORT = 2948;
static constexpr uint8_t MAX_GSM_7BIT_DATA_LEN = 160;
static constexpr uint8_t MAX_UCS2_DATA_LEN = 140;
static constexpr uint8_t BYTE_BITS = 8;
static constexpr uint8_t MAX_ADD_PARAM_LEN = 12;
static constexpr uint8_t GSM_BEAR_DATA_LEN = 140;
static constexpr uint8_t CHARSET_7BIT_BITS = 7;
static constexpr uint8_t GSM7_ESCAPE_CHAR = 0x1B;
static constexpr int32_t GSM7_ESCAPE_SEQ_LEN = 2;
const std::string CT_SMSC = "10659401";

string SmsBaseMessage::GetSmscAddr() const
{
    return scAddress_;
}

void SmsBaseMessage::SetSmscAddr(const string &address)
{
    scAddress_ = address;
}

string SmsBaseMessage::GetOriginatingAddress() const
{
    return originatingAddress_;
}

string SmsBaseMessage::GetVisibleOriginatingAddress() const
{
    if (isEmail_) {
        return emailFrom_;
    }
    return originatingAddress_;
}

enum SmsMessageClass SmsBaseMessage::GetMessageClass() const
{
    return msgClass_;
}

string SmsBaseMessage::GetVisibleMessageBody() const
{
    if (isEmail_) {
        return emailBody_;
    }
    return visibleMessageBody_;
}

string SmsBaseMessage::GetEmailAddress() const
{
    return emailFrom_;
}

string SmsBaseMessage::GetEmailMessageBody() const
{
    return emailBody_;
}

bool SmsBaseMessage::IsEmail() const
{
    return isEmail_;
}

std::vector<uint8_t> SmsBaseMessage::GetRawPdu() const
{
    return rawPdu_;
}

std::string SmsBaseMessage::GetRawUserData() const
{
    return rawUserData_;
}

std::string SmsBaseMessage::GetRawWapPushUserData() const
{
    return rawWapPushUserData_;
}

int64_t SmsBaseMessage::GetScTimestamp() const
{
    return scTimestamp_;
}

// 3GPP TS 23.040 V5.1.0 9.2.3.9 TP Protocol Identifier (TP PID)
bool SmsBaseMessage::IsReplaceMessage()
{
    uint8_t temp = static_cast<uint8_t>(protocolId_);
    uint8_t tempPid = temp & PID_10_LOW;
    bReplaceMessage_ = ((temp & PID_87) == PID_7) && (tempPid > 0) && (tempPid < MAX_REPLY_PID);
    return bReplaceMessage_;
}

// Message Waiting Indication Status storage on the USIM
bool SmsBaseMessage::IsCphsMwi() const
{
    return bCphsMwi_;
}

// 3GPP TS 23.040 V5.1.0 3.2.6 Messages Waiting
bool SmsBaseMessage::IsMwiClear() const
{
    return bMwiClear_;
}

// 3GPP TS 23.040 V5.1.0 3.2.6 Messages Waiting
bool SmsBaseMessage::IsMwiSet() const
{
    return bMwiSet_;
}

// 3GPP TS 23.040 V5.1.0 3.2.6 Messages Waiting
bool SmsBaseMessage::IsMwiNotStore() const
{
    return bMwiNotStore_;
}

int SmsBaseMessage::GetStatus() const
{
    return status_;
}

bool SmsBaseMessage::IsSmsStatusReportMessage() const
{
    return bStatusReportMessage_;
}

bool SmsBaseMessage::HasReplyPath() const
{
    return hasReplyPath_;
}

int SmsBaseMessage::GetProtocolId() const
{
    return protocolId_;
}

std::shared_ptr<SmsConcat> SmsBaseMessage::GetConcatMsg()
{
    smsConcat_ = nullptr;
    for (int i = 0; i < smsUserData_.headerCnt; i++) {
        if (smsUserData_.header[i].udhType == UDH_CONCAT_8BIT) {
            smsConcat_ = std::make_shared<SmsConcat>();
            if (smsConcat_ == nullptr) {
                TELEPHONY_LOGE("smsConcat is nullptr.");
                break;
            }
            smsConcat_->is8Bits = true;
            smsConcat_->totalSeg = smsUserData_.header[i].udh.concat8bit.totalSeg;
            smsConcat_->seqNum = smsUserData_.header[i].udh.concat8bit.seqNum;
            smsConcat_->msgRef = smsUserData_.header[i].udh.concat8bit.msgRef;
            break;
        } else if (smsUserData_.header[i].udhType == UDH_CONCAT_16BIT) {
            smsConcat_ = std::make_shared<SmsConcat>();
            if (smsConcat_ == nullptr) {
                TELEPHONY_LOGE("smsConcat is nullptr.");
                break;
            }
            smsConcat_->is8Bits = false;
            smsConcat_->totalSeg = smsUserData_.header[i].udh.concat16bit.totalSeg;
            smsConcat_->seqNum = smsUserData_.header[i].udh.concat16bit.seqNum;
            smsConcat_->msgRef = smsUserData_.header[i].udh.concat16bit.msgRef;
            break;
        }
    }
    return smsConcat_;
}

std::shared_ptr<SmsAppPortAddr> SmsBaseMessage::GetPortAddress()
{
    portAddress_ = nullptr;
    for (int i = 0; i < smsUserData_.headerCnt; i++) {
        if (smsUserData_.header[i].udhType == UDH_APP_PORT_8BIT) {
            portAddress_ = std::make_shared<SmsAppPortAddr>();
            if (portAddress_ == nullptr) {
                TELEPHONY_LOGE("portAddress_ is nullptr.");
                break;
            }
            portAddress_->is8Bits = true;
            portAddress_->destPort = smsUserData_.header[i].udh.appPort8bit.destPort;
            portAddress_->originPort = smsUserData_.header[i].udh.appPort8bit.originPort;
            break;
        } else if (smsUserData_.header[i].udhType == UDH_APP_PORT_16BIT) {
            portAddress_ = std::make_shared<SmsAppPortAddr>();
            if (portAddress_ == nullptr) {
                TELEPHONY_LOGE("portAddress_ is nullptr.");
                break;
            }
            portAddress_->is8Bits = false;
            portAddress_->destPort = smsUserData_.header[i].udh.appPort16bit.destPort;
            portAddress_->originPort = smsUserData_.header[i].udh.appPort16bit.originPort;
            break;
        }
    }
    return portAddress_;
}

std::shared_ptr<SpecialSmsIndication> SmsBaseMessage::GetSpecialSmsInd()
{
    specialSmsInd_ = nullptr;
    for (int i = 0; i < smsUserData_.headerCnt; i++) {
        if (smsUserData_.header[i].udhType == UDH_SPECIAL_SMS) {
            specialSmsInd_ = std::make_shared<SpecialSmsIndication>();
            if (specialSmsInd_ == nullptr) {
                TELEPHONY_LOGE("specialSmsInd_ is nullptr.");
                break;
            }
            specialSmsInd_->bStore = smsUserData_.header[i].udh.specialInd.bStore;
            specialSmsInd_->msgInd = smsUserData_.header[i].udh.specialInd.msgInd;
            specialSmsInd_->waitMsgNum = smsUserData_.header[i].udh.specialInd.waitMsgNum;
            break;
        }
    }
    return specialSmsInd_;
}

bool SmsBaseMessage::IsConcatMsg()
{
    return (GetConcatMsg() == nullptr) ? false : true;
}

bool SmsBaseMessage::IsWapPushMsg()
{
    std::shared_ptr<SmsAppPortAddr> portAddress = GetPortAddress();
    if (portAddress != nullptr && !portAddress->is8Bits) {
        return portAddress->destPort == WAP_PUSH_PORT;
    }
    return false;
}

void SmsBaseMessage::ConvertMessageClass(enum SmsMessageClass msgClass)
{
    switch (msgClass) {
        case SMS_SIM_MESSAGE:
            msgClass_ = SmsMessageClass::SMS_SIM_MESSAGE;
            break;
        case SMS_INSTANT_MESSAGE:
            msgClass_ = SmsMessageClass::SMS_INSTANT_MESSAGE;
            break;
        case SMS_OPTIONAL_MESSAGE:
            msgClass_ = SmsMessageClass::SMS_OPTIONAL_MESSAGE;
            break;
        case SMS_FORWARD_MESSAGE:
            msgClass_ = SmsMessageClass::SMS_FORWARD_MESSAGE;
            break;
        default:
            msgClass_ = SmsMessageClass::SMS_CLASS_UNKNOWN;
            break;
    }
}

int SmsBaseMessage::GetMsgRef()
{
    return msgRef_;
}

int SmsBaseMessage::GetSegmentSize(
    DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId) const
{
    const int multiSegSms7BitLength = 153;
    const int multiSegSmsUcs2Length = 134;
    const int port = 6;
    const int lang = 3;
    int headerSize = 0;
    int segSize = 0;
    int maxSize = 0;
    if (codingScheme == DATA_CODING_7BIT || codingScheme == DATA_CODING_ASCII7BIT) {
        maxSize = MAX_GSM_7BIT_DATA_LEN;
    } else if (codingScheme == DATA_CODING_8BIT || codingScheme == DATA_CODING_UCS2) {
        maxSize = MAX_UCS2_DATA_LEN;
    }

    if (bPortNum == true) {
        headerSize += port;
    }

    if (langId != MSG_ID_RESERVED_LANG) {
        headerSize += lang;
    }

    if (codingScheme == DATA_CODING_7BIT || codingScheme == DATA_CODING_ASCII7BIT) {
        if ((dataLen + headerSize) > maxSize) {
            segSize = multiSegSms7BitLength;
        } else {
            segSize = dataLen;
        }
    } else if (codingScheme == DATA_CODING_8BIT || codingScheme == DATA_CODING_UCS2) {
        if ((dataLen + headerSize) > maxSize) {
            segSize = multiSegSmsUcs2Length;
        } else {
            segSize = dataLen;
        }
    }

    return segSize;
}

int SmsBaseMessage::GetMaxSegmentSize(
    DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId, int replyAddrLen) const
{
    const int headerLen = 1;
    const int concat = 5;
    const int port = 6;
    const int lang = 3;
    const int reply = 2;
    int headerSize = 0;
    int segSize = 0;
    int maxSize = 0;
    if (codingScheme == DATA_CODING_7BIT || codingScheme == DATA_CODING_ASCII7BIT) {
        maxSize = MAX_GSM_7BIT_DATA_LEN;
    } else if (codingScheme == DATA_CODING_8BIT || codingScheme == DATA_CODING_UCS2) {
        maxSize = MAX_UCS2_DATA_LEN;
    }
    if (bPortNum) {
        headerSize += port;
    }
    if (langId != MSG_ID_RESERVED_LANG) {
        headerSize += lang;
    }
    if (replyAddrLen > 0) {
        headerSize += reply;
        headerSize += replyAddrLen;
    }
    if (codingScheme == DATA_CODING_7BIT || codingScheme == DATA_CODING_ASCII7BIT) {
        if ((dataLen + headerSize) > maxSize) {
            segSize =
                ((GSM_BEAR_DATA_LEN * BYTE_BITS) - ((headerLen + concat + headerSize) * BYTE_BITS)) / CHARSET_7BIT_BITS;
        } else {
            segSize = maxSize - headerSize;
        }
    } else if (codingScheme == DATA_CODING_8BIT || codingScheme == DATA_CODING_UCS2) {
        if ((dataLen + headerSize) > maxSize) {
            segSize = GSM_BEAR_DATA_LEN - (headerLen + concat + headerSize);
        } else {
            segSize = maxSize - headerSize;
        }
    }
    return segSize;
}

void SmsBaseMessage::ConvertSpiltToUtf8(SplitInfo &split, const DataCodingScheme &codingType)
{
    if (split.encodeData.size() <= 0) {
        TELEPHONY_LOGE("data is null");
        return;
    }

    int dataSize = 0;
    uint8_t buff[MAX_MSG_TEXT_LEN + 1] = { 0 };
    switch (codingType) {
        case DATA_CODING_7BIT: {
            MsgLangInfo langInfo = {
                0,
            };
            // the segment is sent with a national language single shift IE when langId is set
            langInfo.bSingleShift = (split.langId != MSG_ID_RESERVED_LANG);
            langInfo.singleLang = split.langId;
            langInfo.bLockingShift = false;
            dataSize = TextCoder::Instance().Gsm7bitToUtf8(
                buff, MAX_MSG_TEXT_LEN, split.encodeData.data(), split.encodeData.size(), langInfo);
            break;
        }
        case DATA_CODING_UCS2: {
            dataSize = TextCoder::Instance().Ucs2ToUtf8(
                buff, MAX_MSG_TEXT_LEN, split.encodeData.data(), split.encodeData.size());
            break;
        }
        default: {
            if (split.encodeData.size() > sizeof(buff)) {
                TELEPHONY_LOGE("AnalsisDeliverMsg data length invalid.");
                return;
            }
            if (memcpy_s(buff, sizeof(buff), split.encodeData.data(), split.encodeData.size()) != EOK) {
                TELEPHONY_LOGE("AnalsisDeliverMsg memcpy_s fail.");
                return;
            }
            dataSize = static_cast<int>(split.encodeData.size());
            buff[dataSize] = '\0';
            break;
        }
    }

    split.text.insert(0, reinterpret_cast<char *>(buff), dataSize);
    TELEPHONY_LOGI("split text");
}
void SmsBaseMessage::SplitMessageUcs2(std::vector<struct SplitInfo> &splitResult, const uint8_t* decodeData,
    int32_t encodeLen, int32_t segSize, DataCodingScheme &codingType)
{
    // this 3 para divide 2 because breakiterator class is init by a uint16_t pointer.
    int32_t utf16Multiples = 2;
    int32_t dataSize = encodeLen / utf16Multiples;
    int32_t segSizeHalf = segSize / utf16Multiples;
    int32_t index = 0;
    int32_t oneByte = 1;
    int32_t bits = 8;
    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    /*
     * decodeData is uint8_t array, in order to init breakiterator class, need a uint16_t array. sample:[0xa0,0xa1,
     * 0xa2,0xa3] become [0xa1a2,0xa3a4]
     */
    uint16_t decodeData16Bit[dataSize];
    for (int32_t i = 0; i < dataSize; i++) {
        decodeData16Bit[i] = (decodeData[i * utf16Multiples] << bits) | decodeData[i * utf16Multiples + oneByte];
    }
    /*
     * init breakiterator class. attention: createCharacterInstance is a factory method, in fact breakiterator is
     * a pure abstract class, this fuction creat a object of subclass rulebasedbreakiterator.
     */
    icu::UnicodeString fullData(decodeData16Bit, dataSize);
    UErrorCode status = U_ZERO_ERROR;
    icu::BreakIterator* fullDataIter = icu::BreakIterator::createCharacterInstance(NULL, status);
    if (U_FAILURE(status)) {
        TELEPHONY_LOGE("Failed to create break iterator");
        return;
    }
    // let breakiterator object point data need to operate
    fullDataIter->setText(fullData);
    // let iterator point zero element
    fullDataIter->first();
    // operation of segment except the last one, such as a pdu is devide to 3 segment, 1 and 2 are operated under.
    while ((dataSize - index) > segSizeHalf) {
        // init struct to store data
        struct SplitInfo splitInfo;
        splitInfo.langId = langId;
        splitInfo.encodeType = codingType;
        /*
         * judge if the end of this segment is boundary, if it is boundary, store number of segsize data in struct
         * and move the index agter this boundary to be the head of next segment
         * if it is not boundary, use previous function or next function(set the para to -1)to find the previous
         * boundary before end of segment
         */
        int32_t nextIndex = FindNextUnicodePosition(index, segSizeHalf, fullDataIter, fullData);
        splitInfo.encodeData = std::vector<uint8_t>(&decodeData[index * utf16Multiples],
            &decodeData[index * utf16Multiples] + (nextIndex - index) * utf16Multiples);
        index = nextIndex;
        ConvertSpiltToUtf8(splitInfo, codingType);
        splitResult.push_back(splitInfo);
        fullDataIter->first();
    }
    // operation of last segment
    struct SplitInfo splitInfo;
    splitInfo.langId = langId;
    splitInfo.encodeType = codingType;
    splitInfo.encodeData = std::vector<uint8_t>(&decodeData[index * utf16Multiples],
        &decodeData[index * utf16Multiples] + (dataSize - index) * utf16Multiples);
    ConvertSpiltToUtf8(splitInfo, codingType);
    splitResult.push_back(splitInfo);
}

/*
 * 3GPP TS 23.038 6.2.1.1: ESC (0x1B) and the following septet are one character, and each segment of a
 * concatenated message is decoded on its own (TS 23.040 9.2.3.24.1), so a 7bit segment must not end
 * between an escape character and its extension code.
 */
static int32_t GetGsm7bitSegmentLen(const uint8_t *data, int32_t start, int32_t total, int32_t segLen)
{
    int32_t end = start + segLen;
    if (end >= total) {
        return segLen;
    }
    int32_t pos = start;
    while (pos < end) {
        pos += (data[pos] == GSM7_ESCAPE_CHAR) ? GSM7_ESCAPE_SEQ_LEN : 1;
    }
    if (pos > end && segLen > 1) {
        return segLen - 1;
    }
    return segLen;
}

/*
 * Cut encodeLen octets of decodeData into segments of at most segSize octets. Every segment takes the coding and
 * language of segInfo, and a 7bit segment never ends between ESC and its extension code.
 */
void SmsBaseMessage::SplitMessageBySize(std::vector<struct SplitInfo> &splitResult, const uint8_t *decodeData,
    int32_t encodeLen, int32_t segSize, const struct SplitInfo &segInfo)
{
    int32_t index = 0;
    while (segSize > 0 && index < encodeLen) {
        int32_t userDataLen = (encodeLen - index > segSize) ? segSize : (encodeLen - index);
        if (segInfo.encodeType == DATA_CODING_7BIT) {
            userDataLen = GetGsm7bitSegmentLen(decodeData, index, encodeLen, userDataLen);
        }
        struct SplitInfo splitInfo = segInfo;
        splitInfo.encodeData = std::vector<uint8_t>(&decodeData[index], &decodeData[index] + userDataLen);
        ConvertSpiltToUtf8(splitInfo, segInfo.encodeType);
        splitResult.push_back(splitInfo);
        index += userDataLen;
    }
}

void SmsBaseMessage::SplitMessage(std::vector<struct SplitInfo> &splitResult, const std::string &text,
    bool force7BitCode, DataCodingScheme &codingType, bool bPortNum, const std::string &desAddr)
{
    std::string msgText(text);
    // init destination array of pdu data
    uint8_t decodeData[(MAX_GSM_7BIT_DATA_LEN * MAX_SEGMENT_NUM) + 1];
    if (memset_s(decodeData, sizeof(decodeData), 0x00, sizeof(decodeData)) != EOK) {
        TELEPHONY_LOGE("SplitMessage memset_s error!");
        return;
    }
    int encodeLen = 0;
    bool bAbnormal = false;
    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    codingType = force7BitCode ? DATA_CODING_7BIT : DATA_CODING_AUTO;
    if (CT_SMSC.compare(desAddr) == 0) {
        codingType = DATA_CODING_8BIT;
    }
    /*
     * src is utf-8 code, DecodeMessage function aim to trans the src to dest unicode method depend on above operation
     * encodeLen means the data length agter trans(although the dest unicode method is ucs2 or utf16, the length is the
     * count of uint8_t) such as utf8 is 0x41, trans utf16 is 0x00,0x41, the length is 2
     * after DecodeMessage function, the codingType will become DATA_CODING_UCS2 although before is DATA_CODING_AUTO
     */
    encodeLen = DecodeMessage(decodeData, sizeof(decodeData), codingType, msgText, bAbnormal, langId);
    if (encodeLen <= 0) {
        TELEPHONY_LOGE("encodeLen Less than or equal to 0");
        return;
    }
    int segSize = 0;
    int segCount = 0;
    // get segment length mainly according to codingType.
    segSize = GetSegmentSize(codingType, encodeLen, bPortNum, langId);
    if (segSize > 0) {
        segCount = ceil((double)encodeLen / (double)segSize);
    }
    /*
     * under code is a special condition: the length of pdu data is over segSize conculated above and codingType is
     * utf16(although the codingType displayed is ucs2). because in this condition a emoji(takeover 4 bytes in utf16)
     * may be cut in 2 parts(first 2 byte in segment1 and last 2 in segment 2), under code will avoid this situation.
     */
    if (codingType == DATA_CODING_UCS2 && segCount > 1) {
        SplitMessageUcs2(splitResult, decodeData, encodeLen, segSize, codingType);
    } else {
        struct SplitInfo segInfo;
        segInfo.langId = langId;
        segInfo.encodeType = codingType;
        SplitMessageBySize(splitResult, decodeData, encodeLen, segSize, segInfo);
    }
}

int32_t SmsBaseMessage::GetSmsSegmentsInfo(const std::string &message, bool force7BitCode, LengthInfo &lenInfo)
{
    uint8_t decodeData[(MAX_GSM_7BIT_DATA_LEN * MAX_SEGMENT_NUM) + 1];
    if (memset_s(decodeData, sizeof(decodeData), 0x00, sizeof(decodeData)) != EOK) {
        TELEPHONY_LOGE("SplitMessage memset_s error!");
        return TELEPHONY_ERR_MEMSET_FAIL;
    }
    const uint8_t smsEncodingUnkown = 0;
    const uint8_t smsEncoding7Bit = 1;
    const uint8_t smsEncoding8Bit = 2;
    const uint8_t smsEncoding16Bit = 3;
    int encodeLen = 0;
    bool bAbnormal = false;
    MSG_LANGUAGE_ID_T langId = MSG_ID_RESERVED_LANG;
    DataCodingScheme codingType = force7BitCode ? DATA_CODING_7BIT : DATA_CODING_AUTO;
    encodeLen = DecodeMessage(decodeData, sizeof(decodeData), codingType, message, bAbnormal, langId);
    if (encodeLen <= 0) {
        TELEPHONY_LOGE("encodeLen Less than or equal to 0");
        return SMS_MMS_DECODE_DATA_EMPTY;
    }
    int segSize = GetMaxSegmentSize(codingType, encodeLen, false, langId, MAX_ADD_PARAM_LEN);
    TELEPHONY_LOGI("segSize = %{public}d", segSize);
    lenInfo.msgEncodeCount = static_cast<uint16_t>(encodeLen);
    if (codingType == DATA_CODING_7BIT || codingType == DATA_CODING_ASCII7BIT) {
        lenInfo.dcs = smsEncoding7Bit;
    } else if (codingType == DATA_CODING_UCS2) {
        lenInfo.dcs = smsEncoding16Bit;
    } else if (codingType == DATA_CODING_8BIT) {
        lenInfo.dcs = smsEncoding8Bit;
    } else {
        lenInfo.dcs = smsEncodingUnkown;
    }
    if (lenInfo.dcs == smsEncoding16Bit) {
        lenInfo.msgEncodeCount = lenInfo.msgEncodeCount / 2;
        segSize = segSize / 2;
    }
    if (segSize != 0) {
        lenInfo.msgRemainCount = static_cast<uint8_t>(((segSize - (lenInfo.msgEncodeCount % segSize))) % segSize);
        lenInfo.msgSegCount = ceil(static_cast<double>(lenInfo.msgEncodeCount) / static_cast<double>(segSize));
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t SmsBaseMessage::GetIndexOnSim() const
{
    return indexOnSim_;
}

void SmsBaseMessage::SetIndexOnSim(int32_t index)
{
    indexOnSim_ = index;
}

int32_t SmsBaseMessage::FindNextUnicodePosition(int32_t index, int32_t segSizeHalf, icu::BreakIterator * fullDataIter,
    const icu::UnicodeString &fullData)
{
    int32_t nextIndex = index + segSizeHalf;
    if (!fullDataIter->isBoundary(nextIndex)) {
        int32_t breakPos = fullDataIter->previous();
        if (breakPos > index) {
            nextIndex = breakPos;
        }
    }
    return nextIndex;
}
} // namespace Telephony
} // namespace OHOS
