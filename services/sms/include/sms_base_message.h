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

#ifndef SMS_BASE_MESSAGE__H
#define SMS_BASE_MESSAGE__H

#include <cmath>
#include <memory>

#include "cdma_sms_transport_message.h"
#include "gsm_sms_tpdu_codec.h"
#include "msg_text_convert_common.h"
#include "sms_common_utils.h"
#include "string_utils.h"
#include "unicode/brkiter.h"

namespace OHOS {
namespace Telephony {
typedef struct {
    uint16_t msgRef;
    uint16_t seqNum;
    uint16_t totalSeg;
    bool is8Bits;
} SmsConcat;

typedef struct {
    uint16_t destPort;
    uint16_t originPort;
    bool is8Bits;
} SmsAppPortAddr;

typedef struct {
    bool bStore;
    uint16_t msgInd;
    uint16_t waitMsgNum;
} SpecialSmsIndication;

struct SplitInfo {
    std::string text = "";
    std::vector<uint8_t> encodeData {};
    DataCodingScheme encodeType = DataCodingScheme::DATA_CODING_7BIT;
    MSG_LANGUAGE_ID_T langId = 0;
};

struct LengthInfo {
    uint8_t dcs = 0;
    uint8_t msgSegCount = 0;
    uint16_t msgEncodeCount = 0;
    uint8_t msgRemainCount = 0;
};

class SmsBaseMessage {
public:
    SmsBaseMessage() = default;
    virtual ~SmsBaseMessage() = default;
    virtual void SetSmscAddr(const std::string &scAddress);
    virtual std::string GetSmscAddr() const;
    virtual std::string GetOriginatingAddress() const;
    virtual std::string GetVisibleOriginatingAddress() const;
    virtual std::string GetVisibleMessageBody() const;
    virtual std::string GetEmailAddress() const;
    virtual std::string GetEmailMessageBody() const;
    virtual bool IsEmail() const;
    virtual enum SmsMessageClass GetMessageClass() const;
    std::vector<uint8_t> GetRawPdu() const;
    std::string GetRawUserData() const;
    std::string GetRawWapPushUserData() const;
    virtual int64_t GetScTimestamp() const;
    virtual int GetStatus() const;
    virtual int GetProtocolId() const;
    virtual bool IsReplaceMessage();
    virtual bool IsCphsMwi() const;
    virtual bool IsMwiClear() const;
    virtual bool IsMwiSet() const;
    virtual bool IsMwiNotStore() const;
    virtual bool IsSmsStatusReportMessage() const;
    virtual bool HasReplyPath() const;
    virtual std::shared_ptr<SmsConcat> GetConcatMsg();
    virtual std::shared_ptr<SmsAppPortAddr> GetPortAddress();
    virtual std::shared_ptr<SpecialSmsIndication> GetSpecialSmsInd();
    virtual bool IsConcatMsg();
    virtual bool IsWapPushMsg();
    virtual void ConvertMessageClass(enum SmsMessageClass msgClass);
    virtual int GetMsgRef();
    virtual int GetSegmentSize(
        DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId) const;
    virtual void SplitMessage(std::vector<struct SplitInfo> &splitResult, const std::string &text, bool force7BitCode,
        DataCodingScheme &codingType, bool bPortNum, const std::string &desAddr);
    virtual int32_t GetIndexOnSim() const;
    virtual void SetIndexOnSim(int32_t index);
    virtual int32_t GetSmsSegmentsInfo(const std::string &message, bool force7BitCode, LengthInfo &lenInfo);
    virtual int GetMaxSegmentSize(
        DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId, int replyAddrLen) const;
    virtual void SplitMessageUcs2(std::vector<struct SplitInfo> &splitResult, const uint8_t* decodeData,
        int32_t encodeLen, int32_t segSize, DataCodingScheme &codingType);

protected:
    constexpr static int16_t MAX_MSG_TEXT_LEN = 1530;
    constexpr static int16_t MAX_REPLY_PID = 8;
    std::string scAddress_;
    std::string originatingAddress_;
    std::string visibleMessageBody_;
    enum SmsMessageClass msgClass_ = SMS_CLASS_UNKNOWN;
    int64_t scTimestamp_;
    int status_;
    int protocolId_;
    bool bReplaceMessage_;
    bool bStatusReportMessage_;
    bool bMwi_;
    bool bMwiSense_;
    bool bCphsMwi_;
    bool bMwiClear_;
    bool bMwiSet_;
    bool bMwiNotStore_;
    bool hasReplyPath_;
    bool bMoreMsg_;
    bool bHeaderInd_;
    int headerCnt_;
    int headerDataLen_;
    int msgRef_;
    bool bCompressed_;
    bool bIndActive_;
    int codingScheme_;
    int codingGroup_;
    std::vector<uint8_t> rawPdu_;
    std::string rawUserData_;
    std::string rawWapPushUserData_;
    struct SmsUDPackage smsUserData_;
    struct SmsTpud smsWapPushUserData_;
    std::shared_ptr<SmsConcat> smsConcat_;
    std::shared_ptr<SmsAppPortAddr> portAddress_;
    std::shared_ptr<SpecialSmsIndication> specialSmsInd_;
    int32_t indexOnSim_ = -1;
    std::string emailFrom_;
    std::string emailBody_;
    bool isEmail_ = false;

private:
    virtual int DecodeMessage(uint8_t *decodeData, unsigned int length, DataCodingScheme &codingType,
        const std::string &msgText, bool &bAbnormal, MSG_LANGUAGE_ID_T &langId) = 0;
    void ConvertSpiltToUtf8(SplitInfo &split, const DataCodingScheme &codingType);
    void SplitMessageBySize(std::vector<struct SplitInfo> &splitResult, const uint8_t *decodeData, int32_t encodeLen,
        int32_t segSize, const struct SplitInfo &segInfo);
    int32_t FindNextUnicodePosition(int32_t index, int32_t segSizeHalf, icu::BreakIterator *fullDataIter,
        const icu::UnicodeString &fullData);
};
} // namespace Telephony
} // namespace OHOS
#endif
