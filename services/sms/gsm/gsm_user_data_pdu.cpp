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

#include "gsm_user_data_pdu.h"

#include "gsm_pdu_hex_value.h"
#include "gsm_sms_common_utils.h"
#include "gsm_user_data_decode.h"
#include "gsm_user_data_encode.h"
#include "securec.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
using namespace std;
static constexpr uint8_t NORMAL_BYTE_BITS = 8;
static constexpr uint8_t SLIDE_DATA_STEP = 2;

bool GsmUserDataPdu::EncodeUserDataPdu(
    SmsWriteBuffer &buffer, const struct SmsUDPackage *(&userData), DataCodingScheme scheme, std::string &destAddr)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }
    auto encodePdu = std::make_shared<GsmUserDataEncode>(shared_from_this());
    if (encodePdu == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    bool result = false;
    switch (scheme) {
        case DATA_CODING_7BIT:
            result = encodePdu->EncodeGsmPdu(buffer, userData);
            break;
        case DATA_CODING_8BIT:
            result = encodePdu->Encode8bitPdu(buffer, userData, destAddr);
            break;
        case DATA_CODING_UCS2:
            result = encodePdu->EncodeUcs2Pdu(buffer, userData);
            break;
        default:
            break;
    }
    TELEPHONY_LOGI("userData encode result:%{public}d", result);
    return result;
}

bool GsmUserDataPdu::DecodeUserDataPdu(
    SmsReadBuffer &buffer, bool bHeaderInd, DataCodingScheme scheme, struct SmsUDPackage *userData)
{
    if (userData == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    if (memset_s(userData, sizeof(struct SmsUDPackage), 0x00, sizeof(struct SmsUDPackage)) != EOK) {
        TELEPHONY_LOGE("memset_s error.");
        return false;
    }

    auto decodePdu = std::make_shared<GsmUserDataDecode>(shared_from_this());
    if (decodePdu == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    bool result = false;
    switch (scheme) {
        case DATA_CODING_7BIT:
            result = decodePdu->DecodeGsmPdu(buffer, bHeaderInd, userData, nullptr);
            break;
        case DATA_CODING_8BIT:
            result = decodePdu->Decode8bitPdu(buffer, bHeaderInd, userData, nullptr);
            break;
        case DATA_CODING_UCS2:
            result = decodePdu->DecodeUcs2Pdu(buffer, bHeaderInd, userData, nullptr);
            break;
        default:
            break;
    }
    TELEPHONY_LOGI("userData decode result:%{public}d", result);
    return result;
}

bool GsmUserDataPdu::DecodeUserDataPdu(SmsReadBuffer &buffer, bool bHeaderInd, DataCodingScheme scheme,
    struct SmsUDPackage *userData, struct SmsTpud *pTPUD)
{
    if (userData == nullptr || pTPUD == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    if (memset_s(userData, sizeof(struct SmsUDPackage), 0x00, sizeof(struct SmsUDPackage)) != EOK) {
        TELEPHONY_LOGE("memset_s error.");
        return false;
    }

    auto decodePdu = std::make_shared<GsmUserDataDecode>(shared_from_this());
    if (decodePdu == nullptr) {
        TELEPHONY_LOGE("nullptr error.");
        return false;
    }

    bool result = false;
    switch (scheme) {
        case DATA_CODING_7BIT:
            result = decodePdu->DecodeGsmPdu(buffer, bHeaderInd, userData, pTPUD);
            break;
        case DATA_CODING_8BIT:
            result = decodePdu->Decode8bitPdu(buffer, bHeaderInd, userData, pTPUD);
            break;
        case DATA_CODING_UCS2:
            result = decodePdu->DecodeUcs2Pdu(buffer, bHeaderInd, userData, pTPUD);
            break;
        case DATA_CODING_EUCKR:
            result = decodePdu->DecodeUcs2Pdu(buffer, bHeaderInd, userData, pTPUD);
            break;
        default:
            break;
    }
    if (!result) {
        TELEPHONY_LOGE("userData decode fail.");
    }
    return result;
}

void GsmUserDataPdu::ResetUserData(struct SmsUDPackage &userData)
{
    userData.length = 0;
    if (memset_s(userData.data, sizeof(userData.data), 0x00, sizeof(userData.data)) != EOK) {
        TELEPHONY_LOGE("memset_s error!");
        return;
    }
}

bool GsmUserDataPdu::GetHeaderCnt(SmsReadBuffer &buffer, struct SmsUDPackage *userData, uint8_t &udhl, uint16_t i)
{
    if (i >= MAX_UD_HEADER_NUM) {
        TELEPHONY_LOGE("index invalid.");
        return false;
    }
    uint16_t headerLen = 0;
    uint16_t current = buffer.GetIndex();
    if (!DecodeHeader(buffer, userData->header[i], headerLen)) {
        TELEPHONY_LOGE("memset_s fail.");
        return false;
    }

    if (headerLen == 0) {
        userData->length = 0;
        if (memset_s(userData->data, sizeof(userData->data), 0x00, sizeof(userData->data)) != EOK) {
            TELEPHONY_LOGE("memset_s fail.");
        }
        return false;
    }
    if ((buffer.GetIndex() - current) > (udhl + SLIDE_DATA_STEP)) {
        userData->length = 0;
        if (memset_s(userData->data, sizeof(userData->data), 0x00, sizeof(userData->data)) != EOK) {
            TELEPHONY_LOGE("memset_s fail.");
        }
        return false;
    }
    userData->headerCnt++;
    return true;
}

void GsmUserDataPdu::EncodeHeader(SmsWriteBuffer &buffer, const struct SmsUDH header)
{
    switch (header.udhType) {
        case UDH_CONCAT_8BIT:
        case UDH_CONCAT_16BIT:
            EncodeHeaderConcat(buffer, header);
            break;
        case UDH_APP_PORT_8BIT:
            EncodeHeaderAppPort8bit(buffer, header);
            break;
        case UDH_APP_PORT_16BIT:
            EncodeHeaderAppPort16bit(buffer, header);
            break;
        case UDH_ALTERNATE_REPLY_ADDRESS:
            EncodeHeaderReplyAddress(buffer, header);
            break;
        case UDH_SINGLE_SHIFT:
            if (!buffer.WriteByte(UDH_SINGLE_SHIFT)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            if (!buffer.WriteByte(HEX_VALUE_01)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            if (!buffer.WriteByte(header.udh.singleShift.langId)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            break;
        case UDH_LOCKING_SHIFT:
            if (!buffer.WriteByte(UDH_LOCKING_SHIFT)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            if (!buffer.WriteByte(HEX_VALUE_01)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            if (!buffer.WriteByte(header.udh.lockingShift.langId)) {
                TELEPHONY_LOGE("write data error.");
                return;
            }
            break;
        default:
            break;
    }
}

void GsmUserDataPdu::EncodeHeaderAppPort8bit(SmsWriteBuffer &buffer, const struct SmsUDH header)
{
    if (!buffer.WriteByte(UDH_APP_PORT_8BIT)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(HEX_VALUE_02)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.appPort8bit.destPort)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.appPort8bit.originPort)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
}

void GsmUserDataPdu::EncodeHeaderAppPort16bit(SmsWriteBuffer &buffer, const struct SmsUDH header)
{
    if (!buffer.WriteByte(UDH_APP_PORT_16BIT)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(HEX_VALUE_04)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(static_cast<uint8_t>(header.udh.appPort16bit.destPort >> NORMAL_BYTE_BITS))) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(static_cast<uint8_t>(header.udh.appPort16bit.destPort & HEX_VALUE_00FF))) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(static_cast<uint8_t>(header.udh.appPort16bit.originPort >> NORMAL_BYTE_BITS))) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(static_cast<uint8_t>(header.udh.appPort16bit.originPort & HEX_VALUE_00FF))) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
}

void GsmUserDataPdu::EncodeHeaderReplyAddress(SmsWriteBuffer &buffer, const struct SmsUDH header)
{
    if (!buffer.WriteByte(UDH_ALTERNATE_REPLY_ADDRESS)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }

    std::string encodedAddr;
    GsmSmsParamCodec codec;
    if (!codec.EncodeAddressPdu(&(header.udh.alternateAddress), encodedAddr)) {
        TELEPHONY_LOGE("EncodeHeader EncodeAddressPdu fail");
        return;
    }
    uint8_t addrLen = encodedAddr.size();
    if (!buffer.WriteByte(addrLen)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (buffer.data_ == nullptr || (buffer.GetIndex() + addrLen) > buffer.GetSize()) {
        TELEPHONY_LOGE("buffer data error.");
        return;
    }
    if (memcpy_s(buffer.data_.get() + buffer.GetIndex(), addrLen, encodedAddr.data(), addrLen) != EOK) {
        TELEPHONY_LOGE("EncodeHeader memcpy_s error");
        return;
    }
}

void GsmUserDataPdu::EncodeHeaderConcat(SmsWriteBuffer &buffer, const struct SmsUDH header)
{
    switch (header.udhType) {
        case UDH_CONCAT_8BIT:
            EncodeHeaderConcat8Bit(buffer, header);
            break;
        case UDH_CONCAT_16BIT:
            EncodeHeaderConcat16Bit(buffer, header);
            break;
        case UDH_NONE:
        default:
            break;
    }
}

void GsmUserDataPdu::EncodeHeaderConcat8Bit(SmsWriteBuffer &buffer, const struct SmsUDH &header)
{
    if (!buffer.WriteByte(UDH_CONCAT_8BIT)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(HEX_VALUE_03)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat8bit.msgRef)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat8bit.totalSeg)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat8bit.seqNum)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
}

void GsmUserDataPdu::EncodeHeaderConcat16Bit(SmsWriteBuffer &buffer, const struct SmsUDH &header)
{
    if (!buffer.WriteByte(UDH_CONCAT_16BIT)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(HEX_VALUE_04)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(static_cast<uint8_t>(header.udh.concat16bit.msgRef >> NORMAL_BYTE_BITS))) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat16bit.msgRef & HEX_VALUE_00FF)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat16bit.totalSeg)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
    if (!buffer.WriteByte(header.udh.concat16bit.seqNum)) {
        TELEPHONY_LOGE("write data error.");
        return;
    }
}

bool GsmUserDataPdu::DecodeHeader(SmsReadBuffer &buffer, struct SmsUDH &pHeader, uint16_t &headerLen)
{
    headerLen = buffer.GetIndex();
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udhType = oneByte;
    switch (pHeader.udhType) {
        case UDH_CONCAT_8BIT:
            if (!DecodeHeaderConcat8Bit(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_CONCAT_16BIT:
            if (!DecodeHeaderConcat16Bit(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_APP_PORT_8BIT:
            if (!DecodeHeaderAppPort8Bit(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_APP_PORT_16BIT:
            if (!DecodeHeaderAppPort16Bit(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        default: {
            if (!DecodeHeaderPartData(buffer, pHeader, oneByte)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
        }
    }
    if (buffer.GetIndex() > headerLen) {
        headerLen = buffer.GetIndex() - headerLen;
    } else {
        headerLen = 0;
    }

    return true;
}

bool GsmUserDataPdu::DecodeHeaderPartData(SmsReadBuffer &buffer, struct SmsUDH &pHeader, uint8_t oneByte)
{
    pHeader.udhType = oneByte;
    switch (pHeader.udhType) {
        case UDH_SPECIAL_SMS:
            if (!DecodeHeaderSpecialSms(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_ALTERNATE_REPLY_ADDRESS:
            if (!DecodeHeaderReplyAddress(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_SINGLE_SHIFT:
            if (!DecodeHeaderSingleShift(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        case UDH_LOCKING_SHIFT:
            if (!DecodeHeaderLockingShift(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        default: {
            if (!DecodeHeaderDefaultCase(buffer, pHeader)) {
                TELEPHONY_LOGE("decode fail.");
                return false;
            }
            break;
        }
    }
    return true;
}

bool GsmUserDataPdu::DecodeHeaderConcat8Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        return false;
    }
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat8bit.msgRef = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat8bit.totalSeg = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat8bit.seqNum = oneByte;
    DebugDecodeHeader(pHeader);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderConcat16Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        return false;
    }
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat16bit.msgRef = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint16_t ref = static_cast<uint16_t>(oneByte);
    ref |= static_cast<uint16_t>(pHeader.udh.concat16bit.msgRef) << NORMAL_BYTE_BITS;
    pHeader.udh.concat16bit.msgRef = ref;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat16bit.totalSeg = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.concat16bit.seqNum = oneByte;
    DebugDecodeHeader(pHeader);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderAppPort8Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        TELEPHONY_LOGE("oneByte 0.");
        return false;
    }

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.appPort8bit.destPort = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.appPort8bit.originPort = oneByte;
    DebugDecodeHeader(pHeader);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderAppPort16Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }

    if (oneByte == 0) {
        TELEPHONY_LOGE("pickByte 0.");
        return false;
    }

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.appPort16bit.destPort = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    uint16_t port = oneByte;
    port |= pHeader.udh.appPort16bit.destPort << NORMAL_BYTE_BITS;
    pHeader.udh.appPort16bit.destPort = static_cast<uint16_t>(port);

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.appPort16bit.originPort = oneByte;

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }

    port = oneByte;
    port |= pHeader.udh.appPort16bit.originPort << NORMAL_BYTE_BITS;
    pHeader.udh.appPort16bit.originPort = static_cast<uint16_t>(port);
    DebugDecodeHeader(pHeader);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderSpecialSms(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte != HEX_VALUE_02) {
        return false;
    }
    TELEPHONY_LOGI("Decoding special sms udh.");
    uint8_t pickByte = 0;
    if (!buffer.PickOneByte(pickByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.specialInd.bStore = static_cast<bool>(pickByte & HEX_VALUE_80);

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.specialInd.msgInd = static_cast<uint16_t>(oneByte & HEX_VALUE_7F);

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.specialInd.waitMsgNum = oneByte;
    return true;
}

bool GsmUserDataPdu::DecodeHeaderReplyAddress(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        return false;
    }
    GsmSmsParamCodec codec;
    if (!codec.DecodeAddressPdu(buffer, &(pHeader.udh.alternateAddress))) {
        return false;
    }
    TELEPHONY_LOGI("alternate reply address [%{private}s]", pHeader.udh.alternateAddress.address);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderSingleShift(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        return false;
    }

    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.singleShift.langId = oneByte;
    TELEPHONY_LOGI("singleShift.langId [%{public}02x]", pHeader.udh.singleShift.langId);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderLockingShift(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    if (oneByte == 0) {
        return false;
    }
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    pHeader.udh.lockingShift.langId = oneByte;
    TELEPHONY_LOGI("lockingShift.langId [%{public}02x]", pHeader.udh.lockingShift.langId);
    return true;
}

bool GsmUserDataPdu::DecodeHeaderDefaultCase(SmsReadBuffer &buffer, struct SmsUDH &pHeader)
{
    uint8_t oneByte = 0;
    TELEPHONY_LOGI("Not Supported Header Type [%{public}02x]", pHeader.udhType);
    if (!buffer.ReadByte(oneByte)) {
        TELEPHONY_LOGE("get data error.");
        return false;
    }
    TELEPHONY_LOGI("IEDL [%{public}u]", oneByte);
    buffer.MoveForward(oneByte);
    return true;
}

void GsmUserDataPdu::DebugDecodeHeader(const struct SmsUDH &pHeader)
{
    switch (pHeader.udhType) {
        case UDH_CONCAT_8BIT: {
            TELEPHONY_LOGI("concat8bit.seqNum [%{public}02x]", pHeader.udh.concat8bit.seqNum);
            break;
        }
        case UDH_CONCAT_16BIT: {
            TELEPHONY_LOGI("concat16bit.seqNum [%{public}02x]", pHeader.udh.concat16bit.seqNum);
            break;
        }
        case UDH_APP_PORT_8BIT: {
            TELEPHONY_LOGI("appPort8bit.destPort [%{public}02x]", pHeader.udh.appPort8bit.destPort);
            TELEPHONY_LOGI("appPort8bit.originPort [%{public}02x]", pHeader.udh.appPort8bit.originPort);
            break;
        }
        case UDH_APP_PORT_16BIT: {
            TELEPHONY_LOGI("appPort16bit.destPort [%{public}04x]", pHeader.udh.appPort16bit.destPort);
            TELEPHONY_LOGI("appPort16bit.originPort [%{public}04x]", pHeader.udh.appPort16bit.originPort);
            break;
        }
        case UDH_SPECIAL_SMS: {
            TELEPHONY_LOGI("Decoding special sms udh.");
            break;
        }
        case UDH_ALTERNATE_REPLY_ADDRESS:
            break;
        case UDH_SINGLE_SHIFT: {
            TELEPHONY_LOGI("singleShift.langId [%{public}02x]", pHeader.udh.singleShift.langId);
            break;
        }
        case UDH_LOCKING_SHIFT: {
            TELEPHONY_LOGI("lockingShift.langId [%{public}02x]", pHeader.udh.lockingShift.langId);
            break;
        }
        default:
            break;
    }
}
} // namespace Telephony
} // namespace OHOS