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

#ifndef GSM_SMS_MESSAGE_H
#define GSM_SMS_MESSAGE_H

#include "gsm_pdu_code_type.h"
#include "sms_base_message.h"

namespace OHOS {
namespace Telephony {
class GsmSmsMessage : public SmsBaseMessage {
public:
    GsmSmsMessage() = default;
    virtual ~GsmSmsMessage() = default;
    void SetFullText(const std::string &text);
    void SetDestAddress(const std::string &destAddress);
    void SetDestPort(uint32_t port);

    std::string GetFullText() const;
    std::string GetDestAddress() const;
    uint16_t GetDestPort();
    bool GetIsSmsText() const;
    bool GetGsm() const;
    std::string GetReplyAddress() const;

    int SetHeaderLang(int index, const DataCodingScheme codingType, const MSG_LANGUAGE_ID_T langId);
    int SetHeaderConcat(int index, const SmsConcat &concat);
    int SetHeaderReply(int index);

    std::shared_ptr<struct SmsTpdu> CreateDefaultSubmitSmsTpdu(const std::string &dest, const std::string &sc,
        const std::string &text, bool bStatusReport, const DataCodingScheme codingScheme);
    std::shared_ptr<struct SmsTpdu> CreateDataSubmitSmsTpdu(const std::string &desAddr, const std::string &scAddr,
        int32_t port, const uint8_t *data, uint32_t dataLen, uint8_t msgRef8bit, DataCodingScheme codingType,
        bool bStatusReport);

    std::shared_ptr<struct EncodeInfo> GetSubmitEncodeInfo(const std::string &sc, bool bMore);
    std::shared_ptr<struct EncodeInfo> GetSubmitEncodeInfoPartData(
        uint8_t *encodeSmscAddr, uint8_t encodeSmscLen, bool bMore);
    std::shared_ptr<struct SmsTpdu> CreateDeliverSmsTpdu();
    std::shared_ptr<struct SmsTpdu> CreateDeliverReportSmsTpdu();
    std::shared_ptr<struct SmsTpdu> CreateStatusReportSmsTpdu();
    static std::shared_ptr<GsmSmsMessage> CreateMessage(const std::string &pdu);

    bool PduAnalysis(const std::string &pdu);
    void ConvertMessageDcs();
    void ConvertUserData();
    bool GetIsTypeZeroInd() const;
    bool GetIsSIMDataTypeDownload() const;
    void ConvertMsgTimeStamp(const struct SmsTimeStamp &times);
    bool IsSpecialMessage() const;
    void SetSmsCodingNationalType(SmsCodingNationalType smsCodingNationalType);
    int GetSegmentSize(
        DataCodingScheme &codingScheme, int dataLen, bool bPortNum, MSG_LANGUAGE_ID_T &langId) const override;

private:
    void AnalysisMsgDeliver(const SmsDeliver &deliver);
    void AnalysisMsgStatusReport(const SmsStatusReport &statusRep);
    void AnalysisMsgSubmit(const SmsSubmit &submit);
    void CreateDefaultSubmit(bool bStatusReport, const DataCodingScheme codingScheme);
    int SetSmsTpduDestAddress(std::shared_ptr<struct SmsTpdu> &tPdu, const std::string &desAddr);
    uint8_t CalcReplyEncodeAddress(const std::string &replyAddress);
    virtual int DecodeMessage(uint8_t *decodeData, unsigned int length, DataCodingScheme &codingType,
        const std::string &msgText, bool &bAbnormal, MSG_LANGUAGE_ID_T &langId);
    bool PduAnalysisMsg();
    void ConvertUserPartData();
    MsgLangInfo GetNationalLangInfo() const;
    void ParseEmailFromMessageBody();
    bool IsEmailAddress(std::string emailFrom);

public:
    static constexpr uint16_t TAPI_NETTEXT_SMDATA_SIZE_MAX = 255;
    static constexpr uint8_t TAPI_SIM_SMSP_ADDRESS_LEN = 20;

private:
    std::string fullText_;
    std::string destAddress_;
    std::string replyAddress_;
    uint16_t destPort_ = -1;
    bool bSmsText_ = false;
    std::shared_ptr<struct SmsTpdu> smsTpdu_;
    SmsCodingNationalType smsCodingNationalType_ = SMS_CODING_NATIONAL_TYPE_DEFAULT;
};

struct EncodeInfo {
    char tpduData_[GsmSmsMessage::TAPI_NETTEXT_SMDATA_SIZE_MAX + 1] = { 0 };
    char smcaData_[GsmSmsMessage::TAPI_SIM_SMSP_ADDRESS_LEN + 1] = { 0 };
    uint16_t tpduLen = 0;
    uint8_t smcaLen = 0;
    bool isMore_ = false;
};
} // namespace Telephony
} // namespace OHOS
#endif
