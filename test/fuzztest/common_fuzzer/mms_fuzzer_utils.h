/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MMS_FUZZER_UTILS_H
#define MMS_FUZZER_UTILS_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "fuzzer/FuzzedDataProvider.h"

#include "mms_address.h"
#include "mms_attachment.h"
#include "mms_body_part.h"
#include "mms_codec_type.h"
#include "mms_content_param.h"
#include "mms_content_type.h"
#include "mms_decode_buffer.h"
#include "mms_encode_buffer.h"
#include "mms_header.h"
#include "mms_msg.h"

namespace OHOS {
namespace Telephony {
namespace SmsMmsFuzz {
constexpr uint32_t MAX_TEXT_SIZE = 96;
constexpr uint32_t MAX_BINARY_SIZE = 1000;
constexpr uint32_t MAX_PART_COUNT = 4;
constexpr uint32_t SINGLE_PART_ENTRY_COUNT = 1;
constexpr uint32_t UNSPECIFIED_CHARSET = 0;
constexpr int FUZZER_SUCCESS = 0;
constexpr uint32_t SHORT_TEXT_SIZE = 32;
constexpr uint32_t MEDIUM_TEXT_SIZE = 48;
constexpr uint32_t MAX_HEADER_VALUE = 4096;
constexpr uint32_t MAX_CHARSET_VALUE = 0x1000;
constexpr uint32_t MAX_MMS_VERSION_VALUE = 0xff;
constexpr uint32_t MMS_VERSION_MIN_VALUE = 0x10;
constexpr uint32_t MMS_VERSION_MAX_VALUE = 0x13;
constexpr uint32_t ADDRESS_TYPE_COUNT = 5;
constexpr uint8_t SHORT_INTEGER_MARK = 0x80;
constexpr uint8_t SHORT_INTEGER_MASK = 0x7f;
constexpr uint8_t INVALID_FIELD_ID = 0xff;
constexpr size_t CONTENT_TYPE_COUNT = 8;
constexpr size_t DISPOSITION_COUNT = 4;
constexpr size_t TRANSFER_ENCODING_COUNT = 3;
constexpr size_t CHARSET_COUNT = 7;
constexpr size_t CONTENT_PARAM_TEXT_FIELD_COUNT = 8;
constexpr size_t MESSAGE_TYPE_COUNT = 9;
constexpr size_t FALLBACK_DATA_SIZE = 4;

constexpr std::array<const char *, CONTENT_TYPE_COUNT> CONTENT_TYPES = {
    "text/plain",
    "text/html",
    "image/jpeg",
    "image/png",
    "application/smil",
    "application/vnd.wap.multipart.mixed",
    "application/vnd.wap.multipart.related",
    "application/vnd.wap.mms-message",
};

constexpr std::array<const char *, DISPOSITION_COUNT> DISPOSITIONS = {
    "from-data",
    "attachment",
    "inline",
    "x-fuzz-disposition",
};

constexpr std::array<const char *, TRANSFER_ENCODING_COUNT> TRANSFER_ENCODINGS = {
    "binary",
    "quoted-printable",
    "x-fuzz-encoding",
};

constexpr std::array<uint32_t, CHARSET_COUNT> CHARSETS = {
    static_cast<uint32_t>(MmsCharSets::UTF_8),
    static_cast<uint32_t>(MmsCharSets::US_ASCII),
    static_cast<uint32_t>(MmsCharSets::ISO_8859_1),
    static_cast<uint32_t>(MmsCharSets::ISO_10646_UCS_2),
    static_cast<uint32_t>(MmsCharSets::SHIFT_JIS),
    static_cast<uint32_t>(MmsCharSets::BIG5),
    UNSPECIFIED_CHARSET,
};

constexpr std::array<const char *, ADDRESS_TYPE_COUNT> ADDRESS_TYPES = {
    "PLMN",
    "IPv4",
    "IPv6",
    "EMAIL",
    "UNKNOWN",
};

constexpr std::array<uint8_t, CONTENT_PARAM_TEXT_FIELD_COUNT> CONTENT_PARAM_TEXT_FIELDS = {
    static_cast<uint8_t>(ContentParam::CT_P_FILENAME),
    static_cast<uint8_t>(ContentParam::CT_P_FILENAME_VALUE),
    static_cast<uint8_t>(ContentParam::CT_P_START_VALUE),
    static_cast<uint8_t>(ContentParam::CT_P_START),
    static_cast<uint8_t>(ContentParam::CT_P_NAME),
    static_cast<uint8_t>(ContentParam::CT_P_NAME_VALUE),
    static_cast<uint8_t>(ContentParam::CT_P_START_INFO),
    static_cast<uint8_t>(ContentParam::CT_P_START_INFO_VALUE),
};

constexpr std::array<uint8_t, MESSAGE_TYPE_COUNT> MESSAGE_TYPES = {
    MMS_MSGTYPE_SEND_REQ,
    MMS_MSGTYPE_SEND_CONF,
    MMS_MSGTYPE_NOTIFICATION_IND,
    MMS_MSGTYPE_NOTIFYRESP_IND,
    MMS_MSGTYPE_RETRIEVE_CONF,
    MMS_MSGTYPE_ACKNOWLEDGE_IND,
    MMS_MSGTYPE_DELIVERY_IND,
    MMS_MSGTYPE_READ_REC_IND,
    MMS_MSGTYPE_READ_ORIG_IND,
};

constexpr std::array<uint8_t, FALLBACK_DATA_SIZE> FALLBACK_BYTES = {
    static_cast<uint8_t>('F'),
    static_cast<uint8_t>('U'),
    static_cast<uint8_t>('Z'),
    static_cast<uint8_t>('Z'),
};

inline uint32_t LastIndex(size_t size)
{
    return static_cast<uint32_t>(size - 1);
}

inline std::string PickContentType(FuzzedDataProvider &cursor)
{
    if (cursor.ConsumeBool()) {
        return CONTENT_TYPES[cursor.ConsumeIntegralInRange<uint32_t>(0, LastIndex(CONTENT_TYPES.size()))];
    }
    return cursor.ConsumeRandomLengthString(MAX_TEXT_SIZE);
}

inline std::string PickDisposition(FuzzedDataProvider &cursor)
{
    return DISPOSITIONS[cursor.ConsumeIntegralInRange<uint32_t>(0, LastIndex(DISPOSITIONS.size()))];
}

inline std::string PickTransferEncoding(FuzzedDataProvider &cursor)
{
    return TRANSFER_ENCODINGS[cursor.ConsumeIntegralInRange<uint32_t>(0,
        LastIndex(TRANSFER_ENCODINGS.size()))];
}

inline uint32_t PickCharset(FuzzedDataProvider &cursor)
{
    return CHARSETS[cursor.ConsumeIntegralInRange<uint32_t>(0, LastIndex(CHARSETS.size()))];
}

inline MmsAddress MakeAddress(FuzzedDataProvider &cursor)
{
    std::string localPart = cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE);
    uint32_t index = cursor.ConsumeIntegralInRange<uint32_t>(0, LastIndex(ADDRESS_TYPES.size()));
    std::string address = localPart + "/TYPE=" + ADDRESS_TYPES[index];
    MmsAddress result;
    result.SetMmsAddressString(address, static_cast<MmsCharSets>(PickCharset(cursor)));
    return result;
}

inline std::unique_ptr<char[]> MakeCharBuffer(const std::vector<uint8_t> &bytes)
{
    auto buffer = std::make_unique<char[]>(bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
        buffer[i] = static_cast<char>(bytes[i]);
    }
    return buffer;
}

inline std::vector<uint8_t> CopyRawBytes(const std::unique_ptr<char[]> &raw, uint32_t len)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(raw[i]));
    }
    return bytes;
}

inline std::unique_ptr<char[]> ReadRawEncodedBytes(MmsEncodeBuffer &encodeBuffer, uint32_t &len)
{
    len = encodeBuffer.GetCurPosition();
    if (len == 0) {
        return nullptr;
    }
    return encodeBuffer.ReadDataBuffer(0, len);
}

inline std::vector<uint8_t> ReadEncodedBytes(MmsEncodeBuffer &encodeBuffer)
{
    uint32_t len = 0;
    auto raw = ReadRawEncodedBytes(encodeBuffer, len);
    if (raw == nullptr) {
        return {};
    }
    return CopyRawBytes(raw, len);
}

inline void FillContentParamTextFields(MmsContentParam &param, FuzzedDataProvider &cursor)
{
    param.SetCharSet(PickCharset(cursor));
    param.SetType(PickContentType(cursor));
    param.SetFileName(cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
    param.SetStart(cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
}

inline void AddContentParamNormalFields(MmsContentParam &param, FuzzedDataProvider &cursor)
{
    uint32_t count = cursor.ConsumeIntegralInRange<uint32_t>(1, MAX_PART_COUNT);
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t field = CONTENT_PARAM_TEXT_FIELDS[cursor.ConsumeIntegralInRange<uint32_t>(0,
            LastIndex(CONTENT_PARAM_TEXT_FIELDS.size()))];
        param.AddNormalField(field, cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
    }
}

// MmsBuffer's default capacity is not the size of a received PDU.
class FuzzDecodeBuffer : public MmsDecodeBuffer {
public:
    FuzzDecodeBuffer()
    {
        totolLength_ = 0;
    }
};

inline std::unique_ptr<MmsDecodeBuffer> MakeDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    auto decodeBuffer = std::make_unique<FuzzDecodeBuffer>();
    if (!bytes.empty()) {
        decodeBuffer->WriteDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    }
    return decodeBuffer;
}

inline void FillAttachmentPathFields(MmsAttachment &attachment, const std::string &fileName, bool isSmil)
{
    attachment.SetAttachmentFilePath("/not_in_app_sandbox/" + fileName, isSmil);
    attachment.SetFileName(fileName);
    attachment.SetContentLocation(fileName);
}

inline void FillAttachmentContentFields(MmsAttachment &attachment, FuzzedDataProvider &cursor)
{
    attachment.SetContentId(cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
    attachment.SetContentDisposition(PickDisposition(cursor));
    attachment.SetContentTransferEncoding(PickTransferEncoding(cursor));
    attachment.SetContentType(PickContentType(cursor));
    attachment.SetCharSet(PickCharset(cursor));
}

inline void FillAttachmentData(MmsAttachment &attachment, FuzzedDataProvider &cursor)
{
    std::vector<uint8_t> bytes = cursor.ConsumeBytes<uint8_t>(MAX_BINARY_SIZE);
    attachment.SetDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
}

inline void FillContentParam(MmsContentParam &param, FuzzedDataProvider &cursor)
{
    FillContentParamTextFields(param, cursor);
    AddContentParamNormalFields(param, cursor);
}

inline MmsContentType MakeContentType(FuzzedDataProvider &cursor)
{
    MmsContentType contentType;
    MmsContentParam param;
    FillContentParam(param, cursor);
    contentType.SetContentType(PickContentType(cursor));
    contentType.SetContentParam(param);
    return contentType;
}

inline MmsAttachment MakeAttachment(FuzzedDataProvider &cursor, bool forceSmil = false)
{
    MmsAttachment attachment;
    std::string fileName = cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE);
    bool isSmil = forceSmil || cursor.ConsumeBool();
    FillAttachmentPathFields(attachment, fileName, isSmil);
    FillAttachmentContentFields(attachment, cursor);
    FillAttachmentData(attachment, cursor);
    return attachment;
}

inline void FillFallbackBodyPart(MmsBodyPart &bodyPart, bool forceSmil)
{
    bodyPart.SetFileName("fallback.dat");
    bodyPart.SetContentType("text/plain");
    bodyPart.SetContentId("<fallback>");
    bodyPart.SetContentLocation("fallback.dat");
    bodyPart.SetContentDisposition("attachment");
    bodyPart.SetSmilFile(forceSmil);
}

inline void FillFallbackAttachmentCodec(MmsAttachment &fallback, bool forceSmil)
{
    fallback.SetContentTransferEncoding("binary");
    fallback.SetContentType("text/plain");
    fallback.SetCharSet(static_cast<uint32_t>(MmsCharSets::UTF_8));
    fallback.SetIsSmilFile(forceSmil);
}

inline void FillFallbackAttachmentMetadata(MmsAttachment &fallback, bool forceSmil)
{
    fallback.SetFileName("fallback.dat");
    fallback.SetContentId("<fallback>");
    fallback.SetContentLocation("fallback.dat");
    fallback.SetContentDisposition("attachment");
    FillFallbackAttachmentCodec(fallback, forceSmil);
}

inline MmsAttachment MakeFallbackAttachment(bool forceSmil)
{
    MmsAttachment fallback;
    FillFallbackAttachmentMetadata(fallback, forceSmil);
    std::vector<uint8_t> bytes(FALLBACK_BYTES.begin(), FALLBACK_BYTES.end());
    fallback.SetDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    return fallback;
}

inline void AttachFallbackBodyPart(MmsBodyPart &bodyPart, bool forceSmil)
{
    MmsAttachment fallback = MakeFallbackAttachment(forceSmil);
    if (!bodyPart.SetAttachment(fallback)) {
        FillFallbackBodyPart(bodyPart, forceSmil);
    }
}

inline MmsBodyPart MakeBodyPart(FuzzedDataProvider &cursor, bool forceSmil = false)
{
    MmsBodyPart bodyPart;
    MmsAttachment attachment = MakeAttachment(cursor, forceSmil);
    if (!bodyPart.SetAttachment(attachment)) {
        AttachFallbackBodyPart(bodyPart, forceSmil);
    }
    return bodyPart;
}

inline uint8_t PickMessageType(FuzzedDataProvider &cursor)
{
    return MESSAGE_TYPES[cursor.ConsumeIntegralInRange<uint32_t>(0, LastIndex(MESSAGE_TYPES.size()))];
}

inline void AddMessageHeaderTextFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    header.SetTextValue(MMS_MESSAGE_ID, cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
    header.SetTextValue(MMS_CONTENT_LOCATION, cursor.ConsumeRandomLengthString(MEDIUM_TEXT_SIZE));
}

inline void AddMessageHeaderSizeFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    header.SetLongValue(MMS_MESSAGE_SIZE, cursor.ConsumeIntegralInRange<uint32_t>(1, MAX_HEADER_VALUE));
    header.SetLongValue(MMS_EXPIRY, cursor.ConsumeIntegralInRange<uint32_t>(1, MAX_HEADER_VALUE));
    header.GetHeaderContentType().SetContentType(cursor.ConsumeBool() ? "application/vnd.wap.multipart.related" :
        PickContentType(cursor));
}

inline void AddCommonHeaderFields(MmsHeader &header, FuzzedDataProvider &cursor, uint8_t messageType)
{
    header.SetOctetValue(MMS_MESSAGE_TYPE, messageType);
    header.SetLongValue(MMS_MMS_VERSION, cursor.ConsumeBool() ?
        cursor.ConsumeIntegralInRange<uint32_t>(MMS_VERSION_MIN_VALUE, MMS_VERSION_MAX_VALUE) :
        cursor.ConsumeIntegralInRange<uint32_t>(0, MAX_MMS_VERSION_VALUE));
    header.SetTextValue(MMS_TRANSACTION_ID, cursor.ConsumeRandomLengthString(SHORT_TEXT_SIZE));
    header.SetLongValue(MMS_DATE, static_cast<int64_t>(cursor.ConsumeIntegral<uint64_t>()));
    AddMessageHeaderTextFields(header, cursor);
    AddMessageHeaderSizeFields(header, cursor);
}

inline void AddAddressFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    MmsAddress from = MakeAddress(cursor);
    MmsAddress to = MakeAddress(cursor);
    MmsAddress cc = MakeAddress(cursor);
    header.AddHeaderAddressValue(MMS_FROM, from);
    header.AddHeaderAddressValue(MMS_TO, to);
    header.AddHeaderAddressValue(MMS_CC, cc);
}

inline void AddClassOctetFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    header.SetOctetValue(MMS_MESSAGE_CLASS, cursor.ConsumeBool() ?
        static_cast<uint8_t>(MmsMessageClass::PERSONAL) : cursor.ConsumeIntegral<uint8_t>());
    header.SetOctetValue(MMS_CONTENT_CLASS, cursor.ConsumeBool() ?
        static_cast<uint8_t>(MmsContentClass::MMS_TEXT) : cursor.ConsumeIntegral<uint8_t>());
}

inline void AddBoolOctetFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    header.SetOctetValue(MMS_DELIVERY_REPORT, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsBoolType::MMS_YES) :
        static_cast<uint8_t>(MmsBoolType::MMS_NO));
    header.SetOctetValue(MMS_REPORT_ALLOWED, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsBoolType::MMS_YES) :
        static_cast<uint8_t>(MmsBoolType::MMS_NO));
}

inline void AddOctetFields(MmsHeader &header, FuzzedDataProvider &cursor)
{
    header.SetOctetValue(MMS_RESPONSE_STATUS, cursor.ConsumeBool() ?
        static_cast<uint8_t>(MmsResponseStatus::MMS_OK) : cursor.ConsumeIntegral<uint8_t>());
    header.SetOctetValue(MMS_STATUS, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsStatus::MMS_RETRIEVED) :
        cursor.ConsumeIntegral<uint8_t>());
    header.SetOctetValue(MMS_READ_STATUS, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsReadStatus::MMS_READ) :
        cursor.ConsumeIntegral<uint8_t>());
    header.SetOctetValue(MMS_PRIORITY, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsPriority::MMS_NORMAL) :
        cursor.ConsumeIntegral<uint8_t>());
    AddClassOctetFields(header, cursor);
    AddBoolOctetFields(header, cursor);
}

inline void FillMmsHeader(MmsHeader &header, FuzzedDataProvider &cursor, uint8_t messageType)
{
    AddCommonHeaderFields(header, cursor, messageType);
    AddAddressFields(header, cursor);
    AddOctetFields(header, cursor);
}

inline void AddMsgAttachments(MmsMsg &msg, FuzzedDataProvider &cursor)
{
    MmsAttachment smil = MakeAttachment(cursor, true);
    MmsAttachment media = MakeAttachment(cursor, false);
    msg.AddAttachment(smil);
    msg.AddAttachment(media);
}

inline void FillMmsMsg(MmsMsg &msg, FuzzedDataProvider &cursor, uint8_t messageType)
{
    FillMmsHeader(msg.GetHeader(), cursor, messageType);
    msg.SetHeaderContentType(cursor.ConsumeBool() ? "application/vnd.wap.multipart.related" : PickContentType(cursor));
    if (messageType == MMS_MSGTYPE_SEND_REQ || messageType == MMS_MSGTYPE_RETRIEVE_CONF) {
        AddMsgAttachments(msg, cursor);
    }
}

} // namespace SmsMmsFuzz
} // namespace Telephony
} // namespace OHOS
#endif // MMS_FUZZER_UTILS_H
