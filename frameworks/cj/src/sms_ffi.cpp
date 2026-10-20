/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sms_ffi.h"

#include "napi_util.h"
#include "short_message.h"
#include "sms_service_manager_client.h"

namespace OHOS {
namespace Telephony {
extern "C" {
void SetCJShortMessage(CJShortMessage* res, ShortMessage* shortMessage)
{
    if (res == nullptr || shortMessage == nullptr) {
        return;
    }
    res->protocolId = shortMessage->GetProtocolId();
    std::u16string smscAddress;
    shortMessage->GetScAddress(smscAddress);
    res->scAddress = MallocCString(NapiUtil::ToUtf8(smscAddress));
    res->scTimestamp = shortMessage->GetScTimestamp();
    res->status = shortMessage->GetStatus();
    res->visibleMessageBody = MallocCString(NapiUtil::ToUtf8(shortMessage->GetVisibleMessageBody()));
    res->visibleRawAddress = MallocCString(NapiUtil::ToUtf8(shortMessage->GetVisibleRawAddress()));
}

CJShortMessage FfiSMSCreateMessage(CArrI32 arr, char* specification)
{
    CJShortMessage res = CJShortMessage { 0 };
    if (arr.head == nullptr || arr.size > 10000) {     // 10000 is max arr size
        res.errCode = JS_ERROR_TELEPHONY_SYSTEM_ERROR;
        return res;
    }
    std::vector<unsigned char> pdu(arr.head, arr.head + arr.size);
    std::string specification8 = specification == nullptr ? "" : specification;
    if (pdu.empty() || specification8.empty()) {
        res.errCode = JS_ERROR_TELEPHONY_ARGUMENT_ERROR;
        return res;
    }
    std::u16string specification16 = NapiUtil::ToUtf16(specification8);
    auto shortMessage = new ShortMessage();
    if (shortMessage == nullptr) {
        res.errCode = JS_ERROR_TELEPHONY_SYSTEM_ERROR;
        return res;
    }
    res.errCode = ShortMessage::CreateMessage(pdu, specification16, *shortMessage);
    if (res.errCode == TELEPHONY_ERR_SUCCESS) {
        res.hasReplyPath = shortMessage->HasReplyPath();
        res.isReplaceMessage = shortMessage->IsReplaceMessage();
        res.isSmsStatusReportMessage = shortMessage->IsSmsStatusReportMessage();
        res.messageClass = shortMessage->GetMessageClass();
        std::vector<unsigned char> pdu_ = shortMessage->GetPdu();
        if (pdu_.size() == 0 || pdu_.size() > 10000) { //   10000 is max pdu size
            delete shortMessage;
            res.errCode = JS_ERROR_TELEPHONY_SYSTEM_ERROR;
            return res;
        }
        res.pdu.head = static_cast<int32_t*>(malloc(sizeof(int32_t) * pdu_.size()));
        if (res.pdu.head == nullptr) {
            delete shortMessage;
            res.errCode = JS_ERROR_TELEPHONY_SYSTEM_ERROR;
            return res;
        }
        for (size_t i = 0; i < pdu_.size(); i++) {
            res.pdu.head[i] = pdu_[i];
        }
        res.pdu.size = static_cast<int64_t>(pdu_.size());
        SetCJShortMessage(&res, shortMessage);
    }
    delete shortMessage;
    if (res.errCode != TELEPHONY_ERR_SUCCESS) {
        res.errCode = ConverErrorCodeForCj(res.errCode);
    }
    return res;
}

int32_t FfiSMSGetDefaultSmsSlotId()
{
    return Singleton<Telephony::SmsServiceManagerClient>::GetInstance().GetDefaultSmsSlotId();
}

RetDataI32 FfiSMSGetDefaultSmsSimId()
{
    RetDataI32 res = RetDataI32 { 0, 0 };
    res.code =
        ConverErrorCodeForCj(Singleton<Telephony::SmsServiceManagerClient>::GetInstance().GetDefaultSmsSimId(res.data));
    return res;
}

bool FfiSMSHasSmsCapability()
{
    return Singleton<Telephony::SmsServiceManagerClient>::GetInstance().HasSmsCapability();
}
}
} // namespace Telephony
} // namespace OHOS