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

#include <algorithm>
#include <cstdint>
#include <cstring>
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
#include "securec.h"

using namespace OHOS::Telephony;
namespace OHOS {
namespace {
constexpr size_t MIN_SIZE_NUM = 1;
constexpr size_t MAX_STRING_SIZE = 256;
constexpr size_t MAX_BUFFER_SIZE = 1024;
constexpr size_t MAX_READ_LEN = 64;
constexpr uint32_t FIELD_COUNT = 64;
constexpr uint32_t MMS_MESSAGE_TYPE_COUNT = 24;
constexpr uint32_t CHARSET_MOD = 0x800;
constexpr uint8_t MMS_MESSAGE_TYPE_BASE = 128;
constexpr uint8_t WELL_KNOWN_CONTENT_TYPE_MOD = 0x55;
constexpr size_t BOOL_MOD = 2;

std::string MakeString(const uint8_t *data, size_t size)
{
    return std::string(reinterpret_cast<const char *>(data), std::min(size, MAX_STRING_SIZE));
}

uint32_t MakeUint32(const uint8_t *data, size_t size)
{
    uint32_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0 && memcpy_s(&value, sizeof(value), data, copyLen) != EOK) {
        return 0;
    }
    return value;
}

uint64_t MakeUint64(const uint8_t *data, size_t size)
{
    uint64_t value = 0;
    size_t copyLen = std::min(size, sizeof(value));
    if (copyLen > 0 && memcpy_s(&value, sizeof(value), data, copyLen) != EOK) {
        return 0;
    }
    return value;
}

std::unique_ptr<char[]> MakeBuffer(const uint8_t *data, size_t size, uint32_t &len)
{
    len = static_cast<uint32_t>(std::min(size, MAX_BUFFER_SIZE));
    if (len == 0) {
        return nullptr;
    }
    auto buffer = std::make_unique<char[]>(len);
    if (memcpy_s(buffer.get(), len, data, len) != EOK) {
        return nullptr;
    }
    return buffer;
}

MmsDecodeBuffer MakeDecodeBuffer(const uint8_t *data, size_t size)
{
    uint32_t len = 0;
    auto buffer = MakeBuffer(data, size, len);
    MmsDecodeBuffer decodeBuffer;
    if (buffer != nullptr) {
        decodeBuffer.WriteDataBuffer(std::move(buffer), len);
    }
    return decodeBuffer;
}

void FuzzMmsBuffer(const uint8_t *data, size_t size)
{
    uint32_t len = 0;
    auto buffer = MakeBuffer(data, size, len);
    MmsBuffer mmsBuffer;
    if (buffer != nullptr) {
        mmsBuffer.WriteDataBuffer(std::move(buffer), len);
    }
    uint32_t readLen = static_cast<uint32_t>(size % MAX_READ_LEN);
    mmsBuffer.ReadDataBuffer(readLen);
    mmsBuffer.ReadDataBuffer(static_cast<uint32_t>(size % (MAX_READ_LEN + MIN_SIZE_NUM)), readLen);
    mmsBuffer.GetCurPosition();
    mmsBuffer.GetSize();
    std::string path = MakeString(data, std::min(size, MAX_READ_LEN));
    mmsBuffer.WriteBufferFromFile(path);
}

void FuzzMmsDecodeBuffer(const uint8_t *data, size_t size)
{
    MmsDecodeBuffer decodeBuffer = MakeDecodeBuffer(data, size);
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
    decodeBuffer.IncreasePointer(static_cast<uint32_t>(size % MAX_READ_LEN));
    decodeBuffer.DecreasePointer(static_cast<uint32_t>(size % MAX_READ_LEN));
    decodeBuffer.UnMarkPosition();
}

void FuzzMmsEncodeBuffer(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    uint32_t number = MakeUint32(data, size);
    uint64_t longNumber = MakeUint64(data, size);
    MmsEncodeBuffer encodeBuffer;
    encodeBuffer.WriteByte(static_cast<uint8_t>(number));
    encodeBuffer.EncodeUintvar(number);
    encodeBuffer.EncodeShortLength(static_cast<uint8_t>(number));
    encodeBuffer.EncodeValueLength(number);
    encodeBuffer.EncodeOctet(static_cast<uint8_t>(number));
    encodeBuffer.EncodeShortInteger(static_cast<uint8_t>(number));
    encodeBuffer.EncodeInteger(number);
    encodeBuffer.EncodeLongInteger(longNumber);
    encodeBuffer.EncodeText(text);
    encodeBuffer.EncodeQuotedText(text);
    encodeBuffer.EncodeTokenText(text);

    uint32_t len = 0;
    auto buffer = MakeBuffer(data, size, len);
    if (buffer != nullptr) {
        encodeBuffer.WriteBuffer(std::move(buffer), len);
    }
    MmsEncodeBuffer copyBuffer;
    copyBuffer.WriteBuffer(encodeBuffer);
}

void FuzzContentParamAndType(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    uint8_t field = static_cast<uint8_t>(MakeUint32(data, size));
    MmsContentParam contentParam;
    contentParam.SetCharSet(MakeUint32(data, size) % CHARSET_MOD);
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

    MmsContentType contentType;
    contentType.SetContentType(text);
    contentType.GetContentType(outText);
    contentType.SetContentParam(contentParam);
    contentType.GetContentParam();
    MmsContentType copyContentType(contentType);
    copyContentType = contentType;
    MmsContentType::GetContentTypeFromString(text);
    MmsContentType::GetContentTypeFromInt(static_cast<uint8_t>(field % WELL_KNOWN_CONTENT_TYPE_MOD));

    int32_t contentLength = 0;
    int32_t valueLength = static_cast<int32_t>(size % MAX_READ_LEN);
    MmsDecodeBuffer contentTypeBuffer = MakeDecodeBuffer(data, size);
    contentType.DecodeMmsContentType(contentTypeBuffer, contentLength);
    MmsDecodeBuffer generalFormBuffer = MakeDecodeBuffer(data, size);
    contentType.DecodeMmsCTGeneralForm(generalFormBuffer, contentLength);
    MmsDecodeBuffer paramBuffer = MakeDecodeBuffer(data, size);
    contentType.DecodeParameter(paramBuffer, valueLength);
    MmsEncodeBuffer encodeBuffer;
    contentType.EncodeTextField(encodeBuffer);
    contentType.EncodeCharsetField(encodeBuffer);
    contentType.EncodeTypeField(encodeBuffer);
    contentType.EncodeMmsBodyPartContentType(encodeBuffer);
    contentType.EncodeMmsBodyPartContentParam(encodeBuffer);
}

void FuzzAddressAndEncodeString(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    MmsCharSets charset = static_cast<MmsCharSets>(MakeUint32(data, size) % CHARSET_MOD);
    MmsAddress address(text, charset);
    address.GetAddressString();
    address.GetAddressCharset();
    address.GetAddressType();
    address.SetMmsAddressString(text, charset);

    MmsEncodeString encodeString;
    encodeString.SetEncodeString(static_cast<uint32_t>(charset), text);
    encodeString.SetAddressString(address);
    encodeString.GetStrEncodeString();
    std::string outText;
    encodeString.GetEncodeString(outText);
    MmsEncodeString copyEncodeString(encodeString);
    MmsDecodeBuffer decodeBuffer = MakeDecodeBuffer(data, size);
    encodeString.DecodeEncodeString(decodeBuffer);
    MmsEncodeBuffer encodeBuffer;
    encodeString.EncodeEncodeString(encodeBuffer);
}

void FuzzBodyPartAndHeader(const uint8_t *data, size_t size, const MmsAttachment &attachment,
    const std::string &text)
{
    uint32_t len = 0;
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
    MmsDecodeBuffer decodeBuffer = MakeDecodeBuffer(data, size);
    uint32_t headerLen = static_cast<uint32_t>(size % MAX_READ_LEN);
    partHeader.DecodeWellKnownHeader(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    partHeader.DecodeApplicationHeader(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    partHeader.DecodeContentLocation(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    partHeader.DecodeContentId(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    partHeader.DecodeContentDisposition(decodeBuffer, headerLen);
    MmsEncodeBuffer encodeBuffer;
    partHeader.EncodeContentLocation(encodeBuffer);
    partHeader.EncodeContentId(encodeBuffer);
    partHeader.EncodeContentDisposition(encodeBuffer);
    partHeader.EncodeContentTransferEncoding(encodeBuffer);
    partHeader.EncodeMmsBodyPartHeader(encodeBuffer);

    MmsBodyPart bodyPart;
    bodyPart.SetAttachment(attachment);
    bodyPart.SetSmilFile(size % BOOL_MOD == 0);
    bodyPart.SetContentType(text);
    bodyPart.SetContentId(text);
    bodyPart.SetContentLocation(text);
    bodyPart.SetContentDisposition(text);
    bodyPart.SetFileName(text);
    bodyPart.IsSmilFile();
    bodyPart.GetContentType(outText);
    bodyPart.GetContentId(outText);
    bodyPart.GetContentLocation(outText);
    bodyPart.GetContentDisposition(outText);
    bodyPart.GetPartFileName();
    bodyPart.ReadBodyPartBuffer(len);
    MmsBodyPart copyBodyPart(bodyPart);
    copyBodyPart = bodyPart;
    decodeBuffer = MakeDecodeBuffer(data, size);
    bodyPart.DecodePartHeader(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    bodyPart.DecodePartBody(decodeBuffer, headerLen);
    decodeBuffer = MakeDecodeBuffer(data, size);
    bodyPart.DecodePart(decodeBuffer, headerLen);
    bodyPart.EncodeMmsBodyPart(encodeBuffer);
}

void FuzzAttachmentAndBodyPart(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    uint32_t len = 0;
    MmsAttachment attachment;
    attachment.SetAttachmentFilePath(text, size % BOOL_MOD == 0);
    attachment.SetContentId(text);
    attachment.SetContentLocation(text);
    attachment.SetContentDisposition(text);
    attachment.SetContentTransferEncoding(text);
    attachment.SetContentType(text);
    attachment.SetFileName(text);
    attachment.SetCharSet(MakeUint32(data, size) % CHARSET_MOD);
    attachment.SetIsSmilFile(size % BOOL_MOD == 1);
    auto dataBuffer = MakeBuffer(data, size, len);
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

    FuzzBodyPartAndHeader(data, size, attachment, text);
}

void FuzzHeaderAndBody(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    uint8_t field = static_cast<uint8_t>((MakeUint32(data, size) % FIELD_COUNT) | 0x80);
    uint8_t value = static_cast<uint8_t>(MakeUint32(data, size));
    MmsHeader header;
    header.SetOctetValue(MMS_MESSAGE_TYPE, MMS_MESSAGE_TYPE_BASE + (value % MMS_MESSAGE_TYPE_COUNT));
    header.SetOctetValue(field, value);
    header.SetLongValue(field, static_cast<int64_t>(MakeUint64(data, size)));
    header.SetTextValue(field, text);
    header.SetEncodeStringValue(field, MakeUint32(data, size) % CHARSET_MOD, text);
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
    MmsDecodeBuffer decodeBuffer = MakeDecodeBuffer(data, size);
    header.DecodeMmsHeader(decodeBuffer);
    MmsEncodeBuffer encodeBuffer;
    header.EncodeMmsHeader(encodeBuffer);

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
    decodeBuffer = MakeDecodeBuffer(data, size);
    body.DecodeMultipart(decodeBuffer);
    decodeBuffer = MakeDecodeBuffer(data, size);
    body.DecodeMmsBody(decodeBuffer, header);
    body.EncodeMmsHeaderContentType(header, encodeBuffer);
    body.EncodeMmsBody(encodeBuffer);
}

void FuzzMmsMsgGetAndCodec(MmsMsg &msg, const uint8_t *data, size_t size, uint8_t field)
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
    auto buffer = MakeBuffer(data, size, len);
    if (buffer != nullptr) {
        msg.DecodeMsg(std::move(buffer), len);
    }
    msg.EncodeMsg(len);
}

void FuzzMmsMsg(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    MmsMsg msg;
    msg.SetMmsVersion(static_cast<uint16_t>(MakeUint32(data, size)));
    msg.SetMmsMessageType(MMS_MESSAGE_TYPE_BASE + (static_cast<uint8_t>(size) % MMS_MESSAGE_TYPE_COUNT));
    msg.SetMmsTransactionId(text);
    msg.SetMmsDate(static_cast<int64_t>(MakeUint64(data, size)));
    msg.SetMmsSubject(text);
    MmsAddress from(text);
    msg.SetMmsFrom(from);
    std::vector<MmsAddress> addresses;
    addresses.push_back(from);
    msg.SetMmsTo(addresses);
    msg.GetMmsTo(addresses);
    uint8_t field = static_cast<uint8_t>((MakeUint32(data, size) % FIELD_COUNT) | 0x80);
    msg.SetHeaderOctetValue(field, static_cast<uint8_t>(size));
    msg.SetHeaderIntegerValue(field, static_cast<int32_t>(MakeUint32(data, size)));
    msg.SetHeaderLongValue(field, static_cast<int64_t>(MakeUint64(data, size)));
    msg.SetHeaderStringValue(field, text);
    msg.SetHeaderEncodedStringValue(field, text, MakeUint32(data, size) % CHARSET_MOD);
    msg.AddHeaderAddressValue(field, from);
    msg.GetHeaderAllAddressValue(field, addresses);
    msg.SetHeaderContentType(text);
    MmsAttachment attachment;
    attachment.SetContentId(text);
    attachment.SetContentLocation(text);
    attachment.SetContentType(text);
    uint32_t len = 0;
    auto buffer = MakeBuffer(data, size, len);
    if (buffer != nullptr) {
        attachment.SetDataBuffer(std::move(buffer), len);
    }
    msg.AddAttachment(attachment);
    FuzzMmsMsgGetAndCodec(msg, data, size, field);
}

void FuzzUtilityCodecs(const uint8_t *data, size_t size)
{
    std::string text = MakeString(data, std::min(size, MAX_BUFFER_SIZE));
    std::string encoded = MmsBase64::Encode(text);
    MmsBase64::Decode(encoded);
    MmsBase64::Decode(text);
    MmsQuotedPrintable::Encode(text);
    std::string outText;
    MmsQuotedPrintable::Decode(text, outText);
    MmsQuotedPrintable::Decode(MmsQuotedPrintable::Encode(text), outText);
    uint32_t charset = MakeUint32(data, size) % CHARSET_MOD;
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
    FuzzMmsBuffer(data, size);
    FuzzMmsDecodeBuffer(data, size);
    FuzzMmsEncodeBuffer(data, size);
    FuzzContentParamAndType(data, size);
    FuzzAddressAndEncodeString(data, size);
    FuzzAttachmentAndBodyPart(data, size);
    FuzzHeaderAndBody(data, size);
    FuzzMmsMsg(data, size);
    FuzzUtilityCodecs(data, size);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoMmsCodecFuzzTest(data, size);
    return 0;
}
