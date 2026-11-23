/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SMS_FUZZER_REPLY_REMOTE_H
#define SMS_FUZZER_REPLY_REMOTE_H

#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "iremote_object.h"
#include "message_option.h"
#include "message_parcel.h"
#include "short_message.h"
#include "sms_fuzzer_utils.h"
#include "sms_service_ipc_interface_code.h"

namespace OHOS {
namespace SmsFuzz {
using Telephony::ShortMessage;
using Telephony::SmsServiceInterfaceCode;

// Shared in-process IRemoteObject mock. It lets the proxy / manager-client fuzzers drive the
// reply parsing code instead of returning early on a null remote, and keeps the IPC boundary
// inside the fuzz process (no samgr round trip).
class SmsServiceReplyRemote final : public IRemoteObject {
public:
    explicit SmsServiceReplyRemote(FuzzedDataProvider &fdp) : IRemoteObject(u""), fdp_(fdp) {}

    int32_t GetObjectRefCount() override
    {
        return 1;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        FillReply(code, reply);
        if (fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_IPC_ERROR) == 0) {
            return REMOTE_CALL_FAILURE;
        }
        return REMOTE_CALL_SUCCESS;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        return REMOTE_CALL_SUCCESS;
    }

private:
    int32_t MakeResult()
    {
        if (fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_RESULT_ERROR) == 0) {
            return REPLY_RESULT_FAILURE;
        }
        return REPLY_RESULT_SUCCESS;
    }

    void FillShortMessage(MessageParcel &reply)
    {
        ShortMessage message;
        message.visibleMessageBody_ = ConsumeText(fdp_);
        message.visibleRawAddress_ = ConsumeText(fdp_);
        message.scAddress_ = ConsumeText(fdp_);
        message.scTimestamp_ = fdp_.ConsumeIntegral<int64_t>();
        message.status_ = fdp_.ConsumeIntegral<int32_t>();
        message.messageClass_ = static_cast<ShortMessage::SmsMessageClass>(
            fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_MESSAGE_CLASS));
        message.simMessageStatus_ = static_cast<ShortMessage::SmsSimMessageStatus>(
            fdp_.ConsumeIntegralInRange<uint32_t>(0, MAX_SIM_STATUS));
        message.Marshalling(reply);
    }

    void FillSimMessageList(MessageParcel &reply)
    {
        reply.WriteInt32(REPLY_RESULT_SUCCESS);
        int32_t count = fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SIM_MESSAGE_COUNT);
        reply.WriteInt32(count);
        for (int32_t index = 0; index < count; ++index) {
            FillShortMessage(reply);
        }
    }

    void FillSplitMessage(MessageParcel &reply)
    {
        reply.WriteInt32(REPLY_RESULT_SUCCESS);
        int32_t count = fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SPLIT_COUNT);
        reply.WriteInt32(count);
        for (int32_t index = 0; index < count; ++index) {
            reply.WriteString16(ConsumeText(fdp_));
        }
    }

    void FillSegmentsInfo(MessageParcel &reply)
    {
        reply.WriteInt32(REPLY_RESULT_SUCCESS);
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegralInRange<int32_t>(0, MAX_SEGMENT_COUNT));
        reply.WriteInt32(fdp_.ConsumeIntegral<int32_t>());
    }

    void FillResultAndString(MessageParcel &reply)
    {
        int32_t result = MakeResult();
        reply.WriteInt32(result);
        if (result == REPLY_RESULT_SUCCESS) {
            reply.WriteString16(ConsumeText(fdp_));
        }
    }

    void FillBoolAndString(MessageParcel &reply)
    {
        bool result = fdp_.ConsumeBool();
        reply.WriteBool(result);
        if (result) {
            reply.WriteString16(ConsumeText(fdp_));
        }
    }

    void FillReply(uint32_t code, MessageParcel &reply)
    {
        switch (static_cast<SmsServiceInterfaceCode>(code)) {
            case SmsServiceInterfaceCode::SET_IMS_SMS_CONFIG:
            case SmsServiceInterfaceCode::HAS_SMS_CAPABILITY:
                reply.WriteBool(fdp_.ConsumeBool());
                break;
            case SmsServiceInterfaceCode::GET_SMSC_ADDRESS:
            case SmsServiceInterfaceCode::GET_IMS_SHORT_MESSAGE_FORMAT:
            case SmsServiceInterfaceCode::DOWNLOAD_MMS:
                FillResultAndString(reply);
                break;
            case SmsServiceInterfaceCode::MMS_BASE64_ENCODE:
            case SmsServiceInterfaceCode::MMS_BASE64_DECODE:
            case SmsServiceInterfaceCode::GET_ENCODE_STRING:
                FillBoolAndString(reply);
                break;
            case SmsServiceInterfaceCode::GET_ALL_SIM_MESSAGE:
                FillSimMessageList(reply);
                break;
            case SmsServiceInterfaceCode::SPLIT_MESSAGE:
                FillSplitMessage(reply);
                break;
            case SmsServiceInterfaceCode::GET_SMS_SEGMENTS_INFO:
                FillSegmentsInfo(reply);
                break;
            case SmsServiceInterfaceCode::IS_IMS_SMS_SUPPORTED:
                reply.WriteInt32(MakeResult());
                reply.WriteBool(fdp_.ConsumeBool());
                break;
            case SmsServiceInterfaceCode::CREATE_MESSAGE:
                reply.WriteInt32(MakeResult());
                FillShortMessage(reply);
                break;
            default:
                reply.WriteInt32(MakeResult());
                break;
        }
    }

    FuzzedDataProvider &fdp_;
};
} // namespace SmsFuzz
} // namespace OHOS
#endif // SMS_FUZZER_REPLY_REMOTE_H
