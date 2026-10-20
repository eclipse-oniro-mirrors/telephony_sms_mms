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

#include "cdma_sms_transport_message.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
static constexpr uint8_t PARAMETER_ID_BYTES = 1;
static constexpr uint8_t PARAMETER_LEN_BYTES = 1;
static constexpr uint16_t CAT_UNKNOWN = 0x8001;
static constexpr uint8_t PARAMETER_SIZE_MAX = 9;

CdmaSmsTransportMessage::~CdmaSmsTransportMessage()
{
    parameter_.clear();
    parameter_.shrink_to_fit();
}

bool CdmaSmsTransportMessage::Encode(SmsWriteBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }
    if (parameter_.size() == 0) {
        TELEPHONY_LOGE("parameter is empty");
        return false;
    }

    pdu.SetIndex(0);
    if (!pdu.WriteByte(type_)) {
        TELEPHONY_LOGE("type write error");
        return false;
    }
    for (size_t i = 0; i < parameter_.size(); i++) {
        if (parameter_[i] == nullptr || !parameter_[i]->Encode(pdu)) {
            TELEPHONY_LOGE("parameter encode error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsTransportMessage::Decode(SmsReadBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return false;
    }
    if (parameter_.size() == 0) {
        TELEPHONY_LOGE("parameter is empty");
        return false;
    }

    pdu.SetIndex(0);
    if (!pdu.ReadByte(type_)) {
        TELEPHONY_LOGE("type read error");
        return false;
    }
    if (type_ != P2P && type_ != BROADCAST && type_ != ACK) {
        TELEPHONY_LOGE("type[%{public}d] is invalid", type_);
        return false;
    }
    for (size_t i = 0; i < parameter_.size(); i++) {
        if (parameter_[i] == nullptr || !parameter_[i]->Decode(pdu)) {
            TELEPHONY_LOGE("parameter decode error");
            return false;
        }
    }
    return true;
}

bool CdmaSmsTransportMessage::IsEmpty()
{
    if (parameter_.size() == 0) {
        TELEPHONY_LOGE("parameter is empty");
        return true;
    }
    return false;
}

std::unique_ptr<CdmaSmsTransportMessage> CdmaSmsTransportMessage::CreateTransportMessage(CdmaTransportMsg &msg)
{
    if (msg.type == CdmaTransportMsgType::P2P) {
        return std::make_unique<CdmaSmsP2pMessage>(msg.data.p2p);
    } else if (msg.type == CdmaTransportMsgType::BROADCAST) {
        return std::make_unique<CdmaSmsBroadcastMessage>(msg.data.broadcast);
    } else if (msg.type == CdmaTransportMsgType::ACK) {
        return std::make_unique<CdmaSmsAckMessage>(msg.data.ack);
    } else {
        TELEPHONY_LOGW("CreateTransportMessage: type [%{public}d] not support", static_cast<uint8_t>(msg.type));
        return nullptr;
    }
}

std::unique_ptr<CdmaSmsTransportMessage> CdmaSmsTransportMessage::CreateTransportMessage(
    CdmaTransportMsg &msg, SmsReadBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return nullptr;
    }

    uint8_t type = RESERVED;
    if (!pdu.ReadByte(type)) {
        TELEPHONY_LOGE("type read error");
        return nullptr;
    }
    if (type == P2P) {
        msg.type = CdmaTransportMsgType::P2P;
        return std::make_unique<CdmaSmsP2pMessage>(msg.data.p2p, pdu);
    } else if (type == BROADCAST) {
        msg.type = CdmaTransportMsgType::BROADCAST;
        return std::make_unique<CdmaSmsBroadcastMessage>(msg.data.broadcast, pdu);
    } else if (type == ACK) {
        msg.type = CdmaTransportMsgType::ACK;
        return std::make_unique<CdmaSmsAckMessage>(msg.data.ack, pdu);
    } else {
        msg.type = CdmaTransportMsgType::RESERVED;
        TELEPHONY_LOGW("CreateTransportMessage: type [%{public}d] not support", type);
        return nullptr;
    }
}

CdmaSmsP2pMessage::CdmaSmsP2pMessage(CdmaP2PMsg &msg)
{
    type_ = P2P;
    // Teleservice Identifier
    parameter_.push_back(std::make_unique<CdmaSmsTeleserviceId>(msg.teleserviceId));
    // Service Category
    if (msg.serviceCtg < CAT_UNKNOWN) {
        parameter_.push_back(std::make_unique<CdmaSmsServiceCategory>(msg.serviceCtg));
    }
    // Destination Address
    parameter_.push_back(std::make_unique<CdmaSmsAddressParameter>(msg.address, CdmaSmsParameterRecord::DEST_ADDRESS));
    // Destination Subaddress
    if (msg.subAddress.addrLen > 0) {
        parameter_.push_back(
            std::make_unique<CdmaSmsSubaddress>(msg.subAddress, CdmaSmsParameterRecord::DEST_SUB_ADDRESS));
    }
    // Bearer Reply Option
    if (msg.replySeq > 0) {
        parameter_.push_back(std::make_unique<CdmaSmsBearerReply>(msg.replySeq));
    }
    // Bearer Data
    parameter_.push_back(std::make_unique<CdmaSmsBearerData>(msg.telesvcMsg));
}

CdmaSmsP2pMessage::CdmaSmsP2pMessage(CdmaP2PMsg &msg, SmsReadBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return;
    }

    type_ = P2P;
    msg.serviceCtg = CAT_UNKNOWN;
    uint8_t id = 0;
    uint8_t len = 0;
    while ((parameter_.size() < PARAMETER_SIZE_MAX) && pdu.ReadByte(id) && pdu.ReadByte(len)) {
        if (id == CdmaSmsParameterRecord::TELESERVICE_ID) {
            parameter_.push_back(std::make_unique<CdmaSmsTeleserviceId>(msg.teleserviceId));
        } else if (id == CdmaSmsParameterRecord::SERVICE_CATEGORY) {
            parameter_.push_back(std::make_unique<CdmaSmsServiceCategory>(msg.serviceCtg));
        } else if (id == CdmaSmsParameterRecord::ORG_ADDRESS || id == CdmaSmsParameterRecord::DEST_ADDRESS) {
            parameter_.push_back(std::make_unique<CdmaSmsAddressParameter>(msg.address, id));
        } else if (id == CdmaSmsParameterRecord::ORG_SUB_ADDRESS || id == CdmaSmsParameterRecord::DEST_SUB_ADDRESS) {
            parameter_.push_back(std::make_unique<CdmaSmsSubaddress>(msg.subAddress, id));
        } else if (id == CdmaSmsParameterRecord::BEARER_REPLY_OPTION) {
            parameter_.push_back(std::make_unique<CdmaSmsBearerReply>(msg.replySeq));
        } else if (id == CdmaSmsParameterRecord::BEARER_DATA) {
            uint16_t index = pdu.GetIndex();
            parameter_.push_back(std::make_unique<CdmaSmsBearerData>(msg.telesvcMsg, pdu));
            pdu.SetIndex(index);
        } else {
            TELEPHONY_LOGE("parameter ID[%{public}d] not support", id);
            return;
        }
        pdu.MoveForward(len);
    }
}

CdmaSmsBroadcastMessage::CdmaSmsBroadcastMessage(CdmaBroadCastMsg &msg)
{
    type_ = BROADCAST;
    // Service Category
    parameter_.push_back(std::make_unique<CdmaSmsServiceCategory>(msg.serviceCtg));
    // Bearer Data
    parameter_.push_back(std::make_unique<CdmaSmsBearerData>(msg.telesvcMsg));
}

CdmaSmsBroadcastMessage::CdmaSmsBroadcastMessage(CdmaBroadCastMsg &msg, SmsReadBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return;
    }

    type_ = BROADCAST;
    msg.serviceCtg = CAT_UNKNOWN;
    uint8_t id = 0;
    uint8_t len = 0;
    while ((parameter_.size() < PARAMETER_SIZE_MAX) && pdu.ReadByte(id) && pdu.ReadByte(len)) {
        if (id == CdmaSmsParameterRecord::SERVICE_CATEGORY) {
            parameter_.push_back(std::make_unique<CdmaSmsServiceCategory>(msg.serviceCtg));
            uint16_t index = pdu.GetIndex();
            pdu.MoveBack(PARAMETER_ID_BYTES + PARAMETER_LEN_BYTES); // back to ID field
            if (parameter_.empty() || parameter_.back() == nullptr) {
                TELEPHONY_LOGE("parameter is invalid");
                return;
            }
            parameter_.back()->Decode(pdu);
            pdu.SetIndex(index);
        } else if (id == CdmaSmsParameterRecord::BEARER_DATA) {
            uint16_t index = pdu.GetIndex();
            bool isCMAS = false;
            if (msg.serviceCtg >= static_cast<uint16_t>(SmsServiceCtg::CMAS_PRESIDENTIAL) &&
                msg.serviceCtg <= static_cast<uint16_t>(SmsServiceCtg::CMAS_TEST)) {
                isCMAS = true;
            }
            parameter_.push_back(std::make_unique<CdmaSmsBearerData>(msg.telesvcMsg, pdu, isCMAS));
            pdu.SetIndex(index);
        } else {
            TELEPHONY_LOGE("parameter ID[%{public}d] not support", id);
            return;
        }
        pdu.MoveForward(len);
    }
}

CdmaSmsAckMessage::CdmaSmsAckMessage(CdmaAckMsg &msg)
{
    type_ = ACK;
    // Destination Address
    parameter_.push_back(std::make_unique<CdmaSmsAddressParameter>(msg.address, CdmaSmsParameterRecord::DEST_ADDRESS));
    if (msg.subAddress.addrLen > 0) {
        // Destination Subaddress
        parameter_.push_back(
            std::make_unique<CdmaSmsSubaddress>(msg.subAddress, CdmaSmsParameterRecord::DEST_SUB_ADDRESS));
    }
    // Cause Codes
    parameter_.push_back(std::make_unique<CdmaSmsCauseCodes>(msg.causeCode));
}

CdmaSmsAckMessage::CdmaSmsAckMessage(CdmaAckMsg &msg, SmsReadBuffer &pdu)
{
    if (pdu.IsEmpty()) {
        TELEPHONY_LOGE("pdu is empty");
        return;
    }

    type_ = ACK;
    uint8_t id = 0;
    uint8_t len = 0;
    while ((parameter_.size() < PARAMETER_SIZE_MAX) && pdu.ReadByte(id) && pdu.ReadByte(len)) {
        if (id == CdmaSmsParameterRecord::DEST_ADDRESS) {
            parameter_.push_back(std::make_unique<CdmaSmsAddressParameter>(msg.address, id));
        } else if (id == CdmaSmsParameterRecord::DEST_SUB_ADDRESS) {
            parameter_.push_back(std::make_unique<CdmaSmsSubaddress>(msg.subAddress, id));
        } else if (id == CdmaSmsParameterRecord::CAUSE_CODES) {
            parameter_.push_back(std::make_unique<CdmaSmsCauseCodes>(msg.causeCode));
        } else {
            TELEPHONY_LOGE("parameter ID[%{public}d] not support", id);
            return;
        }
        pdu.MoveForward(len);
    }
}

} // namespace Telephony
} // namespace OHOS
