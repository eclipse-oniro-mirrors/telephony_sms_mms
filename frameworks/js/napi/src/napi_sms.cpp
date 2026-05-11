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

#include "napi_sms.h"

#include "delivery_callback.h"
#include "napi_mms.h"
#include "napi_send_recv_mms.h"
#include "send_callback.h"
#include "sms_mms_errors.h"

#define PARAMETER_COUNT_3 3
#define PARAMETERS_INDEX_2 2
#define PARAMETERS_INDEX_3 3

namespace OHOS {
namespace Telephony {
namespace {
const std::string SLOT_ID_STR = "slotId";
const std::string DESTINATION_HOST_STR = "destinationHost";
const std::string SERVICE_CENTER_STR = "serviceCenter";
const std::string CONTENT_STR = "content";
const std::string DESTINATION_PORT_STR = "destinationPort";
const std::string SEND_CALLBACK_STR = "sendCallback";
const std::string DELIVERY_CALLBACK_STR = "deliveryCallback";
const std::string IS_PERSIST_STR = "isPersist";
static const int32_t DEFAULT_REF_COUNT = 1;
static bool g_validPort = false;
} // namespace

static int32_t GetDefaultSmsSlotId()
{
    return DEFAULT_SIM_SLOT_ID;
}

static inline bool IsValidSlotId(int32_t slotId)
{
    return ((slotId >= DEFAULT_SIM_SLOT_ID) && (slotId < SIM_SLOT_COUNT));
}

static bool InValidSlotIdOrInValidPort(int32_t slotId, uint16_t port)
{
    if (!IsValidSlotId(slotId)) {
        TELEPHONY_LOGE("InValidSlotIdOrInValidPort invalid slotid");
        return true;
    }

    if (!g_validPort) {
        TELEPHONY_LOGE("InValidSlotIdOrInValidPort invalid port");
        return true;
    }
    g_validPort = false;
    return false;
}

static __attribute__((noinline)) bool IsParamValid(napi_env env, void *data)
{
    if (data == nullptr) {
        NapiUtil::ThrowParameterError(env);
        TELEPHONY_LOGE("data is nullptr!");
        return false;
    }
    return true;
}

static int32_t ActuallySendTextMessage(SendMessageContext &parameter, std::unique_ptr<SendCallback> sendCallback,
    std::unique_ptr<DeliveryCallback> deliveryCallback)
{
    if (!IsValidSlotId(parameter.slotId)) {
        auto result = ISendShortMessageCallback::SmsSendResult::SEND_SMS_FAILURE_UNKNOWN;
        sendCallback->OnSmsSendResult(result);
        if (deliveryCallback != nullptr) {
            deliveryCallback->OnSmsDeliveryResult(u"");
        }
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(parameter.slotId, parameter.destinationHost,
        parameter.serviceCenter, parameter.textContent, sendCallback.release(), deliveryCallback.release());
}

static int32_t ActuallySendTextMessageWithoutSave(SendMessageContext &parameter,
    std::unique_ptr<SendCallback> sendCallback, std::unique_ptr<DeliveryCallback> deliveryCallback)
{
    if (!IsValidSlotId(parameter.slotId) || parameter.isPersist) {
        auto result = ISendShortMessageCallback::SmsSendResult::SEND_SMS_FAILURE_UNKNOWN;
        sendCallback->OnSmsSendResult(result);
        if (deliveryCallback != nullptr) {
            deliveryCallback->OnSmsDeliveryResult(u"");
        }
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return Singleton<SmsServiceManagerClient>::GetInstance().SendMessageWithoutSave(parameter.slotId,
        parameter.destinationHost, parameter.serviceCenter, parameter.textContent, sendCallback.release(),
        deliveryCallback.release());
}

static int32_t ActuallySendDataMessage(SendMessageContext &parameter, std::unique_ptr<SendCallback> sendCallback,
    std::unique_ptr<DeliveryCallback> deliveryCallback)
{
    if (InValidSlotIdOrInValidPort(parameter.slotId, parameter.destinationPort)) {
        auto result = ISendShortMessageCallback::SmsSendResult::SEND_SMS_FAILURE_UNKNOWN;
        sendCallback->OnSmsSendResult(result);
        if (deliveryCallback != nullptr) {
            deliveryCallback->OnSmsDeliveryResult(u"");
        }
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    if (parameter.rawDataContent.size() > 0) {
        uint16_t arrayLength = static_cast<uint16_t>(parameter.rawDataContent.size());
        return Singleton<SmsServiceManagerClient>::GetInstance().SendMessage(parameter.slotId,
            parameter.destinationHost, parameter.serviceCenter, parameter.destinationPort, &parameter.rawDataContent[0],
            arrayLength, sendCallback.release(), deliveryCallback.release());
    }
    return TELEPHONY_ERR_ARGUMENT_INVALID;
}

static int32_t ActuallySendMessage(napi_env env, SendMessageContext &parameter)
{
    bool hasSendCallback = parameter.sendCallbackRef != nullptr;
    std::unique_ptr<SendCallback> sendCallback =
        std::make_unique<SendCallback>(hasSendCallback, env, parameter.thisVarRef, parameter.sendCallbackRef);
    bool hasDeliveryCallback = parameter.deliveryCallbackRef != nullptr;
    std::unique_ptr<DeliveryCallback> deliveryCallback = std::make_unique<DeliveryCallback>(
        hasDeliveryCallback, env, parameter.thisVarRef, parameter.deliveryCallbackRef);
    if (parameter.messageType == TEXT_MESSAGE_PARAMETER_MATCH && parameter.isPersist) {
        if (hasDeliveryCallback) {
            return ActuallySendTextMessage(parameter, std::move(sendCallback), std::move(deliveryCallback));
        }
        return ActuallySendTextMessage(parameter, std::move(sendCallback), nullptr);
    } else if (parameter.messageType == TEXT_MESSAGE_PARAMETER_MATCH && !parameter.isPersist) {
        if (hasDeliveryCallback) {
            return ActuallySendTextMessageWithoutSave(parameter, std::move(sendCallback), std::move(deliveryCallback));
        }
        return ActuallySendTextMessageWithoutSave(parameter, std::move(sendCallback), nullptr);
    } else if (parameter.messageType == RAW_DATA_MESSAGE_PARAMETER_MATCH) {
        if (hasDeliveryCallback) {
            return ActuallySendDataMessage(parameter, std::move(sendCallback), std::move(deliveryCallback));
        }
        return ActuallySendDataMessage(parameter, std::move(sendCallback), nullptr);
    }
    return TELEPHONY_ERR_ARGUMENT_INVALID;
}

static void NativeSendMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto asyncContext = static_cast<SendMessageContext *>(data);
    asyncContext->errorCode = ActuallySendMessage(env, *asyncContext);
    if (asyncContext->errorCode == TELEPHONY_ERR_SUCCESS) {
        asyncContext->resolved = true;
    }
    TELEPHONY_LOGI("NativeSendMessage end resolved = %{public}d", asyncContext->resolved);
}

static void SendMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto asyncContext = static_cast<SendMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (asyncContext->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            asyncContext->errorCode, "sendMessage", "ohos.permission.SEND_MESSAGES");
        TELEPHONY_LOGI("asyncContext->errorCode:%{public}d.", error.errorCode);
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (asyncContext->destinationHost.capacity() != 0) {
        asyncContext->destinationHost = u"";
    }
    if (asyncContext->serviceCenter.capacity() != 0) {
        asyncContext->serviceCenter = u"";
    }
    if (asyncContext->textContent.capacity() != 0) {
        asyncContext->textContent = u"";
    }
    if (asyncContext->messageType == RAW_DATA_MESSAGE_PARAMETER_MATCH) {
        std::vector<unsigned char>().swap(asyncContext->rawDataContent);
    }
    NapiUtil::Handle1ValueCallback(env, asyncContext, callbackValue);
}

static void GetOptionalProperty(napi_env env, napi_value object, SendMessageContext &context)
{
    if (NapiUtil::HasNamedProperty(env, object, SERVICE_CENTER_STR)) {
        napi_value serviceCenterValue = NapiUtil::GetNamedProperty(env, object, SERVICE_CENTER_STR);
        context.serviceCenter = NapiSmsUtil::GetU16StrFromNapiValue(env, serviceCenterValue);
    }
    if (NapiUtil::HasNamedProperty(env, object, SEND_CALLBACK_STR)) {
        napi_value sendCallbackValue = NapiUtil::GetNamedProperty(env, object, SEND_CALLBACK_STR);
        napi_create_reference(env, sendCallbackValue, DEFAULT_REF_COUNT, &context.sendCallbackRef);
    }
    if (NapiUtil::HasNamedProperty(env, object, DELIVERY_CALLBACK_STR)) {
        napi_value deliveryCallbackValue = NapiUtil::GetNamedProperty(env, object, DELIVERY_CALLBACK_STR);
        napi_create_reference(env, deliveryCallbackValue, DEFAULT_REF_COUNT, &context.deliveryCallbackRef);
    }
    if (NapiUtil::HasNamedProperty(env, object, IS_PERSIST_STR)) {
        napi_value isPersistValue = nullptr;
        napi_get_named_property(env, object, IS_PERSIST_STR.data(), &isPersistValue);
        napi_get_value_bool(env, isPersistValue, &context.isPersist);
    }
}

static void ParseMessageParameter(
    int32_t messageMatchResult, napi_env env, napi_value object, SendMessageContext &context)
{
    context.messageType = messageMatchResult;
    context.slotId = GetDefaultSmsSlotId();
    napi_value slotIdValue = nullptr;
    napi_get_named_property(env, object, SLOT_ID_STR.data(), &slotIdValue);
    napi_get_value_int32(env, slotIdValue, &context.slotId);
    napi_value destinationHostValue = NapiUtil::GetNamedProperty(env, object, DESTINATION_HOST_STR);
    context.destinationHost = NapiSmsUtil::GetU16StrFromNapiValue(env, destinationHostValue);
    GetOptionalProperty(env, object, context);
    napi_value contentValue = NapiUtil::GetNamedProperty(env, object, CONTENT_STR);
    if (messageMatchResult == TEXT_MESSAGE_PARAMETER_MATCH) {
        char contentChars[MAX_TEXT_SHORT_MESSAGE_LENGTH] = {0};
        size_t contentLength = 0;
        napi_get_value_string_utf8(env, contentValue, contentChars, MAX_TEXT_SHORT_MESSAGE_LENGTH, &contentLength);
        std::string text(contentChars, contentLength);
        context.textContent = NapiUtil::ToUtf16(text);
    }
    if (messageMatchResult == RAW_DATA_MESSAGE_PARAMETER_MATCH) {
        int32_t destinationPort = INVALID_PORT;
        napi_value destinationPortValue = nullptr;
        napi_get_named_property(env, object, DESTINATION_PORT_STR.data(), &destinationPortValue);
        napi_get_value_int32(env, destinationPortValue, &destinationPort);
        TELEPHONY_LOGI("SendMessage destinationPort: %{private}d", destinationPort);
        if (destinationPort >= MIN_PORT && destinationPort <= MAX_PORT) {
            g_validPort = true;
        }
        context.destinationPort = static_cast<uint16_t>(destinationPort);
        napi_value elementValue = nullptr;
        int32_t element = 0;
        uint32_t valueArraySize = 0;
        napi_get_array_length(env, contentValue, &valueArraySize);
        auto arraySize = static_cast<int32_t>(valueArraySize);
        for (int32_t i = 0; i < arraySize; i++) {
            napi_get_element(env, contentValue, i, &elementValue);
            napi_get_value_int32(env, elementValue, &element);
            context.rawDataContent.push_back((uint8_t)element);
        }
    }
}

static napi_value SendMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = 1;
    napi_value parameters[1] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    int32_t messageMatchResult = NapiSmsUtil::MatchSendMessageParameters(env, parameters, parameterCount);
    if (messageMatchResult == MESSAGE_PARAMETER_NOT_MATCH) {
        TELEPHONY_LOGE("SendMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto asyncContext = std::make_unique<SendMessageContext>().release();
    if (asyncContext == nullptr) {
        TELEPHONY_LOGE("SendMessage SendMessageContext is nullptr.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    ParseMessageParameter(messageMatchResult, env, parameters[0], *asyncContext);
    napi_create_reference(env, thisVar, DEFAULT_REF_COUNT, &asyncContext->thisVarRef);
    napi_value resourceName = nullptr;
    napi_create_string_utf8(env, "SendMessage", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(env, nullptr, resourceName, NativeSendMessage, SendMessageCallback,
        static_cast<void *>(asyncContext), &(asyncContext->work));
    napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_default);
    return NapiUtil::CreateUndefined(env);
}

static napi_value SendShortMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    int32_t messageMatchResult = NapiSmsUtil::MatchSendShortMessageParameters(env, parameters, parameterCount);
    if (messageMatchResult == MESSAGE_PARAMETER_NOT_MATCH) {
        TELEPHONY_LOGE("SendShortMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto asyncContext = std::make_unique<SendMessageContext>().release();
    if (asyncContext == nullptr) {
        TELEPHONY_LOGE("SendMessage SendMessageContext is nullptr.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    ParseMessageParameter(messageMatchResult, env, parameters[0], *asyncContext);
    napi_create_reference(env, thisVar, DEFAULT_REF_COUNT, &asyncContext->thisVarRef);

    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &asyncContext->callbackRef);
    }
    napi_value result =
        NapiUtil::HandleAsyncWork(env, asyncContext, "SendShortMessage", NativeSendMessage, SendMessageCallback);
    return result;
}

static void NativeCreateMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto asyncContext = static_cast<CreateMessageContext *>(data);
    if (asyncContext->specification.empty() || asyncContext->pdu.empty()) {
        asyncContext->errorCode = TELEPHONY_ERR_ARGUMENT_INVALID;
        return;
    }
    std::u16string specification16 = NapiUtil::ToUtf16(asyncContext->specification);
    auto shortMessageObj = new ShortMessage();
    asyncContext->errorCode = ShortMessage::CreateMessage(asyncContext->pdu, specification16, *shortMessageObj);
    if (asyncContext->errorCode == TELEPHONY_ERR_SUCCESS) {
        asyncContext->resolved = true;
        asyncContext->shortMessage = shortMessageObj;
    } else {
        TELEPHONY_LOGI("NativeCreateMessage CreateMessage faied");
        delete shortMessageObj;
    }
}

static napi_value CreateShortMessageValue(napi_env env, const ShortMessage &shortMessage)
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    NapiUtil::SetPropertyStringUtf8(
        env, object, "visibleMessageBody", NapiUtil::ToUtf8(shortMessage.GetVisibleMessageBody()));
    NapiUtil::SetPropertyStringUtf8(
        env, object, "visibleRawAddress", NapiUtil::ToUtf8(shortMessage.GetVisibleRawAddress()));
    NapiUtil::SetPropertyInt32(env, object, "messageClass", shortMessage.GetMessageClass());
    NapiUtil::SetPropertyInt32(env, object, "protocolId", shortMessage.GetProtocolId());
    std::u16string smscAddress;
    shortMessage.GetScAddress(smscAddress);
    NapiUtil::SetPropertyStringUtf8(env, object, "scAddress", NapiUtil::ToUtf8(smscAddress));
    NapiUtil::SetPropertyInt64(env, object, "scTimestamp", shortMessage.GetScTimestamp());
    NapiUtil::SetPropertyBoolean(env, object, "isReplaceMessage", shortMessage.IsReplaceMessage());
    NapiUtil::SetPropertyBoolean(env, object, "hasReplyPath", shortMessage.HasReplyPath());
    NapiSmsUtil::SetPropertyArray(env, object, "pdu", shortMessage.GetPdu());
    NapiUtil::SetPropertyInt32(env, object, "status", shortMessage.GetStatus());
    NapiUtil::SetPropertyBoolean(env, object, "isSmsStatusReportMessage", shortMessage.IsSmsStatusReportMessage());
    return object;
}

static void CreateMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto asyncContext = static_cast<CreateMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (status == napi_ok) {
        if (asyncContext->resolved) {
            if (asyncContext->shortMessage != nullptr) {
                callbackValue = CreateShortMessageValue(env, *(asyncContext->shortMessage));
                delete asyncContext->shortMessage;
                asyncContext->shortMessage = nullptr;
            }
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(asyncContext->errorCode);
            callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
        }
    } else {
        callbackValue = NapiUtil::CreateErrorMessage(
            env, "create message error,cause napi_status = " + std::to_string(status));
    }
    if (asyncContext->pdu.capacity() != 0) {
        std::vector<unsigned char>().swap(asyncContext->pdu);
    }
    if (asyncContext->specification.capacity() != 0) {
        std::string().swap(asyncContext->specification);
    }
    NapiUtil::Handle2ValueCallback(env, asyncContext, callbackValue);
}

static napi_value CreateMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = THREE_PARAMETERS;
    napi_value parameters[THREE_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!NapiSmsUtil::MatchCreateMessageParameter(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("CreateMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto asyncContext = std::make_unique<CreateMessageContext>().release();
    if (asyncContext == nullptr) {
        TELEPHONY_LOGE("CreateMessage CreateMessageContext is nullptr.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    asyncContext->specification = NapiUtil::Get64StringFromValue(env, parameters[1]);
    TELEPHONY_LOGI("CreateMessage specification = %s", asyncContext->specification.c_str());
    uint32_t arrayLength = 0;
    napi_get_array_length(env, parameters[0], &arrayLength);
    napi_value elementValue = nullptr;
    int32_t element = 0;
    for (uint32_t i = 0; i < arrayLength; i++) {
        napi_get_element(env, parameters[0], i, &elementValue);
        napi_get_value_int32(env, elementValue, &element);
        asyncContext->pdu.push_back(static_cast<unsigned char>(element));
    }
    TELEPHONY_LOGI("CreateMessage pdu size = %{public}zu", asyncContext->pdu.size());
    if (parameterCount == THREE_PARAMETERS) {
        napi_create_reference(env, parameters[PARAMETERS_INDEX_2], DEFAULT_REF_COUNT, &(asyncContext->callbackRef));
    }
    return NapiUtil ::HandleAsyncWork(
        env, asyncContext, "CreateMessage", NativeCreateMessage, CreateMessageCallback);
}

static bool MatchSetDefaultSmsSlotIdParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number});
        }
        case TWO_PARAMETERS:
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_function});
        default: {
            return false;
        }
    }
}

static void NativeSetDefaultSmsSlotId(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SetDefaultSmsSlotIdContext *>(data);
    if (!IsValidSlotId(context->slotId) && (context->slotId != DEFAULT_SIM_SLOT_ID_REMOVE)) {
        TELEPHONY_LOGE("NativeSetDefaultSmsSlotId slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().SetDefaultSmsSlotId(context->slotId);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
    TELEPHONY_LOGI("NativeSetDefaultSmsSlotId end resolved = %{public}d", context->resolved);
}

static void SetDefaultSmsSlotIdCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SetDefaultSmsSlotIdContext *>(data);
    TELEPHONY_LOGI("SetDefaultSmsSlotIdCallback status = %{public}d", status);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "setDefaultSmsSlotId", "ohos.permission.SET_TELEPHONY_STATE");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value SetDefaultSmsSlotId(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchSetDefaultSmsSlotIdParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("SetDefaultSmsSlotId parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<SetDefaultSmsSlotIdContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(
        env, context, "SetDefaultSmsSlotId", NativeSetDefaultSmsSlotId, SetDefaultSmsSlotIdCallback);
    return result;
}

static bool MatchGetDefaultSmsSlotIdParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case NONE_PARAMETER: {
            return true;
        }
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, {napi_function});
        }
        default: {
            return false;
        }
    }
}

static void NativeGetDefaultSmsSlotId(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetDefaultSmsSlotIdContext *>(data);
    context->defaultSmsSlotId = Singleton<SmsServiceManagerClient>::GetInstance().GetDefaultSmsSlotId();
    TELEPHONY_LOGI("NativeGetDefaultSmsSlotId defaultSmsSlotId  = %{public}d", context->defaultSmsSlotId);
    if (context->defaultSmsSlotId >= SIM_SLOT_0) {
        context->resolved = true;
    } else {
        context->resolved = false;
    }
}

static void GetDefaultSmsSlotIdCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetDefaultSmsSlotIdContext *>(data);
    napi_value callbackValue = nullptr;
    if (status == napi_ok) {
        if (context->resolved) {
            napi_create_int32(env, context->defaultSmsSlotId, &callbackValue);
        } else {
            callbackValue = NapiUtil::CreateErrorMessage(env, "get default sms slot id error");
        }
    } else {
        callbackValue = NapiUtil::CreateErrorMessage(
            env, "get default sms slot id error cause napi_status = " + std::to_string(status));
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetDefaultSmsSlotId(napi_env env, napi_callback_info info)
{
    size_t parameterCount = 1;
    napi_value parameters[1] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    NAPI_ASSERT(env, MatchGetDefaultSmsSlotIdParameters(env, parameters, parameterCount), "type mismatch");
    auto context = std::make_unique<GetDefaultSmsSlotIdContext>().release();
    napi_status statusValue = napi_get_value_int32(env, parameters[0], &context->defaultSmsSlotId);
    TELEPHONY_LOGI("GetDefaultSmsSlotId statusValue = %{private}d", statusValue);
    if (parameterCount == 1) {
        napi_create_reference(env, parameters[0], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(
        env, context, "SetDefaultSmsSlotId", NativeGetDefaultSmsSlotId, GetDefaultSmsSlotIdCallback);
}

static void NativeGetDefaultSmsSimId(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetDefaultSmsSimIdContext *>(data);
    int32_t simId = 0;
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().GetDefaultSmsSimId(simId);
    TELEPHONY_LOGI("result = %{public}d", context->errorCode);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->defaultSmsSimId = simId;
        context->resolved = true;
    } else {
        context->resolved = false;
    }
}

static void GetDefaultSmsSimIdCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetDefaultSmsSimIdContext *>(data);
    napi_value callbackValue = nullptr;
    if (status == napi_ok) {
        if (context->resolved) {
            napi_create_int32(env, context->defaultSmsSimId, &callbackValue);
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(context->errorCode);
            callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
        }
    } else {
        callbackValue = NapiUtil::CreateErrorMessage(
            env, "get default sms sim id error cause napi_status = " + std::to_string(status));
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetDefaultSmsSimId(napi_env env, napi_callback_info info)
{
    size_t parameterCount = 1;
    napi_value parameters[1] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchGetDefaultSmsSlotIdParameters(env, parameters, parameterCount)) {
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<GetDefaultSmsSimIdContext>().release();
    napi_status statusValue = napi_get_value_int32(env, parameters[0], &context->defaultSmsSimId);
    TELEPHONY_LOGI("statusValue = %{private}d", statusValue);
    if (parameterCount == 1) {
        napi_create_reference(env, parameters[0], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(
        env, context, "GetDefaultSmsSimId", NativeGetDefaultSmsSimId, GetDefaultSmsSimIdCallback);
}

static bool MatchSetSmscAddrParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case TWO_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_string});
        }
        case THREE_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_string, napi_function});
        }
        default: {
            return false;
        }
    }
}

static void NativeSetSmscAddr(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SetSmscAddrContext *>(data);
    if (!IsValidSlotId(context->slotId)) {
        TELEPHONY_LOGE("NativeSetSmscAddr slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().SetScAddress(context->slotId,
        NapiUtil::ToUtf16(context->smscAddr));
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
    TELEPHONY_LOGI("NativeSetSmscAddr resolved = %{public}d", context->resolved);
}

static void SetSmscAddrCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SetSmscAddrContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "setSmscAddr", "ohos.permission.SET_TELEPHONY_STATE");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->smscAddr.capacity() != 0) {
        std::string().swap(context->smscAddr);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value SetSmscAddr(napi_env env, napi_callback_info info)
{
    size_t parameterCount = THREE_PARAMETERS;
    napi_value parameters[THREE_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchSetSmscAddrParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("SetSmscAddr parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    TELEPHONY_LOGI("SetSmscAddr start after MatchSetSmscAddrParameters");
    auto context = std::make_unique<SetSmscAddrContext>().release();
    TELEPHONY_LOGI("SetSmscAddr start after SetSmscAddrContext contruct");
    napi_get_value_int32(env, parameters[0], &context->slotId);
    context->smscAddr = NapiUtil::Get64StringFromValue(env, parameters[1]);
    if (parameterCount == THREE_PARAMETERS) {
        napi_create_reference(env, parameters[PARAMETERS_INDEX_2], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    TELEPHONY_LOGI("SetSmscAddr before end");
    return NapiUtil::HandleAsyncWork(env, context, "SetSmscAddr", NativeSetSmscAddr, SetSmscAddrCallback);
}

static bool MatchGetSmscAddrParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number});
        }
        case TWO_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_function});
        }
        default: {
            return false;
        }
    }
}

static void NativeGetSmscAddr(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmscAddrContext *>(data);
    if (!IsValidSlotId(context->slotId)) {
        TELEPHONY_LOGE("NativeGetSmscAddr slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    std::u16string smscAddress;
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().GetScAddress(context->slotId, smscAddress);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->smscAddr = NapiUtil::ToUtf8(smscAddress);
        context->resolved = true;
    }
}

static void GetSmscAddrCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmscAddrContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_string_utf8(env, context->smscAddr.data(), context->smscAddr.length(), &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "getSmscAddr", "ohos.permission.GET_TELEPHONY_STATE");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->smscAddr.capacity() != 0) {
        std::string().swap(context->smscAddr);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetSmscAddr(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchGetSmscAddrParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("GetSmscAddr parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<GetSmscAddrContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(env, context, "GetSmscAddr", NativeGetSmscAddr, GetSmscAddrCallback);
}

static bool MatchAddSimMessageParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    bool typeMatch = false;
    switch (parameterCount) {
        case ONE_PARAMETER: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object});
            break;
        }
        case TWO_PARAMETERS: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object, napi_function});
            break;
        }
        default: {
            break;
        }
    }
    if (typeMatch) {
        return NapiSmsUtil::MatchObjectProperty(env, parameters[0],
            {
                {"slotId", napi_number},
                {"smsc", napi_string},
                {"status", napi_number},
                {"pdu", napi_string},
            });
    }
    return false;
}

static void NativeAddSimMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<AddSimMessageContext *>(data);
    int32_t wrapStatus = static_cast<int32_t>(context->status);
    TELEPHONY_LOGI("NativeAddSimMessage start wrapStatus = %{public}d", wrapStatus);
    if (wrapStatus != MESSAGE_UNKNOWN_STATUS) {
        ISmsServiceInterface::SimMessageStatus status =
            static_cast<ISmsServiceInterface::SimMessageStatus>(wrapStatus);
        context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().AddSimMessage(
            context->slotId, NapiUtil::ToUtf16(context->smsc), NapiUtil::ToUtf16(context->pdu), status);
        if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
            context->resolved = true;
        }
        TELEPHONY_LOGI("NativeAddSimMessage context->resolved = %{public}d", context->resolved);
    } else {
        context->errorCode = SMS_MMS_UNKNOWN_SIM_MESSAGE_STATUS;
    }
}

static void AddSimMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<AddSimMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "addSimMessage", "ohos.permission.SEND_MESSAGES");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->smsc.capacity() != 0) {
        std::string().swap(context->smsc);
    }
    if (context->pdu.capacity() != 0) {
        std::string().swap(context->pdu);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value AddSimMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchAddSimMessageParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("AddSimMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<AddSimMessageContext>().release();
    napi_value slotIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "slotId");
    if (slotIdValue != nullptr) {
        napi_get_value_int32(env, slotIdValue, &context->slotId);
    }
    napi_value smscValue = NapiUtil::GetNamedProperty(env, parameters[0], "smsc");
    if (smscValue != nullptr) {
        context->smsc = NapiUtil::Get64StringFromValue(env, smscValue);
    }
    napi_value pduValue = NapiUtil::GetNamedProperty(env, parameters[0], "pdu");
    if (pduValue != nullptr) {
        context->pdu = NapiUtil::GetStringFromValue(env, pduValue);
    }
    napi_value statusValue = NapiUtil::GetNamedProperty(env, parameters[0], "status");
    if (statusValue != nullptr) {
        int32_t messageStatus = static_cast<int32_t>(MESSAGE_UNKNOWN_STATUS);
        napi_get_value_int32(env, statusValue, &messageStatus);
        context->status = NapiSmsUtil::WrapSimMessageStatus(messageStatus);
    }
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(env, context, "AddSimMessage", NativeAddSimMessage, AddSimMessageCallback);
}

static bool MatchDelSimMessageParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case TWO_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_number});
        }
        case THREE_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_number, napi_function});
        }
        default: {
            return false;
        }
    }
}
static void NativeDelSimMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<DelSimMessageContext *>(data);
    if (!IsValidSlotId(context->slotId)) {
        TELEPHONY_LOGE("NativeDelSimMessage slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().DelSimMessage(context->slotId,
        context->msgIndex);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
    TELEPHONY_LOGI("NativeDelSimMessage resolved = %{public}d", context->resolved);
}

static void DelSimMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<DelSimMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "delSimMessage", "ohos.permission.SEND_MESSAGES");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value DelSimMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = 3;
    napi_value parameters[3] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchDelSimMessageParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("DelSimMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<DelSimMessageContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    napi_get_value_int32(env, parameters[1], &context->msgIndex);
    if (parameterCount == PARAMETER_COUNT_3) {
        napi_create_reference(env, parameters[PARAMETERS_INDEX_2], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(env, context, "DelSimMessage", NativeDelSimMessage, DelSimMessageCallback);
}

static bool MatchUpdateSimMessageParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    bool typeMatch = false;
    switch (parameterCount) {
        case ONE_PARAMETER: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object});
            break;
        }
        case TWO_PARAMETERS: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object, napi_function});
            break;
        }
        default: {
            break;
        }
    }
    if (typeMatch) {
        bool propertyMatchResult = NapiSmsUtil::MatchObjectProperty(env, parameters[0],
            {
                {"slotId", napi_number},
                {"msgIndex", napi_number},
                {"newStatus", napi_number},
                {"pdu", napi_string},
                {"smsc", napi_string},
            });
        TELEPHONY_LOGI(
            "MatchUpdateSimMessageParameters start propertyMatchResult = %{public}d", propertyMatchResult);
        return propertyMatchResult;
    }
    TELEPHONY_LOGI("MatchUpdateSimMessageParameters end");
    return false;
}

static void NativeUpdateSimMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<UpdateSimMessageContext *>(data);
    int32_t newStatus = static_cast<int32_t>(context->newStatus);
    TELEPHONY_LOGI("NativeUpdateSimMessage newStatus = %{public}d", newStatus);
    if (!context->pdu.empty() && (newStatus > -1)) {
        std::string msgPud(context->pdu.begin(), context->pdu.end());
        TELEPHONY_LOGD("NativeUpdateSimMessage msgPud = %{private}s", msgPud.c_str());
        context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().UpdateSimMessage(context->slotId,
            context->msgIndex, static_cast<ISmsServiceInterface::SimMessageStatus>(context->newStatus),
            NapiUtil::ToUtf16(context->pdu), NapiUtil::ToUtf16(context->smsc));
        if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
            context->resolved = true;
        }
    } else {
        TELEPHONY_LOGI("NativeUpdateSimMessage resolved false cause parameter invalided");
        context->errorCode = SMS_MMS_UNKNOWN_SIM_MESSAGE_STATUS;
    }
}

static void UpdateSimMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<UpdateSimMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "updateSimMessage", "ohos.permission.SEND_MESSAGES");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->smsc.capacity() != 0) {
        std::string().swap(context->smsc);
    }
    if (context->pdu.capacity() != 0) {
        std::string().swap(context->pdu);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value UpdateSimMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchUpdateSimMessageParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("UpdateSimMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    TELEPHONY_LOGI("UpdateSimMessage start parameter match passed");
    auto context = std::make_unique<UpdateSimMessageContext>().release();
    napi_value slotIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "slotId");
    if (slotIdValue != nullptr) {
        napi_get_value_int32(env, slotIdValue, &context->slotId);
    }
    napi_value msgIndexValue = NapiUtil::GetNamedProperty(env, parameters[0], "msgIndex");
    if (msgIndexValue != nullptr) {
        napi_get_value_int32(env, msgIndexValue, &context->msgIndex);
    }
    napi_value newStatusValue = NapiUtil::GetNamedProperty(env, parameters[0], "newStatus");
    if (newStatusValue != nullptr) {
        int32_t newStatus = static_cast<int32_t>(MESSAGE_UNKNOWN_STATUS);
        napi_get_value_int32(env, newStatusValue, &newStatus);
        context->newStatus = NapiSmsUtil::WrapSimMessageStatus(newStatus);
    }
    napi_value pudValue = NapiUtil::GetNamedProperty(env, parameters[0], "pdu");
    if (pudValue != nullptr) {
        context->pdu = NapiUtil::GetStringFromValue(env, pudValue);
    }
    TELEPHONY_LOGD("UpdateSimMessage pdu = %{private}s", context->pdu.c_str());
    napi_value smscValue = NapiUtil::GetNamedProperty(env, parameters[0], "smsc");
    if (smscValue != nullptr) {
        context->smsc = NapiUtil::Get64StringFromValue(env, smscValue);
    }
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    TELEPHONY_LOGI("UpdateSimMessage start before HandleAsyncWork");
    return NapiUtil::HandleAsyncWork(
        env, context, "UpdateSimMessage", NativeUpdateSimMessage, UpdateSimMessageCallback);
}

static bool MatchGetAllSimMessagesParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number});
        }
        case TWO_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_function});
        }
        default: {
            return false;
        }
    }
}

static void NativeGetAllSimMessages(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetAllSimMessagesContext *>(data);
    if (!IsValidSlotId(context->slotId)) {
        TELEPHONY_LOGE("NativeGetAllSimMessages slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().
        GetAllSimMessages(context->slotId, context->messageArray);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    } else {
        TELEPHONY_LOGE("NativeGetAllSimMessages context->resolved == false");
    }
}

static napi_value CreateSimShortMessageValue(napi_env env, const ShortMessage &shortMessage)
{
    napi_value simObject = nullptr;
    napi_value object = CreateShortMessageValue(env, shortMessage);
    napi_create_object(env, &simObject);
    std::string shortMessageKey("shortMessage");
    napi_set_named_property(env, simObject, shortMessageKey.c_str(), object);
    NapiUtil::SetPropertyInt32(env, simObject, "simMessageStatus", shortMessage.GetIccMessageStatus());
    NapiUtil::SetPropertyInt32(env, simObject, "indexOnSim", shortMessage.GetIndexOnSim());
    return simObject;
}

static void GetAllSimMessagesCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetAllSimMessagesContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_array(env, &callbackValue);
        int32_t arraySize = static_cast<int32_t>(context->messageArray.size());
        for (int32_t i = 0; i < arraySize; i++) {
            ShortMessage message = context->messageArray[i];
            napi_value itemValue = CreateSimShortMessageValue(env, message);
            napi_set_element(env, callbackValue, i, itemValue);
        }
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "getAllSimMessages", "ohos.permission.RECEIVE_SMS");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->messageArray.capacity() != 0) {
        std::vector<ShortMessage>().swap(context->messageArray);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetAllSimMessages(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchGetAllSimMessagesParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("GetAllSimMessages parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<GetAllSimMessagesContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(
        env, context, "GetAllSimMessages", NativeGetAllSimMessages, GetAllSimMessagesCallback);
    return result;
}

static bool MatchSetCBConfigParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    bool typeMatch = false;
    switch (parameterCount) {
        case ONE_PARAMETER: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object});
            break;
        }
        case TWO_PARAMETERS: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object, napi_function});
            break;
        }
        default: {
            break;
        }
    }
    if (typeMatch) {
        return NapiSmsUtil::MatchObjectProperty(env, parameters[0],
            {
                {"slotId", napi_number},
                {"enable", napi_boolean},
                {"startMessageId", napi_number},
                {"endMessageId", napi_number},
                {"ranType", napi_number},
            });
    }
    return false;
}

static void NativeSetCBConfig(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<CBConfigContext *>(data);
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().SetCBConfig(
        context->slotId, context->enable, context->startMessageId, context->endMessageId, context->ranType);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
    TELEPHONY_LOGD("NativeSetCBConfig end resolved = %{public}d", context->resolved);
}

static void SetCBConfigCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<CBConfigContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "setCBConfig", "ohos.permission.RECEIVE_SMS");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value SetCBConfig(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchSetCBConfigParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("SetCBConfig parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<CBConfigContext>().release();
    napi_value slotIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "slotId");
    if (slotIdValue != nullptr) {
        napi_get_value_int32(env, slotIdValue, &context->slotId);
    }
    napi_value enableValue = NapiUtil::GetNamedProperty(env, parameters[0], "enable");
    if (enableValue != nullptr) {
        napi_get_value_bool(env, enableValue, &context->enable);
    }
    napi_value startMessageIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "startMessageId");
    if (startMessageIdValue != nullptr) {
        napi_get_value_int32(env, startMessageIdValue, &context->startMessageId);
    }
    napi_value endMessageIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "endMessageId");
    if (endMessageIdValue != nullptr) {
        napi_get_value_int32(env, endMessageIdValue, &context->endMessageId);
    }
    napi_value ranTypeValue = NapiUtil::GetNamedProperty(env, parameters[0], "ranType");
    if (ranTypeValue != nullptr) {
        napi_get_value_int32(env, ranTypeValue, &context->ranType);
    }
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(env, context, "SetCBConfig", NativeSetCBConfig, SetCBConfigCallback);
    return result;
}

static bool MatchSetCBConfigListParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    bool typeMatch = false;
    switch (parameterCount) {
        case ONE_PARAMETER: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object});
            break;
        }
        case TWO_PARAMETERS: {
            typeMatch = NapiUtil::MatchParameters(env, parameters, {napi_object, napi_function});
            break;
        }
        default: {
            break;
        }
    }
    if (typeMatch) {
        return NapiSmsUtil::MatchObjectProperty(env, parameters[0],
            {
                {"slotId", napi_number},
                {"ranType", napi_number},
            });
    }
    return false;
}

static void NativeSetCBConfigList(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<CBConfigListContext *>(data);
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().SetCBConfigList(
        context->slotId, context->messageIds, context->ranType);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
}

static void SetCBConfigListCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<CBConfigListContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_undefined(env, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "setCBConfigList", "ohos.permission.RECEIVE_SMS");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle1ValueCallback(env, context, callbackValue);
}

static napi_value SetCBConfigList(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchSetCBConfigListParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("SetCBConfigList parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<CBConfigListContext>().release();
    napi_value slotIdValue = NapiUtil::GetNamedProperty(env, parameters[0], "slotId");
    if (slotIdValue != nullptr) {
        napi_get_value_int32(env, slotIdValue, &context->slotId);
    }
    napi_value ranTypeValue = NapiUtil::GetNamedProperty(env, parameters[0], "ranType");
    if (ranTypeValue != nullptr) {
        napi_get_value_int32(env, ranTypeValue, &context->ranType);
    }
    napi_value messageIdsValue = NapiUtil::GetNamedProperty(env, parameters[0], "messageIds");
    if (messageIdsValue != nullptr) {
        NapiUtil::ConvertToArrayFromValue(env, messageIdsValue, context->messageIds);
    }
    if (context->messageIds.size() > CB_RANGE_LIST_MAX_SIZE) {
        NapiUtil::ThrowParameterError(env);
        delete context;
        return nullptr;
    }
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    return NapiUtil::HandleAsyncWork(env, context, "SetCBConfigList", NativeSetCBConfigList, SetCBConfigListCallback);
}

static bool MatchSplitMessageParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case ONE_PARAMETER:
            return NapiUtil::MatchParameters(env, parameters, {napi_string});
        case TWO_PARAMETERS:
            return NapiUtil::MatchParameters(env, parameters, {napi_string, napi_function});
        default:
            return false;
    }
}

static void NativeSplitMessage(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SplitMessageContext *>(data);
    std::u16string content = NapiUtil::ToUtf16(context->content);
    context->errorCode =
        Singleton<SmsServiceManagerClient>::GetInstance().SplitMessage(content, context->messageArray);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
}

static void SplitMessageCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SplitMessageContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_array(env, &callbackValue);
        int32_t arraySize = static_cast<int32_t>(context->messageArray.size());
        TELEPHONY_LOGI("napi_sms messageArray.size =  %{public}d", arraySize);
        for (int32_t i = 0; i < arraySize; i++) {
            napi_value itemValue = nullptr;
            std::string message = NapiUtil::ToUtf8(context->messageArray[i]);
            napi_create_string_utf8(env, message.data(), message.size(), &itemValue);
            napi_set_element(env, callbackValue, i, itemValue);
        }
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "splitMessage", "ohos.permission.SEND_MESSAGES");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->content.capacity() != 0) {
        std::string().swap(context->content);
    }
    if (context->messageArray.capacity() != 0) {
        std::vector<std::u16string>().swap(context->messageArray);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value SplitMessage(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchSplitMessageParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("SplitMessage parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<SplitMessageContext>().release();
    context->content = NapiUtil::Get64StringFromValue(env, parameters[0]);
    TELEPHONY_LOGD("napi_sms splitMessage context->content = %{private}s", context->content.c_str());
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], MAX_TEXT_SHORT_MESSAGE_LENGTH, &context->callbackRef);
    }
    napi_value result =
        NapiUtil::HandleAsyncWork(env, context, "SplitMessage", NativeSplitMessage, SplitMessageCallback);
    return result;
}

static napi_value HasSmsCapability(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_boolean(env, Singleton<SmsServiceManagerClient>::GetInstance().HasSmsCapability(), &result);
    return result;
}

static bool MatchGetSmsSegmentsInfoParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case THREE_PARAMETERS:
            return NapiUtil::MatchParameters(env, parameters, {napi_number, napi_string, napi_boolean});
        case FOUR_PARAMETERS:
            return NapiUtil::MatchParameters(
                env, parameters, {napi_number, napi_string, napi_boolean, napi_function});
        default:
            return false;
    }
}

static void NativeGetSmsSegmentsInfo(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmsSegmentsInfoContext *>(data);
    if (!IsValidSlotId(context->slotId)) {
        TELEPHONY_LOGE("NativeGetSmsSegmentsInfo slotId is invalid");
        context->errorCode = ERROR_SLOT_ID_INVALID;
        return;
    }
    std::u16string content = NapiUtil::ToUtf16(context->content);
    ISmsServiceInterface::SmsSegmentsInfo info;
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().GetSmsSegmentsInfo(context->slotId,
        content, context->force7BitCode, info);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
        context->splitCount = info.msgSegCount;
        context->encodeCount = info.msgEncodingCount;
        context->encodeCountRemaining = info.msgRemainCount;
        context->scheme = info.msgCodeScheme;
    } else {
        TELEPHONY_LOGE("NativeGetSmsSegmentsInfo context->resolved == false");
    }
}

static void GetSmsSegmentsInfoCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmsSegmentsInfoContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_object(env, &callbackValue);
        NapiUtil::SetPropertyInt32(env, callbackValue, "splitCount", context->splitCount);
        NapiUtil::SetPropertyInt32(env, callbackValue, "encodeCount", context->encodeCount);
        NapiUtil::SetPropertyInt32(env, callbackValue, "encodeCountRemaining", context->encodeCountRemaining);
        NapiUtil::SetPropertyInt32(env, callbackValue, "scheme", static_cast<int32_t>(context->scheme));
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "getSmsSegmentsInfo", "ohos.permission.GET_TELEPHONY_STATE");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    if (context->content.capacity() != 0) {
        std::string().swap(context->content);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetSmsSegmentsInfo(napi_env env, napi_callback_info info)
{
    size_t parameterCount = FOUR_PARAMETERS;
    napi_value parameters[FOUR_PARAMETERS] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchGetSmsSegmentsInfoParameters(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("GetSmsSegmentsInfo parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<GetSmsSegmentsInfoContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    context->content = NapiUtil::GetStringFromValue(env, parameters[1]);
    napi_get_value_bool(env, parameters[PARAMETERS_INDEX_2], &context->force7BitCode);
    if (parameterCount == FOUR_PARAMETERS) {
        napi_create_reference(env, parameters[PARAMETERS_INDEX_3], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(
        env, context, "GetSmsSegmentsInfo", NativeGetSmsSegmentsInfo, GetSmsSegmentsInfoCallback);
    return result;
}

static bool MatchIsImsSmsSupportedParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, { napi_number });
        }
        case TWO_PARAMETERS: {
            return NapiUtil::MatchParameters(env, parameters, { napi_number, napi_function });
        }
        default: {
            return false;
        }
    }
}

static void NativeIsImsSmsSupported(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<IsImsSmsSupportedContext *>(data);
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().IsImsSmsSupported(
        context->slotId, context->setResult);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    } else {
        TELEPHONY_LOGE("NativeIsImsSmsSupported context->resolved == false");
    }
}

static void IsImsSmsSupportedCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<IsImsSmsSupportedContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_get_boolean(env, context->setResult, &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageForJs(context->errorCode);
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value IsImsSmsSupported(napi_env env, napi_callback_info info)
{
    size_t paramsCount = TWO_PARAMETERS;
    napi_value params[TWO_PARAMETERS] = { 0 };
    napi_value arg = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &paramsCount, params, &arg, &data);
    if (!MatchIsImsSmsSupportedParameters(env, params, paramsCount)) {
        TELEPHONY_LOGE("IsImsSmsSupported parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<IsImsSmsSupportedContext>().release();
    napi_get_value_int32(env, params[0], &context->slotId);
    if (paramsCount == TWO_PARAMETERS) {
        napi_create_reference(env, params[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(
        env, context, "IsImsSmsSupported", NativeIsImsSmsSupported, IsImsSmsSupportedCallback);
    return result;
}

static void NativeGetImsShortMessageFormat(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SingleValueContext<std::u16string> *>(data);
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance().GetImsShortMessageFormat(context->value);
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
    TELEPHONY_LOGE("errorCode:%{public}d", context->errorCode);
}

static void NativeGetSmsShortCodeType(napi_env env, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmsShortCodeTypeContext *>(data);
    int32_t smsShortCodeType = static_cast<int32_t>(context->smsShortCodeType);
    context->errorCode = Singleton<SmsServiceManagerClient>::GetInstance()
        .GetSmsShortCodeType(context->slotId, context->destAddr, smsShortCodeType);
    if (smsShortCodeType == static_cast<int32_t>(SmsShortCodeType::SMS_SHORT_CODE_TYPE_NOT_PREMIUM) ||
        smsShortCodeType == static_cast<int32_t>(SmsShortCodeType::SMS_SHORT_CODE_TYPE_POSSIBLE_PREMIUM)) {
        context->smsShortCodeType = static_cast<SmsShortCodeType>(smsShortCodeType);
    } else {
        context->smsShortCodeType = SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN;
    }
    if (context->errorCode == TELEPHONY_ERR_SUCCESS) {
        context->resolved = true;
    }
}

static void GetSmsShortCodeTypeCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<GetSmsShortCodeTypeContext *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_int32(env, static_cast<int32_t>(context->smsShortCodeType), &callbackValue);
    } else {
        napi_create_int32(env, static_cast<int32_t>(SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN), &callbackValue);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static napi_value GetSmsShortCodeType(napi_env env, napi_callback_info info)
{
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!NapiUtil::MatchParameters(env, parameters, { napi_number, napi_string })) {
        TELEPHONY_LOGE("GetSmsShortCodeType parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<GetSmsShortCodeTypeContext>().release();
    napi_get_value_int32(env, parameters[0], &context->slotId);
    context->destAddr = NapiUtil::GetStringFromValue(env, parameters[1]);
    napi_value result = NapiUtil::HandleAsyncWork(
        env, context, "GetSmsShortCodeType", NativeGetSmsShortCodeType, GetSmsShortCodeTypeCallback);
    return result;
}

static void GetImsShortMessageFormatCallback(napi_env env, napi_status status, void *data)
{
    if (!IsParamValid(env, data)) {
        return;
    }
    auto context = static_cast<SingleValueContext<std::u16string> *>(data);
    napi_value callbackValue = nullptr;
    if (context->resolved) {
        std::string toUtf8Value =
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.to_bytes(context->value);
        napi_create_string_utf8(env, toUtf8Value.c_str(), toUtf8Value.size(), &callbackValue);
    } else {
        JsError error = NapiUtil::ConverErrorMessageWithPermissionForJs(
            context->errorCode, "getImsShortMessageFormat", "ohos.permission.GET_TELEPHONY_STATE");
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

static bool MatchGetImsShortMessageFormatParameters(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    switch (parameterCount) {
        case NONE_PARAMETER: {
            return true;
        }
        case ONE_PARAMETER: {
            return NapiUtil::MatchParameters(env, parameters, { napi_function });
        }
        default: {
            return false;
        }
    }
}

static napi_value GetImsShortMessageFormat(napi_env env, napi_callback_info info)
{
    size_t paramsCount = ONE_PARAMETER;
    napi_value params[ONE_PARAMETER] = { 0 };
    napi_value arg = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, &paramsCount, params, &arg, &data);
    if (!MatchGetImsShortMessageFormatParameters(env, params, paramsCount)) {
        TELEPHONY_LOGE("GetImsShortMessageFormat parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<SingleValueContext<std::u16string>>().release();
    if (paramsCount == ONE_PARAMETER) {
        napi_create_reference(env, params[0], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    napi_value result = NapiUtil::HandleAsyncWork(env, context, "GetImsShortMessageFormat",
        NativeGetImsShortMessageFormat, GetImsShortMessageFormatCallback);
    return result;
}

static napi_value CreateEnumConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisArg = nullptr;
    void *data = nullptr;

    napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, &data);
    napi_value global = nullptr;
    napi_get_global(env, &global);
    return thisArg;
}

static void CreateEnumClass(napi_env env, napi_value exports, const char *className,
    const napi_property_descriptor *desc, size_t count)
{
    napi_value result = nullptr;
    napi_define_class(env, className, NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr, count, desc, &result);
    napi_set_named_property(env, exports, className, result);
}

static napi_value CreateEnumSendSmsResult(napi_env env, napi_value exports)
{
    napi_value success = nullptr;
    napi_value unknow = nullptr;
    napi_value radioOff = nullptr;
    napi_value serviceUnavailable = nullptr;

    napi_create_int32(env, (int32_t)SendSmsResult::SEND_SMS_SUCCESS, &success);
    napi_create_int32(env, (int32_t)SendSmsResult::SEND_SMS_FAILURE_UNKNOWN, &unknow);
    napi_create_int32(env, (int32_t)SendSmsResult::SEND_SMS_FAILURE_RADIO_OFF, &radioOff);
    napi_create_int32(env, (int32_t)SendSmsResult::SEND_SMS_FAILURE_SERVICE_UNAVAILABLE, &serviceUnavailable);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_SUCCESS", success),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_UNKNOWN", unknow),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_RADIO_OFF", radioOff),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_SERVICE_UNAVAILABLE", serviceUnavailable),
    };
    CreateEnumClass(env, exports, "SendSmsResult", desc, sizeof(desc) / sizeof(desc[0]));
    return exports;
}

static napi_value CreateEnumShortMessageClass(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "UNKNOWN", NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::UNKNOWN))),
        DECLARE_NAPI_STATIC_PROPERTY("INSTANT_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::INSTANT_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY("OPTIONAL_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::OPTIONAL_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "SIM_MESSAGE", NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::SIM_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY("FORWARD_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::FORWARD_MESSAGE))),
    };
    CreateEnumClass(env, exports, "ShortMessageClass", desc, sizeof(desc) / sizeof(desc[0]));
    return exports;
}

static napi_value CreateEnumMessageStatusClass(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_FREE",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_FREE))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_READ",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_READ))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_UNREAD",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_UNREAD))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_SENT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_SENT))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_UNSENT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_UNSENT))),
    };
    CreateEnumClass(env, exports, "SimMessageStatus", desc, sizeof(desc) / sizeof(desc[0]));
    return exports;
}

static napi_value CreateEnumRanType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "TYPE_GSM", NapiUtil::ToInt32Value(env, static_cast<int32_t>(RanType::TYPE_GSM))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "TYPE_CDMA", NapiUtil::ToInt32Value(env, static_cast<int32_t>(RanType::TYPE_CDMA))),
    };
    CreateEnumClass(env, exports, "RanType", desc, sizeof(desc) / sizeof(desc[0]));
    return exports;
}

static napi_value CreateEnumSmsSegmentsInfo(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_UNKNOWN",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_UNKNOWN))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_7BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_7BIT))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_8BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_8BIT))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_16BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_16BIT))),
    };
    CreateEnumClass(env, exports, "SmsEncodingScheme", desc, sizeof(desc) / sizeof(desc[0]));
    return exports;
}

static napi_value InitEnumSendSmsResult(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "SEND_SMS_SUCCESS", NapiUtil::ToInt32Value(env, static_cast<int32_t>(SEND_SMS_SUCCESS))),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_UNKNOWN",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(SEND_SMS_FAILURE_UNKNOWN))),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_RADIO_OFF",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(SEND_SMS_FAILURE_RADIO_OFF))),
        DECLARE_NAPI_STATIC_PROPERTY("SEND_SMS_FAILURE_SERVICE_UNAVAILABLE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(SEND_SMS_FAILURE_SERVICE_UNAVAILABLE))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_value InitEnumShortMessageClass(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "UNKNOWN", NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::UNKNOWN))),
        DECLARE_NAPI_STATIC_PROPERTY("INSTANT_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::INSTANT_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY("OPTIONAL_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::OPTIONAL_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "SIM_MESSAGE", NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::SIM_MESSAGE))),
        DECLARE_NAPI_STATIC_PROPERTY("FORWARD_MESSAGE",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(ShortMessageClass::FORWARD_MESSAGE))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_value InitEnumMessageStatusClass(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_FREE",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_FREE))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_READ",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_READ))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_UNREAD",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_UNREAD))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_SENT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_SENT))),
        DECLARE_NAPI_STATIC_PROPERTY("SIM_MESSAGE_STATUS_UNSENT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ShortMessage::SmsSimMessageStatus::SMS_SIM_MESSAGE_STATUS_UNSENT))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_value InitEnumRanType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "TYPE_GSM", NapiUtil::ToInt32Value(env, static_cast<int32_t>(RanType::TYPE_GSM))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "TYPE_CDMA", NapiUtil::ToInt32Value(env, static_cast<int32_t>(RanType::TYPE_CDMA))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_value InitEnumSmsSegmentsInfo(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_UNKNOWN",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_UNKNOWN))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_7BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_7BIT))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_8BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_8BIT))),
        DECLARE_NAPI_STATIC_PROPERTY("SMS_ENCODING_16BIT",
            NapiUtil::ToInt32Value(
                env, static_cast<int32_t>(ISmsServiceInterface::SmsEncodingScheme::SMS_ENCODING_16BIT))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value DeclareSmsInterface(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_WRITABLE_FUNCTION("sendMessage", SendMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("sendShortMessage", SendShortMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("createMessage", CreateMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("setDefaultSmsSlotId", SetDefaultSmsSlotId),
        DECLARE_NAPI_WRITABLE_FUNCTION("getDefaultSmsSlotId", GetDefaultSmsSlotId),
        DECLARE_NAPI_WRITABLE_FUNCTION("getDefaultSmsSimId", GetDefaultSmsSimId),
        DECLARE_NAPI_WRITABLE_FUNCTION("setSmscAddr", SetSmscAddr),
        DECLARE_NAPI_WRITABLE_FUNCTION("getSmscAddr", GetSmscAddr),
        DECLARE_NAPI_WRITABLE_FUNCTION("addSimMessage", AddSimMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("delSimMessage", DelSimMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("updateSimMessage", UpdateSimMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("getAllSimMessages", GetAllSimMessages),
        DECLARE_NAPI_WRITABLE_FUNCTION("setCBConfig", SetCBConfig),
        DECLARE_NAPI_WRITABLE_FUNCTION("setCBConfigList", SetCBConfigList),
        DECLARE_NAPI_WRITABLE_FUNCTION("splitMessage", SplitMessage),
        DECLARE_NAPI_WRITABLE_FUNCTION("hasSmsCapability", HasSmsCapability),
        DECLARE_NAPI_WRITABLE_FUNCTION("getSmsSegmentsInfo", GetSmsSegmentsInfo),
        DECLARE_NAPI_WRITABLE_FUNCTION("isImsSmsSupported", IsImsSmsSupported),
        DECLARE_NAPI_WRITABLE_FUNCTION("getImsShortMessageFormat", GetImsShortMessageFormat),
        DECLARE_NAPI_WRITABLE_FUNCTION("decodeMms", NapiMms::DecodeMms),
        DECLARE_NAPI_WRITABLE_FUNCTION("encodeMms", NapiMms::EncodeMms),
        DECLARE_NAPI_WRITABLE_FUNCTION("sendMms", NapiSendRecvMms::SendMms),
        DECLARE_NAPI_WRITABLE_FUNCTION("downloadMms", NapiSendRecvMms::DownloadMms),
        DECLARE_NAPI_WRITABLE_FUNCTION("getSmsShortCodeType", GetSmsShortCodeType),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

EXTERN_C_START
napi_value InitNapiSmsRegistry(napi_env env, napi_value exports)
{
    DeclareSmsInterface(env, exports);
    CreateEnumSendSmsResult(env, exports);
    CreateEnumShortMessageClass(env, exports);
    CreateEnumMessageStatusClass(env, exports);
    CreateEnumRanType(env, exports);
    CreateEnumSmsSegmentsInfo(env, exports);
    InitEnumSendSmsResult(env, exports);
    InitEnumShortMessageClass(env, exports);
    InitEnumMessageStatusClass(env, exports);
    InitEnumRanType(env, exports);
    InitEnumSmsSegmentsInfo(env, exports);
    NapiMms::InitEnumMmsCharSets(env, exports);
    NapiMms::InitEnumMessageType(env, exports);
    NapiMms::InitEnumPriorityType(env, exports);
    NapiMms::InitEnumVersionType(env, exports);
    NapiMms::InitEnumDispositionType(env, exports);
    NapiMms::InitEnumReportAllowedType(env, exports);
    NapiMms::InitSupportEnumMmsCharSets(env, exports);
    NapiMms::InitSupportEnumMessageType(env, exports);
    NapiMms::InitSupportEnumPriorityType(env, exports);
    NapiMms::InitSupportEnumVersionType(env, exports);
    NapiMms::InitSupportEnumDispositionType(env, exports);
    NapiMms::InitSupportEnumReportAllowedType(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module g_smsModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = InitNapiSmsRegistry,
    .nm_modname = "telephony.sms",
    .nm_priv = ((void *)0),
    .reserved = {(void *)0},
};

extern "C" __attribute__((constructor)) void RegisterTelephonySmsModule(void)
{
    napi_module_register(&g_smsModule);
}
} // namespace Telephony
} // namespace OHOS
