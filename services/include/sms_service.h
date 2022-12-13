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

#ifndef SMS_SERVICE_H
#define SMS_SERVICE_H

#include <memory>

#include "sms_interface_stub.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "sms_state_handler.h"

namespace OHOS {
namespace Telephony {
enum ServiceRunningState { STATE_NOT_START, STATE_RUNNING };

class SmsService : public SystemAbility, public SmsInterfaceStub, public std::enable_shared_from_this<SmsService> {
    DECLARE_DELAYED_SINGLETON(SmsService)
    DECLARE_SYSTEM_ABILITY(SmsService) // necessary
public:
    void OnStart() override;
    void OnStop() override;
    int32_t Dump(std::int32_t fd, const std::vector<std::u16string> &args) override;
    std::string GetBindTime();

    /**
     * @brief SendMessage
     * Sends a text Type SMS message.
     * @param slotId [in]
     * @param desAddr [in]
     * @param scAddr [in]
     * @param text [in]
     * @param sendCallback [in]
     * @param deliverCallback [in]
     */
    void SendMessage(int32_t slotId, const std::u16string desAddr, const std::u16string scAddr,
        const std::u16string text, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliveryCallback) override;

    /**
     * @brief SendMessage
     * Sends a data Type SMS message.
     * @param slotId [in]
     * @param desAddr [in]
     * @param scAddr [in]
     * @param port [in]
     * @param data [in]
     * @param dataLen [in]
     * @param sendCallback [in]
     * @param deliverCallback [in]
     */
    void SendMessage(int32_t slotId, const std::u16string desAddr, const std::u16string scAddr, uint16_t port,
        const uint8_t *data, uint16_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
        const sptr<IDeliveryShortMessageCallback> &deliveryCallback) override;

    /**
     * @brief SetSmscAddr
     * Sets the address for the Short Message Service Center (SMSC) based on a specified slot ID.
     * @param slotId [in]
     * @param scAddr [in]
     * @return true
     * @return false
     */
    bool SetSmscAddr(int32_t slotId, const std::u16string &scAddr) override;

    /**
     * @brief GetSmscAddr
     * Obtains the SMSC address based on a specified slot ID.
     * @param slotId [in]
     * @return std::u16string
     */
    std::u16string GetSmscAddr(int32_t slotId) override;

    /**
     * @brief AddSimMessage
     * Add a sms to sim card.
     * @param slotId [in]
     * @param smsc [in]
     * @param pdu [in]
     * @param status [in]
     * @return true
     * @return false
     */
    bool AddSimMessage(
        int32_t slotId, const std::u16string &smsc, const std::u16string &pdu, SimMessageStatus status) override;

    /**
     * @brief DelSimMessage
     * Delete a sms in the sim card.
     * @param slotId [in]
     * @param msgIndex [in]
     * @return true
     * @return false
     */
    bool DelSimMessage(int32_t slotId, uint32_t msgIndex) override;

    /**
     * @brief UpdateSimMessage
     * Update a sms in the sim card.
     * @param slotId [in]
     * @param msgIndex [in]
     * @param newStatus [in]
     * @param pdu [in]
     * @param smsc [in]
     * @return true
     * @return false
     */
    bool UpdateSimMessage(int32_t slotId, uint32_t msgIndex, SimMessageStatus newStatus, const std::u16string &pdu,
        const std::u16string &smsc) override;

    /**
     * @brief GetAllSimMessages
     * Get sim card all the sms.
     * @param slotId [in]
     * @return std::vector<ShortMessage>
     */
    std::vector<ShortMessage> GetAllSimMessages(int32_t slotId) override;

    /**
     * @brief SetCBConfig
     * Configure a cell broadcast in a certain band range.
     * @param slotId [in]
     * @param enable [in]
     * @param fromMsgId [in]
     * @param toMsgId [in]
     * @param netType [in]
     * @return true
     * @return false
     */
    bool SetCBConfig(int32_t slotId, bool enable, uint32_t fromMsgId, uint32_t toMsgId, uint8_t netType) override;

    /**
     * @brief SetImsSmsConfig enable or disable IMS SMS.
     * @param slotId Indicates the card slot index number,
     * ranging from {@code 0} to the maximum card slot index number supported by the device.
     * @param enable Indicates enable or disable Ims sms
     * ranging {@code 0} disable Ims sms {@code 1} enable Ims sms
     * @return Returns {@code true} if enable or disable Ims Sms success; returns {@code false} otherwise.
     */
    bool SetImsSmsConfig(int32_t slotId, int32_t enable) override;

    /**
     * @brief SetDefaultSmsSlotId
     * Set the Default Sms Slot Id To SmsService
     * @param slotId [in]
     * @return true
     * @return false
     */
    bool SetDefaultSmsSlotId(int32_t slotId) override;

    /**
     * @brief GetDefaultSmsSlotId
     * Get the Default Sms Slot Id From SmsService
     * @return int32_t
     */
    int32_t GetDefaultSmsSlotId() override;

    /**
     * @brief SplitMessage
     * calculate Sms Message Split Segment count
     * @param message [in]
     * @return std::vector<std::u16string>
     */
    std::vector<std::u16string> SplitMessage(const std::u16string &message) override;

    /**
     * @brief GetSmsSegmentsInfo
     * calculate the Sms Message Segments Info
     * @param slotId [in]
     * @param message [in]
     * @param force7BitCode [in]
     * @param info [out]
     * @return true
     * @return false
     */
    bool GetSmsSegmentsInfo(int32_t slotId, const std::u16string &message, bool force7BitCode,
        ISmsServiceInterface::SmsSegmentsInfo &info) override;

    /**
     * @brief IsImsSmsSupported
     * Check Sms Is supported Ims newtwork
     * Hide this for inner system use
     * @return true
     * @return false
     */
    bool IsImsSmsSupported(int32_t slotId) override;

    /**
     * @brief GetImsShortMessageFormat
     * Get the Ims Short Message Format 3gpp/3gpp2
     * Hide this for inner system use
     * @return std::u16string
     */
    std::u16string GetImsShortMessageFormat() override;

    /**
     * @brief HasSmsCapability
     * Check whether it is supported Sms Capability
     * @return true
     * @return false
     */
    bool HasSmsCapability() override;

    /**
     * @brief GetServiceRunningState
     * Get service running state
     * @return ServiceRunningState
     */
    int32_t GetServiceRunningState();

    /**
     * @brief GetSpendTime
     * Get service start spend time
     * @return Spend time
     */
    int64_t GetSpendTime();

    /**
     * @brief GetEndTime
     * Get service start finish time
     * @return Spend time
     */
    int64_t GetEndTime();

    /**
     * transfer a string from GSM to UTF8
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertGSM7bitToUTF8bit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &dataSize) override;

    /**
     * transfer a string from EUCKR to UTF8
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertEUCKRToUTF8bit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &dataSize) override;

    /**
     * transfer a string from SHIFTJIS to UTF8
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertSHIFTJISToUTF8bit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &dataSize) override;

    /**
     * transfer a string from UCS2 to UTF8
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertUCS2ToUTF8bit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &dataSize) override;

    /**
     * transfer a string from UTF8 to UCS2
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertUTF8ToUCS2bit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &dataSize) override;

    /**
     * transfer a string from CDMA UTF8 to AUTO
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param getCodingType Indicates return code type
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertCdmaUTF8ToAutobit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &getCodingType, int32_t &dataSize) override;

    /**
     * transfer a string from GSM UTF8 to AUTO
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param getCodingType Indicates return code type
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertGsmUTF8ToAutobit(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &getCodingType, int32_t &dataSize) override;

    /**
     * transfer a string from UTF8 to GSM
     * @param pDestText Indicates destination char array,
     * @param maxLength Indicates destination array max length
     * @param pSrcText Indicates source char array
     * @param srcTextLen Indicates source array length
     * @param langIdVal Indicates return language id
     * @param abnormal Indicates return whether include abnormal character
     * @param dataSize Indicates return transfer length
     * @return Returns {@code true} if transfer success; returns {@code false} otherwise
     */
    bool ConvertUTF8ToGSM7bitfunc(unsigned char *pDestText, int32_t maxLength, const unsigned char *pSrcText,
        int32_t srcTextLen, int32_t &langIdVal, int32_t &abnormal, int32_t &decodeLen) override;

    /**
     * mms base64 encode
     * @param src Indicates source string,
     * @param dest Indicates destination string
     * @return Returns {@code true} if encode success; returns {@code false} otherwise
     */
    bool GetBase64Encode(std::string src, std::string &dest) override;

    /**
     * mms base64 decode
     * @param src Indicates source string,
     * @param dest Indicates destination string
     * @return Returns {@code true} if decode success; returns {@code false} otherwise
     */
    bool GetBase64Decode(std::string src, std::string &dest) override;

    /**
     * Get Encode String
     * @param encodeString Indicates output string,
     * @param charset Indicates character set,
     * @param valLength Indicates input string length,
     * @param strEncodeString Indicates input string
     * @return Returns {@code true} if decode success; returns {@code false} otherwise
     */
    bool GetEncodeStringFunc(
        std::string &encodeString, uint32_t charset, uint32_t valLength, std::string strEncodeString) override;

private:
    bool Init();
    void WaitCoreServiceToInit();
    bool NoPermissionOrParametersCheckFail(
        int32_t slotId, const std::u16string desAddr, const sptr<ISendShortMessageCallback> &sendCallback);
    bool ValidDestinationAddress(std::string desAddr);
    void TrimSmscAddr(std::string &sca);

private:
    int64_t bindTime_ = 0;
    int64_t endTime_ = 0;
    int64_t spendTime_ = 0;
    bool registerToService_ = false;
    ServiceRunningState state_ = ServiceRunningState::STATE_NOT_START;
    std::shared_ptr<SmsStateHandler> smsStateHandler_;
};
} // namespace Telephony
} // namespace OHOS
#endif