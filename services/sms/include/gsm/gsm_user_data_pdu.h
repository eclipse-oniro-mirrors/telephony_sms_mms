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

#ifndef GSM_USER_DATA_PDU_H
#define GSM_USER_DATA_PDU_H

#include "gsm_sms_param_codec.h"

namespace OHOS {
namespace Telephony {
class GsmUserDataPdu : public std::enable_shared_from_this<GsmUserDataPdu> {
public:
    GsmUserDataPdu() = default;
    virtual ~GsmUserDataPdu() = default;
    bool DecodeUserDataPdu(
        SmsReadBuffer &buffer, bool bHeaderInd, DataCodingScheme CodingScheme, struct SmsUDPackage *pUserData);
    bool DecodeUserDataPdu(SmsReadBuffer &buffer, bool bHeaderInd, DataCodingScheme CodingScheme,
        struct SmsUDPackage *pUserData, struct SmsTpud *pTPUD);
    bool DecodeHeader(SmsReadBuffer &buffer, struct SmsUDH &pHeader, uint16_t &headerLen);
    bool DecodeHeaderList(SmsReadBuffer &buffer, uint16_t udhStart, uint8_t udhl, struct SmsUDPackage &userData);
    void DebugDecodeHeader(const struct SmsUDH &pHeader);
    bool EncodeUserDataPdu(SmsWriteBuffer &buffer, const struct SmsUDPackage *(&pUserData),
        DataCodingScheme codingScheme, std::string &destAddr);
    void EncodeHeader(SmsWriteBuffer &buffer, const struct SmsUDH header);
    bool GetHeaderCnt(SmsReadBuffer &buffer, struct SmsUDPackage *pUserData, uint8_t &udhl, uint16_t i);
    void ResetUserData(struct SmsUDPackage &userData);

private:
    bool DecodeHeaderPartData(SmsReadBuffer &buffer, struct SmsUDH &pHeader, uint8_t oneByte);
    bool DecodeHeaderConcat8Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderConcat16Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderAppPort8Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderAppPort16Bit(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderSpecialSms(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderReplyAddress(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderSingleShift(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderLockingShift(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    bool DecodeHeaderDefaultCase(SmsReadBuffer &buffer, struct SmsUDH &pHeader);
    void EncodeHeaderAppPort8bit(SmsWriteBuffer &buffer, const struct SmsUDH header);
    void EncodeHeaderAppPort16bit(SmsWriteBuffer &buffer, const struct SmsUDH header);
    void EncodeHeaderReplyAddress(SmsWriteBuffer &buffer, const struct SmsUDH header);
    void EncodeHeaderConcat(SmsWriteBuffer &buffer, const struct SmsUDH header);
    void EncodeHeaderConcat8Bit(SmsWriteBuffer &buffer, const struct SmsUDH &header);
    void EncodeHeaderConcat16Bit(SmsWriteBuffer &buffer, const struct SmsUDH &header);
};
} // namespace Telephony
} // namespace OHOS
#endif