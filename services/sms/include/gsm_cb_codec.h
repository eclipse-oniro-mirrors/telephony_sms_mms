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

#ifndef GSM_CB_CODEC_H
#define GSM_CB_CODEC_H

#include <ctime>
#include <string>
#include <unordered_map>

#include "gsm_cb_pdu_decode_buffer.h"

namespace OHOS {
namespace Telephony {
class GsmCbCodec : public std::enable_shared_from_this<GsmCbCodec> {
public:
    static constexpr uint8_t CB_IOS639_LANG_SIZE = 3;
    /** warning type of a message that carries no ETWS warning type */
    static constexpr uint16_t ETWS_WARNING_TYPE_UNKNOWN = 0xFFFF;
    /**
     * from 3GPP TS 23.041 V4.1.0 (2001-06) 5 section Network Architecture
     * GSM Cell broadcast message 0x01
     * UMTSCell broadcast message 0x02
     * CDMA broadcast message 0x03
     */
    enum CBNetType {
        GSM_NET_CB = 1,
        UMTS_NET_CB,
        CDMA_NET_CB,
    };

    /**
     * Primary ETWS message
     * GSM Secondary ETWS message
     * UMTS Secondary ETWS message
     * CDMA Seconday ETWS message
     */
    enum EtwsCBType {
        ETWS_PRIMARY = 0,
        ETWS_GSM,
        ETWS_UMTS,
        ETWS_CDMA,
    };

    /**
     * CBS Message
     * Schedule Message
     * CBS41 Message
     * JAVA-CB Message
     */
    enum GsmCbMsgType {
        GSM_UNKNOWN = 0,
        GSM_CBS,
        GSM_SCHEDULE,
        GSM_CBS41,
        GSM_JAVACBS,
        GSM_ETWS,
    };

    /**
     * from 3GPP TS 23.038 V4.3.0 (2001-09) 5 section
     * CBS Data Coding Scheme
     */
    enum GsmCbMsgLangType {
        CB_LANG_GERMAN = 0x00,
        CB_LANG_ENGLISH = 0x01,
        CB_LANG_ITALIAN = 0x02,
        CB_LANG_FRENCH = 0x03,
        CB_LANG_SPANISH = 0x04,
        CB_LANG_DUTCH = 0x05,
        CB_LANG_SWEDISH = 0x06,
        CB_LANG_DANISH = 0x07,
        CB_LANG_PORTUGUESE = 0x08,
        CB_LANG_FINNISH = 0x09,
        CB_LANG_NORWEGIAN = 0x0a,
        CB_LANG_GREEK = 0x0b,
        CB_LANG_TURKISH = 0x0c,
        CB_LANG_HUNGARIAN = 0x0d,
        CB_LANG_POLISH = 0x0e,
        CB_MSG_UNSPECIFIED = 0x0f,

        CB_LANG_ISO639 = 0x10,

        CB_LANG_CZECH = 0x20,
        CB_LANG_HEBREW = 0x21,
        CB_LANG_ARABIC = 0x22,
        CB_LANG_RUSSIAN = 0x23,
        CB_LANG_ICELANDIC = 0x24,
        CB_LANG_RESERVED_25 = 0x25,
        CB_LANG_RESERVED_26 = 0x26,
        CB_LANG_RESERVED_27 = 0x27,
        CB_LANG_RESERVED_28 = 0x28,
        CB_LANG_RESERVED_29 = 0x29,
        CB_LANG_RESERVED_2A = 0x2a,
        CB_LANG_RESERVED_2B = 0x2b,
        CB_LANG_RESERVED_2C = 0x2c,
        CB_LANG_RESERVED_2D = 0x2d,
        CB_LANG_RESERVED_2E = 0x2e,
        CB_LANG_RESERVED_2F = 0x2f,

        CB_LANG_DUMMY = 0xFF
    };

    enum GSMCbMsgSubType {
        /* SMS Specific Message Type */
        NORMAL_MSG = 0, /** Text SMS message */
        CB_SMS, /** Cell Broadcasting  message */
        JAVACB_SMS, /** JAVA Cell Broadcasting  message */

        CMAS_PRESIDENTIAL, /** CMAS CLASS */
        CMAS_EXTREME,
        CMAS_SEVERE,
        CMAS_AMBER,
        CMAS_TEST,
        CMAS_EXERCISE,
        CMAS_OPERATOR_DEFINED,
    };

    /**
     * CMAS(Commercial Mobile Alert System) 4370-4399 (0x1112-0x112F)
     */
    enum CmasMsgType : unsigned short {
        PRESIDENTIAL = 4370,
        PRESIDENTIAL_SPANISH = 4383,

        EXTREME_OBSERVED = 4371,
        EXTREME_OBSERVED_SPANISH = 4384,
        EXTREME_LIKELY = 4372,
        EXTREME_LIKELY_SPANISH = 4385,

        SEVERE_OBSERVED = 4373,
        SEVERE_OBSERVED_SPANISH = 4386,
        SEVERE_LIKELY = 4374,
        SEVERE_LIKELY_SPANISH = 4387,
        ALERT_OBSERVED_DEFUALT = 4375,
        ALERT_OBSERVED_SPANISH = 4388,
        ALERT_LIKELY = 4376,
        ALERT_LIKELY_SPANISH = 4389,
        EXPECTED_OBSERVED = 4377,
        EXPECTED_OBSERVED_SPANISH = 4390,
        EXPECTED_LIKELY = 4378,
        EXPECTED_LIKELY_SPANISH = 4391,

        AMBER_ALERT = 4379,
        AMBER_ALERT_SPANISH = 4392,
        MONTHLY_ALERT = 4380,
        MONTHLY_ALERT_SPANISH = 4393,
        EXERCISE_ALERT = 4381,
        EXERCISE_ALERT_SPANISH = 4394,

        OPERATOR_ALERT = 4382,
        OPERATOR_ALERT_SPANISH = 4395,
    };

    enum SmsCbCodingGroupType {
        SMS_CBMSG_CODGRP_GENERAL_DCS, /** Bits 7..4 00xx */
        SMS_CBMSG_CODGRP_WAP, /** 1110 Cell Broadcast */
        SMS_CBMSG_CODGRP_CLASS_CODING, /** 1111 Cell Broadcast */
    };

    /**
     * from 3GPP TS 23.038 V4.3.0 (2001-09) 5 section
     * CBS Data Coding Scheme Coding Group (bits 7..4)
     */
    enum SmsCbDcsCodingGroup : uint8_t {
        SMS_CBMSG_DCS_CODGRP_GSM7_LANG = 0x00, /** 0000 Language using GSM 7 bit default alphabet */
        SMS_CBMSG_DCS_CODGRP_ISO639 = 0x01, /** 0001 ISO 639 language code in next 2 octets */
        SMS_CBMSG_DCS_CODGRP_GSM7_LANG2 = 0x02, /** 0010 Language using GSM 7 bit default alphabet */
        SMS_CBMSG_DCS_CODGRP_GSM7_LANG3 = 0x03, /** 0011 Language using GSM 7 bit default alphabet */
    };

    enum GsmCbLangType : unsigned short {
        LANG_UNKNOWN = 0x00,
        LANG_ENGLISH = 0x01,
        LANG_FRENCH = 0x02,
        LANG_SPANISH = 0x03,
        LANG_JAPANESE = 0x04,
        LANG_KOREAN = 0x05,
        LANG_CHINESE = 0x06,
        LANG_HEBREW = 0x07,
    };

    /**
     * from 3GPP TS 23.041 V4.1.0 (2001-06) 9.4.1.2.1 section
     * Serial Number
     */
    struct GsmCBMsgSerialNum {
        uint8_t geoScope = 0;
        uint8_t updateNum = 0;
        unsigned short msgCode = 0;
    };

    /**
     * from 3GPP TS 23.038 V4.3.0 (2001-09) 5 section
     * CBS Data Coding Scheme
     */
    struct GsmCbMsgDcs {
        uint8_t codingGroup = 0; /**  Coding group, GSM 03.38 */
        uint8_t classType = 0; /** The message class */
        bool bCompressed = false; /** if text is compressed this is TRUE */
        uint8_t codingScheme = 0; /** How to encode a message. */
        uint8_t langType = 0;
        uint8_t iso639Lang[GsmCbCodec::CB_IOS639_LANG_SIZE] = { 0 }; /* 2 GSM chars and a CR char */
        bool bUDH = false;
        uint8_t rawData = 0;
    };

    /**
     * from 3GPP TS 23.041 V4.1.0 (2001-06) 9.4.1.2 section
     * Message Parameter
     */
    struct GsmCbMessageHeader {
        bool bEtwsMessage;
        EtwsCBType cbEtwsType;
        CBNetType cbNetType;
        uint8_t cbMsgType;
        unsigned short warningType;
        bool bEtwsEmergencyUserAlert = false; /** Emergency User Alert bit of the ETWS primary Warning-Type */
        bool bEtwsPopup = false; /** Popup bit of the ETWS primary Warning-Type */
        GsmCBMsgSerialNum serialNum;
        unsigned short msgId;
        uint8_t langType;
        GsmCbMsgDcs dcs;
        uint8_t page;
        uint8_t totalPages;
        time_t recvTime;

        bool operator==(const GsmCbMessageHeader &other) const
        {
            return serialNum.geoScope == other.serialNum.geoScope && serialNum.msgCode == other.serialNum.msgCode &&
                   msgId == other.msgId;
        }
    };
    GsmCbCodec() = default;
    ~GsmCbCodec() = default;
    bool operator==(const GsmCbCodec &other) const;
    static std::shared_ptr<GsmCbCodec> CreateCbMessage(const std::string &pdu);
    static std::shared_ptr<GsmCbCodec> CreateCbMessage(const std::vector<uint8_t> &pdu);
    std::shared_ptr<GsmCbMessageHeader> GetCbHeader() const;
    void ConvertToUTF8(const std::string &raw, std::string &message) const;
    void DecodeCbMsgDCS(const uint8_t dcsData, const unsigned short iosData, GsmCbMsgDcs &dcs) const;
    std::string GetCbMessageRaw() const;
    unsigned short EncodeCbSerialNum(const GsmCBMsgSerialNum &snFields);
    bool GetFormat(int8_t &cbFormat) const;
    bool GetPriority(int8_t &cbPriority) const;
    bool GetGeoScope(uint8_t &geoScope) const;
    bool GetSerialNum(uint16_t &cbSerial) const;
    bool GetServiceCategory(uint16_t &cbCategoty) const;
    bool GetWarningType(uint16_t &type) const;
    bool GetCmasSeverity(uint8_t &severity) const;
    bool GetCmasUrgency(uint8_t &urgency) const;
    bool GetCmasCertainty(uint8_t &certainty) const;
    bool GetCmasCategory(uint8_t &cmasCategory) const;
    bool GetCmasResponseType(uint8_t &cmasRes) const;
    bool GetMessageId(uint16_t &msgId) const;
    bool GetCmasMessageClass(uint8_t &cmasClass) const;
    bool GetMsgType(uint8_t &msgType) const;
    bool GetLangType(uint8_t &lan) const;
    bool GetDcs(uint8_t &dcs) const;
    bool GetLanguageCode(std::string &languageCode) const;
    bool GetCodingGroup(uint8_t &codingGroup) const;
    bool GetReceiveTime(int64_t &receiveTime) const;
    int64_t GetRecvTime() const;
    void GetPduData(std::vector<uint8_t> &dataPdu);
    void PduAfterDiscardNullBlock(const std::vector<unsigned char> &pdu);

    bool IsEtwsPrimary(bool &primary) const;
    bool IsEtwsMessage(bool &etws) const;
    bool IsCmasMessage(bool &cmas) const;
    bool IsEtwsEmergencyUserAlert(bool &alert) const;
    bool IsEtwsPopupAlert(bool &alert) const;
    bool IsSinglePageMsg() const;
    void SetCbMessageRaw(std::string &raw);
    std::string ToString() const;

private:
    static const std::unordered_map<uint8_t, std::string> &GetGsm7BitLangMap();
    bool PduAnalysis(const std::vector<uint8_t> &pdu);
    void DecodeIos639Dcs(const uint8_t dcsData, const unsigned short iosData, GsmCbMsgDcs &dcs) const;
    void DecodeGeneralDcs(const uint8_t dcsData, GsmCbMsgDcs &dcs) const;
    uint8_t CMASClass(const uint16_t messageId) const;
    bool ParamsCheck(const std::vector<uint8_t> &pdu);

private:
    std::shared_ptr<GsmCbMessageHeader> cbHeader_ { nullptr };
    std::string messageRaw_;
    std::shared_ptr<GsmCbPduDecodeBuffer> cbPduBuffer_ { nullptr };
};
} // namespace Telephony
} // namespace OHOS
#endif // GSM_CB_CODEC_H