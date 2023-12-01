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

#include "gsm_user_data_decode.h"

#include "gsm_pdu_hex_value.h"
#include "gsm_sms_common_utils.h"
#include "securec.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint8_t GSM_USER_DATA_LEN = 160;
static constexpr uint8_t NORMAL_BYTE_BITS = 8;
static constexpr uint8_t GSM_ENCODE_BITS = 7;
static constexpr uint8_t SLIDE_DATA_STEP = 2;
static constexpr uint8_t UCS2_USER_DATA_LEN = 140;
static constexpr uint8_t WAP_PUSH_UDHL = 7;

GsmUserDataDecode::GsmUserDataDecode(std::shared_ptr<GsmUserDataPdu> data)
{
    userData_ = data;
}

GsmUserDataDecode::~GsmUserDataDecode() {}

bool GsmUserDataDecode::DecodeGsmPdu(
    SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData, struct SmsTpud *pTPUD)
{
    uint8_t udl = 0;
    uint8_t fillBits = 0;
    if (!DecodeGsmHeadPdu(buffer, bHeaderInd, userData, pTPUD, udl, fillBits)) {
        TELEPHONY_LOGE("decode gsm head error");
        return false;
    }
    return DecodeGsmBodyPdu(buffer, bHeaderInd, userData, pTPUD, udl, fillBits);
}

bool GsmUserDataDecode::DecodeGsmHeadPdu(SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData,
    struct SmsTpud *pTPUD, uint8_t &udl, uint8_t &fillBits)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error");
        return false;
    }

    /* UDL */
    if (!buffer.ReadByte(udl)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (udl > GSM_USER_DATA_LEN) {
        userData->length = 0;
        userData->headerCnt = 0;
        TELEPHONY_LOGE("udl error.");
        return false;
    }

    /* Setting for Wap Push */
    if (pTPUD != nullptr && udl > 0) {
        pTPUD->udl = udl;
        if (udl > sizeof(pTPUD->ud)) {
            TELEPHONY_LOGE("udl length error");
            return false;
        }
        uint8_t len = udl;
        if (buffer.GetIndex() + udl >= buffer.GetSize()) {
            len = buffer.GetSize() - buffer.GetIndex();
        }
        if (buffer.data_ == nullptr || len > sizeof(pTPUD->ud)) {
            TELEPHONY_LOGE("buffer error.");
            return false;
        }
        if (memcpy_s(pTPUD->ud, sizeof(pTPUD->ud), buffer.data_.get() + buffer.GetIndex(), len) != EOK) {
            TELEPHONY_LOGE("memcpy_s error");
            return false;
        }
        pTPUD->ud[len] = '\0';
    }
    return DecodeGsmHeadPduPartData(buffer, bHeaderInd, userData, udl, fillBits);
}

bool GsmUserDataDecode::DecodeGsmHeadPduPartData(
    SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData, uint8_t &udl, uint8_t &fillBits)
{
    if (userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    uint8_t udhl = 0;
    /* Decode User Data Header */
    if (bHeaderInd) {
        /* UDHL */
        if (!buffer.ReadByte(udhl)) {
            TELEPHONY_LOGE("get data error.");
            return false;
        }
        userData->headerCnt = 0;
        uint8_t current = buffer.GetIndex();
        for (int i = 0; buffer.GetIndex() < current + udhl && i < MAX_UD_HEADER_NUM; i++) {
            uint16_t headerLen = 0;
            if (!userData_->DecodeHeader(buffer, userData->header[i], headerLen)) {
                TELEPHONY_LOGE("DecodeHeader error");
                return false;
            }
            if (headerLen == 0) {
                userData_->ResetUserData(*userData);
                TELEPHONY_LOGE("headerLen error");
                return false;
            }
            if (buffer.GetIndex() >= buffer.GetSize()) {
                TELEPHONY_LOGE("data buffer error)");
                userData_->ResetUserData(*userData);
                return false;
            }
            userData->headerCnt++;
        }
    } else {
        userData->headerCnt = 0;
    }

    if (udhl > 0) {
        fillBits = ((udl * GSM_ENCODE_BITS) - ((udhl + 1) * NORMAL_BYTE_BITS)) % GSM_ENCODE_BITS;
        udl = ((udl * GSM_ENCODE_BITS) - ((udhl + 1) * NORMAL_BYTE_BITS)) / GSM_ENCODE_BITS;
    }
    return true;
}

bool GsmUserDataDecode::DecodeGsmBodyPdu(SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData,
    struct SmsTpud *pTPUD, uint8_t &udl, uint8_t fillBits)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    GsmSmsCommonUtils utils;
    uint8_t length = 0;
    bool ret = utils.Unpack7bitChar(
        buffer, udl, fillBits, reinterpret_cast<uint8_t *>(userData->data), MAX_USER_DATA_LEN + 1, length);
    if (ret) {
        userData->length = length;
    }
    TELEPHONY_LOGI("decode gsm body result:%{public}d", ret);
    return ret;
}

bool GsmUserDataDecode::Decode8bitPdu(
    SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData, struct SmsTpud *pTPUD)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    /* UDL */
    uint8_t udl = 0;
    if (!buffer.ReadByte(udl)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    TELEPHONY_LOGI("udl:%{public}d", udl);
    uint16_t current = buffer.GetIndex();

    if (udl > UCS2_USER_DATA_LEN) {
        userData->length = 0;
        TELEPHONY_LOGE("udl length error");
        return false;
    }

    /* Setting for Wap Push */
    if (pTPUD != nullptr && udl > WAP_PUSH_UDHL) {
        pTPUD->udl = udl - WAP_PUSH_UDHL;
        if (pTPUD->udl > sizeof(pTPUD->ud)) {
            TELEPHONY_LOGE("udl length error");
            return false;
        }

        if (buffer.data_ == nullptr || (buffer.GetIndex() + pTPUD->udl + WAP_PUSH_UDHL > buffer.GetSize())) {
            TELEPHONY_LOGE("buffer error.");
            return false;
        }
        if (memcpy_s(pTPUD->ud, sizeof(pTPUD->ud), buffer.data_.get() + buffer.GetIndex() + WAP_PUSH_UDHL,
                pTPUD->udl) != EOK) {
            TELEPHONY_LOGE("memcpy_s error.");
            return false;
        }
        pTPUD->ud[pTPUD->udl] = '\0';
    }
    return Decode8bitPduPartData(buffer, bHeaderInd, userData, pTPUD, current, udl);
}

bool GsmUserDataDecode::Decode8bitPduPartData(SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData,
    struct SmsTpud *pTPUD, uint16_t current, uint8_t udl)
{
    if (userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    uint8_t udhl = 0;
    /* Decode User Data Header */
    if (bHeaderInd) {
        /* UDHL */
        if (!buffer.ReadByte(udhl)) {
            TELEPHONY_LOGE("get data error.");
            return false;
        }
        userData->headerCnt = 0;
        current = buffer.GetIndex();
        for (uint8_t i = 0; (buffer.GetIndex() - current) < udhl && i < MAX_UD_HEADER_NUM; i++) {
            if (!userData_->GetHeaderCnt(buffer, userData, udhl, i)) {
                TELEPHONY_LOGI("data error");
                return false;
            }
        }
    } else {
        userData->headerCnt = 0;
    }

    if (udhl > 0 && udl > udhl + 1) {
        userData->length = (udl) - (udhl + 1);
        buffer.MoveForward(HEX_VALUE_01);
    } else {
        userData->length = udl;
    }
    if (udl > sizeof(pTPUD->ud) || buffer.GetIndex() >= buffer.GetSize()) {
        TELEPHONY_LOGE("udl length or buffer error");
        return false;
    }
    uint8_t remain = buffer.GetSize() - buffer.GetIndex();
    uint8_t len = userData->length < remain ? userData->length : remain;
    if (buffer.data_ == nullptr || len > sizeof(userData->data)) {
        TELEPHONY_LOGE("buffer error.");
        return false;
    }
    if (memcpy_s(userData->data, sizeof(userData->data), buffer.data_.get() + buffer.GetIndex(), len) != EOK) {
        TELEPHONY_LOGE("memcpy_s error");
        return false;
    }
    buffer.MoveForward(userData->length);
    return true;
}

bool GsmUserDataDecode::DecodeUcs2Pdu(
    SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData, struct SmsTpud *pTPUD)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    uint16_t current = buffer.GetIndex();

    /* UDL */
    uint8_t udl = 0;
    if (!buffer.ReadByte(udl)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (udl > UCS2_USER_DATA_LEN || (udl + buffer.GetIndex() - 1) > buffer.GetSize()) {
        TELEPHONY_LOGE("udl error.");
        userData->length = 0;
        userData->headerCnt = 0;
        return false;
    }
    if (buffer.data_ == nullptr) {
        TELEPHONY_LOGE("buffer error.");
        return false;
    }
    /* Setting for Wap Push */
    if (pTPUD != nullptr) {
        pTPUD->udl = udl;
        if (udl > sizeof(pTPUD->ud) || (buffer.GetIndex() + udl) > buffer.GetSize()) {
            TELEPHONY_LOGE("udl error.");
            return false;
        }
        if (memcpy_s(pTPUD->ud, sizeof(pTPUD->ud), buffer.data_.get() + buffer.GetIndex(), udl) != EOK) {
            TELEPHONY_LOGE("memcpy_s error.");
            return false;
        }
        pTPUD->ud[udl] = '\0';
    }
    return DecodeUcs2PduPartData(buffer, bHeaderInd, userData, current, udl);
}

bool GsmUserDataDecode::DecodeUcs2PduPartData(
    SmsReadBuffer &buffer, bool bHeaderInd, struct SmsUDPackage *userData, uint16_t current, uint8_t udl)
{
    if (userData_ == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    uint8_t udhl = 0;
    userData->headerCnt = 0;
    /* Decode User Data Header */
    if (bHeaderInd == true) {
        /* UDHL */
        if (!buffer.ReadByte(udhl)) {
            TELEPHONY_LOGE("get data error.");
            return false;
        }
        for (uint8_t i = 0; buffer.GetIndex() < udhl + current && i < MAX_UD_HEADER_NUM; i++) {
            uint16_t headerLen;
            if (!userData_->DecodeHeader(buffer, userData->header[i], headerLen)) {
                TELEPHONY_LOGE("DecodeHeader error");
                return false;
            }
            if (headerLen == 0 || buffer.GetIndex() > (current + udhl + SLIDE_DATA_STEP)) {
                TELEPHONY_LOGI("data error");
                userData_->ResetUserData(*userData);
                return false;
            }
            userData->headerCnt++;
        }
    }

    if (udhl > 0 && udl > udhl + 1) {
        userData->length = (udl) - (udhl + 1);
    } else {
        userData->length = udl;
    }

    if (buffer.GetIndex() >= buffer.GetSize()) {
        TELEPHONY_LOGE("udl length or buffer error");
        return false;
    }
    uint8_t remain = buffer.GetSize() - buffer.GetIndex();
    uint8_t len = userData->length < remain ? userData->length : remain;
    if (buffer.data_ == nullptr || len > sizeof(userData->data)) {
        TELEPHONY_LOGE("buffer error.");
        return false;
    }
    if (memcpy_s(userData->data, sizeof(userData->data), buffer.data_.get() + buffer.GetIndex(), len) != EOK) {
        TELEPHONY_LOGE("memcpy_s error.");
        return false;
    }

    buffer.MoveForward(userData->length);
    uint8_t index = userData->length >= sizeof(userData->data) ? sizeof(userData->data) - 1 : userData->length;
    userData->data[index] = 0;
    return true;
}
} // namespace Telephony
} // namespace OHOS