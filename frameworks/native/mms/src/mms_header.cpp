/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "mms_header.h"

#include <chrono>
#include <cinttypes>
#include <iostream>
#include <utility>
#include <random>

#include "telephony_log_wrapper.h"
#include "mms_charset.h"

namespace OHOS {
namespace Telephony {
using namespace std;
MmsHeader::MmsHeader()
{
    InitOctetHandleFun();
    InitLongHandleFun();
    InitStringHandleFun();
    InitTextStringHandleFun();
}

void MmsHeader::InitOctetHandleFun()
{
    auto func = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldOctetValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_DELIVERY_REPORT] = func; // MmsBoolType
    memberFuncMap_[MMS_READ_REPORT] = func;
    memberFuncMap_[MMS_REPORT_ALLOWED] = func;
    memberFuncMap_[MMS_STORE] = func;
    memberFuncMap_[MMS_STORED] = func;
    memberFuncMap_[MMS_TOTALS] = func;
    memberFuncMap_[MMS_QUOTAS] = func;
    memberFuncMap_[MMS_DISTRIBUTION_INDICATOR] = func;
    memberFuncMap_[MMS_ADAPTATION_ALLOWED] = func;
    memberFuncMap_[MMS_MESSAGE_CLASS] = func;
    memberFuncMap_[MMS_PRIORITY] = func; // MmsPriority
    memberFuncMap_[MMS_RESPONSE_STATUS] = func; // MmsResponseStatus
    memberFuncMap_[MMS_SENDER_VISIBILITY] = func; // MmsSenderVisibilityType
    memberFuncMap_[MMS_STATUS] = func; // MmsStatus
    memberFuncMap_[MMS_RETRIEVE_STATUS] = func; // MmsRetrieveStatus
    memberFuncMap_[MMS_READ_STATUS] = func; // MmsReadStatus
    memberFuncMap_[MMS_REPLY_CHARGING] = func; // MmsReplyCharging
    memberFuncMap_[MMS_MM_STATE] = func; // MmsMmState
    memberFuncMap_[MMS_MM_FLAGS] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldMMFlag(fieldId, buff, len);
    }; // MmsMmFlags
    memberFuncMap_[MMS_STORE_STATUS] = func; // MmsStoreStatus
    memberFuncMap_[MMS_MBOX_TOTALS] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldMBox(fieldId, buff, len);
    }; // MmsMboxTotals
    memberFuncMap_[MMS_MBOX_QUOTAS] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldMBox(fieldId, buff, len);
    }; // MmsMboxQuotas
    memberFuncMap_[MMS_RECOMMENDED_RETRIEVAL_MODE] = func; // MmsRecommendedRetrievalMode
    memberFuncMap_[MMS_CONTENT_CLASS] = func; // MmsContentClass
    memberFuncMap_[MMS_CANCEL_STATUS] = func; // MmsCancelStatus
    memberFuncMap_[MMS_MESSAGE_TYPE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeMmsMsgType(fieldId, buff, len);
    };
    memberFuncMap_[MMS_ATTRIBUTES] = func;
    memberFuncMap_[MMS_DRM_CONTENT] = func;
}

void MmsHeader::InitLongHandleFun()
{
    memberFuncMap_[MMS_CONTENT_TYPE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeMmsContentType(fieldId, buff, len);
    };
    memberFuncMap_[MMS_ELEMENT_DESCRIPTOR] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeMmsContentType(fieldId, buff, len);
    };
    memberFuncMap_[MMS_DATE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldLongValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_REPLY_CHARGING_SIZE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldLongValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_MESSAGE_SIZE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldLongValue(fieldId, buff, len);
    };

    memberFuncMap_[MMS_MESSAGE_COUNT] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldIntegerValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_START] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldIntegerValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_LIMIT] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldIntegerValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_MMS_VERSION] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldIntegerValue(fieldId, buff, len);
    };

    memberFuncMap_[MMS_DELIVERY_TIME] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldDate(fieldId, buff, len);
    };
    memberFuncMap_[MMS_EXPIRY] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldDate(fieldId, buff, len);
    };
    memberFuncMap_[MMS_REPLY_CHARGING_DEADLINE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldDate(fieldId, buff, len);
    }; // MmsReplyChargingDeadline
    memberFuncMap_[MMS_PREVIOUSLY_SENT_DATE] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldPreviouslySentDate(fieldId, buff, len);
    };
}

void MmsHeader::InitStringHandleFun()
{
    auto func = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldEncodedStringValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_RESPONSE_TEXT] = func;
    memberFuncMap_[MMS_SUBJECT] = func;
    memberFuncMap_[MMS_RETRIEVE_TEXT] = func;
    memberFuncMap_[MMS_PREVIOUSLY_SENT_BY] = func;

    memberFuncMap_[MMS_STORE_STATUS_TEXT] = func;
    memberFuncMap_[MMS_RECOMMENDED_RETRIEVAL_MODE_TEXT] = func;
    memberFuncMap_[MMS_STATUS_TEXT] = func;

    memberFuncMap_[MMS_BCC] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldAddressModelValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_CC] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldAddressModelValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_TO] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldAddressModelValue(fieldId, buff, len);
    };

    memberFuncMap_[MMS_FROM] = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFromValue(fieldId, buff, len);
    };
}

void MmsHeader::InitTextStringHandleFun()
{
    auto func = [this](uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len) {
        return DecodeFieldTextStringValue(fieldId, buff, len);
    };
    memberFuncMap_[MMS_CONTENT_LOCATION] = func;
    memberFuncMap_[MMS_TRANSACTION_ID] = func;
    memberFuncMap_[MMS_MESSAGE_ID] = func;
    memberFuncMap_[MMS_REPLACE_ID] = func;
    memberFuncMap_[MMS_CANCEL_ID] = func;
    memberFuncMap_[MMS_APPLIC_ID] = func;
    memberFuncMap_[MMS_REPLY_APPLIC_ID] = func;
    memberFuncMap_[MMS_REPLY_CHARGING_ID] = func;
    memberFuncMap_[MMS_AUX_APPLIC_INFO] = func;
}

void MmsHeader::DumpMmsHeader()
{
    bool find = false;
    std::string fieldName;
    for (auto it = octetValueMap_.begin(); it != octetValueMap_.end(); it++) {
        find = FindHeaderFieldName(it->first, fieldName);
        TELEPHONY_LOGI("%{private}s : 0x%{private}02X", find ? fieldName.c_str() : "unkown field", it->second);
    }

    for (auto it = longValueMap_.begin(); it != longValueMap_.end(); it++) {
        find = FindHeaderFieldName(it->first, fieldName);
        TELEPHONY_LOGI("%{private}s : %{private}" PRId64 "", find ? fieldName.c_str() : "unkown field", it->second);
    }

    for (auto it = textValueMap_.begin(); it != textValueMap_.end(); it++) {
        find = FindHeaderFieldName(it->first, fieldName);
        TELEPHONY_LOGI("%{private}s : %{private}s", find ? fieldName.c_str() : "unkown field", it->second.c_str());
    }

    for (auto it = encodeStringsMap_.begin(); it != encodeStringsMap_.end(); it++) {
        find = FindHeaderFieldName(it->first, fieldName);
        std::string enString;
        if (find) {
            it->second.GetEncodeString(enString);
        }
        TELEPHONY_LOGI("%{private}s : %{private}s", find ? fieldName.c_str() : "unkown field", enString.c_str());
    }

    for (auto it = addressMap_.begin(); it != addressMap_.end(); it++) {
        find = FindHeaderFieldName(it->first, fieldName);
        std::vector<MmsAddress> address = it->second;
        for (auto itAddress : address) {
            std::string enString = itAddress.GetAddressString();
            TELEPHONY_LOGI("%{private}s : %{private}s", find ? fieldName.c_str() : "unkown field", enString.c_str());
        }
    }
}

bool MmsHeader::DecodeMmsHeader(MmsDecodeBuffer &decodeBuffer)
{
    const uint8_t setHighestBitOne = 0x80;
    uint8_t fieldCode = 0xff;
    uint8_t oneByte = 0;
    int32_t len = 0;
    while (decodeBuffer.GetOneByte(oneByte)) {
        fieldCode = (oneByte | setHighestBitOne);
        auto decodeFunc = memberFuncMap_.find(fieldCode);
        if (decodeFunc != memberFuncMap_.end()) {
            auto fun = decodeFunc->second;
            if (fun != nullptr && !fun(fieldCode, decodeBuffer, len)) {
                TELEPHONY_LOGE("The fieldId[%{public}d] decode header fail.", fieldCode);
                return false;
            }
            if (fieldCode == MMS_CONTENT_TYPE) {
                break;
            }
        } else {
            TELEPHONY_LOGI("DecodeMmsMsgUnKnownField:%{public}02X", fieldCode);
            DecodeMmsMsgUnKnownField(decodeBuffer);
        }
    }
    return true;
}

bool MmsHeader::EncodeMmsHeader(MmsEncodeBuffer &encodeBuffer)
{
    uint8_t messageType = 0;
    const uint8_t defTranScactionIdLen = 24;
    const int64_t defVersion = static_cast<int64_t>(MmsVersionType::MMS_VERSION_1_3);
    if (!GetOctetValue(MMS_MESSAGE_TYPE, messageType)) {
        TELEPHONY_LOGE("Not Set Mms Message Type Error.");
        return false;
    }

    std::string transactionId = "";
    if (!GetStringValue(MMS_TRANSACTION_ID, transactionId) && IsHaveTransactionId(messageType)) {
        SetTextValue(MMS_TRANSACTION_ID, MakeTransactionId(defTranScactionIdLen));
    }

    int64_t mmsVersion = 0;
    if (!GetLongValue(MMS_MMS_VERSION, mmsVersion)) {
        mmsVersion = defVersion;
        SetLongValue(MMS_MMS_VERSION, mmsVersion);
    }
    if (mmsVersion < static_cast<int64_t>(MmsVersionType::MMS_VERSION_1_0) ||
        mmsVersion > static_cast<int64_t>(MmsVersionType::MMS_VERSION_1_3)) {
        SetLongValue(MMS_MMS_VERSION, defVersion);
    }

    if (!EncodeCommontFieldValue(encodeBuffer)) {
        TELEPHONY_LOGE("Encode Commont Header Field Err.");
        return false;
    }
    switch (messageType) {
        case MMS_MSGTYPE_SEND_REQ:
            return EncodeMmsSendReq(encodeBuffer);
        case MMS_MSGTYPE_SEND_CONF:
            return EncodeMmsSendConf(encodeBuffer);
        case MMS_MSGTYPE_NOTIFICATION_IND:
            return EncodeMmsNotificationInd(encodeBuffer);
        case MMS_MSGTYPE_NOTIFYRESP_IND:
            return EnocdeMmsNotifyRespInd(encodeBuffer);
        case MMS_MSGTYPE_RETRIEVE_CONF:
            return EnocdeMmsRetrieveConf(encodeBuffer);
        case MMS_MSGTYPE_ACKNOWLEDGE_IND:
            return EnocdeMmsAcknowledgeInd(encodeBuffer);
        case MMS_MSGTYPE_DELIVERY_IND:
            return EnocdeMmsDeliveryInd(encodeBuffer);
        case MMS_MSGTYPE_READ_REC_IND:
            return EncodeMmsReadRecInd(encodeBuffer);
        case MMS_MSGTYPE_READ_ORIG_IND:
            return EncodeMmsReadOrigInd(encodeBuffer);
        default:
            TELEPHONY_LOGE("Mms Message Type unSupported Err.");
            return false;
    }
}

bool MmsHeader::GetOctetValue(uint8_t fieldId, uint8_t &value) const
{
    value = 0;
    auto it = octetValueMap_.find(fieldId);
    if (it != octetValueMap_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool MmsHeader::SetOctetValue(uint8_t fieldId, uint8_t value)
{
    if (mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_OCTET_TYPE) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] is not belong to octet.", fieldId);
        return false;
    }
    if (CheckBooleanValue(fieldId, value)) {
        return true;
    }
    bool match = false;
    if (!SetOctetValuePartData(fieldId, value, match)) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] value invalid.", fieldId);
        return false;
    }

    switch (fieldId) {
        case MMS_CONTENT_CLASS:
            if (value < static_cast<uint8_t>(MmsContentClass::MMS_TEXT) ||
                value > static_cast<uint8_t>(MmsContentClass::MMS_CONTENT_RICH)) {
                TELEPHONY_LOGE("The MMS_CONTENT_CLASS value invalid.");
                return false;
            }
            break;
        case MMS_CANCEL_STATUS:
            if (value < static_cast<uint8_t>(MmsCancelStatus::MMS_CANCEL_REQUEST_SUCCESSFULLY_RECEIVED) ||
                value > static_cast<uint8_t>(MmsCancelStatus::MMS_CANCEL_REQUEST_CORRUPTED)) {
                TELEPHONY_LOGE("The MMS_CANCEL_STATUS value invalid.");
                return false;
            }
            break;
        case MMS_MESSAGE_CLASS:
            if (value < static_cast<uint8_t>(MmsMessageClass::PERSONAL) ||
                value > static_cast<uint8_t>(MmsMessageClass::AUTO)) {
                TELEPHONY_LOGE("The MMS_MESSAGE_CLASS value invalid.");
                return false;
            }
            break;
        default:
            if (!match) {
                TELEPHONY_LOGE("The 111fieldId[%{public}02X] value invalid.", fieldId);
                return false;
            }
    }
    octetValueMap_[fieldId] = value;
    return true;
}

bool MmsHeader::SetOctetValuePartData(uint8_t fieldId, uint8_t value, bool &match)
{
    if (!SetOctetValuePortionData(fieldId, value, match)) {
        return false;
    }
    if (match) {
        return match;
    }
    switch (fieldId) {
        case MMS_READ_STATUS:
            if (value < static_cast<uint8_t>(MmsReadStatus::MMS_READ) ||
                value > static_cast<uint8_t>(MmsReadStatus::MMS_DELETED_WITHOUT_BEING_READ)) {
                TELEPHONY_LOGE("The MMS_READ_STATUS value invalid.");
                return false;
            }
            break;
        case MMS_REPLY_CHARGING:
            if (value < static_cast<uint8_t>(MmsReplyCharging::MMS_REQUESTED) ||
                value > static_cast<uint8_t>(MmsReplyCharging::MMS_ACCEPTED_TEXT_ONLY)) {
                TELEPHONY_LOGE("The MMS_REPLY_CHARGING value invalid.");
                return false;
            }
            break;
        case MMS_MM_STATE:
            if (value < static_cast<uint8_t>(MmsMmState::MMS_MM_STATE_DRAFT) ||
                value > static_cast<uint8_t>(MmsMmState::MMS_MM_STATE_FORWARDED)) {
                TELEPHONY_LOGE("The MMS_MM_STATE value invalid.");
                return false;
            }
            break;
        case MMS_MM_FLAGS:
            if (value < static_cast<uint8_t>(MmsMmFlags::MMS_ADD_TOKEN) ||
                value > static_cast<uint8_t>(MmsMmFlags::MMS_FILTER_TOKEN)) {
                TELEPHONY_LOGE("The MMS_MM_FLAGS value invalid.");
                return false;
            }
            break;
        case MMS_STORE_STATUS:
            if (!CheckStoreStatus(value)) {
                TELEPHONY_LOGE("The MMS_STORE_STATUS value invalid.");
                return false;
            }
            break;
        default:
            match = false;
            return true;
    }
    return true;
}

bool MmsHeader::SetOctetValuePortionData(uint8_t fieldId, uint8_t value, bool &match)
{
    match = true;
    switch (fieldId) {
        case MMS_MESSAGE_TYPE:
            if (value < static_cast<uint8_t>(MmsMsgType::MMS_MSGTYPE_SEND_REQ) ||
                value > static_cast<uint8_t>(MmsMsgType::MMS_MSGTYPE_FORWARD_CONF)) {
                TELEPHONY_LOGE("The MMS_MESSAGE_TYPE value invalid.");
                return false;
            }
            break;
        case MMS_PRIORITY:
            if (value < static_cast<uint8_t>(MmsPriority::MMS_LOW) ||
                value > static_cast<uint8_t>(MmsPriority::MMS_HIGH)) {
                TELEPHONY_LOGE("The MMS_PRIORITY value invalid.");
                return false;
            }
            break;
        case MMS_RESPONSE_STATUS:
            if (!CheckResponseStatus(value)) {
                TELEPHONY_LOGE("The MMS_RESPONSE_STATUS value invalid.");
                return false;
            }
            break;
        case MMS_SENDER_VISIBILITY:
            if (value < static_cast<uint8_t>(MmsSenderVisibility::MMS_HIDE) ||
                value > static_cast<uint8_t>(MmsSenderVisibility::MMS_SHOW)) {
                TELEPHONY_LOGE("The MMS_SENDER_VISIBILITY value invalid.");
                return false;
            }
            break;
        case MMS_STATUS:
            if (value < static_cast<uint8_t>(MmsStatus::MMS_EXPIRED) ||
                value > static_cast<uint8_t>(MmsStatus::MMS_UNREACHABLE)) {
                TELEPHONY_LOGE("The MMS_STATUS value invalid.");
                return false;
            }
            break;
        case MMS_RETRIEVE_STATUS:
            if (!CheckRetrieveStatus(value)) {
                TELEPHONY_LOGE("The MMS_RETRIEVE_STATUS value invalid.");
                return false;
            }
            break;
        default:
            match = false;
            return true;
    }
    return true;
}

bool MmsHeader::GetLongValue(uint8_t fieldId, int64_t &value) const
{
    value = 0;
    auto it = longValueMap_.find(fieldId);
    if (it != longValueMap_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool MmsHeader::SetLongValue(uint8_t fieldId, int64_t value)
{
    if (mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_INTEGER_TYPE &&
        mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_LONG_TYPE) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] value is not belong to LongValue.", fieldId);
        return false;
    }
    longValueMap_[fieldId] = value;
    return true;
}

bool MmsHeader::GetTextValue(uint8_t fieldId, std::string &value) const
{
    value.clear();
    auto it = textValueMap_.find(fieldId);
    if (it != textValueMap_.end()) {
        value.assign(it->second);
        return true;
    }
    return false;
}

bool MmsHeader::SetTextValue(uint8_t fieldId, std::string value)
{
    if (value.empty()) {
        TELEPHONY_LOGE("fieldId[%{public}d] The Value Is Empty Error.", fieldId);
        return false;
    }
    if (mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_TEXT_TYPE) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] value is not belong to TextValue.", fieldId);
        return false;
    }
    textValueMap_[fieldId] = value;
    return true;
}

bool MmsHeader::GetEncodeStringValue(uint8_t fieldId, MmsEncodeString &value) const
{
    auto it = encodeStringsMap_.find(fieldId);
    if (it != encodeStringsMap_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool MmsHeader::SetEncodeStringValue(uint8_t fieldId, uint32_t charset, const std::string &value)
{
    if (value.empty()) {
        TELEPHONY_LOGE("fieldId[%{public}d] The Value Is Empty Error.", fieldId);
        return false;
    }
    if (mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_ENCODE_TEXT_TYPE) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] value is not belong to EncodeString.", fieldId);
        return false;
    }
    MmsEncodeString encodeString;
    if (!encodeString.SetEncodeString(charset, value)) {
        TELEPHONY_LOGE("MmsHeader SetEncodeString fail.");
        return false;
    }
    encodeStringsMap_[fieldId] = encodeString;
    return true;
}

bool MmsHeader::AddHeaderAddressValue(uint8_t fieldId, MmsAddress &address)
{
    std::string strAddress = address.GetAddressString();
    if (strAddress.empty() && fieldId != MMS_FROM) {
        TELEPHONY_LOGE("Address is empty error.");
        return false;
    }
    if (mmsHaderCateg_.FindFieldDes(fieldId) != MmsFieldValueType::MMS_FIELD_ENCODE_ADDR_TYPE) {
        TELEPHONY_LOGE("The fieldId[%{public}02X] value is not belong to EncodeString.", fieldId);
        return false;
    }
    auto it = addressMap_.find(fieldId);
    if (it != addressMap_.end()) {
        it->second.push_back(address);
        return true;
    } else {
        std::vector<MmsAddress> addressValue;
        addressValue.push_back(address);
        auto ret = addressMap_.emplace(fieldId, addressValue);
        return ret.second;
    }
}

bool MmsHeader::GetHeaderAllAddressValue(uint8_t fieldId, std::vector<MmsAddress> &addressValue)
{
    addressValue.clear();
    auto it = addressMap_.find(fieldId);
    if (it != addressMap_.end()) {
        addressValue.assign(it->second.begin(), it->second.end());
        return true;
    }
    return false;
}

bool MmsHeader::GetStringValue(uint8_t fieldId, std::string &value) const
{
    if (GetTextValue(fieldId, value)) {
        return true;
    } else {
        value.clear();
        MmsEncodeString eValue;
        if (!GetEncodeStringValue(fieldId, eValue)) {
            TELEPHONY_LOGE("The fieldId[%{public}d] GetEncodeStringValue fail.", fieldId);
            return false;
        }
        eValue.GetEncodeString(value);
    }
    return true;
}

MmsContentType &MmsHeader::GetHeaderContentType()
{
    return mmsContentType_;
}

bool MmsHeader::FindHeaderFieldName(const uint8_t fieldId, std::string &fieldName)
{
    for (unsigned int i = 0; i < sizeof(mmsHeaderNames) / sizeof(mmsHeaderNames[0]); i++) {
        if (fieldId == mmsHeaderNames[i].key) {
            fieldName.clear();
            fieldName.append(mmsHeaderNames[i].value);
            return true;
        }
    }
    return false;
}

bool MmsHeader::DecodeMmsMsgType(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader FindHeaderFieldName fail.", fieldId);
        return false;
    }

    uint8_t oneByte = 0;
    if (!buff.GetOneByte(oneByte)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }
    if (oneByte < MMS_MSGTYPE_SEND_REQ || oneByte > MMS_MSGTYPE_CANCEL_CONF) {
        TELEPHONY_LOGE("MmsHeader message type not support.");
        return false;
    }
    octetValueMap_[fieldId] = oneByte;
    return true;
}

/**
 * @brief DecodeFieldAddressModelValue
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.21 From Field
 * From-value = Value-length (Address-present-token Encoded-string-value | Insert-address-token)
 * Address-present-token = <Octet 128>
 * Insert-address-token = <Octet 129>
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFieldAddressModelValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader FindHeaderFieldName fail.", fieldId);
        return false;
    }

    MmsEncodeString encodeString;
    encodeString.DecodeEncodeString(buff);

    std::string encodeAddress;
    encodeString.GetEncodeString(encodeAddress);
    MmsAddress address(encodeAddress);

    std::vector<MmsAddress> vecAddress;
    auto it = addressMap_.find(fieldId);
    if (it != addressMap_.end()) {
        vecAddress = it->second;
        it->second.push_back(address);
    } else {
        vecAddress.push_back(address);
        addressMap_.emplace(fieldId, vecAddress);
    }
    return true;
}

bool MmsHeader::DecodeFieldOctetValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader FindHeaderFieldName fail.", fieldId);
        return false;
    }
    uint8_t oneByte = 0;
    if (!buff.GetOneByte(oneByte)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }
    octetValueMap_[fieldId] = oneByte;
    return true;
}

bool MmsHeader::DecodeFieldLongValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    uint64_t value = 0;
    if (!buff.DecodeLongInteger(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeLongInteger fail.", fieldId);
        return false;
    }
    longValueMap_[fieldId] = static_cast<int64_t>value;
    return true;
}

std::string MmsHeader::MakeTransactionId(uint32_t len)
{
    const uint32_t englishLettersNum = 25;
    const uint32_t digitalNum = 9;
    const uint32_t typeNum = 2;

    std::default_random_engine e;
    std::string transactionId = "";
    std::uniform_int_distribution<unsigned> typeNumRandom(0, typeNum);
    std::uniform_int_distribution<unsigned> digitalNumRandom(0, digitalNum);
    std::uniform_int_distribution<unsigned> englishLettersNumRandom(0, englishLettersNum);
    for (uint32_t i = 0; i < len; i++) {
        switch (typeNumRandom(e)) {
            case 0:
                transactionId += 'A' + (englishLettersNumRandom(e));
                break;
            case 1:
                transactionId += '0' + (digitalNumRandom(e));
                break;
            default:
                transactionId += 'a' + (englishLettersNumRandom(e));
        }
    }
    return transactionId;
}

bool MmsHeader::DecodeFieldTextStringValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    std::string tempString;
    uint32_t tempLen = 0;
    if (!buff.DecodeText(tempString, tempLen)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeText fail.", fieldId);
        return false;
    }

    len = (int32_t)tempLen;
    textValueMap_[fieldId] = tempString;
    return true;
}

bool MmsHeader::DecodeFieldEncodedStringValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    MmsEncodeString encodeString;
    if (!encodeString.DecodeEncodeString(buff)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeEncodeString fail.", fieldId);
        return false;
    }
    encodeStringsMap_[fieldId] = encodeString;
    return true;
}

/**
 * @brief DecodeFromValue
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.21 From Field
 * From-value = Value-length (Address-present-token Encoded-string-value | Insert-address-token)
 * Address-present-token = <Octet 128>
 * Insert-address-token = <Octet 129>
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFromValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    // Value-length (Address-present-token Encoded-string-value | Insert-address-token)
    const int addressPresentToken = 128;
    const int insertAddressToken = 129;
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    uint32_t valueLength = 0;
    if (!buff.DecodeValueLength(valueLength)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeValueLength fail.", fieldId);
        return false;
    }

    uint8_t oneByte = 0;
    if (!buff.GetOneByte(oneByte)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }

    if (oneByte == addressPresentToken) {
        // Address-present-token
        MmsEncodeString encodeString;
        encodeString.DecodeEncodeString(buff);

        std::string encodeAddress;
        encodeString.GetEncodeString(encodeAddress);
        if (encodeAddress.empty()) {
            encodeAddress = encodeString.GetStrEncodeString();
        }
        MmsAddress address(encodeAddress);

        std::vector<MmsAddress> vecAddress;
        vecAddress.push_back(address);
        addressMap_.emplace(fieldId, vecAddress);
        return true;
    } else if (oneByte == insertAddressToken) {
        // Insert-address-token
        MmsAddress address("Insert-address-token");
        std::vector<MmsAddress> vecAddress;
        vecAddress.push_back(address);
        addressMap_.emplace(fieldId, vecAddress);
        return true;
    }
    return false;
}

bool MmsHeader::GetSmilFileName(std::string &smileFileName)
{
    smileFileName = "";
    const uint8_t startValue = static_cast<uint8_t>(ContentParam::CT_P_START_VALUE);
    const uint8_t paramStart = static_cast<uint8_t>(ContentParam::CT_P_START);
    uint8_t fieldIds[] = {startValue, paramStart};

    for (unsigned int i = 0; i < sizeof(fieldIds); i++) {
        auto start = mmsContentType_.GetContentParam().GetParamMap().find(fieldIds[i]);
        if (start != mmsContentType_.GetContentParam().GetParamMap().end()) {
            smileFileName = start->second;
            return true;
        }
    }
    return true;
}

bool MmsHeader::DecodeMmsContentType(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader FindHeaderFieldName fail.", fieldId);
        return false;
    }
    if (mmsContentType_.DecodeMmsContentType(buff, len)) {
        return true;
    }
    return false;
}

bool MmsHeader::DecodeMmsMsgUnKnownField(MmsDecodeBuffer &decodeBuffer)
{
    const uint8_t minFieldValue = 0x80;
    uint8_t oneByte = 0;
    while (oneByte < minFieldValue) {
        if (!decodeBuffer.GetOneByte(oneByte)) {
            TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
            return false;
        }
    }
    decodeBuffer.DecreasePointer(1);
    return true;
}

bool MmsHeader::DecodeFieldIntegerValue(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    uint64_t value = 0;
    if (!buff.DecodeInteger(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeInteger fail.", fieldId);
        return false;
    }
    longValueMap_[fieldId] = static_cast<int64_t>value;
    return true;
}

/**
 * @brief DecodeFieldDate
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.14
 * Value-length (Absolute-token Date-value | Relative-token Delta-seconds-value)
 * Absolute-token = <Octet 128>
 * Relative-token = <Octet 129>
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFieldDate(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    const uint8_t relativeToken = 0x81;
    uint32_t length = 0;
    if (!buff.DecodeValueLength(length)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeValueLength fail.", fieldId);
        return false;
    }
    /* Absolute-token or Relative-token */
    uint8_t token = 0;
    if (!buff.GetOneByte(token)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }
    /* Date-value or Delta-seconds-value */
    uint64_t timeValue = 0;
    if (!buff.DecodeLongInteger(timeValue)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeLongInteger fail.", fieldId);
        return false;
    }
    if (relativeToken == token) {
        /* need to convert the Delta-seconds-value
         * into Date-value */
        chrono::system_clock::duration timePoint = chrono::system_clock::now().time_since_epoch();
        long timeStamp = chrono::duration_cast<chrono::seconds>(timePoint).count();
        timeValue += static_cast<uint64_t>(timeStamp);
    }
    longValueMap_[fieldId] = static_cast<int64_t>timeValue;
    return true;
}

/**
 * @brief DecodeFieldPreviouslySentDate
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.24 X-Mms-Previously-Sent-Date Field
 * Previously-sent-date-value = Value-length Forwarded-count-value Date-value
 * Forwarded-count-value = Integer-value
 * Date-value = Long-integer
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFieldPreviouslySentDate(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    /* Previously-sent-date-value =
     * Value-length Forwarded-count-value Date-value */
    /* parse value-length */
    uint32_t length = 0;
    uint64_t count = 0;
    uint64_t perviouslySentDate = 0;
    if (!buff.DecodeValueLength(length)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeValueLength fail.", fieldId);
        return false;
    }
    /* parse Forwarded-count-value */
    if (!buff.DecodeInteger(count)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeInteger fail.", fieldId);
        return false;
    }
    if (!buff.DecodeLongInteger(perviouslySentDate)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeLongInteger fail.", fieldId);
        return false;
    }
    longValueMap_[fieldId] = static_cast<int64_t>perviouslySentDate;
    return true;
}

/**
 * @brief DecodeFieldMBox
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.25
 * Value-length (Message-quota-token | Size-quota-token) Integer-Value
 * Message-quota-token = <Octet 128>
 * Size-quota-token = <Octet 129>
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFieldMBox(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    /* Value-length
     * (Message-total-token | Size-total-token) Integer-Value */
    uint32_t length = 0;
    if (!buff.DecodeValueLength(length)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeValueLength fail.", fieldId);
        return false;
    }
    uint8_t token = 0;
    if (!buff.GetOneByte(token)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }
    uint64_t value = 0;
    if (!buff.DecodeInteger(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeInteger fail.", fieldId);
        return false;
    }
    return true;
}

/**
 * @brief DecodeFieldMMFlag
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.32 X-Mms-MM-Flags Field
 * Value-length ( Add-token | Remove-token | Filter-token ) Encoded-string-value
 * Add-token = <Octet 128>
 * Remove-token = <Octet 129>
 * Filter-token = <Octet 130>
 * @param fieldId
 * @param buff
 * @param len
 * @return true
 * @return false
 */
bool MmsHeader::DecodeFieldMMFlag(uint8_t fieldId, MmsDecodeBuffer &buff, int32_t &len)
{
    std::string fieldName;
    if (!FindHeaderFieldName(fieldId, fieldName)) {
        TELEPHONY_LOGE("MmsHeader FindHeaderFieldName fail.");
        return false;
    }

    uint32_t length = 0;
    if (!buff.DecodeValueLength(length)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeValueLength fail.", fieldId);
        return false;
    }
    uint8_t token = 0;
    if (!buff.GetOneByte(token)) {
        TELEPHONY_LOGE("MmsHeader GetOneByte fail.");
        return false;
    }
    MmsEncodeString encodeString;
    if (!encodeString.DecodeEncodeString(buff)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader DecodeEncodeString fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::IsHaveBody()
{
    uint8_t msgType = 0;
    GetOctetValue(MMS_MESSAGE_TYPE, msgType);
    if (msgType == MMS_MSGTYPE_SEND_REQ || msgType == MMS_MSGTYPE_RETRIEVE_CONF) {
        return true;
    }
    return false;
}

bool MmsHeader::EncodeOctetValue(MmsEncodeBuffer &buff, uint8_t fieldId, uint8_t value)
{
    if (!buff.WriteByte(fieldId)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!buff.WriteByte(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeShortIntegerValue(MmsEncodeBuffer &buff, uint8_t fieldId, int64_t value)
{
    if (!buff.WriteByte(fieldId)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!buff.EncodeShortInteger(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeShortInteger fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeTextStringValue(MmsEncodeBuffer &buff, uint8_t fieldId, std::string value)
{
    if (value.empty()) {
        TELEPHONY_LOGE("fieldId[%{public}d] EncodeTextStringValue Value Empty fail.", fieldId);
        return false;
    }
    if (!buff.WriteByte(fieldId)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!buff.EncodeText(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeText fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeEncodeStringValue(MmsEncodeBuffer &buff, uint8_t fieldId, MmsEncodeString value)
{
    std::string valueUtf8;
    if (!value.GetEncodeString(valueUtf8)) {
        TELEPHONY_LOGE("fieldId[%{public}d] GetEncodeString Error", fieldId);
        return false;
    }
    if (valueUtf8.empty()) {
        TELEPHONY_LOGE("fieldId[%{public}d] GetEncodeString Empty Error", fieldId);
        return false;
    }

    if (!buff.WriteByte(fieldId)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!value.EncodeEncodeString(buff)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeEncodeString fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeLongIntergerValue(MmsEncodeBuffer &buff, uint8_t fieldId, int64_t value)
{
    if (!buff.WriteByte(fieldId)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!buff.EncodeLongInteger(value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeLongInteger fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeOctetValueFromMap(MmsEncodeBuffer &buff, uint8_t fieldId)
{
    uint8_t value = 0;
    if (!GetOctetValue(fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
        return false;
    }
    if (!EncodeOctetValue(buff, fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeOctetValue fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeTextStringValueFromMap(MmsEncodeBuffer &buff, uint8_t fieldId)
{
    std::string value = "";
    if (!GetTextValue(fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] GetTextValue fail.", fieldId);
        return false;
    }
    if (!EncodeTextStringValue(buff, fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeTextStringValue fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EnocdeEncodeStringValueFromMap(MmsEncodeBuffer &buff, uint8_t fieldId)
{
    MmsEncodeString value;
    if (!GetEncodeStringValue(fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader GetEncodeStringValue fail.", fieldId);
        return false;
    }
    if (!EncodeEncodeStringValue(buff, fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeEncodeStringValue fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeLongIntergerValueFromMap(MmsEncodeBuffer &buff, uint8_t fieldId)
{
    int64_t value = 0;
    if (!GetLongValue(fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader GetLongValue fail.", fieldId);
        return false;
    }
    if (!EncodeLongIntergerValue(buff, fieldId, value)) {
        TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeLongIntergerValue fail.", fieldId);
        return false;
    }
    return true;
}

bool MmsHeader::EncodeFieldExpriyValue(MmsEncodeBuffer &buff, int64_t value)
{
    if (!buff.WriteByte(MMS_EXPIRY)) {
        TELEPHONY_LOGE("MmsHeader WriteByte fail.");
        return false;
    }

    const uint8_t relativeToken = 129;
    MmsEncodeBuffer tempBuf;
    if (!tempBuf.EncodeOctet(relativeToken)) {
        TELEPHONY_LOGE("MmsHeader EncodeOctet fail.");
        return false;
    }
    if (!tempBuf.EncodeLongInteger(value)) {
        TELEPHONY_LOGE("MmsHeader EncodeLongInteger fail.");
        return false;
    }
    if (!buff.EncodeValueLength(tempBuf.GetCurPosition())) {
        TELEPHONY_LOGE("MmsHeader EncodeValueLength fail.");
        return false;
    }
    if (!buff.WriteBuffer(tempBuf)) {
        TELEPHONY_LOGE("MmsHeader WriteByte fail.");
        return false;
    }
    return true;
}

/**
 * @brief EncodeFieldFromValue
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:7.3.21 From Field
 * From-value = Value-length (Address-present-token Encoded-string-value | Insert-address-token)
 * Address-present-token = <Octet 128>
 * Insert-address-token = <Octet 129>
 * @param buff
 * @param addr
 * @return true
 * @return false
 */
bool MmsHeader::EncodeFieldFromValue(MmsEncodeBuffer &buff, std::vector<MmsAddress> &addr)
{
    auto it = addressMap_.find(MMS_FROM);
    if (it != addressMap_.end()) {
        if (!buff.WriteByte(MMS_FROM)) {
            TELEPHONY_LOGE("MmsHeader WriteByte fail.");
            return false;
        }
        // Value-length (Address-present-token Encoded-string-value | Insert-address-token)
        const uint8_t addressPresentToken = 128;
        const uint8_t insertAddressToken = 129;
        MmsEncodeBuffer tempBuff;
        if (addr.empty() || addr[0].GetAddressString().empty()) {
            if (!tempBuff.WriteByte(insertAddressToken)) {
                TELEPHONY_LOGE("MmsHeader WriteByte fail.");
                return false;
            }
        } else {
            if (!tempBuff.WriteByte(addressPresentToken)) {
                TELEPHONY_LOGE("MmsHeader WriteByte fail.");
                return false;
            }

            MmsEncodeString encodeString;
            encodeString.SetAddressString(addr[0]);
            if (!encodeString.EncodeEncodeString(tempBuff)) {
                TELEPHONY_LOGE("MmsHeader EncodeEncodeString fail.");
                return false;
            }
        }
        if (!buff.EncodeValueLength(tempBuff.GetCurPosition())) {
            TELEPHONY_LOGE("MmsHeader EncodeValueLength fail.");
            return false;
        }
        if (!buff.WriteBuffer(tempBuff)) {
            TELEPHONY_LOGE("MmsHeader WriteByte fail.");
            return false;
        }
        addressMap_.erase(it);
    }
    return true;
}

bool MmsHeader::EncodeMultipleAddressValue(MmsEncodeBuffer &buff, uint8_t fieldId, std::vector<MmsAddress> &addrs)
{
    if (addrs.empty()) {
        TELEPHONY_LOGE("MmsHeader address is invalid.");
        return false;
    }

    for (auto addr : addrs) {
        if (!buff.WriteByte(fieldId)) {
            TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader WriteByte fail.", fieldId);
            return false;
        }
        MmsEncodeString encodeString;
        encodeString.SetAddressString(addr);
        if (!encodeString.EncodeEncodeString(buff)) {
            TELEPHONY_LOGE("The fieldId[%{public}d] MmsHeader EncodeEncodeString fail.", fieldId);
            return false;
        }
    }
    return true;
}

bool MmsHeader::EcondeFieldMessageClassValue(MmsEncodeBuffer &buff)
{
    uint8_t valueOctet = 0;
    if (GetOctetValue(MMS_MESSAGE_CLASS, valueOctet)) {
        return EncodeOctetValue(buff, MMS_MESSAGE_CLASS, valueOctet);
    }

    std::string strValue = "";
    if (GetTextValue(MMS_MESSAGE_CLASS, strValue)) {
        if (strValue == "personal") {
            valueOctet = static_cast<uint8_t>(MmsMessageClass::PERSONAL);
        } else if (strValue == "advertisement") {
            valueOctet = static_cast<uint8_t>(MmsMessageClass::ADVERTISEMENT);
        } else if (strValue == "informational") {
            valueOctet = static_cast<uint8_t>(MmsMessageClass::INFORMATIONAL);
        } else if (strValue == "auto") {
            valueOctet = static_cast<uint8_t>(MmsMessageClass::AUTO);
        }

        if (valueOctet != 0) {
            return EncodeOctetValue(buff, MMS_MESSAGE_CLASS, valueOctet);
        }
        return EncodeTextStringValue(buff, MMS_MESSAGE_CLASS, strValue);
    }
    return false;
}

bool MmsHeader::EncodeCommontFieldValue(MmsEncodeBuffer &buff)
{
    uint8_t messageType = 0;
    GetOctetValue(MMS_MESSAGE_TYPE, messageType);
    if (!EncodeOctetValue(buff, MMS_MESSAGE_TYPE, messageType)) {
        TELEPHONY_LOGE("MmsHeader EncodeOctetValue fail.");
        return false;
    }

    std::string transactionId = "";
    if (GetTextValue(MMS_TRANSACTION_ID, transactionId)) {
        if (!EncodeTextStringValue(buff, MMS_TRANSACTION_ID, transactionId)) {
            TELEPHONY_LOGE("MmsHeader EncodeTextStringValue fail.");
            return false;
        }
    }

    int64_t version = 0;
    GetLongValue(MMS_MMS_VERSION, version); // 8bit shortInteger
    if (!EncodeShortIntegerValue(buff, MMS_MMS_VERSION, version)) {
        TELEPHONY_LOGE("MmsHeader EncodeShortIntegerValue fail.");
        return false;
    }
    return true;
}

/**
 * @brief EncodeMmsSendReq
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.1.1 Send Request
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EncodeMmsSendReq(MmsEncodeBuffer &buff)
{
    // FROM
    std::vector<MmsAddress> fromAddress;
    GetHeaderAllAddressValue(MMS_FROM, fromAddress);
    if (!EncodeFieldFromValue(buff, fromAddress)) {
        TELEPHONY_LOGE("encode mandatory from field error.");
        return false;
    }
    // Optional
    int64_t mmsExpriy = 0;
    if (GetLongValue(MMS_EXPIRY, mmsExpriy)) {
        EncodeFieldExpriyValue(buff, mmsExpriy);
    }
    // CC BCC TO
    for (auto it = addressMap_.begin(); it != addressMap_.end(); it++) {
        EncodeMultipleAddressValue(buff, it->first, it->second);
    }

    EcondeFieldMessageClassValue(buff); // MMS_MESSAGE_CLASS
    EncodeLongIntergerValueFromMap(buff, MMS_DATE);
    EnocdeEncodeStringValueFromMap(buff, MMS_SUBJECT);
    EncodeTextStringValueFromMap(buff, MMS_MESSAGE_CLASS);
    EncodeOctetValueFromMap(buff, MMS_PRIORITY);
    EncodeOctetValueFromMap(buff, MMS_SENDER_VISIBILITY);
    EncodeOctetValueFromMap(buff, MMS_DELIVERY_REPORT);
    EncodeOctetValueFromMap(buff, MMS_READ_REPORT);
    return true;
}

/**
 * @brief EncodeMmsSendConf
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.1.2 Send Confirmationt
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EncodeMmsSendConf(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeOctetValueFromMap(buff, MMS_RESPONSE_STATUS)) {
        TELEPHONY_LOGE("encode mandatory response status field error.");
        return false;
    }
    // Optional
    EncodeTextStringValueFromMap(buff, MMS_MESSAGE_ID);
    return true;
}

/**
 * @brief EncodeMmsNotificationInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.2 Multimedia Message Notification
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EncodeMmsNotificationInd(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeTextStringValueFromMap(buff, MMS_CONTENT_LOCATION)) {
        TELEPHONY_LOGE("encode mandatory content location field error.");
        return false;
    }
    if (!EncodeLongIntergerValueFromMap(buff, MMS_MESSAGE_SIZE)) {
        TELEPHONY_LOGE("encode mandatory message size field error.");
        return false;
    }
    int64_t mmsExpriy = 0;
    if (!GetLongValue(MMS_EXPIRY, mmsExpriy)) {
        TELEPHONY_LOGE("get mandatory expriy field no set error.");
        return false;
    }
    if (!EncodeFieldExpriyValue(buff, mmsExpriy)) {
        TELEPHONY_LOGE("encode mandatory expriy field error.");
        return false;
    }
    if (!EcondeFieldMessageClassValue(buff)) { // MMS_MESSAGE_CLASS
        TELEPHONY_LOGE("encode mandatory message class field error.");
        return false;
    }

    // Optional
    std::vector<MmsAddress> fromAddress;
    if (GetHeaderAllAddressValue(MMS_FROM, fromAddress)) {
        EncodeFieldFromValue(buff, fromAddress);
    }
    EncodeOctetValueFromMap(buff, MMS_CONTENT_CLASS);
    EnocdeEncodeStringValueFromMap(buff, MMS_SUBJECT);
    EncodeOctetValueFromMap(buff, MMS_DELIVERY_REPORT);
    return true;
}

/**
 * @brief EnocdeMmsNotifyRespInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.2 Multimedia Message Notification
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EnocdeMmsNotifyRespInd(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeOctetValueFromMap(buff, MMS_STATUS)) {
        TELEPHONY_LOGE("encode mandatory mms status field error.");
        return false;
    }
    // Optional
    EncodeOctetValueFromMap(buff, MMS_REPORT_ALLOWED);
    return true;
}

/**
 * @brief EnocdeMmsRetrieveConf
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.3 Retrieval of Multimedia Message
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EnocdeMmsRetrieveConf(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeLongIntergerValueFromMap(buff, MMS_DATE)) {
        TELEPHONY_LOGE("encode mandatory mms date field error.");
        return false;
    }

    // Optional
    EncodeTextStringValueFromMap(buff, MMS_MESSAGE_ID); // Conditional
    std::vector<MmsAddress> fromAddress;
    if (GetHeaderAllAddressValue(MMS_FROM, fromAddress)) {
        EncodeFieldFromValue(buff, fromAddress);
    }
    std::vector<MmsAddress> toAddress;
    if (GetHeaderAllAddressValue(MMS_TO, toAddress)) {
        if (!EncodeMultipleAddressValue(buff, MMS_TO, toAddress)) {
            TELEPHONY_LOGE("MmsHeader EncodeMultipleAddressValue fail.");
            return false;
        }
    }
    std::vector<MmsAddress> ccAddress;
    if (GetHeaderAllAddressValue(MMS_CC, ccAddress)) {
        if (!EncodeMultipleAddressValue(buff, MMS_CC, ccAddress)) {
            TELEPHONY_LOGE("MmsHeader EncodeMultipleAddressValue fail.");
            return false;
        }
    }
    EcondeFieldMessageClassValue(buff); // MMS_MESSAGE_CLASS
    EnocdeEncodeStringValueFromMap(buff, MMS_SUBJECT);
    EncodeOctetValueFromMap(buff, MMS_PRIORITY);
    EncodeOctetValueFromMap(buff, MMS_DELIVERY_REPORT);
    EncodeOctetValueFromMap(buff, MMS_READ_REPORT);
    EncodeOctetValueFromMap(buff, MMS_RETRIEVE_STATUS);
    EnocdeEncodeStringValueFromMap(buff, MMS_RETRIEVE_TEXT);
    // MMS_PREVIOUSLY_SENT_BY unSupported
    // MMS_PREVIOUSLY_SENT_DATE unSupported
    // MMS_MM_STATE unSupported
    // MMS_MM_FLAGS unSupported
    // MMS_REPLY_CHARGING unSupported
    // MMS_REPLY_CHARGING_DEADLINE unSupported
    // MMS_REPLY_CHARGING_SIZE unSupported
    // MMS_REPLY_CHARGING_ID unSupported
    // MMS_DISTRIBUTION_INDICATOR unSupported
    // MMS_APPLIC_ID unSupported
    // MMS_REPLY_APPLIC_ID unSupported
    // MMS_AUX_APPLIC_INFO unSupported
    // MMS_CONTENT_CLASS unSupported
    // MMS_DRM_CONTENT unSupported
    // MMS_REPLACE_ID unSupported
    return true;
}

/**
 * @brief EnocdeMmsAcknowledgeInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.4 Delivery Acknowledgement
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EnocdeMmsAcknowledgeInd(MmsEncodeBuffer &buff)
{
    // Optional
    EncodeOctetValueFromMap(buff, MMS_REPORT_ALLOWED);
    return true;
}

/**
 * @brief EnocdeMmsDeliveryInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.6 Delivery Reporting
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EnocdeMmsDeliveryInd(MmsEncodeBuffer &buff)
{
    // Mandatory
    std::vector<MmsAddress> toAddress;
    if (!GetHeaderAllAddressValue(MMS_TO, toAddress)) {
        TELEPHONY_LOGE("get mandatory mms to field no set error.");
        return false;
    }
    if (!EncodeMultipleAddressValue(buff, MMS_TO, toAddress)) {
        TELEPHONY_LOGE("encode mandatory mms to field error.");
        return false;
    }
    if (!EncodeTextStringValueFromMap(buff, MMS_MESSAGE_ID)) {
        TELEPHONY_LOGE("encode mandatory mms message id field error.");
        return false;
    }
    if (!EncodeLongIntergerValueFromMap(buff, MMS_DATE)) {
        TELEPHONY_LOGE("encode mandatory mms date field error.");
        return false;
    }
    if (!EncodeOctetValueFromMap(buff, MMS_STATUS)) {
        TELEPHONY_LOGE("encode mandatory mms status field error.");
        return false;
    }
    // Optional
    return true;
}

/**
 * @brief EncodeMmsReadRecInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.7.2 PDU Read Report
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EncodeMmsReadRecInd(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeTextStringValueFromMap(buff, MMS_MESSAGE_ID)) {
        TELEPHONY_LOGE("encode mandatory mms message id field error.");
        return false;
    }
    std::vector<MmsAddress> toAddress;
    if (!GetHeaderAllAddressValue(MMS_TO, toAddress)) {
        TELEPHONY_LOGE("get mandatory mms to field no set error.");
        return false;
    }
    if (!EncodeMultipleAddressValue(buff, MMS_TO, toAddress)) {
        TELEPHONY_LOGE("encode mandatory mms to field error.");
        return false;
    }

    std::vector<MmsAddress> fromAddress;
    GetHeaderAllAddressValue(MMS_FROM, fromAddress);
    if (!EncodeFieldFromValue(buff, fromAddress)) {
        TELEPHONY_LOGE("encode mandatory mms from field error.");
        return false;
    }

    if (!EncodeOctetValueFromMap(buff, MMS_READ_STATUS)) {
        TELEPHONY_LOGE("encode mandatory mms read status field error.");
        return false;
    }
    // Optional
    EncodeLongIntergerValueFromMap(buff, MMS_DATE);
    // MMS_APPLIC_ID unSupported
    // MMS_REPLY_APPLIC_ID unSupported
    // MMS_AUX_APPLIC_INFO unSupported
    return true;
}

/**
 * @brief EncodeMmsReadOrigInd
 * OMA-TS-MMS_ENC-V1_3-20110913-A   section:6.7.2 PDU Read Report
 * @param buff
 * @return true
 * @return false
 */
bool MmsHeader::EncodeMmsReadOrigInd(MmsEncodeBuffer &buff)
{
    // Mandatory
    if (!EncodeTextStringValueFromMap(buff, MMS_MESSAGE_ID)) {
        TELEPHONY_LOGE("encode mandatory mms message id field error.");
        return false;
    }
    std::vector<MmsAddress> toAddress;
    if (!GetHeaderAllAddressValue(MMS_TO, toAddress)) {
        TELEPHONY_LOGE("get mandatory mms to field no set error.");
        return false;
    }
    if (!EncodeMultipleAddressValue(buff, MMS_TO, toAddress)) {
        TELEPHONY_LOGE("encode mandatory mms to field error.");
        return false;
    }

    std::vector<MmsAddress> fromAddress;
    GetHeaderAllAddressValue(MMS_FROM, fromAddress);
    if (!EncodeFieldFromValue(buff, fromAddress)) {
        TELEPHONY_LOGE("encode mandatory mms from field error.");
        return false;
    }

    if (!EncodeOctetValueFromMap(buff, MMS_READ_STATUS)) {
        TELEPHONY_LOGE("encode mandatory mms read status field error.");
        return false;
    }
    if (!EncodeLongIntergerValueFromMap(buff, MMS_DATE)) {
        TELEPHONY_LOGE("encode mandatory mms date field error.");
        return false;
    }
    // Optional
    // MMS_APPLIC_ID unSupported
    // MMS_REPLY_APPLIC_ID unSupported
    // MMS_AUX_APPLIC_INFO unSupported
    return true;
}

bool MmsHeader::IsHaveTransactionId(uint8_t messageType)
{
    if (messageType == MMS_MSGTYPE_SEND_REQ || messageType == MMS_MSGTYPE_SEND_CONF ||
        messageType == MMS_MSGTYPE_NOTIFICATION_IND || messageType == MMS_MSGTYPE_NOTIFYRESP_IND ||
        messageType == MMS_MSGTYPE_RETRIEVE_CONF || messageType == MMS_MSGTYPE_ACKNOWLEDGE_IND) {
        return true;
    }
    return false;
}

bool MmsHeader::CheckResponseStatus(uint8_t value)
{
    if (value >= static_cast<uint8_t>(MmsResponseStatus::MMS_OK) &&
        value <= static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_UNSUPPORTED_MESSAGE)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_TRANSIENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_TRANSIENT_PARTIAL_SUCCESS)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_PERMANENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsResponseStatus::MMS_ERROR_PERMANENT_LACK_OF_PREPAID)) {
        return true;
    }
    return false;
}

bool MmsHeader::CheckRetrieveStatus(uint8_t value)
{
    if (value == static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_OK)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_TRANSIENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_TRANSIENT_NETWORK_PROBLEM)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_PERMANENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsRetrieveStatus::MMS_RETRIEVE_STATUS_ERROR_PERMANENT_CONTENT_UNSUPPORTED)) {
        return true;
    }
    return false;
}

bool MmsHeader::CheckStoreStatus(uint8_t value)
{
    if (value == static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_SUCCESS)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_TRANSIENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_TRANSIENT_NETWORK_PROBLEM)) {
        return true;
    }
    if (value >= static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_PERMANENT_FAILURE) &&
        value <= static_cast<uint8_t>(MmsStoreStatus::MMS_STORE_STATUS_ERROR_PERMANENT_MMBOX_FULL)) {
        return true;
    }
    return false;
}

bool MmsHeader::CheckBooleanValue(uint8_t fieldId, uint8_t value)
{
    switch (fieldId) {
        case MMS_DELIVERY_REPORT:
        case MMS_READ_REPORT:
        case MMS_REPORT_ALLOWED:
        case MMS_STORE:
        case MMS_STORED:
        case MMS_TOTALS:
        case MMS_QUOTAS:
        case MMS_DISTRIBUTION_INDICATOR:
        case MMS_DRM_CONTENT:
        case MMS_ADAPTATION_ALLOWED:
            if (value < static_cast<uint8_t>(MmsBoolType::MMS_YES) ||
                value > static_cast<uint8_t>(MmsBoolType::MMS_NO)) {
                return false;
            }
            break;
        default:
            return false;
    }

    octetValueMap_[fieldId] = value;
    return true;
}
} // namespace Telephony
} // namespace OHOS
