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

#include "sms_interface_stub.h"

#include "sms_interface_manager.h"
#include "sms_receive_reliability_handler.h"
#include "sms_service.h"
#include "string_utils.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "telephony_types.h"

namespace OHOS {
namespace Telephony {
using namespace std;
constexpr static int32_t CB_RANGE_LIST_MAX_SIZE = 256;
static int32_t SMS_SHORT_CODE_TYPE_UNKNOWN = -1;
static inline bool IsValidSlotId(int32_t slotId)
{
    return ((slotId >= DEFAULT_SIM_SLOT_ID) && (slotId < SIM_SLOT_COUNT_MD));
}

SmsInterfaceStub::SmsInterfaceStub()
{
    memberFuncMap_[SmsServiceInterfaceCode::TEXT_BASED_SMS_DELIVERY] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSendSmsTextRequest(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SEND_SMS_TEXT_WITHOUT_SAVE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSendSmsTextWithoutSaveRequest(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::DATA_BASED_SMS_DELIVERY] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSendSmsDataRequest(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SET_SMSC_ADDRESS] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSetSmscAddr(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_SMSC_ADDRESS] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetSmscAddr(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::ADD_SIM_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnAddSimMessage(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::DEL_SIM_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnDelSimMessage(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::UPDATE_SIM_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnUpdateSimMessage(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetAllSimMessages(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SET_CB_CONFIG] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSetCBConfig(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SET_CB_CONFIG_LIST] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSetCBConfigList(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSetImsSmsConfig(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SET_DEFAULT_SMS_SLOT_ID] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSetDefaultSmsSlotId(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_DEFAULT_SMS_SLOT_ID] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetDefaultSmsSlotId(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_DEFAULT_SMS_SIM_ID] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetDefaultSmsSimId(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SPLIT_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSplitMessage(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetSmsSegmentsInfo(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_IMS_SHORT_MESSAGE_FORMAT] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetImsShortMessageFormat(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnIsImsSmsSupported(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::HAS_SMS_CAPABILITY] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnHasSmsCapability(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_SMS_SHORT_CODE_TYPE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetSmsShortCodeType(data, reply, option); };
    RegisterServiceCode();
}

void SmsInterfaceStub::RegisterServiceCode()
{
    memberFuncMap_[SmsServiceInterfaceCode::CREATE_MESSAGE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnCreateMessage(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::MMS_BASE64_ENCODE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetBase64Encode(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::MMS_BASE64_DECODE] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetBase64Decode(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::GET_ENCODE_STRING] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnGetEncodeStringFunc(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::SEND_MMS] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnSendMms(data, reply, option); };
    memberFuncMap_[SmsServiceInterfaceCode::DOWNLOAD_MMS] = [this](MessageParcel &data,
        MessageParcel &reply, MessageOption &option) { OnDownloadMms(data, reply, option); };
}

SmsInterfaceStub::~SmsInterfaceStub()
{
    slotSmsInterfaceManagerMap_.clear();
    memberFuncMap_.clear();
}

void SmsInterfaceStub::InitModule()
{
    static bool bInitModule = false;
    if (bInitModule) {
        return;
    }
    bInitModule = true;
    std::lock_guard<std::mutex> lock(mutex_);
    for (int32_t slotId = 0; slotId < SIM_SLOT_COUNT_MD; ++slotId) {
        slotSmsInterfaceManagerMap_[slotId] = std::make_shared<SmsInterfaceManager>(slotId);
        if (slotSmsInterfaceManagerMap_[slotId] == nullptr) {
            TELEPHONY_LOGE("SmsInterfaceStub InitModule slotSmsInterfaceManagerMap_[%{public}d] is nullptr", slotId);
            return;
        }
        slotSmsInterfaceManagerMap_[slotId]->InitInterfaceManager();

        TelFFRTUtils::Submit([slotId]() {
            auto reliabilityHandler = std::make_shared<SmsReceiveReliabilityHandler>(slotId);
            if (reliabilityHandler == nullptr) {
                TELEPHONY_LOGE("reliabilityHandler nullptr");
                return;
            }
            if (!reliabilityHandler->DeleteExpireSmsFromDB()) {
                return;
            }
            if (!reliabilityHandler->CheckSmsCapable()) {
                TELEPHONY_LOGE("sms receive capable unSupport");
                return;
            }
            reliabilityHandler->SmsReceiveReliabilityProcessing();
        });
    }
}

std::shared_ptr<SmsInterfaceManager> SmsInterfaceStub::GetSmsInterfaceManager(int32_t slotId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<uint32_t, std::shared_ptr<SmsInterfaceManager>>::iterator iter =
        slotSmsInterfaceManagerMap_.find(slotId);
    if (iter != slotSmsInterfaceManagerMap_.end()) {
        return iter->second;
    }
    return nullptr;
}

std::shared_ptr<SmsInterfaceManager> SmsInterfaceStub::GetSmsInterfaceManager()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &iter : slotSmsInterfaceManagerMap_) {
        if (iter.second != nullptr) {
            return iter.second;
        }
    }
    return nullptr;
}

void SmsInterfaceStub::OnSendSmsTextRequest(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    sptr<ISendShortMessageCallback> sendCallback = nullptr;
    sptr<IDeliveryShortMessageCallback> deliveryCallback = nullptr;
    int32_t slotId = data.ReadInt32();
    u16string desAddr = data.ReadString16();
    u16string scAddr = data.ReadString16();
    u16string text = data.ReadString16();
    if (!IsValidSlotId(slotId)) {
        TELEPHONY_LOGE("invalid slotId：%{public}d", slotId);
        return;
    }

    sptr<IRemoteObject> remoteSendCallback = data.ReadRemoteObject();
    sptr<IRemoteObject> remoteDeliveryCallback = data.ReadRemoteObject();
    if (remoteSendCallback != nullptr) {
        sendCallback = iface_cast<ISendShortMessageCallback>(remoteSendCallback);
    }
    if (remoteDeliveryCallback != nullptr) {
        deliveryCallback = iface_cast<IDeliveryShortMessageCallback>(remoteDeliveryCallback);
    }
    std::string bundleName = data.ReadString();
    bool isMmsApp = (bundleName == MMS_APP);
    TELEPHONY_LOGI("MessageID::TEXT_BASED_SMS_DELIVERY %{public}d;isMmsApp:%{public}d;bundleName = %{public}s",
        slotId, isMmsApp, bundleName.c_str());
    RemoveSpacesInDesAddr(desAddr);
    int32_t result = SendMessage(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback, isMmsApp);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnSendSmsTextWithoutSaveRequest(MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    sptr<ISendShortMessageCallback> sendCallback = nullptr;
    sptr<IDeliveryShortMessageCallback> deliveryCallback = nullptr;
    int32_t slotId = data.ReadInt32();
    u16string desAddr = data.ReadString16();
    u16string scAddr = data.ReadString16();
    u16string text = data.ReadString16();
    if (!IsValidSlotId(slotId)) {
        TELEPHONY_LOGE("invalid slotId: %{public}d", slotId);
        return;
    }

    sptr<IRemoteObject> remoteSendCallback = data.ReadRemoteObject();
    sptr<IRemoteObject> remoteDeliveryCallback = data.ReadRemoteObject();
    if (remoteSendCallback != nullptr) {
        sendCallback = iface_cast<ISendShortMessageCallback>(remoteSendCallback);
    }
    if (remoteDeliveryCallback != nullptr) {
        deliveryCallback = iface_cast<IDeliveryShortMessageCallback>(remoteDeliveryCallback);
    }
    TELEPHONY_LOGI("MessageID::SEND_SMS_TEXT_WITHOUT_SAVE %{public}d", slotId);
    RemoveSpacesInDesAddr(desAddr);
    std::string bundleName = data.ReadString();
    TELEPHONY_LOGI("bundleName = %{public}s", bundleName.c_str());
    int32_t result = SendMessageWithoutSave(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnSendSmsDataRequest(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    sptr<ISendShortMessageCallback> sendCallback = nullptr;
    sptr<IDeliveryShortMessageCallback> deliveryCallback = nullptr;
    int32_t slotId = data.ReadInt32();
    u16string desAddr = data.ReadString16();
    u16string scAddr = data.ReadString16();
    int16_t port = data.ReadInt16();
    if (!IsValidSlotId(slotId)) {
        TELEPHONY_LOGE("invalid slotId：%{public}d", slotId);
        return;
    }

    sptr<IRemoteObject> remoteSendCallback = data.ReadRemoteObject();
    sptr<IRemoteObject> remoteDeliveryCallback = data.ReadRemoteObject();
    if (remoteSendCallback != nullptr) {
        sendCallback = iface_cast<ISendShortMessageCallback>(remoteSendCallback);
    }
    if (remoteDeliveryCallback != nullptr) {
        deliveryCallback = iface_cast<IDeliveryShortMessageCallback>(remoteDeliveryCallback);
    }
    int16_t dataLen = data.ReadInt16();
    if (dataLen < 1) {
        TELEPHONY_LOGE("dataLen is invalid");
        return;
    }
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(dataLen));
    if (buffer == nullptr) {
        return;
    }
    RemoveSpacesInDesAddr(desAddr);
    int32_t result = SendMessage(slotId, desAddr, scAddr, port, buffer, dataLen, sendCallback, deliveryCallback);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::RemoveSpacesInDesAddr(std::u16string &desAddr)
{
    // Remove spaces in desAddr
    if (desAddr.empty() || desAddr.size() >= MAX_ADDRESS_LEN) {
        TELEPHONY_LOGE("RemoveSpacesInDesAddr desAddr is invalid");
        return;
    }

    std::u16string storeAddr = desAddr;
    int32_t count = static_cast<int32_t>(desAddr.size());
    int32_t indexDes = 0;
    int32_t indexResult = 0;
    while (indexDes < count) {
        if (desAddr[indexDes] != ' ') {
            storeAddr[indexResult] = desAddr[indexDes];
            indexResult++;
        }
        indexDes++;
    }
    desAddr = storeAddr.substr(0, indexResult);
}

void SmsInterfaceStub::OnSetSmscAddr(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    std::u16string scAddr = data.ReadString16();
    int32_t result = SetSmscAddr(slotId, scAddr);
    TELEPHONY_LOGI("set smsc result:%{public}d", result == TELEPHONY_ERR_SUCCESS);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnGetSmscAddr(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::u16string smscAddress;
    int32_t slotId = data.ReadInt32();
    int32_t result = GetSmscAddr(slotId, smscAddress);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetSmscAddr write reply failed.");
        return;
    }
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetSmscAddr result is not TELEPHONY_ERR_SUCCESS.");
        return;
    }

    if (!reply.WriteString16(smscAddress)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetSmscAddr write reply failed.");
        return;
    }
}

void SmsInterfaceStub::OnAddSimMessage(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    std::u16string smsc = data.ReadString16();
    std::u16string pdu = data.ReadString16();
    uint32_t status = data.ReadUint32();
    if (status > SIM_MESSAGE_STATUS_SENT || status < SIM_MESSAGE_STATUS_UNREAD) {
        return;
    }
    int32_t result = AddSimMessage(slotId, smsc, pdu, static_cast<SimMessageStatus>(status));
    TELEPHONY_LOGI("AddSimMessage result %{public}d", result);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnDelSimMessage(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    uint32_t msgIndex = data.ReadUint32();
    int32_t result = DelSimMessage(slotId, msgIndex);
    TELEPHONY_LOGI("DelSimMessage result %{public}d", result);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnUpdateSimMessage(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    uint32_t msgIndex = data.ReadUint32();
    uint32_t newStatus = data.ReadUint32();
    std::u16string pdu = data.ReadString16();
    std::u16string smsc = data.ReadString16();
    if (newStatus > SIM_MESSAGE_STATUS_SENT || newStatus < SIM_MESSAGE_STATUS_UNREAD) {
        return;
    }
    int32_t result = UpdateSimMessage(slotId, msgIndex, static_cast<SimMessageStatus>(newStatus), pdu, smsc);
    TELEPHONY_LOGI("UpdateSimMessage result %{public}d", result);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnGetAllSimMessages(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::vector<ShortMessage> message;
    int32_t slotId = data.ReadInt32();
    int32_t result = GetAllSimMessages(slotId, message);
    TELEPHONY_LOGI("GetAllSimMessages result %{public}d size %{public}zu", result, message.size());
    reply.WriteInt32(result);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetSmscAddr result is not TELEPHONY_ERR_SUCCESS.");
        return;
    }
    int32_t resultLen = static_cast<int32_t>(message.size());
    reply.WriteInt32(resultLen);
    for (const auto &v : message) {
        v.Marshalling(reply);
    }
}

void SmsInterfaceStub::OnSetCBConfig(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    TELEPHONY_LOGD("set cb config slotId:%{public}d", slotId);
    bool enable = data.ReadBool();
    uint32_t fromMsgId = data.ReadUint32();
    uint32_t toMsgId = data.ReadUint32();
    uint8_t ranType = data.ReadUint8();
    int32_t result = SetCBConfig(slotId, enable, fromMsgId, toMsgId, ranType);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("OnSetCBConfig fail, result:%{public}d, slotId:%{public}d", result, slotId);
    }
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnSetCBConfigList(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    int32_t messageIdsSize = data.ReadInt32();
    if (messageIdsSize > CB_RANGE_LIST_MAX_SIZE) {
        return;
    }
    std::vector<int32_t> messageIds;
    for (int32_t i = 0; i < messageIdsSize; i++) {
        messageIds.push_back(data.ReadInt32());
    }
    int32_t ranType = data.ReadInt32();
    int32_t result = SetCBConfigList(slotId, messageIds, ranType);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("OnSetCBConfigList fail, result:%{public}d, slotId:%{public}d", result, slotId);
    }
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnSetImsSmsConfig(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    bool result = false;
    int32_t slotId = data.ReadInt32();
    int32_t enable = data.ReadInt32();
    result = SetImsSmsConfig(slotId, enable);
    TELEPHONY_LOGI("SetImsSmsConfig result %{public}d", result);
    reply.WriteBool(result);
}

void SmsInterfaceStub::OnSetDefaultSmsSlotId(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    int32_t result = SetDefaultSmsSlotId(slotId);
    TELEPHONY_LOGI("SetDefaultSmsSlotId result %{public}d", result);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnGetDefaultSmsSlotId(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t result = 0;
    result = GetDefaultSmsSlotId();
    TELEPHONY_LOGI("SetDefaultSmsSlotId result %{public}d", result);
    reply.WriteInt32(result);
}

void SmsInterfaceStub::OnGetDefaultSmsSimId(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t result = 0;
    int32_t simId = 0;
    result = GetDefaultSmsSimId(simId);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("write int32 reply failed.");
        return;
    }
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("result %{public}d", result);
        return;
    }
    if (!reply.WriteInt32(simId)) {
        TELEPHONY_LOGE("write int32 reply failed.");
        return;
    }
}

void SmsInterfaceStub::OnSplitMessage(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::vector<std::u16string> splitMessage;
    std::u16string message = data.ReadString16();
    int32_t result = SplitMessage(message, splitMessage);
    reply.WriteInt32(result);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnSplitMessage result is not TELEPHONY_ERR_SUCCESS.");
        return;
    }
    int32_t resultLen = static_cast<int32_t>(splitMessage.size());
    TELEPHONY_LOGI("SplitMessage size %{public}d", resultLen);
    reply.WriteInt32(resultLen);
    for (const auto &item : splitMessage) {
        reply.WriteString16(item);
    }
}

void SmsInterfaceStub::OnGetSmsSegmentsInfo(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    std::u16string message = data.ReadString16();
    bool force7BitCode = data.ReadBool();

    SmsSegmentsInfo segInfo;
    int32_t result = GetSmsSegmentsInfo(slotId, message, force7BitCode, segInfo);
    reply.WriteInt32(result);

    if (result == TELEPHONY_ERR_SUCCESS) {
        reply.WriteInt32(segInfo.msgSegCount);
        reply.WriteInt32(segInfo.msgEncodingCount);
        reply.WriteInt32(segInfo.msgRemainCount);
        reply.WriteInt32(static_cast<int32_t>(segInfo.msgCodeScheme));
    }
}

void SmsInterfaceStub::OnIsImsSmsSupported(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    bool isSupported = false;
    int32_t result = IsImsSmsSupported(slotId, isSupported);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnIsImsSmsSupported write reply failed.");
        return;
    }
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnIsImsSmsSupported result is not TELEPHONY_ERR_SUCCESS.");
        return;
    }
    if (!reply.WriteBool(isSupported)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnIsImsSmsSupported write reply failed.");
        return;
    }
}

void SmsInterfaceStub::OnGetImsShortMessageFormat(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::u16string format;
    int32_t result = GetImsShortMessageFormat(format);
    reply.WriteInt32(result);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetImsShortMessageFormat result is not TELEPHONY_ERR_SUCCESS.");
        return;
    }
    reply.WriteString16(format);
}

void SmsInterfaceStub::OnGetSmsShortCodeType(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    std::string desAddr = data.ReadString();
    int32_t smsShortCodeType = SMS_SHORT_CODE_TYPE_UNKNOWN;
    int32_t result = GetSmsShortCodeType(slotId, desAddr, smsShortCodeType);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnGetSmsShortCodeType result is not TELEPHONY_ERR_SUCCESS.");
        reply.WriteInt32(SMS_SHORT_CODE_TYPE_UNKNOWN);
        return;
    }
    reply.WriteInt32(smsShortCodeType);
}

void SmsInterfaceStub::OnHasSmsCapability(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    reply.WriteBool(HasSmsCapability());
}

void SmsInterfaceStub::OnCreateMessage(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::string pdu = data.ReadString();
    std::string specification = data.ReadString();
    ShortMessage message;
    int32_t result = CreateMessage(pdu, specification, message);

    reply.WriteInt32(result);
    if (result != TELEPHONY_ERR_SUCCESS) {
        return;
    }
    if (!message.Marshalling(reply)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnCreateMessage fail");
    }
}

void SmsInterfaceStub::OnGetBase64Encode(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    bool result = false;

    u16string src = data.ReadString16();
    std::string dest;
    result = GetBase64Encode(StringUtils::ToUtf8(src), dest);
    reply.WriteBool(result);
    if (!result) {
        return;
    }
    reply.WriteString16(StringUtils::ToUtf16(dest));
}

void SmsInterfaceStub::OnGetBase64Decode(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    bool result = false;
    u16string src = data.ReadString16();
    std::string dest;
    result = GetBase64Decode(StringUtils::ToUtf8(src), dest);
    reply.WriteBool(result);
    if (!result) {
        return;
    }
    reply.WriteString16(StringUtils::ToUtf16(dest));
}

void SmsInterfaceStub::OnGetEncodeStringFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    bool result = false;
    uint32_t charset = data.ReadUint32();
    uint32_t valLength = data.ReadUint32();
    u16string strEncodeString = data.ReadString16();
    std::string str = StringUtils::ToUtf8(strEncodeString);
    std::string encodeString;

    if (valLength != str.length()) {
        TELEPHONY_LOGE("invalid valLength!");
        return;
    }
    result = GetEncodeStringFunc(encodeString, charset, valLength, str);
    reply.WriteBool(result);
    if (!result) {
        return;
    }
    reply.WriteString16(StringUtils::ToUtf16(encodeString));
}

void SmsInterfaceStub::OnSendMms(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    TELEPHONY_LOGI("send mms slotId:%{public}d", slotId);
    u16string mmsc = data.ReadString16();
    u16string mmsData = data.ReadString16();
    u16string ua = data.ReadString16();
    u16string uaprof = data.ReadString16();
    int64_t time = data.ReadInt64();
    std::string bundleName = data.ReadString();
    TELEPHONY_LOGI("SmsInterfaceStub::OnSendMms read time stamp :%{public}s;bundleName:%{public}s",
        std::to_string(time).c_str(), bundleName.c_str());
    bool isMmsApp = (bundleName == MMS_APP);
    int32_t result = SendMms(slotId, mmsc, mmsData, ua, uaprof, time, isMmsApp);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnSendMms write reply failed");
        return;
    }
}

void SmsInterfaceStub::OnDownloadMms(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int32_t slotId = data.ReadInt32();
    TELEPHONY_LOGI("download mms slotId:%{public}d", slotId);
    u16string mmsc = data.ReadString16();
    u16string mmsData = data.ReadString16();
    u16string ua = data.ReadString16();
    u16string uaprof = data.ReadString16();
    int32_t result = DownloadMms(slotId, mmsc, mmsData, ua, uaprof);
    if (!reply.WriteInt32(result)) {
        TELEPHONY_LOGE("SmsInterfaceStub::OnDownloadMms write reply failed");
        return;
    }
    reply.WriteString16(mmsData);
    TELEPHONY_LOGI("SmsInterfaceStub::OnDownloadMms dbUrls:%{public}s", StringUtils::ToUtf8(mmsData).c_str());
}

int SmsInterfaceStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    TELEPHONY_LOGD("SmsInterfaceStub::OnRemoteRequest code:%{public}d", code);
    std::u16string myDescripter = SmsInterfaceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (myDescripter != remoteDescripter) {
        TELEPHONY_LOGE("descriptor checked fail");
        return TELEPHONY_ERR_DESCRIPTOR_MISMATCH;
    }

    auto itFunc = memberFuncMap_.find(static_cast<SmsServiceInterfaceCode>(code));
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            memberFunc(data, reply, option);
            return TELEPHONY_ERR_SUCCESS;
        }
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace Telephony
} // namespace OHOS
