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
#include "napi_mms.h"

#include "ability.h"
#include "napi_base_context.h"
#include "napi_mms_pdu.h"
#include "napi_mms_pdu_helper.h"
#include "sms_constants_utils.h"
#include "telephony_permission.h"

namespace OHOS {
namespace Telephony {
namespace {
const std::string mmsTypeKey = "mmsType";
const std::string attachmentKey = "attachment";
static const int32_t DEFAULT_REF_COUNT = 1;
} // namespace

static void SetPropertyArray(napi_env env, napi_value object, const std::string &name, MmsAttachmentContext &context)
{
    if (context.inBuff == nullptr) {
        TELEPHONY_LOGE("inBuff is nullptr");
        return;
    }
    napi_value array = nullptr;
    napi_create_array(env, &array);
    for (uint32_t i = 0; i < context.inBuffLen; i++) {
        napi_value element = nullptr;
        napi_create_int32(env, context.inBuff[i], &element);
        napi_set_element(env, array, i, element);
    }
    napi_set_named_property(env, object, name.c_str(), array);
}

int32_t WrapDecodeMmsStatus(int32_t status)
{
    switch (status) {
        case MmsMsgType::MMS_MSGTYPE_SEND_REQ: {
            return MessageType::TYPE_MMS_SEND_REQ;
        }
        case MmsMsgType::MMS_MSGTYPE_SEND_CONF: {
            return MessageType::TYPE_MMS_SEND_CONF;
        }
        case MmsMsgType::MMS_MSGTYPE_NOTIFICATION_IND: {
            return MessageType::TYPE_MMS_NOTIFICATION_IND;
        }
        case MmsMsgType::MMS_MSGTYPE_NOTIFYRESP_IND: {
            return MessageType::TYPE_MMS_RESP_IND;
        }
        case MmsMsgType::MMS_MSGTYPE_RETRIEVE_CONF: {
            return MessageType::TYPE_MMS_RETRIEVE_CONF;
        }
        case MmsMsgType::MMS_MSGTYPE_ACKNOWLEDGE_IND: {
            return MessageType::TYPE_MMS_ACKNOWLEDGE_IND;
        }
        case MmsMsgType::MMS_MSGTYPE_DELIVERY_IND: {
            return MessageType::TYPE_MMS_DELIVERY_IND;
        }
        case MmsMsgType::MMS_MSGTYPE_READ_ORIG_IND: {
            return MessageType::TYPE_MMS_READ_ORIG_IND;
        }
        case MmsMsgType::MMS_MSGTYPE_READ_REC_IND: {
            return MessageType::TYPE_MMS_READ_REC_IND;
        }
        default: {
            return MESSAGE_UNKNOWN_STATUS;
        }
    }
}

int32_t WrapEncodeMmsStatus(int32_t status)
{
    switch (status) {
        case MessageType::TYPE_MMS_SEND_REQ: {
            return MmsMsgType::MMS_MSGTYPE_SEND_REQ;
        }
        case MessageType::TYPE_MMS_SEND_CONF: {
            return MmsMsgType::MMS_MSGTYPE_SEND_CONF;
        }
        case MessageType::TYPE_MMS_NOTIFICATION_IND: {
            return MmsMsgType::MMS_MSGTYPE_NOTIFICATION_IND;
        }
        case MessageType::TYPE_MMS_RESP_IND: {
            return MmsMsgType::MMS_MSGTYPE_NOTIFYRESP_IND;
        }
        case MessageType::TYPE_MMS_RETRIEVE_CONF: {
            return MmsMsgType::MMS_MSGTYPE_RETRIEVE_CONF;
        }
        case MessageType::TYPE_MMS_ACKNOWLEDGE_IND: {
            return MmsMsgType::MMS_MSGTYPE_ACKNOWLEDGE_IND;
        }
        case MessageType::TYPE_MMS_DELIVERY_IND: {
            return MmsMsgType::MMS_MSGTYPE_DELIVERY_IND;
        }
        case MessageType::TYPE_MMS_READ_ORIG_IND: {
            return MmsMsgType::MMS_MSGTYPE_READ_ORIG_IND;
        }
        case MessageType::TYPE_MMS_READ_REC_IND: {
            return MmsMsgType::MMS_MSGTYPE_READ_REC_IND;
        }
        default: {
            return MESSAGE_UNKNOWN_STATUS;
        }
    }
}

std::string parseDispositionValue(int32_t value)
{
    switch (value) {
        case FROM_DATA:
            return "from-data";
        case ATTACHMENT:
            return "attachment";
        case INLINE:
            return "inline";
        default:
            TELEPHONY_LOGE("Invalid contentDisposition value");
            return "";
    }
}

int32_t formatDispositionValue(const std::string &value)
{
    if (std::string("from-data") == value) {
        return FROM_DATA;
    } else if (std::string("attachment") == value) {
        return ATTACHMENT;
    } else {
        return INLINE;
    }
}

void GetMmsSendConf(MmsMsg mmsMsg, MmsSendConfContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsSendConf start");
    asyncContext.responseState = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_RESPONSE_STATUS);
    asyncContext.transactionId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_TRANSACTION_ID);
    asyncContext.version = static_cast<uint16_t>(mmsMsg.GetHeaderIntegerValue(MmsFieldCode::MMS_MMS_VERSION));
    asyncContext.messageId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID);
    TELEPHONY_LOGI("napi_mms GetMmsSendConf end");
}

void GetMmsSendReq(MmsMsg mmsMsg, MmsSendReqContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsSendConf end");
    asyncContext.from = mmsMsg.GetMmsFrom();
    mmsMsg.GetMmsTo(asyncContext.to);
    asyncContext.transactionId = mmsMsg.GetMmsTransactionId();
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.date = mmsMsg.GetMmsDate();
    mmsMsg.GetHeaderAllAddressValue(MmsFieldCode::MMS_CC, asyncContext.cc);
    mmsMsg.GetHeaderAllAddressValue(MmsFieldCode::MMS_BCC, asyncContext.bcc);
    asyncContext.subject = mmsMsg.GetMmsSubject();
    asyncContext.messageClass = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_MESSAGE_CLASS);
    asyncContext.expiry = mmsMsg.GetHeaderIntegerValue(MmsFieldCode::MMS_EXPIRY);
    asyncContext.priority = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_PRIORITY);
    asyncContext.senderVisibility = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_SENDER_VISIBILITY);
    asyncContext.deliveryReport = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT);
    asyncContext.readReport = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_READ_REPORT);
    asyncContext.contentType = mmsMsg.GetHeaderContentType();
    TELEPHONY_LOGI("napi_mms GetMmsSendReq end");
}

void GetMmsNotificationInd(MmsMsg mmsMsg, MmsNotificationIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsNotificationInd start");
    asyncContext.transactionId = mmsMsg.GetMmsTransactionId();
    asyncContext.messageClass = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_MESSAGE_CLASS);
    asyncContext.messageSize = mmsMsg.GetHeaderLongValue(MmsFieldCode::MMS_MESSAGE_SIZE);
    asyncContext.expiry = mmsMsg.GetHeaderIntegerValue(MmsFieldCode::MMS_EXPIRY);
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.from = mmsMsg.GetMmsFrom();
    asyncContext.subject = mmsMsg.GetMmsSubject();
    asyncContext.deliveryReport = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT);
    asyncContext.contentLocation = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_CONTENT_LOCATION);
    asyncContext.contentClass = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_CONTENT_CLASS);
}

void GetMmsRespInd(MmsMsg mmsMsg, MmsRespIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsRespInd start");
    asyncContext.transactionId = mmsMsg.GetMmsTransactionId();
    asyncContext.status = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_STATUS);
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.reportAllowed = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_REPORT_ALLOWED);
    TELEPHONY_LOGI("napi_mms GetMmsRespInd end");
}

void GetMmsRetrieveConf(MmsMsg mmsMsg, MmsRetrieveConfContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsRetrieveConf start");
    asyncContext.transactionId = mmsMsg.GetMmsTransactionId();
    asyncContext.messageId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID);
    asyncContext.date = mmsMsg.GetMmsDate();
    asyncContext.version = mmsMsg.GetMmsVersion();
    mmsMsg.GetMmsTo(asyncContext.to);
    asyncContext.from = mmsMsg.GetMmsFrom();
    mmsMsg.GetHeaderAllAddressValue(MmsFieldCode::MMS_CC, asyncContext.cc);
    asyncContext.subject = mmsMsg.GetMmsSubject();
    asyncContext.priority = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_PRIORITY);
    asyncContext.deliveryReport = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT);
    asyncContext.readReport = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_READ_REPORT);
    asyncContext.retrieveStatus = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_RETRIEVE_STATUS);
    asyncContext.retrieveText = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_RETRIEVE_TEXT);
    asyncContext.contentType = mmsMsg.GetHeaderContentType();
    TELEPHONY_LOGI("napi_mms GetMmsRetrieveConf end");
}

void GetMmsAcknowledgeInd(MmsMsg mmsMsg, MmsAcknowledgeIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsAcknowledgeInd start");
    asyncContext.transactionId = mmsMsg.GetMmsTransactionId();
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.reportAllowed = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_REPORT_ALLOWED);
    TELEPHONY_LOGI("napi_mms GetMmsAcknowledgeInd end");
}

void GetMmsDeliveryInd(MmsMsg mmsMsg, MmsDeliveryIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsDeliveryInd start");
    asyncContext.messageId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID);
    asyncContext.date = mmsMsg.GetMmsDate();
    std::vector<MmsAddress> toAddress;
    bool result = mmsMsg.GetMmsTo(toAddress);
    if (result) {
        asyncContext.to.assign(toAddress.begin(), toAddress.end());
    }
    asyncContext.status = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_STATUS);
    asyncContext.version = mmsMsg.GetMmsVersion();

    TELEPHONY_LOGI("napi_mms GetMmsDeliveryInd end");
}

void GetMmsReadOrigInd(MmsMsg mmsMsg, MmsReadOrigIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsReadOrigInd start");
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.messageId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID);
    mmsMsg.GetMmsTo(asyncContext.to);
    asyncContext.from = mmsMsg.GetMmsFrom();
    asyncContext.date = mmsMsg.GetMmsDate();
    asyncContext.readStatus = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_READ_STATUS);

    TELEPHONY_LOGI("napi_mms GetMmsReadOrigInd end");
}

void GetMmsReadRecInd(MmsMsg mmsMsg, MmsReadRecIndContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms GetMmsReadRecInd start");
    asyncContext.version = mmsMsg.GetMmsVersion();
    asyncContext.messageId = mmsMsg.GetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID);
    mmsMsg.GetMmsTo(asyncContext.to);
    asyncContext.from = mmsMsg.GetMmsFrom();
    asyncContext.date = mmsMsg.GetMmsDate();
    asyncContext.readStatus = mmsMsg.GetHeaderOctetValue(MmsFieldCode::MMS_READ_STATUS);

    TELEPHONY_LOGI("napi_mms GetMmsReadRecInd end");
}

void getAttachmentByDecodeMms(MmsMsg &mmsMsg, DecodeMmsContext &context)
{
    std::vector<MmsAttachment> attachment;
    mmsMsg.GetAllAttachment(attachment);
    if (attachment.empty()) {
        return;
    }
    for (auto it : attachment) {
        MmsAttachmentContext attachmentContext;
        attachmentContext.path = it.GetAttachmentFilePath();
        attachmentContext.fileName = it.GetFileName();
        attachmentContext.contentId = it.GetContentId();
        attachmentContext.contentLocation = it.GetContentLocation();
        attachmentContext.contentDisposition = it.GetContentDisposition();
        attachmentContext.contentTransferEncoding = it.GetContentTransferEncoding();
        attachmentContext.contentType = it.GetContentType();
        attachmentContext.isSmil = it.IsSmilFile();
        attachmentContext.charset = static_cast<int32_t>(it.GetCharSet());
        std::unique_ptr<char[]> buffer = nullptr;
        buffer = it.GetDataBuffer(attachmentContext.inBuffLen);
        attachmentContext.inBuff = std::move(buffer);
        context.attachment.push_back(std::move(attachmentContext));
    }
}

void NativeDecodeMms(napi_env env, void *data)
{
    if (data == nullptr) {
        TELEPHONY_LOGE("napi_mms data nullptr");
        return;
    }
    auto context = static_cast<DecodeMmsContext *>(data);
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        context->errorCode = TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
        return;
    }
    MmsMsg mmsMsg;
    bool mmsResult = false;
    if (context->messageMatchResult == TEXT_MESSAGE_PARAMETER_MATCH) {
        mmsResult = mmsMsg.DecodeMsg(context->textFilePath);
    } else if (context->messageMatchResult == RAW_DATA_MESSAGE_PARAMETER_MATCH) {
        mmsResult = mmsMsg.DecodeMsg(std::move(context->inBuffer), context->inLen);
    }
    if (!mmsResult) {
        TELEPHONY_LOGE("napi_mms DecodeMsg error!");
        context->errorCode = TELEPHONY_ERR_FAIL;
        return;
    }
    mmsMsg.DumpMms();
    int32_t messageType = WrapDecodeMmsStatus(static_cast<int32_t>(mmsMsg.GetMmsMessageType()));
    context->messageType = messageType;
    if (messageType == MessageType::TYPE_MMS_SEND_CONF) {
        GetMmsSendConf(mmsMsg, context->sendConf);
    } else if (messageType == MessageType::TYPE_MMS_SEND_REQ) {
        GetMmsSendReq(mmsMsg, context->sendReq);
    } else if (messageType == MessageType::TYPE_MMS_NOTIFICATION_IND) {
        GetMmsNotificationInd(mmsMsg, context->notificationInd);
    } else if (messageType == MessageType::TYPE_MMS_RESP_IND) {
        GetMmsRespInd(mmsMsg, context->respInd);
    } else if (messageType == MessageType::TYPE_MMS_RETRIEVE_CONF) {
        GetMmsRetrieveConf(mmsMsg, context->retrieveConf);
    } else if (messageType == MessageType::TYPE_MMS_ACKNOWLEDGE_IND) {
        GetMmsAcknowledgeInd(mmsMsg, context->acknowledgeInd);
    } else if (messageType == MessageType::TYPE_MMS_DELIVERY_IND) {
        GetMmsDeliveryInd(mmsMsg, context->deliveryInd);
    } else if (messageType == MessageType::TYPE_MMS_READ_ORIG_IND) {
        GetMmsReadOrigInd(mmsMsg, context->readOrigInd);
    } else if (messageType == MessageType::TYPE_MMS_READ_REC_IND) {
        GetMmsReadRecInd(mmsMsg, context->readRecInd);
    }
    getAttachmentByDecodeMms(mmsMsg, *context);
    context->errorCode = TELEPHONY_ERR_SUCCESS;
    context->resolved = true;
    TELEPHONY_LOGI("napi_mms NativeDecodeMms finish");
}

napi_value CreateAttachmentValue(napi_env env, MmsAttachmentContext &context)
{
    TELEPHONY_LOGI("napi_mms CreateAttachmentValue  start");
    napi_value attachment = nullptr;
    napi_create_object(env, &attachment);
    NapiUtil::SetPropertyStringUtf8(env, attachment, "path", context.path);
    NapiUtil::SetPropertyStringUtf8(env, attachment, "fileName", context.fileName);
    NapiUtil::SetPropertyStringUtf8(env, attachment, "contentId", context.contentId);
    NapiUtil::SetPropertyStringUtf8(env, attachment, "contentLocation", context.contentLocation);
    NapiUtil::SetPropertyInt32(
        env, attachment, "contentDisposition", formatDispositionValue(context.contentDisposition));
    NapiUtil::SetPropertyStringUtf8(env, attachment, "contentTransferEncoding", context.contentTransferEncoding);
    NapiUtil::SetPropertyStringUtf8(env, attachment, "contentType", context.contentType);
    NapiUtil::SetPropertyBoolean(env, attachment, "isSmil", context.isSmil);
    NapiUtil::SetPropertyInt32(env, attachment, "charset", context.charset);
    SetPropertyArray(env, attachment, "inBuff", context);
    return attachment;
}

void ParseAddress(napi_env env, napi_value outValue, const std::string &name, MmsAddress mmsAddress)
{
    napi_value addressObj = nullptr;
    napi_create_object(env, &addressObj);
    NapiUtil::SetPropertyStringUtf8(env, addressObj, "address", mmsAddress.GetAddressString());
    NapiUtil::SetPropertyInt32(env, addressObj, "charset", static_cast<int32_t>(mmsAddress.GetAddressCharset()));
    napi_set_named_property(env, outValue, name.c_str(), addressObj);
}

void ParseAddressArr(napi_env env, napi_value outValue, const std::string &name, std::vector<MmsAddress> addressArr)
{
    napi_value toArr = nullptr;
    napi_create_array(env, &toArr);
    for (size_t i = 0; i < addressArr.size(); i++) {
        napi_value addressObj = nullptr;
        napi_create_object(env, &addressObj);
        NapiUtil::SetPropertyStringUtf8(env, addressObj, "address", addressArr[i].GetAddressString());
        NapiUtil::SetPropertyInt32(env, addressObj, "charset", static_cast<int32_t>(addressArr[i].GetAddressCharset()));
        napi_set_element(env, toArr, i, addressObj);
    }
    napi_set_named_property(env, outValue, name.c_str(), toArr);
}

void ParseSendReqValue(napi_env env, napi_value object, MmsSendReqContext &sendReqContext)
{
    TELEPHONY_LOGI("napi_mms  ParseSendReqValue start");
    napi_value sendReqObj = nullptr;
    napi_create_object(env, &sendReqObj);
    ParseAddress(env, sendReqObj, "from", sendReqContext.from);
    ParseAddressArr(env, sendReqObj, "to", sendReqContext.to);
    NapiUtil::SetPropertyStringUtf8(env, sendReqObj, "transactionId", sendReqContext.transactionId);
    NapiUtil::SetPropertyInt32(env, sendReqObj, "version", sendReqContext.version);
    NapiUtil::SetPropertyInt64(env, sendReqObj, "date", sendReqContext.date);
    ParseAddressArr(env, sendReqObj, "cc", sendReqContext.cc);
    ParseAddressArr(env, sendReqObj, "bcc", sendReqContext.bcc);
    NapiUtil::SetPropertyStringUtf8(env, sendReqObj, "subject", sendReqContext.subject);
    NapiUtil::SetPropertyInt32(env, sendReqObj, "messageClass", static_cast<int32_t>(sendReqContext.messageClass));
    NapiUtil::SetPropertyInt32(env, sendReqObj, "expiry", sendReqContext.expiry);
    NapiUtil::SetPropertyInt32(env, sendReqObj, "priority", static_cast<int32_t>(sendReqContext.priority));
    NapiUtil::SetPropertyInt32(
        env, sendReqObj, "senderVisibility", static_cast<int32_t>(sendReqContext.senderVisibility));
    NapiUtil::SetPropertyInt32(env, sendReqObj, "deliveryReport", static_cast<int32_t>(sendReqContext.deliveryReport));
    NapiUtil::SetPropertyInt32(env, sendReqObj, "readReport", static_cast<int32_t>(sendReqContext.readReport));
    NapiUtil::SetPropertyStringUtf8(env, sendReqObj, "contentType", sendReqContext.contentType);
    napi_set_named_property(env, object, mmsTypeKey.c_str(), sendReqObj);
    TELEPHONY_LOGI("napi_mms  ParseSendReqValue end");
}

void ParseSendConfValue(napi_env env, napi_value object, MmsSendConfContext &sendConfContext)
{
    TELEPHONY_LOGI("napi_mms  ParseSendConfValue start");
    napi_value sendConfObj = nullptr;
    napi_create_object(env, &sendConfObj);
    NapiUtil::SetPropertyInt32(env, sendConfObj, "responseState", static_cast<int32_t>(sendConfContext.responseState));
    NapiUtil::SetPropertyStringUtf8(env, sendConfObj, "transactionId", sendConfContext.transactionId);
    NapiUtil::SetPropertyInt32(env, sendConfObj, "version", sendConfContext.version);
    NapiUtil::SetPropertyStringUtf8(env, sendConfObj, "messageId", sendConfContext.messageId);
    napi_set_named_property(env, object, mmsTypeKey.c_str(), sendConfObj);
    TELEPHONY_LOGI("napi_mms  ParseSendConfValue end");
}

void ParseNotificationIndValue(napi_env env, napi_value object, MmsNotificationIndContext &notificationContext)
{
    TELEPHONY_LOGI("napi_mms  ParseNotificationIndValue start");
    napi_value notificationObj = nullptr;
    napi_create_object(env, &notificationObj);
    NapiUtil::SetPropertyStringUtf8(env, notificationObj, "transactionId", notificationContext.transactionId);
    NapiUtil::SetPropertyInt32(
        env, notificationObj, "messageClass", static_cast<int32_t>(notificationContext.messageClass));
    NapiUtil::SetPropertyInt64(env, notificationObj, "messageSize", notificationContext.messageSize);
    NapiUtil::SetPropertyInt32(env, notificationObj, "expiry", notificationContext.expiry);
    NapiUtil::SetPropertyInt32(env, notificationObj, "version", notificationContext.version);
    ParseAddress(env, notificationObj, "from", notificationContext.from);
    NapiUtil::SetPropertyStringUtf8(env, notificationObj, "subject", notificationContext.subject);
    NapiUtil::SetPropertyInt32(
        env, notificationObj, "deliveryReport", static_cast<int32_t>(notificationContext.deliveryReport));
    NapiUtil::SetPropertyStringUtf8(env, notificationObj, "contentLocation", notificationContext.contentLocation);
    NapiUtil::SetPropertyInt32(
        env, notificationObj, "contentClass", static_cast<int32_t>(notificationContext.contentClass));
    napi_set_named_property(env, object, mmsTypeKey.c_str(), notificationObj);
}

void ParseRespIndValue(napi_env env, napi_value object, MmsRespIndContext &respIndContext)
{
    TELEPHONY_LOGI("napi_mms  ParseRespIndValue start");
    napi_value respIndObj = nullptr;
    napi_create_object(env, &respIndObj);
    NapiUtil::SetPropertyStringUtf8(env, respIndObj, "transactionId", respIndContext.transactionId);
    NapiUtil::SetPropertyInt32(env, respIndObj, "status", static_cast<int32_t>(respIndContext.status));
    NapiUtil::SetPropertyInt32(env, respIndObj, "version", static_cast<int32_t>(respIndContext.version));
    NapiUtil::SetPropertyInt32(env, respIndObj, "reportAllowed", static_cast<int32_t>(respIndContext.reportAllowed));
    napi_set_named_property(env, object, mmsTypeKey.c_str(), respIndObj);
    TELEPHONY_LOGI("napi_mms  ParseRespIndValue end");
}

void ParseRetrieveConfValue(napi_env env, napi_value object, MmsRetrieveConfContext &retrieveConfContext)
{
    TELEPHONY_LOGI("napi_mms  ParseRetrieveConfValue start");
    napi_value retrieveConfObj = nullptr;
    napi_create_object(env, &retrieveConfObj);
    NapiUtil::SetPropertyStringUtf8(env, retrieveConfObj, "transactionId", retrieveConfContext.transactionId);
    NapiUtil::SetPropertyStringUtf8(env, retrieveConfObj, "messageId", retrieveConfContext.messageId);
    NapiUtil::SetPropertyInt64(env, retrieveConfObj, "date", retrieveConfContext.date);
    NapiUtil::SetPropertyInt32(env, retrieveConfObj, "version", retrieveConfContext.version);
    ParseAddressArr(env, retrieveConfObj, "to", retrieveConfContext.to);
    ParseAddress(env, retrieveConfObj, "from", retrieveConfContext.from);
    ParseAddressArr(env, retrieveConfObj, "cc", retrieveConfContext.cc);
    NapiUtil::SetPropertyStringUtf8(env, retrieveConfObj, "subject", retrieveConfContext.subject);
    NapiUtil::SetPropertyInt32(env, retrieveConfObj, "priority", static_cast<int32_t>(retrieveConfContext.priority));
    NapiUtil::SetPropertyInt32(
        env, retrieveConfObj, "deliveryReport", static_cast<int32_t>(retrieveConfContext.deliveryReport));
    NapiUtil::SetPropertyInt32(
        env, retrieveConfObj, "readReport", static_cast<int32_t>(retrieveConfContext.readReport));
    NapiUtil::SetPropertyInt32(
        env, retrieveConfObj, "retrieveStatus", static_cast<int32_t>(retrieveConfContext.retrieveStatus));
    NapiUtil::SetPropertyStringUtf8(env, retrieveConfObj, "retrieveText", retrieveConfContext.retrieveText);
    NapiUtil::SetPropertyStringUtf8(env, retrieveConfObj, "contentType", retrieveConfContext.contentType);
    napi_set_named_property(env, object, mmsTypeKey.c_str(), retrieveConfObj);
    TELEPHONY_LOGI("napi_mms  ParseRetrieveConfValue end");
}

void ParseAcknowledgeIndValue(napi_env env, napi_value object, MmsAcknowledgeIndContext &acknowledgeIndContext)
{
    TELEPHONY_LOGI("napi_mms  ParseAcknowledgeIndValue start");
    napi_value acknowledgeIndObj = nullptr;
    napi_create_object(env, &acknowledgeIndObj);
    NapiUtil::SetPropertyStringUtf8(env, acknowledgeIndObj, "transactionId", acknowledgeIndContext.transactionId);
    NapiUtil::SetPropertyInt32(env, acknowledgeIndObj, "version", acknowledgeIndContext.version);
    NapiUtil::SetPropertyInt32(
        env, acknowledgeIndObj, "reportAllowed", static_cast<int32_t>(acknowledgeIndContext.reportAllowed));
    napi_set_named_property(env, object, mmsTypeKey.c_str(), acknowledgeIndObj);
    TELEPHONY_LOGI("napi_mms  ParseAcknowledgeIndValue end");
}

void ParseDeliveryIndValue(napi_env env, napi_value object, MmsDeliveryIndContext &deliveryIndContext)
{
    TELEPHONY_LOGI("napi_mms  ParseDeliveryIndValue start");
    napi_value deliveryIndObj = nullptr;
    napi_create_object(env, &deliveryIndObj);
    NapiUtil::SetPropertyStringUtf8(env, deliveryIndObj, "messageId", deliveryIndContext.messageId);
    NapiUtil::SetPropertyInt64(env, deliveryIndObj, "date", deliveryIndContext.date);
    ParseAddressArr(env, deliveryIndObj, "to", deliveryIndContext.to);
    NapiUtil::SetPropertyInt32(env, deliveryIndObj, "status", static_cast<int32_t>(deliveryIndContext.status));
    NapiUtil::SetPropertyInt32(env, deliveryIndObj, "version", deliveryIndContext.version);
    napi_set_named_property(env, object, mmsTypeKey.c_str(), deliveryIndObj);
    TELEPHONY_LOGI("napi_mms  ParseDeliveryIndValue end");
}

void ParseReadOrigIndValue(napi_env env, napi_value object, MmsReadOrigIndContext &readOrigIndContext)
{
    TELEPHONY_LOGI("napi_mms  ParseReadOrigIndValue start");
    napi_value readOrigIndObj = nullptr;
    napi_create_object(env, &readOrigIndObj);
    NapiUtil::SetPropertyInt32(env, readOrigIndObj, "version", readOrigIndContext.version);
    NapiUtil::SetPropertyStringUtf8(env, readOrigIndObj, "messageId", readOrigIndContext.messageId);
    ParseAddressArr(env, readOrigIndObj, "to", readOrigIndContext.to);
    ParseAddress(env, readOrigIndObj, "from", readOrigIndContext.from);
    NapiUtil::SetPropertyInt64(env, readOrigIndObj, "date", readOrigIndContext.date);
    NapiUtil::SetPropertyInt32(env, readOrigIndObj, "readStatus", static_cast<int32_t>(readOrigIndContext.readStatus));
    napi_set_named_property(env, object, mmsTypeKey.c_str(), readOrigIndObj);
    TELEPHONY_LOGI("napi_mms  ParseReadOrigIndValue end");
}

void ParseReadRecIndValue(napi_env env, napi_value object, MmsReadRecIndContext &readRecIndContext)
{
    TELEPHONY_LOGI("napi_mms  ParseReadRecIndValue start");
    napi_value readRecIndObj = nullptr;
    napi_create_object(env, &readRecIndObj);
    NapiUtil::SetPropertyInt32(env, readRecIndObj, "version", readRecIndContext.version);
    NapiUtil::SetPropertyStringUtf8(env, readRecIndObj, "messageId", readRecIndContext.messageId);
    ParseAddressArr(env, readRecIndObj, "to", readRecIndContext.to);
    ParseAddress(env, readRecIndObj, "from", readRecIndContext.from);
    NapiUtil::SetPropertyInt64(env, readRecIndObj, "date", readRecIndContext.date);
    NapiUtil::SetPropertyInt32(env, readRecIndObj, "readStatus", static_cast<int32_t>(readRecIndContext.readStatus));
    napi_set_named_property(env, object, mmsTypeKey.c_str(), readRecIndObj);
    TELEPHONY_LOGI("napi_mms  ParseReadRecIndValue end");
}

napi_value CreateDecodeMmsValue(napi_env env, DecodeMmsContext &asyncContext)
{
    TELEPHONY_LOGI("napi_mms  CreateDecodeMmsValue start");
    napi_value object = nullptr;
    napi_value attachmentArr = nullptr;
    napi_create_object(env, &object);
    napi_create_array(env, &attachmentArr);
    NapiUtil::SetPropertyInt32(env, object, "messageType", static_cast<int32_t>(asyncContext.messageType));
    if (asyncContext.attachment.size() > 0) {
        int i = 0;
        for (std::vector<MmsAttachmentContext>::iterator it = asyncContext.attachment.begin();
             it != asyncContext.attachment.end(); ++it) {
            napi_value attachNapi = CreateAttachmentValue(env, *it);
            napi_set_element(env, attachmentArr, i, attachNapi);
            i++;
        }
        napi_set_named_property(env, object, attachmentKey.c_str(), attachmentArr);
    }
    int32_t messageType = asyncContext.messageType;
    if (messageType == MessageType::TYPE_MMS_SEND_REQ) {
        ParseSendReqValue(env, object, asyncContext.sendReq);
    } else if (messageType == MessageType::TYPE_MMS_SEND_CONF) {
        ParseSendConfValue(env, object, asyncContext.sendConf);
    } else if (messageType == MessageType::TYPE_MMS_NOTIFICATION_IND) {
        ParseNotificationIndValue(env, object, asyncContext.notificationInd);
    } else if (messageType == MessageType::TYPE_MMS_RESP_IND) {
        ParseRespIndValue(env, object, asyncContext.respInd);
    } else if (messageType == MessageType::TYPE_MMS_RETRIEVE_CONF) {
        ParseRetrieveConfValue(env, object, asyncContext.retrieveConf);
    } else if (messageType == MessageType::TYPE_MMS_ACKNOWLEDGE_IND) {
        ParseAcknowledgeIndValue(env, object, asyncContext.acknowledgeInd);
    } else if (messageType == MessageType::TYPE_MMS_DELIVERY_IND) {
        ParseDeliveryIndValue(env, object, asyncContext.deliveryInd);
    } else if (messageType == MessageType::TYPE_MMS_READ_ORIG_IND) {
        ParseReadOrigIndValue(env, object, asyncContext.readOrigInd);
    } else if (messageType == MessageType::TYPE_MMS_READ_REC_IND) {
        ParseReadRecIndValue(env, object, asyncContext.readRecInd);
    }
    return object;
}

void DecodeMmsCallback(napi_env env, napi_status status, void *data)
{
    TELEPHONY_LOGI("napi_mms DecodeMmsCallback start");
    if (data == nullptr) {
        TELEPHONY_LOGE("data nullptr");
        return;
    }
    auto decodeMmsContext = static_cast<DecodeMmsContext *>(data);
    napi_value callbackValue = nullptr;

    if (status == napi_ok) {
        if (decodeMmsContext->resolved) {
            callbackValue = CreateDecodeMmsValue(env, *decodeMmsContext);
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(decodeMmsContext->errorCode);
            callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
        }
    } else {
        callbackValue =
            NapiUtil::CreateErrorMessage(env, "decode mms error,cause napi_status = " + std::to_string(status));
    }
    NapiUtil::Handle2ValueCallback(env, decodeMmsContext, callbackValue);
}

void ParseDecodeMmsParam(napi_env env, napi_value object, DecodeMmsContext &context)
{
    TELEPHONY_LOGI("napi_mms ParseDecodeMmsParam start");
    if (context.messageMatchResult == TEXT_MESSAGE_PARAMETER_MATCH) {
        TELEPHONY_LOGI("napi_mms  messageMatchResult == TEXT");
        char contentChars[MAX_TEXT_SHORT_MESSAGE_LENGTH] = { 0 };
        size_t contentLength = 0;
        napi_get_value_string_utf8(env, object, contentChars, MAX_TEXT_SHORT_MESSAGE_LENGTH, &contentLength);
        context.textFilePath = std::string(contentChars, 0, contentLength);
    } else if (context.messageMatchResult == RAW_DATA_MESSAGE_PARAMETER_MATCH) {
        TELEPHONY_LOGI("napi_mms  messageMatchResult  == RAW_DATA");
        napi_value elementValue = nullptr;
        int32_t element = 0;
        uint32_t arrayLength = 0;
        napi_get_array_length(env, object, &arrayLength);
        if (arrayLength > MMS_PDU_MAX_SIZE) {
            TELEPHONY_LOGE("arrayLength over size error");
            return;
        }
        context.inLen = arrayLength;
        TELEPHONY_LOGI("napi_mms ParseDecodeMmsParam arrayLength = %{public}d", arrayLength);
        context.inBuffer = std::make_unique<char[]>(arrayLength);
        if (context.inBuffer == nullptr) {
            TELEPHONY_LOGE("make unique error");
            return;
        }
        for (uint32_t i = 0; i < arrayLength; i++) {
            napi_get_element(env, object, i, &elementValue);
            napi_get_value_int32(env, elementValue, &element);
            context.inBuffer[i] = static_cast<char>(element);
        }
    }
}

int32_t GetMatchDecodeMmsResult(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    TELEPHONY_LOGI("napi_mms GetMatchDecodeMmsResult start");
    int32_t paramsTypeMatched = MESSAGE_PARAMETER_NOT_MATCH;
    switch (parameterCount) {
        case ONE_PARAMETER:
            paramsTypeMatched = NapiUtil::MatchParameters(env, parameters, { napi_object }) ||
                                NapiUtil::MatchParameters(env, parameters, { napi_string });
            break;
        case TWO_PARAMETERS:
            paramsTypeMatched = NapiUtil::MatchParameters(env, parameters, { napi_object, napi_function }) ||
                                NapiUtil::MatchParameters(env, parameters, { napi_string, napi_function });
            break;
        default:
            return MESSAGE_PARAMETER_NOT_MATCH;
    }
    if (!paramsTypeMatched) {
        return MESSAGE_PARAMETER_NOT_MATCH;
    }

    bool filePathIsStr = NapiUtil::MatchValueType(env, parameters[0], napi_string);
    bool filePathIsObj = NapiUtil::MatchValueType(env, parameters[0], napi_object);
    bool filePathIsArray = false;
    if (filePathIsObj) {
        napi_is_array(env, parameters[0], &filePathIsArray);
    }
    if (filePathIsStr) {
        return TEXT_MESSAGE_PARAMETER_MATCH;
    } else if (filePathIsArray) {
        return RAW_DATA_MESSAGE_PARAMETER_MATCH;
    } else {
        return MESSAGE_PARAMETER_NOT_MATCH;
    }
}

napi_value NapiMms::DecodeMms(napi_env env, napi_callback_info info)
{
    TELEPHONY_LOGI("napi_mms DecodeMms start");
    napi_value result = nullptr;
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    int32_t messageMatchResult = GetMatchDecodeMmsResult(env, parameters, parameterCount);
    if (messageMatchResult == MESSAGE_PARAMETER_NOT_MATCH) {
        TELEPHONY_LOGE("DecodeMms parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<DecodeMmsContext>().release();
    if (context == nullptr) {
        TELEPHONY_LOGE("DecodeMms DecodeMmsContext is nullptr.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    context->messageMatchResult = messageMatchResult;
    ParseDecodeMmsParam(env, parameters[0], *context);
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }
    result = NapiUtil::HandleAsyncWork(env, context, "DecodeMms", NativeDecodeMms, DecodeMmsCallback);
    return result;
}

bool MatchEncodeMms(napi_env env, const napi_value parameters[], size_t parameterCount)
{
    TELEPHONY_LOGI("napi_mms MatchEncodeMms start");
    bool paramsTypeMatched = false;
    switch (parameterCount) {
        case ONE_PARAMETER:
            paramsTypeMatched = NapiUtil::MatchParameters(env, parameters, { napi_object });
            break;
        case TWO_PARAMETERS:
            paramsTypeMatched = NapiUtil::MatchParameters(env, parameters, { napi_object, napi_function });
            break;
        default:
            return false;
    }
    if (!paramsTypeMatched) {
        TELEPHONY_LOGE("encodeMms parameter not match");
        return false;
    }
    if (NapiUtil::HasNamedProperty(env, parameters[0], "attachment")) {
        return NapiUtil::MatchObjectProperty(env, parameters[0],
            {
                { "messageType", napi_number },
                { "mmsType", napi_object },
                { "attachment", napi_object },
            });
    } else {
        return NapiUtil::MatchObjectProperty(env, parameters[0],
            {
                { "messageType", napi_number },
                { "mmsType", napi_object },
            });
    }
    return false;
}

bool GetNapiBooleanValue(napi_env env, napi_value napiValue, std::string name, bool defValue = false)
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        bool result = defValue;
        napi_get_value_bool(env, value, &result);
        return result;
    } else {
        return defValue;
    }
}

std::string GetNapiStringValue(napi_env env, napi_value napiValue, std::string name, std::string defValue = "")
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        return NapiUtil::GetStringFromValue(env, value);
    } else {
        return defValue;
    }
}

int32_t GetNapiInt32Value(napi_env env, napi_value napiValue, std::string name, int32_t defValue = 0)
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        int32_t intValue = 0;
        napi_status getIntStatus = napi_get_value_int32(env, value, &intValue);
        if (getIntStatus == napi_ok) {
            return intValue;
        }
    }
    return defValue;
}

int64_t GetNapiInt64Value(napi_env env, napi_value napiValue, std::string name, int64_t defValue = 0)
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        int64_t intValue = 0;
        napi_status getIntStatus = napi_get_value_int64(env, value, &intValue);
        if (getIntStatus == napi_ok) {
            return intValue;
        }
    }
    return defValue;
}

uint32_t GetNapiUint32Value(napi_env env, napi_value napiValue, std::string name, uint32_t defValue = 0)
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        uint32_t uint32Value = 0;
        napi_status status = napi_get_value_uint32(env, value, &uint32Value);
        if (status == napi_ok) {
            return uint32Value;
        }
    }
    return defValue;
}

uint8_t GetNapiUint8Value(napi_env env, napi_value napiValue, const std::string &name, uint8_t defValue = 0)
{
    int32_t value = GetNapiInt32Value(env, napiValue, name, defValue);
    if (value > UINT8_MAX || value < 0) {
        return defValue;
    }
    return static_cast<uint8_t>(value);
}

MmsCharSets formatMmsCharSet(int32_t charsetInt)
{
    switch (charsetInt) {
        case static_cast<int32_t>(MmsCharSets::BIG5):
            return MmsCharSets::BIG5;
        case static_cast<int32_t>(MmsCharSets::ISO_10646_UCS_2):
            return MmsCharSets::ISO_10646_UCS_2;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_1):
            return MmsCharSets::ISO_8859_1;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_2):
            return MmsCharSets::ISO_8859_2;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_3):
            return MmsCharSets::ISO_8859_3;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_4):
            return MmsCharSets::ISO_8859_4;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_5):
            return MmsCharSets::ISO_8859_5;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_6):
            return MmsCharSets::ISO_8859_6;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_7):
            return MmsCharSets::ISO_8859_7;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_8):
            return MmsCharSets::ISO_8859_8;
        case static_cast<int32_t>(MmsCharSets::ISO_8859_9):
            return MmsCharSets::ISO_8859_9;
        case static_cast<int32_t>(MmsCharSets::SHIFT_JIS):
            return MmsCharSets::SHIFT_JIS;
        case static_cast<int32_t>(MmsCharSets::US_ASCII):
            return MmsCharSets::US_ASCII;
        default:
            return MmsCharSets::UTF_8;
    }
}

MmsAddress ReadMmsAddress(napi_env env, napi_value value)
{
    std::string address = GetNapiStringValue(env, value, "address");
    int32_t charset = GetNapiInt32Value(env, value, "charset");
    MmsAddress mmsAddress(address, formatMmsCharSet(charset));
    return mmsAddress;
}

void ReadMmsAddress(napi_env env, napi_value napiValue, std::string name, std::vector<MmsAddress> &array)
{
    napi_value value = NapiUtil::GetNamedProperty(env, napiValue, name);
    if (value != nullptr) {
        uint32_t arrayLength = 0;
        napi_get_array_length(env, value, &arrayLength);
        napi_value elementValue = nullptr;
        for (uint32_t i = 0; i < arrayLength; i++) {
            napi_get_element(env, value, i, &elementValue);
            MmsAddress eachMmsAddress = ReadMmsAddress(env, elementValue);
            array.push_back(eachMmsAddress);
        }
    }
}

bool HasNamedProperty(napi_env env, napi_value napiValue, std::vector<std::string> checkName)
{
    for (std::string item : checkName) {
        if (!NapiUtil::HasNamedProperty(env, napiValue, item)) {
            TELEPHONY_LOGE("Missed param with %{public}s", item.c_str());
            return false;
        }
    }
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsSendReqContext &sendReq)
{
    TELEPHONY_LOGI("mms_napi ReadEncodeMmsType start");
    if (!HasNamedProperty(env, napiValue, { "from", "transactionId", "contentType" })) {
        return false;
    }
    napi_value from = NapiUtil::GetNamedProperty(env, napiValue, "from");
    sendReq.from = ReadMmsAddress(env, from);
    if (sendReq.from.GetAddressString().empty()) {
        return false;
    }
    ReadMmsAddress(env, napiValue, "to", sendReq.to);
    sendReq.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    sendReq.version = GetNapiInt32Value(env, napiValue, "version");
    sendReq.date = GetNapiInt64Value(env, napiValue, "date");
    ReadMmsAddress(env, napiValue, "cc", sendReq.cc);
    ReadMmsAddress(env, napiValue, "bcc", sendReq.bcc);
    sendReq.subject = GetNapiStringValue(env, napiValue, "subject");
    sendReq.messageClass = GetNapiUint8Value(env, napiValue, "messageClass");
    sendReq.expiry = GetNapiInt32Value(env, napiValue, "expiry");
    sendReq.priority = GetNapiUint8Value(env, napiValue, "priority");
    sendReq.senderVisibility = GetNapiUint8Value(env, napiValue, "senderVisibility");
    sendReq.deliveryReport = GetNapiUint8Value(env, napiValue, "deliveryReport");
    sendReq.readReport = GetNapiUint8Value(env, napiValue, "readReport");
    sendReq.contentType = GetNapiStringValue(env, napiValue, "contentType");
    TELEPHONY_LOGI("mms_napi ReadEncodeMmsType end");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsSendConfContext &sendConf)
{
    if (!HasNamedProperty(env, napiValue, { "responseState", "transactionId" })) {
        return false;
    }
    sendConf.responseState = GetNapiUint8Value(env, napiValue, "responseState");
    sendConf.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    sendConf.version = GetNapiInt32Value(env, napiValue, "version");
    sendConf.messageId = GetNapiStringValue(env, napiValue, "messageId");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsNotificationIndContext &notificationInd)
{
    std::vector<std::string> checkName = { "transactionId", "messageClass", "messageSize", "expiry",
        "contentLocation" };
    if (!HasNamedProperty(env, napiValue, checkName)) {
        return false;
    }
    notificationInd.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    notificationInd.messageClass = GetNapiUint8Value(env, napiValue, "messageClass");
    notificationInd.messageSize = GetNapiInt64Value(env, napiValue, "messageSize");
    notificationInd.expiry = GetNapiInt32Value(env, napiValue, "expiry");
    notificationInd.version = static_cast<uint16_t>(GetNapiInt32Value(env, napiValue, "version"));
    napi_value from = NapiUtil::GetNamedProperty(env, napiValue, "from");
    notificationInd.from = ReadMmsAddress(env, from);
    notificationInd.subject = GetNapiStringValue(env, napiValue, "subject");
    notificationInd.deliveryReport = GetNapiUint8Value(env, napiValue, "deliveryReport");
    notificationInd.contentLocation = GetNapiStringValue(env, napiValue, "contentLocation");
    notificationInd.contentClass = GetNapiInt32Value(env, napiValue, "contentClass");
    notificationInd.charset = GetNapiUint32Value(env, napiValue, "charset", static_cast<uint32_t>(MmsCharSets::UTF_8));
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsRespIndContext &respInd)
{
    if (!HasNamedProperty(env, napiValue, { "transactionId", "status" })) {
        return false;
    }
    respInd.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    respInd.status = GetNapiUint8Value(env, napiValue, "status");
    respInd.version = GetNapiInt32Value(env, napiValue, "version");
    respInd.reportAllowed = GetNapiUint8Value(env, napiValue, "reportAllowed");
    TELEPHONY_LOGI("respInd.reportAllowed = %{public}d", respInd.reportAllowed);
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsRetrieveConfContext &retrieveConf)
{
    if (!HasNamedProperty(env, napiValue, { "transactionId", "messageId", "date", "contentType" })) {
        return false;
    }
    retrieveConf.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    retrieveConf.messageId = GetNapiStringValue(env, napiValue, "messageId");
    retrieveConf.date = GetNapiInt64Value(env, napiValue, "date");
    retrieveConf.version = (uint16_t)GetNapiInt32Value(env, napiValue, "version");
    ReadMmsAddress(env, napiValue, "to", retrieveConf.to);
    napi_value from = NapiUtil::GetNamedProperty(env, napiValue, "from");
    retrieveConf.from = ReadMmsAddress(env, from);
    ReadMmsAddress(env, napiValue, "cc", retrieveConf.cc);
    retrieveConf.subject = GetNapiStringValue(env, napiValue, "subject");
    retrieveConf.priority = GetNapiUint8Value(env, napiValue, "priority");
    retrieveConf.deliveryReport = GetNapiUint8Value(env, napiValue, "deliveryReport");
    retrieveConf.readReport = GetNapiUint8Value(env, napiValue, "readReport");
    retrieveConf.retrieveStatus = GetNapiUint8Value(env, napiValue, "retrieveStatus");
    retrieveConf.retrieveText = GetNapiStringValue(env, napiValue, "retrieveText");
    retrieveConf.contentType = GetNapiStringValue(env, napiValue, "contentType");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsAcknowledgeIndContext &acknowledgeInd)
{
    if (!HasNamedProperty(env, napiValue, { "transactionId" })) {
        return false;
    }
    acknowledgeInd.transactionId = GetNapiStringValue(env, napiValue, "transactionId");
    acknowledgeInd.version = GetNapiInt32Value(env, napiValue, "version");
    acknowledgeInd.reportAllowed = GetNapiUint8Value(env, napiValue, "reportAllowed");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsDeliveryIndContext &deliveryInd)
{
    if (!HasNamedProperty(env, napiValue, { "messageId", "date", "to", "status" })) {
        return false;
    }
    deliveryInd.messageId = GetNapiStringValue(env, napiValue, "messageId");
    deliveryInd.date = GetNapiInt64Value(env, napiValue, "date");
    ReadMmsAddress(env, napiValue, "to", deliveryInd.to);
    deliveryInd.status = GetNapiUint8Value(env, napiValue, "status");
    deliveryInd.version = GetNapiInt32Value(env, napiValue, "version");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsReadOrigIndContext &readOrigInd)
{
    if (!HasNamedProperty(env, napiValue, { "version", "messageId", "to", "from", "date", "readStatus" })) {
        return false;
    }
    readOrigInd.version = GetNapiInt32Value(env, napiValue, "version");
    readOrigInd.messageId = GetNapiStringValue(env, napiValue, "messageId");
    ReadMmsAddress(env, napiValue, "to", readOrigInd.to);
    napi_value from = NapiUtil::GetNamedProperty(env, napiValue, "from");
    readOrigInd.from = ReadMmsAddress(env, from);
    readOrigInd.date = GetNapiInt64Value(env, napiValue, "date");
    readOrigInd.readStatus = GetNapiUint8Value(env, napiValue, "readStatus");
    return true;
}

bool ReadEncodeMmsType(napi_env env, napi_value napiValue, MmsReadRecIndContext &readRecInd)
{
    if (!HasNamedProperty(env, napiValue, { "version", "messageId", "to", "from", "date", "readStatus" })) {
        return false;
    }
    readRecInd.version = GetNapiInt32Value(env, napiValue, "version");
    readRecInd.messageId = GetNapiStringValue(env, napiValue, "messageId");
    ReadMmsAddress(env, napiValue, "to", readRecInd.to);
    napi_value from = NapiUtil::GetNamedProperty(env, napiValue, "from");
    readRecInd.from = ReadMmsAddress(env, from);
    readRecInd.date = GetNapiInt64Value(env, napiValue, "date");
    readRecInd.readStatus = GetNapiUint8Value(env, napiValue, "readStatus");
    TELEPHONY_LOGI("context->readRecInd.readStatus = %{public}d", readRecInd.readStatus);
    return true;
}

MmsAttachmentContext BuildMmsAttachment(napi_env env, napi_value value)
{
    MmsAttachmentContext attachmentContext;
    attachmentContext.path = GetNapiStringValue(env, value, "path");
    attachmentContext.fileName = GetNapiStringValue(env, value, "fileName");
    attachmentContext.contentId = GetNapiStringValue(env, value, "contentId");
    attachmentContext.contentLocation = GetNapiStringValue(env, value, "contentLocation");
    attachmentContext.contentDisposition = parseDispositionValue(GetNapiInt32Value(env, value, "contentDisposition"));
    attachmentContext.contentTransferEncoding = GetNapiStringValue(env, value, "contentTransferEncoding");
    attachmentContext.contentType = GetNapiStringValue(env, value, "contentType");
    attachmentContext.isSmil = GetNapiBooleanValue(env, value, "isSmil");
    napi_value inBuffValue = NapiUtil::GetNamedProperty(env, value, "inBuff");
    if (inBuffValue != nullptr) {
        uint32_t arrayLength = 0;
        int32_t elementInt = 0;
        napi_get_array_length(env, inBuffValue, &arrayLength);
        if (arrayLength > MMS_PDU_MAX_SIZE) {
            TELEPHONY_LOGE("arrayLength over size error");
            return attachmentContext;
        }
        attachmentContext.inBuffLen = arrayLength;
        attachmentContext.inBuff = std::make_unique<char[]>(arrayLength);
        if (attachmentContext.inBuff == nullptr) {
            TELEPHONY_LOGE("make unique error");
            return attachmentContext;
        }
        napi_value elementValue = nullptr;
        for (uint32_t i = 0; i < arrayLength; i++) {
            napi_get_element(env, inBuffValue, i, &elementValue);
            napi_get_value_int32(env, elementValue, &elementInt);
            attachmentContext.inBuff[i] = static_cast<char>(elementInt);
        }
    }
    attachmentContext.charset = GetNapiInt32Value(env, value, "charset", static_cast<int32_t>(MmsCharSets::UTF_8));
    return attachmentContext;
}

bool ReadEncodeMmsAttachment(napi_env env, napi_value value, std::vector<MmsAttachmentContext> &attachment)
{
    uint32_t arrayLength = 0;
    napi_get_array_length(env, value, &arrayLength);
    napi_value elementValue = nullptr;
    for (uint32_t i = 0; i < arrayLength; i++) {
        napi_get_element(env, value, i, &elementValue);
        MmsAttachmentContext mmsAttachmentContext = BuildMmsAttachment(env, elementValue);
        attachment.push_back(std::move(mmsAttachmentContext));
    }
    return true;
}

bool EncodeMmsType(napi_env env, napi_value mmsTypeValue, EncodeMmsContext &context)
{
    bool result = false;
    switch (context.messageType) {
        case TYPE_MMS_SEND_REQ:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.sendReq);
            break;
        case TYPE_MMS_SEND_CONF:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.sendConf);
            break;
        case TYPE_MMS_NOTIFICATION_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.notificationInd);
            break;
        case TYPE_MMS_RESP_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.respInd);
            break;
        case TYPE_MMS_RETRIEVE_CONF:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.retrieveConf);
            break;
        case TYPE_MMS_ACKNOWLEDGE_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.acknowledgeInd);
            break;
        case TYPE_MMS_DELIVERY_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.deliveryInd);
            break;
        case TYPE_MMS_READ_ORIG_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.readOrigInd);
            break;
        case TYPE_MMS_READ_REC_IND:
            result = ReadEncodeMmsType(env, mmsTypeValue, context.readRecInd);
            break;
        default:
            TELEPHONY_LOGE("napi_mms EncodeMms param messageType is incorrect");
            break;
    }
    return result;
}

bool ParseEncodeMmsParam(napi_env env, napi_value object, EncodeMmsContext &context)
{
    TELEPHONY_LOGI("mms_napi ParseEncodeMmsParam start");
    napi_value messageTypeValue = NapiUtil::GetNamedProperty(env, object, "messageType");
    if (messageTypeValue == nullptr) {
        TELEPHONY_LOGE("messageTypeValue == nullptr");
        return false;
    }
    napi_get_value_int32(env, messageTypeValue, &context.messageType);

    napi_value mmsTypeValue = NapiUtil::GetNamedProperty(env, object, "mmsType");
    if (mmsTypeValue == nullptr) {
        TELEPHONY_LOGE("mmsTypeValue == nullptr");
        return false;
    }
    bool result = EncodeMmsType(env, mmsTypeValue, context);
    if (NapiUtil::HasNamedProperty(env, object, "attachment")) {
        napi_value napiAttachment = NapiUtil::GetNamedProperty(env, object, "attachment");
        if (napiAttachment != nullptr) {
            result = ReadEncodeMmsAttachment(env, napiAttachment, context.attachment);
        }
    }
    return result;
}

bool SetAttachmentToCore(MmsMsg &mmsMsg, std::vector<MmsAttachmentContext> &attachment)
{
    if (attachment.size() > 0) {
        for (auto it = attachment.begin(); it != attachment.end(); it++) {
            MmsAttachment itAttachment;
            if (it->path.size() > 0) {
                itAttachment.SetAttachmentFilePath(it->path, it->isSmil);
            }
            itAttachment.SetIsSmilFile(it->isSmil);
            if (it->fileName.size() > 0) {
                itAttachment.SetFileName(it->fileName);
            }
            if (it->contentId.size() > 0) {
                itAttachment.SetContentId(it->contentId);
            }
            if (it->contentLocation.size() > 0) {
                itAttachment.SetContentLocation(it->contentLocation);
            }
            if (it->contentDisposition.size() > 0) {
                itAttachment.SetContentDisposition(it->contentDisposition);
            }
            if (it->contentTransferEncoding.size() > 0) {
                itAttachment.SetContentTransferEncoding(it->contentTransferEncoding);
            }
            if (it->contentType.size() > 0) {
                itAttachment.SetContentType(it->contentType);
            }
            if (it->charset != DEFAULT_ERROR) {
                itAttachment.SetCharSet(it->charset);
            }
            if (it->inBuffLen > 0) {
                itAttachment.SetDataBuffer(std::move(it->inBuff), it->inBuffLen);
            }
            if (!mmsMsg.AddAttachment(itAttachment)) {
                TELEPHONY_LOGE("attachment file error");
                return false;
            }
        }
    }
    return true;
}

void setSendReqToCore(MmsMsg &mmsMsg, MmsSendReqContext &context)
{
    mmsMsg.SetMmsFrom(context.from);
    if (context.to.size() > 0) {
        mmsMsg.SetMmsTo(context.to);
    }
    if (context.transactionId.size() > 0) {
        mmsMsg.SetMmsTransactionId(context.transactionId);
    }
    if (context.version > 0) {
        mmsMsg.SetMmsVersion(context.version);
    }
    if (context.date > 0) {
        mmsMsg.SetMmsDate(context.date);
    }
    if (context.cc.size() > 0) {
        for (MmsAddress address : context.cc) {
            mmsMsg.AddHeaderAddressValue(MmsFieldCode::MMS_CC, address);
        }
    }
    if (context.bcc.size() > 0) {
        for (MmsAddress address : context.bcc) {
            mmsMsg.AddHeaderAddressValue(MmsFieldCode::MMS_BCC, address);
        }
    }
    if (context.subject.size() > 0) {
        mmsMsg.SetMmsSubject(context.subject);
    }
    if (context.messageClass > 0) {
        mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_MESSAGE_CLASS, context.messageClass);
    }
    if (context.expiry > 0) {
        mmsMsg.SetHeaderIntegerValue(MmsFieldCode::MMS_EXPIRY, context.expiry);
    }
    if (context.priority > 0) {
        mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_PRIORITY, context.priority);
    }
    if (context.senderVisibility > 0) {
        mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_SENDER_VISIBILITY, context.senderVisibility);
    }
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT, context.deliveryReport);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_READ_REPORT, context.readReport);
    mmsMsg.SetHeaderContentType(context.contentType);
}

void setSendConfToCore(MmsMsg &mmsMsg, MmsSendConfContext &context)
{
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_RESPONSE_STATUS, context.responseState);
    mmsMsg.SetMmsTransactionId(context.transactionId);
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID, context.messageId);
}

void setNotificationIndToCore(MmsMsg &mmsMsg, MmsNotificationIndContext &context)
{
    mmsMsg.SetMmsTransactionId(context.transactionId);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_MESSAGE_CLASS, context.messageClass);
    mmsMsg.SetHeaderLongValue(MmsFieldCode::MMS_MESSAGE_SIZE, context.messageSize);
    mmsMsg.SetHeaderIntegerValue(MmsFieldCode::MMS_EXPIRY, context.expiry);
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetMmsFrom(context.from);
    mmsMsg.SetMmsSubject(context.subject);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT, context.deliveryReport);
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_CONTENT_LOCATION, context.contentLocation);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_CONTENT_CLASS, context.contentClass);
}

void setRespIndToCore(MmsMsg &mmsMsg, MmsRespIndContext &context)
{
    mmsMsg.SetMmsTransactionId(context.transactionId);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_STATUS, context.status);
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_REPORT_ALLOWED, context.reportAllowed);
}

void setRetrieveConfToCore(MmsMsg &mmsMsg, MmsRetrieveConfContext &context)
{
    mmsMsg.SetMmsTransactionId(context.transactionId);
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID, context.messageId);
    mmsMsg.SetMmsDate(context.date);
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetMmsTo(context.to);
    mmsMsg.SetMmsFrom(context.from);
    if (context.cc.size() > 0) {
        for (MmsAddress address : context.cc) {
            mmsMsg.AddHeaderAddressValue(MmsFieldCode::MMS_CC, address);
        }
    }
    mmsMsg.SetMmsSubject(context.subject);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_PRIORITY, context.priority);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_DELIVERY_REPORT, context.deliveryReport);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_READ_REPORT, context.readReport);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_RETRIEVE_STATUS, context.retrieveStatus);
    if (!context.retrieveText.empty()) {
        mmsMsg.SetHeaderEncodedStringValue(
            MmsFieldCode::MMS_RETRIEVE_TEXT, context.retrieveText, (uint32_t)MmsCharSets::UTF_8);
    }
    mmsMsg.SetHeaderContentType(context.contentType);
}

void setAcknowledgeIndToCore(MmsMsg &mmsMsg, MmsAcknowledgeIndContext &context)
{
    mmsMsg.SetMmsTransactionId(context.transactionId);
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_REPORT_ALLOWED, context.reportAllowed);
}

void setDeliveryIndToCore(MmsMsg &mmsMsg, MmsDeliveryIndContext &context)
{
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID, context.messageId);
    mmsMsg.SetMmsDate(context.date);
    mmsMsg.SetMmsTo(context.to);
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_STATUS, context.status);
    mmsMsg.SetMmsVersion(context.version);
}
void setReadOrigIndToCore(MmsMsg &mmsMsg, MmsReadOrigIndContext &context)
{
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID, context.messageId);
    mmsMsg.SetMmsTo(context.to);
    mmsMsg.SetMmsFrom(context.from);
    if (context.date != 0) {
        mmsMsg.SetMmsDate(context.date);
    }
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_READ_STATUS, context.readStatus);
}

void setReadRecIndToCore(MmsMsg &mmsMsg, MmsReadRecIndContext &context)
{
    mmsMsg.SetMmsVersion(context.version);
    mmsMsg.SetHeaderStringValue(MmsFieldCode::MMS_MESSAGE_ID, context.messageId);
    mmsMsg.SetMmsTo(context.to);
    mmsMsg.SetMmsFrom(context.from);
    if (context.date != 0) {
        mmsMsg.SetMmsDate(context.date);
    }
    mmsMsg.SetHeaderOctetValue(MmsFieldCode::MMS_READ_STATUS, context.readStatus);
}

void SetRequestToCore(MmsMsg &mmsMsg, EncodeMmsContext *context)
{
    if (context == nullptr) {
        TELEPHONY_LOGE("context is nullptr");
        return;
    }
    switch (context->messageType) {
        case MessageType::TYPE_MMS_SEND_REQ:
            setSendReqToCore(mmsMsg, context->sendReq);
            break;
        case MessageType::TYPE_MMS_SEND_CONF:
            setSendConfToCore(mmsMsg, context->sendConf);
            break;
        case MessageType::TYPE_MMS_NOTIFICATION_IND:
            setNotificationIndToCore(mmsMsg, context->notificationInd);
            break;
        case MessageType::TYPE_MMS_RESP_IND:
            setRespIndToCore(mmsMsg, context->respInd);
            break;
        case MessageType::TYPE_MMS_RETRIEVE_CONF:
            setRetrieveConfToCore(mmsMsg, context->retrieveConf);
            break;
        case MessageType::TYPE_MMS_ACKNOWLEDGE_IND:
            setAcknowledgeIndToCore(mmsMsg, context->acknowledgeInd);
            break;
        case MessageType::TYPE_MMS_DELIVERY_IND:
            setDeliveryIndToCore(mmsMsg, context->deliveryInd);
            break;
        case MessageType::TYPE_MMS_READ_ORIG_IND:
            setReadOrigIndToCore(mmsMsg, context->readOrigInd);
            break;
        case MessageType::TYPE_MMS_READ_REC_IND:
            setReadRecIndToCore(mmsMsg, context->readRecInd);
            break;
        default:
            break;
    }
}

void NativeEncodeMms(napi_env env, void *data)
{
    if (data == nullptr) {
        TELEPHONY_LOGE("NativeEncodeMms data is nullptr");
        NapiUtil::ThrowParameterError(env);
        return;
    }

    EncodeMmsContext *context = static_cast<EncodeMmsContext *>(data);
    if (context == nullptr) {
        TELEPHONY_LOGE("context is nullptr");
        return;
    }
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        context->errorCode = TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
        return;
    }
    MmsMsg mmsMsg;
    mmsMsg.SetMmsMessageType(static_cast<uint8_t>(WrapEncodeMmsStatus(context->messageType)));
    if (!SetAttachmentToCore(mmsMsg, context->attachment)) {
        context->errorCode = TELEPHONY_ERR_ARGUMENT_INVALID;
        context->resolved = false;
        return;
    }
    SetRequestToCore(mmsMsg, context);
    auto encodeResult = mmsMsg.EncodeMsg(context->bufferLen);
    if (encodeResult != nullptr) {
        context->outBuffer = std::move(encodeResult);
        context->errorCode = TELEPHONY_ERR_SUCCESS;
        context->resolved = true;
    } else {
        context->errorCode = TELEPHONY_ERR_FAIL;
        context->resolved = false;
    }
    TELEPHONY_LOGD("napi_mms NativeEncodeMms length:%{private}d", context->bufferLen);
}

void EncodeMmsCallback(napi_env env, napi_status status, void *data)
{
    if (data == nullptr) {
        TELEPHONY_LOGE("data is nullptr");
        return;
    }
    auto context = static_cast<EncodeMmsContext *>(data);

    napi_value callbackValue = nullptr;
    if (context->resolved) {
        napi_create_array(env, &callbackValue);
        for (uint32_t i = 0; i < context->bufferLen; i++) {
            napi_value itemValue = nullptr;
            int32_t element = context->outBuffer[i];
            napi_create_int32(env, element, &itemValue);
            napi_set_element(env, callbackValue, i, itemValue);
        }
    } else {
        JsError error = NapiUtil::ConverErrorMessageForJs(context->errorCode);
        callbackValue = NapiUtil::CreateErrorMessage(env, error.errorMessage, error.errorCode);
    }
    NapiUtil::Handle2ValueCallback(env, context, callbackValue);
}

napi_value NapiMms::EncodeMms(napi_env env, napi_callback_info info)
{
    TELEPHONY_LOGI("napi_mms EncodeMms start");
    napi_value result = nullptr;
    size_t parameterCount = TWO_PARAMETERS;
    napi_value parameters[TWO_PARAMETERS] = { 0 };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    napi_get_cb_info(env, info, &parameterCount, parameters, &thisVar, &data);
    if (!MatchEncodeMms(env, parameters, parameterCount)) {
        TELEPHONY_LOGE("EncodeMms parameter matching failed.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    auto context = std::make_unique<EncodeMmsContext>().release();
    if (context == nullptr) {
        TELEPHONY_LOGE("EncodeMms EncodeMmsContext is nullptr.");
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }

    if (!ParseEncodeMmsParam(env, parameters[0], *context)) {
        free(context);
        context = nullptr;
        NapiUtil::ThrowParameterError(env);
        return nullptr;
    }
    if (parameterCount == TWO_PARAMETERS) {
        napi_create_reference(env, parameters[1], DEFAULT_REF_COUNT, &context->callbackRef);
    }

    result = NapiUtil::HandleAsyncWork(env, context, "EncodeMms", NativeEncodeMms, EncodeMmsCallback);
    TELEPHONY_LOGI("napi_mms EncodeMms end");
    return result;
}

napi_value NapiMms::InitEnumMmsCharSets(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("BIG5", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::BIG5))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_10646_UCS_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_10646_UCS_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_1", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_1))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_3", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_3))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_4", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_4))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_5", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_5))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_6", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_6))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_7", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_7))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_8", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_8))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_9", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_9))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "SHIFT_JIS", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::SHIFT_JIS))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "US_ASCII", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::US_ASCII))),
        DECLARE_NAPI_STATIC_PROPERTY("UTF_8", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::UTF_8))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
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

napi_value NapiMms::InitSupportEnumMmsCharSets(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("BIG5", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::BIG5))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_10646_UCS_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_10646_UCS_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_1", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_1))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_3", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_3))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_4", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_4))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_5", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_5))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_6", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_6))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_7", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_7))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_8", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_8))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ISO_8859_9", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::ISO_8859_9))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "SHIFT_JIS", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::SHIFT_JIS))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "US_ASCII", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::US_ASCII))),
        DECLARE_NAPI_STATIC_PROPERTY("UTF_8", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsCharSets::UTF_8))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "MmsCharSets", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "MmsCharSets", result);
    return exports;
}

napi_value NapiMms::InitEnumMessageType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_SEND_REQ",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_SEND_REQ))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_SEND_CONF",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_SEND_CONF))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_NOTIFICATION_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_NOTIFICATION_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_RESP_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_NOTIFYRESP_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_RETRIEVE_CONF",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_RETRIEVE_CONF))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_ACKNOWLEDGE_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_ACKNOWLEDGE_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_DELIVERY_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_DELIVERY_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_READ_REC_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_READ_REC_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_READ_ORIG_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_READ_ORIG_IND))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value NapiMms::InitSupportEnumMessageType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_SEND_REQ",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_SEND_REQ))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_SEND_CONF",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_SEND_CONF))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_NOTIFICATION_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_NOTIFICATION_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_RESP_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_NOTIFYRESP_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_RETRIEVE_CONF",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_RETRIEVE_CONF))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_ACKNOWLEDGE_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_ACKNOWLEDGE_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_DELIVERY_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_DELIVERY_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_READ_REC_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_READ_REC_IND))),
        DECLARE_NAPI_STATIC_PROPERTY("TYPE_MMS_READ_ORIG_IND",
            NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsMsgType::MMS_MSGTYPE_READ_ORIG_IND))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "MessageType", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "MessageType", result);
    return exports;
}

napi_value NapiMms::InitEnumPriorityType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_LOW", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_LOW))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_NORMAL", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_NORMAL))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_HIGH", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_HIGH))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value NapiMms::InitSupportEnumPriorityType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_LOW", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_LOW))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_NORMAL", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_NORMAL))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_HIGH", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsPriority::MMS_HIGH))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "MmsPriorityType", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "MmsPriorityType", result);
    return exports;
}

napi_value NapiMms::InitEnumVersionType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_0", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_0))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_1", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_1))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_3", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_3))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value NapiMms::InitSupportEnumVersionType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_0", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_0))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_1", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_1))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_2", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_2))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_VERSION_1_3", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsVersionType::MMS_VERSION_1_3))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "MmsVersionType", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "MmsVersionType", result);
    return exports;
}

napi_value NapiMms::InitEnumDispositionType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "FROM_DATA", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::FROM_DATA))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ATTACHMENT", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::ATTACHMENT))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "INLINE", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::INLINE))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value NapiMms::InitSupportEnumDispositionType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "FROM_DATA", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::FROM_DATA))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "ATTACHMENT", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::ATTACHMENT))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "INLINE", NapiUtil::ToInt32Value(env, static_cast<int32_t>(DispositionValue::INLINE))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "DispositionType", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "DispositionType", result);
    return exports;
}

napi_value NapiMms::InitEnumReportAllowedType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_YES", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsBoolType::MMS_YES))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_NO", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsBoolType::MMS_NO))),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value NapiMms::InitSupportEnumReportAllowedType(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_YES", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsBoolType::MMS_YES))),
        DECLARE_NAPI_STATIC_PROPERTY(
            "MMS_NO", NapiUtil::ToInt32Value(env, static_cast<int32_t>(MmsBoolType::MMS_NO))),
    };
    napi_value result = nullptr;
    napi_define_class(env, "ReportType", NAPI_AUTO_LENGTH, CreateEnumConstructor, nullptr,
        sizeof(desc) / sizeof(desc[0]), desc, &result);
    napi_set_named_property(env, exports, "ReportType", result);
    return exports;
}
} // namespace Telephony
} // namespace OHOS
