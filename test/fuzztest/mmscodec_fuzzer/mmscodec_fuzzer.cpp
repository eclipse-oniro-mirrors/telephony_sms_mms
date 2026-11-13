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

#include "mmscodec_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <memory>
#include <string>
#include <vector>

#include "addsmstoken_fuzzer.h"
#include "mms_address.h"
#include "mms_attachment.h"
#include "mms_base64.h"
#include "mms_body.h"
#include "mms_body_part.h"
#include "mms_body_part_header.h"
#include "mms_buffer.h"
#include "mms_charset.h"
#include "mms_codec_type.h"
#include "mms_content_param.h"
#include "mms_content_type.h"
#include "mms_decode_buffer.h"
#include "mms_encode_buffer.h"
#include "mms_encode_string.h"
#include "mms_header.h"
#include "mms_msg.h"
#include "mms_quoted_printable.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr size_t MAX_BUFFER_SIZE = 1024;
constexpr size_t MAX_READ_LEN = 64;
constexpr uint8_t ASCII_PRINTABLE_BASE = 0x20;
constexpr uint8_t ASCII_PRINTABLE_RANGE = 0x5F;
constexpr uint32_t FIELD_COUNT = 64;
constexpr uint32_t MMS_MESSAGE_TYPE_COUNT = 24;
constexpr uint32_t CHARSET_MOD = 0x800;
constexpr uint8_t MMS_MESSAGE_TYPE_BASE = 128;
constexpr uint8_t WELL_KNOWN_CONTENT_TYPE_MOD = 0x55;
constexpr uint32_t GROUP_COUNT = 6;

std::string MakeString(FuzzedDataProvider &fdp)
{
    size_t len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_STRING_SIZE);
    std::vector<uint8_t> raw = fdp.ConsumeBytes<uint8_t>(len);
    std::string text;
    text.reserve(raw.size());
    for (uint8_t byte : raw) {
        text.push_back(static_cast<char>(ASCII_PRINTABLE_BASE + byte % ASCII_PRINTABLE_RANGE));
    }
    return text;
}

uint32_t MakeReadLen(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_READ_LEN);
}

std::unique_ptr<char[]> MakeBuffer(FuzzedDataProvider &fdp, uint32_t &len)
{
    size_t requestLen = fdp.ConsumeIntegralInRange<size_t>(0, MAX_BUFFER_SIZE);
    std::vector<uint8_t> raw = fdp.ConsumeBytes<uint8_t>(requestLen);
    len = static_cast<uint32_t>(raw.size());
    if (len == 0) {
        return nullptr;
    }
    auto buffer = std::make_unique<char[]>(len);
    for (uint32_t index = 0; index < len; ++index) {
        buffer[index] = static_cast<char>(raw[index]);
    }
    return buffer;
}

void WriteDecodeBuffer(MmsDecodeBuffer &decodeBuffer, FuzzedDataProvider &fdp)
{
    uint32_t len = 0;
    auto buffer = MakeBuffer(fdp, len);
    if (buffer != nullptr) {
        decodeBuffer.WriteDataBuffer(std::move(buffer), len);
    }
}

void FuzzMmsBuffer(FuzzedDataProvider &fdp)
{
    uint32_t len = 0;
    auto buffer = MakeBuffer(fdp, len);
    MmsBuffer mmsBuffer;
    if (buffer != nullptr) {
        mmsBuffer.WriteDataBuffer(std::move(buffer), len);
    }
    uint32_t readLen = MakeReadLen(fdp);
    mmsBuffer.ReadDataBuffer(readLen);
    mmsBuffer.ReadDataBuffer(MakeReadLen(fdp), readLen);
    mmsBuffer.GetCurPosition();
    mmsBuffer.GetSize();
    mmsBuffer.WriteBufferFromFile(MakeString(fdp));
}

void FuzzMmsDecodeBuffer(FuzzedDataProvider &fdp)
{
    MmsDecodeBuffer decodeBuffer;
    WriteDecodeBuffer(decodeBuffer, fdp);
    uint8_t oneByte = 0;
    uint8_t shortLen = 0;
    uint8_t shortInteger = 0;
    uint32_t valueLength = 0;
    uint32_t returnLength = 0;
    uint32_t uintVar = 0;
    uint32_t count = 0;
    uint64_t longInteger = 0;
    std::string text;
    decodeBuffer.PeekOneByte(oneByte);
    decodeBuffer.GetOneByte(oneByte);
    decodeBuffer.MarkPosition();
    decodeBuffer.DecodeIsString();
    decodeBuffer.DecodeShortLength(shortLen);
    decodeBuffer.DecodeValueLengthReturnLen(valueLength, returnLength);
    decodeBuffer.DecodeValueLength(valueLength);
    decodeBuffer.DecodeUintvar(uintVar, count);
    decodeBuffer.DecodeText(text, valueLength);
    MmsDecodeBuffer::CharIsToken(oneByte);
    decodeBuffer.DecodeTokenText(text, valueLength);
    decodeBuffer.DecodeQuotedText(text, valueLength);
    decodeBuffer.DecodeShortInteger(shortInteger);
    decodeBuffer.DecodeIsShortInt();
    decodeBuffer.DecodeLongInteger(longInteger);
    decodeBuffer.DecodeInteger(longInteger);
    decodeBuffer.DecodeIsValueLength();
    decodeBuffer.IncreasePointer(MakeReadLen(fdp));
    decodeBuffer.DecreasePointer(MakeReadLen(fdp));
    decodeBuffer.UnMarkPosition();
}

void FuzzMmsEncodeBuffer(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    uint32_t number = fdp.ConsumeIntegral<uint32_t>();
    uint64_t longNumber = fdp.ConsumeIntegral<uint64_t>();
    MmsEncodeBuffer encodeBuffer;
    encodeBuffer.WriteByte(fdp.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeUintvar(number);
    encodeBuffer.EncodeShortLength(fdp.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeValueLength(number);
    encodeBuffer.EncodeOctet(fdp.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeShortInteger(fdp.ConsumeIntegral<uint8_t>());
    encodeBuffer.EncodeInteger(fdp.ConsumeIntegral<int32_t>());
    encodeBuffer.EncodeLongInteger(longNumber);
    encodeBuffer.EncodeText(text);
    encodeBuffer.EncodeQuotedText(text);
    encodeBuffer.EncodeTokenText(text);

    uint32_t len = 0;
    auto buffer = MakeBuffer(fdp, len);
    if (buffer != nullptr) {
        encodeBuffer.WriteBuffer(std::move(buffer), len);
    }
    MmsEncodeBuffer copyBuffer;
    copyBuffer.WriteBuffer(encodeBuffer);
}

void FuzzContentParam(MmsContentParam &contentParam, FuzzedDataProvider &fdp, const std::string &text)
{
    uint8_t field = fdp.ConsumeIntegral<uint8_t>();
    contentParam.SetCharSet(fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD));
    contentParam.GetCharSet();
    contentParam.SetType(text);
    contentParam.GetType();
    contentParam.SetFileName(text);
    contentParam.SetStart(text);
    std::string outText;
    contentParam.GetFileName(outText);
    contentParam.GetStart(outText);
    contentParam.AddNormalField(field, text);
    contentParam.GetNormalField(field, outText);
    contentParam.GetParamMap();
}

void FuzzContentType(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    MmsContentParam contentParam;
    FuzzContentParam(contentParam, fdp, text);
    std::string outText;
    MmsContentType contentType;
    contentType.SetContentType(text);
    contentType.GetContentType(outText);
    contentType.SetContentParam(contentParam);
    contentType.GetContentParam();
    MmsContentType copyContentType(contentType);
    copyContentType = contentType;
    MmsContentType::GetContentTypeFromString(text);
    uint8_t wellKnownType = fdp.ConsumeIntegralInRange<uint8_t>(0, WELL_KNOWN_CONTENT_TYPE_MOD);
    MmsContentType::GetContentTypeFromInt(wellKnownType);

    int32_t contentLength = 0;
    uint32_t valueLength = MakeReadLen(fdp);
    MmsDecodeBuffer contentTypeBuffer;
    WriteDecodeBuffer(contentTypeBuffer, fdp);
    contentType.DecodeMmsContentType(contentTypeBuffer, contentLength);
    MmsDecodeBuffer generalFormBuffer;
    WriteDecodeBuffer(generalFormBuffer, fdp);
    contentType.DecodeMmsCTGeneralForm(generalFormBuffer, contentLength);
    MmsDecodeBuffer paramBuffer;
    WriteDecodeBuffer(paramBuffer, fdp);
    contentType.DecodeParameter(paramBuffer, valueLength);
    MmsEncodeBuffer encodeBuffer;
    contentType.EncodeTextField(encodeBuffer);
    contentType.EncodeCharsetField(encodeBuffer);
    contentType.EncodeTypeField(encodeBuffer);
    contentType.EncodeMmsBodyPartContentType(encodeBuffer);
    contentType.EncodeMmsBodyPartContentParam(encodeBuffer);
}

void FuzzAddressAndEncodeString(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    MmsCharSets charset = static_cast<MmsCharSets>(fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD));
    MmsAddress address(text, charset);
    address.GetAddressString();
    address.GetAddressCharset();
    address.GetAddressType();
    address.SetMmsAddressString(text, charset);

    MmsEncodeString encodeString;
    encodeString.SetEncodeString(fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD), text);
    encodeString.SetAddressString(address);
    encodeString.GetStrEncodeString();
    std::string outText;
    encodeString.GetEncodeString(outText);
    MmsEncodeString copyEncodeString(encodeString);
    MmsDecodeBuffer decodeBuffer;
    WriteDecodeBuffer(decodeBuffer, fdp);
    encodeString.DecodeEncodeString(decodeBuffer);
    MmsEncodeBuffer encodeBuffer;
    encodeString.EncodeEncodeString(encodeBuffer);
}

void FuzzMmsBodyPart(FuzzedDataProvider &fdp, const MmsAttachment &attachment, const std::string &text)
{
    MmsBodyPart bodyPart;
    MmsAttachment copyAttachment(attachment);
    bodyPart.SetAttachment(copyAttachment);
    bodyPart.SetSmilFile(fdp.ConsumeBool());
    bodyPart.SetContentType(text);
    bodyPart.SetContentId(text);
    bodyPart.SetContentLocation(text);
    bodyPart.SetContentDisposition(text);
    bodyPart.SetFileName(text);
    bodyPart.IsSmilFile();
    std::string outText;
    bodyPart.GetContentType(outText);
    bodyPart.GetContentId(outText);
    bodyPart.GetContentLocation(outText);
    bodyPart.GetContentDisposition(outText);
    bodyPart.GetPartFileName();
    uint32_t len = 0;
    bodyPart.ReadBodyPartBuffer(len);
    MmsBodyPart copyBodyPart(bodyPart);
    copyBodyPart = bodyPart;
    uint32_t headerLen = MakeReadLen(fdp);
    MmsDecodeBuffer partHeaderBuffer;
    WriteDecodeBuffer(partHeaderBuffer, fdp);
    bodyPart.DecodePartHeader(partHeaderBuffer, headerLen);
    MmsDecodeBuffer partBodyBuffer;
    WriteDecodeBuffer(partBodyBuffer, fdp);
    bodyPart.DecodePartBody(partBodyBuffer, headerLen);
    MmsDecodeBuffer partBuffer;
    WriteDecodeBuffer(partBuffer, fdp);
    bodyPart.DecodePart(partBuffer, headerLen);
    MmsEncodeBuffer encodeBuffer;
    bodyPart.EncodeMmsBodyPart(encodeBuffer);
}

void FuzzBodyPartHeader(FuzzedDataProvider &fdp, const std::string &text)
{
    MmsBodyPartHeader partHeader;
    partHeader.SetContentId(text);
    partHeader.SetContentLocation(text);
    partHeader.SetContentDisposition(text);
    partHeader.SetContentTransferEncoding(text);
    std::string outText;
    partHeader.GetContentId(outText);
    partHeader.GetContentLocation(outText);
    partHeader.GetContentDisposition(outText);
    partHeader.GetContentTransferEncoding(outText);
    uint32_t headerLen = MakeReadLen(fdp);
    MmsDecodeBuffer wellKnownBuffer;
    WriteDecodeBuffer(wellKnownBuffer, fdp);
    partHeader.DecodeWellKnownHeader(wellKnownBuffer, headerLen);
    MmsDecodeBuffer applicationBuffer;
    WriteDecodeBuffer(applicationBuffer, fdp);
    partHeader.DecodeApplicationHeader(applicationBuffer, headerLen);
    MmsDecodeBuffer contentLocationBuffer;
    WriteDecodeBuffer(contentLocationBuffer, fdp);
    partHeader.DecodeContentLocation(contentLocationBuffer, headerLen);
    MmsDecodeBuffer contentIdBuffer;
    WriteDecodeBuffer(contentIdBuffer, fdp);
    partHeader.DecodeContentId(contentIdBuffer, headerLen);
    MmsDecodeBuffer contentDispositionBuffer;
    WriteDecodeBuffer(contentDispositionBuffer, fdp);
    partHeader.DecodeContentDisposition(contentDispositionBuffer, headerLen);
    MmsEncodeBuffer encodeBuffer;
    partHeader.EncodeContentLocation(encodeBuffer);
    partHeader.EncodeContentId(encodeBuffer);
    partHeader.EncodeContentDisposition(encodeBuffer);
    partHeader.EncodeContentTransferEncoding(encodeBuffer);
    partHeader.EncodeMmsBodyPartHeader(encodeBuffer);
}

void FuzzAttachmentAndBodyPart(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    uint32_t len = 0;
    MmsAttachment attachment;
    attachment.SetAttachmentFilePath(text, fdp.ConsumeBool());
    attachment.SetContentId(text);
    attachment.SetContentLocation(text);
    attachment.SetContentDisposition(text);
    attachment.SetContentTransferEncoding(text);
    attachment.SetContentType(text);
    attachment.SetFileName(text);
    attachment.SetCharSet(fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD));
    attachment.SetIsSmilFile(fdp.ConsumeBool());
    auto dataBuffer = MakeBuffer(fdp, len);
    if (dataBuffer != nullptr) {
        attachment.SetDataBuffer(std::move(dataBuffer), len);
    }
    attachment.GetAttachmentFilePath();
    attachment.GetContentId();
    attachment.GetContentLocation();
    attachment.GetContentDisposition();
    attachment.GetContentTransferEncoding();
    attachment.GetContentType();
    attachment.GetFileName();
    attachment.GetCharSet();
    attachment.IsSmilFile();
    attachment.GetDataBuffer(len);
    MmsAttachment copyAttachment(attachment);

    FuzzBodyPartHeader(fdp, text);
    FuzzMmsBodyPart(fdp, copyAttachment, text);
}

void FuzzMmsBodyCodec(FuzzedDataProvider &fdp, MmsHeader &header, const std::string &text,
    MmsEncodeBuffer &encodeBuffer)
{
    MmsBody body;
    MmsBodyPart bodyPart;
    bodyPart.SetContentId(text);
    bodyPart.SetContentLocation(text);
    bodyPart.SetContentType(text);
    body.AddMmsBodyPart(bodyPart);
    body.GetBodyPartCount();
    body.IsBodyPartExist(bodyPart);
    std::vector<MmsBodyPart> bodyParts;
    body.GetMmsBodyPart(bodyParts);
    MmsDecodeBuffer multipartBuffer;
    WriteDecodeBuffer(multipartBuffer, fdp);
    body.DecodeMultipart(multipartBuffer);
    MmsDecodeBuffer bodyBuffer;
    WriteDecodeBuffer(bodyBuffer, fdp);
    body.DecodeMmsBody(bodyBuffer, header);
    body.EncodeMmsHeaderContentType(header, encodeBuffer);
    body.EncodeMmsBody(encodeBuffer);
}

void FuzzHeaderAndBody(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    uint8_t messageType = MMS_MESSAGE_TYPE_BASE + fdp.ConsumeIntegralInRange<uint8_t>(0, MMS_MESSAGE_TYPE_COUNT);
    uint8_t field = static_cast<uint8_t>((fdp.ConsumeIntegralInRange<uint32_t>(0, FIELD_COUNT) | 0x80));
    uint8_t value = fdp.ConsumeIntegral<uint8_t>();
    MmsHeader header;
    header.SetOctetValue(MMS_MESSAGE_TYPE, messageType);
    header.SetOctetValue(field, value);
    header.SetLongValue(field, fdp.ConsumeIntegral<int64_t>());
    header.SetTextValue(field, text);
    header.SetEncodeStringValue(field, fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD), text);
    MmsAddress address(text);
    header.AddHeaderAddressValue(field, address);
    header.GetHeaderContentType().SetContentType(text);
    header.GetHeaderContentType().GetContentParam().SetStart(text);
    uint8_t octetValue = 0;
    int64_t longValue = 0;
    std::string stringValue;
    MmsEncodeString encodeString;
    std::vector<MmsAddress> addresses;
    header.GetOctetValue(field, octetValue);
    header.GetLongValue(field, longValue);
    header.GetTextValue(field, stringValue);
    header.GetEncodeStringValue(field, encodeString);
    header.GetHeaderAllAddressValue(field, addresses);
    header.GetStringValue(field, stringValue);
    header.GetSmilFileName(stringValue);
    header.IsHaveBody();
    MmsDecodeBuffer headerBuffer;
    WriteDecodeBuffer(headerBuffer, fdp);
    header.DecodeMmsHeader(headerBuffer);
    MmsEncodeBuffer encodeBuffer;
    header.EncodeMmsHeader(encodeBuffer);

    FuzzMmsBodyCodec(fdp, header, text, encodeBuffer);
}

void FuzzMmsMsgGetAndCodec(FuzzedDataProvider &fdp, MmsMsg &msg, uint8_t field)
{
    std::vector<MmsAttachment> attachments;
    msg.GetAllAttachment(attachments);
    msg.GetMmsVersion();
    msg.GetMmsMessageType();
    msg.GetMmsTransactionId();
    msg.GetMmsDate();
    msg.GetMmsSubject();
    msg.GetMmsFrom();
    msg.GetHeaderOctetValue(field);
    msg.GetHeaderIntegerValue(field);
    msg.GetHeaderLongValue(field);
    msg.GetHeaderStringValue(field);
    msg.GetHeaderContentType();
    msg.GetHeaderContentTypeStart();
    uint32_t len = 0;
    auto buffer = MakeBuffer(fdp, len);
    if (buffer != nullptr) {
        msg.DecodeMsg(std::move(buffer), len);
    }
    msg.EncodeMsg(len);
}

void FuzzMmsMsg(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    MmsMsg msg;
    msg.SetMmsVersion(fdp.ConsumeIntegral<uint16_t>());
    msg.SetMmsMessageType(MMS_MESSAGE_TYPE_BASE + fdp.ConsumeIntegralInRange<uint8_t>(0, MMS_MESSAGE_TYPE_COUNT));
    msg.SetMmsTransactionId(text);
    msg.SetMmsDate(fdp.ConsumeIntegral<int64_t>());
    msg.SetMmsSubject(text);
    MmsAddress from(text);
    msg.SetMmsFrom(from);
    std::vector<MmsAddress> addresses;
    addresses.push_back(from);
    msg.SetMmsTo(addresses);
    msg.GetMmsTo(addresses);
    uint8_t field = static_cast<uint8_t>((fdp.ConsumeIntegralInRange<uint32_t>(0, FIELD_COUNT) | 0x80));
    msg.SetHeaderOctetValue(field, fdp.ConsumeIntegral<uint8_t>());
    msg.SetHeaderIntegerValue(field, fdp.ConsumeIntegral<int32_t>());
    msg.SetHeaderLongValue(field, fdp.ConsumeIntegral<int64_t>());
    msg.SetHeaderStringValue(field, text);
    msg.SetHeaderEncodedStringValue(field, text, fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD));
    msg.AddHeaderAddressValue(field, from);
    msg.GetHeaderAllAddressValue(field, addresses);
    msg.SetHeaderContentType(text);
    MmsAttachment attachment;
    attachment.SetContentId(text);
    attachment.SetContentLocation(text);
    attachment.SetContentType(text);
    uint32_t len = 0;
    auto buffer = MakeBuffer(fdp, len);
    if (buffer != nullptr) {
        attachment.SetDataBuffer(std::move(buffer), len);
    }
    msg.AddAttachment(attachment);
    FuzzMmsMsgGetAndCodec(fdp, msg, field);
}

void FuzzUtilityCodecs(FuzzedDataProvider &fdp)
{
    std::string text = MakeString(fdp);
    MmsBase64::Decode(text);
    std::string outText;
    MmsQuotedPrintable::Decode(text, outText);
    uint32_t charset = fdp.ConsumeIntegralInRange<uint32_t>(0, CHARSET_MOD);
    std::string charsetText;
    DelayedSingleton<MmsCharSet>::GetInstance()->GetCharSetStrFromInt(charsetText, charset);
    DelayedSingleton<MmsCharSet>::GetInstance()->GetCharSetIntFromString(charset, text);
}
} // namespace

void DoMmsCodecFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE_NUM) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    switch (fdp.ConsumeIntegralInRange<uint32_t>(0, GROUP_COUNT - 1)) {
        case 0:
            FuzzMmsBuffer(fdp);
            FuzzMmsDecodeBuffer(fdp);
            break;
        case 1:
            FuzzMmsEncodeBuffer(fdp);
            FuzzContentType(fdp);
            break;
        case 2:
            FuzzAddressAndEncodeString(fdp);
            FuzzAttachmentAndBodyPart(fdp);
            break;
        case 3:
            FuzzHeaderAndBody(fdp);
            break;
        case 4:
            FuzzMmsMsg(fdp);
            break;
        default:
            FuzzUtilityCodecs(fdp);
            break;
    }
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoMmsCodecFuzzTest(data, size);
    return 0;
}
