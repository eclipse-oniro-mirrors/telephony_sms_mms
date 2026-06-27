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

#ifndef MMS_FUZZER_UTILS_H
#define MMS_FUZZER_UTILS_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
constexpr uint32_t MIN_BUFFER_SIZE = 1;
constexpr uint32_t MAX_TEXT_SIZE = 96;
constexpr uint32_t MAX_BINARY_SIZE = 192;
constexpr uint32_t MAX_PART_COUNT = 4;
constexpr uint32_t EMPTY_SIZE = 0;
constexpr int FUZZER_SUCCESS = 0;
constexpr uint32_t SHORT_TEXT_SIZE = 32;
constexpr uint32_t MEDIUM_TEXT_SIZE = 48;
constexpr uint32_t MAX_HEADER_VALUE = 4096;
constexpr uint32_t MAX_CHARSET_VALUE = 0x1000;
constexpr uint32_t MAX_MMS_VERSION_VALUE = 0xff;
constexpr uint32_t MMS_VERSION_MIN_VALUE = 0x10;
constexpr uint32_t MMS_VERSION_MAX_VALUE = 0x13;
constexpr uint32_t PRINTABLE_ASCII_RANGE = 95;
constexpr uint32_t ADDRESS_TYPE_COUNT = 5;
constexpr uint32_t UINT32_BYTE_COUNT = sizeof(uint32_t);
constexpr uint32_t UINT64_BYTE_COUNT = sizeof(uint64_t);
constexpr uint32_t BYTE_BITS = 8;
constexpr uint8_t BYTE_MASK = 0xffU;
constexpr uint8_t BOOL_LOW_BIT_MASK = 0x01;
constexpr uint8_t SHORT_INTEGER_MARK = 0x80;
constexpr uint8_t SHORT_INTEGER_MASK = 0x7f;
constexpr uint8_t INVALID_FIELD_ID = 0xff;
constexpr uint8_t ZERO_BYTE = 0x00;
constexpr char PRINTABLE_ASCII_FIRST = ' ';
constexpr char DEFAULT_BUFFER_FILL = 'F';
constexpr size_t CONTENT_TYPE_COUNT = 8;
constexpr size_t DISPOSITION_COUNT = 4;
constexpr size_t TRANSFER_ENCODING_COUNT = 3;
constexpr size_t CHARSET_COUNT = 7;
constexpr size_t CONTENT_PARAM_TEXT_FIELD_COUNT = 8;
constexpr size_t MESSAGE_TYPE_COUNT = 9;
constexpr size_t FALLBACK_DATA_SIZE = 4;

static_assert(FUZZER_SUCCESS == static_cast<int>(EMPTY_SIZE));
static_assert(MAX_CHARSET_VALUE > MAX_TEXT_SIZE);
static_assert(MAX_MMS_VERSION_VALUE > MMS_VERSION_MAX_VALUE);
static_assert(SHORT_INTEGER_MARK > SHORT_INTEGER_MASK);
static_assert(INVALID_FIELD_ID > SHORT_INTEGER_MARK);

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
    EMPTY_SIZE,
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

class DataCursor {
public:
    DataCursor(const uint8_t *data, size_t size) : data_(data), size_(size) {}

    uint8_t ConsumeByte()
    {
        if (data_ == nullptr || offset_ >= size_) {
            return static_cast<uint8_t>((size_ + offset_) & static_cast<size_t>(BYTE_MASK));
        }
        return data_[offset_++];
    }

    bool ConsumeBool()
    {
        return (ConsumeByte() & BOOL_LOW_BIT_MASK) != EMPTY_SIZE;
    }

    uint32_t ConsumeUint32(uint32_t minValue, uint32_t maxValue)
    {
        uint32_t value = ConsumeInteger<uint32_t>(UINT32_BYTE_COUNT);
        if (maxValue <= minValue) {
            return minValue;
        }
        return minValue + (value % (maxValue - minValue + MIN_BUFFER_SIZE));
    }

    uint64_t ConsumeUint64()
    {
        return ConsumeInteger<uint64_t>(UINT64_BYTE_COUNT);
    }

    std::string ConsumeText(uint32_t maxLen)
    {
        uint32_t len = ConsumeUint32(EMPTY_SIZE, maxLen);
        std::string result;
        result.reserve(len);
        for (uint32_t i = EMPTY_SIZE; i < len; ++i) {
            result.push_back(MakePrintable(ConsumeByte()));
        }
        return result;
    }

    std::vector<uint8_t> ConsumeBytes(uint32_t maxLen)
    {
        uint32_t len = ConsumeUint32(EMPTY_SIZE, maxLen);
        std::vector<uint8_t> result;
        result.reserve(len);
        for (uint32_t i = EMPTY_SIZE; i < len; ++i) {
            result.push_back(ConsumeByte());
        }
        return result;
    }

private:
    template<typename T>
    T ConsumeInteger(uint32_t byteCount)
    {
        T value = static_cast<T>(EMPTY_SIZE);
        for (uint32_t i = EMPTY_SIZE; i < byteCount; ++i) {
            value = static_cast<T>((value << BYTE_BITS) | ConsumeByte());
        }
        return value;
    }

    static char MakePrintable(uint8_t value)
    {
        return static_cast<char>(PRINTABLE_ASCII_FIRST + (value % PRINTABLE_ASCII_RANGE));
    }

    const uint8_t *data_ = nullptr;
    size_t size_ = static_cast<size_t>(EMPTY_SIZE);
    size_t offset_ = static_cast<size_t>(EMPTY_SIZE);
};

inline std::string NonEmpty(std::string value, const std::string &fallback)
{
    return value.empty() ? fallback : value;
}

inline uint32_t LastIndex(size_t size)
{
    return static_cast<uint32_t>(size - MIN_BUFFER_SIZE);
}

inline std::string PickContentType(DataCursor &cursor)
{
    if (cursor.ConsumeBool()) {
        return CONTENT_TYPES[cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(CONTENT_TYPES.size()))];
    }
    return NonEmpty(cursor.ConsumeText(MAX_TEXT_SIZE), "application/octet-stream");
}

inline std::string PickDisposition(DataCursor &cursor)
{
    return DISPOSITIONS[cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(DISPOSITIONS.size()))];
}

inline std::string PickTransferEncoding(DataCursor &cursor)
{
    return TRANSFER_ENCODINGS[cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(TRANSFER_ENCODINGS.size()))];
}

inline uint32_t PickCharset(DataCursor &cursor)
{
    return CHARSETS[cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(CHARSETS.size()))];
}

inline MmsAddress MakeAddress(DataCursor &cursor)
{
    std::string localPart = NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "+8613812345678");
    uint32_t index = cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(ADDRESS_TYPES.size()));
    std::string address = localPart + "/TYPE=" + ADDRESS_TYPES[index];
    MmsAddress result;
    result.SetMmsAddressString(address, static_cast<MmsCharSets>(PickCharset(cursor)));
    return result;
}

inline std::unique_ptr<char[]> MakeCharBuffer(const std::vector<uint8_t> &bytes)
{
    uint32_t len = std::max<uint32_t>(MIN_BUFFER_SIZE, static_cast<uint32_t>(bytes.size()));
    auto buffer = std::make_unique<char[]>(len);
    for (uint32_t i = EMPTY_SIZE; i < len; ++i) {
        uint8_t value = i < bytes.size() ? bytes[i] : static_cast<uint8_t>(DEFAULT_BUFFER_FILL);
        buffer[i] = static_cast<char>(value);
    }
    return buffer;
}

inline std::vector<uint8_t> CopyRawBytes(const std::unique_ptr<char[]> &raw, uint32_t len)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(len);
    for (uint32_t i = EMPTY_SIZE; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(raw[i]));
    }
    return bytes;
}

inline std::unique_ptr<char[]> ReadRawEncodedBytes(MmsEncodeBuffer &encodeBuffer, uint32_t &len)
{
    len = encodeBuffer.GetCurPosition();
    if (len == EMPTY_SIZE) {
        return nullptr;
    }
    return encodeBuffer.ReadDataBuffer(EMPTY_SIZE, len);
}

inline std::vector<uint8_t> ReadEncodedBytes(MmsEncodeBuffer &encodeBuffer)
{
    uint32_t len = EMPTY_SIZE;
    auto raw = ReadRawEncodedBytes(encodeBuffer, len);
    if (raw == nullptr) {
        return {};
    }
    return CopyRawBytes(raw, len);
}

static_assert(sizeof(&ReadEncodedBytes) >= MIN_BUFFER_SIZE);

inline void FillContentParamTextFields(MmsContentParam &param, DataCursor &cursor)
{
    param.SetCharSet(PickCharset(cursor));
    param.SetType(PickContentType(cursor));
    param.SetFileName(NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "part.txt"));
    param.SetStart(NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "<smil>"));
}

inline void AddContentParamNormalFields(MmsContentParam &param, DataCursor &cursor)
{
    uint32_t count = cursor.ConsumeUint32(MIN_BUFFER_SIZE, MAX_PART_COUNT);
    for (uint32_t i = EMPTY_SIZE; i < count; ++i) {
        uint8_t field = CONTENT_PARAM_TEXT_FIELDS[cursor.ConsumeUint32(EMPTY_SIZE,
            LastIndex(CONTENT_PARAM_TEXT_FIELDS.size()))];
        param.AddNormalField(field, NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "field"));
    }
}

inline std::unique_ptr<MmsDecodeBuffer> MakeDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    std::vector<uint8_t> payload = bytes;
    if (payload.empty()) {
        payload.push_back(ZERO_BYTE);
    }
    auto decodeBuffer = std::make_unique<MmsDecodeBuffer>();
    decodeBuffer->WriteDataBuffer(MakeCharBuffer(payload), static_cast<uint32_t>(payload.size()));
    return decodeBuffer;
}

static_assert(sizeof(&MakeDecodeBuffer) >= MIN_BUFFER_SIZE);

inline void FillAttachmentPathFields(MmsAttachment &attachment, const std::string &fileName, bool isSmil)
{
    attachment.SetAttachmentFilePath("/not_in_app_sandbox/" + fileName, isSmil);
    attachment.SetFileName(fileName);
    attachment.SetContentLocation(NonEmpty(fileName, "part.dat"));
}

inline void FillAttachmentContentFields(MmsAttachment &attachment, DataCursor &cursor)
{
    attachment.SetContentId(NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "part-1"));
    attachment.SetContentDisposition(PickDisposition(cursor));
    attachment.SetContentTransferEncoding(PickTransferEncoding(cursor));
    attachment.SetContentType(PickContentType(cursor));
    attachment.SetCharSet(PickCharset(cursor));
}

inline void FillAttachmentData(MmsAttachment &attachment, DataCursor &cursor)
{
    std::vector<uint8_t> bytes = cursor.ConsumeBytes(MAX_BINARY_SIZE);
    if (bytes.empty()) {
        bytes.push_back(static_cast<uint8_t>('A'));
    }
    attachment.SetDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
}

inline void FillContentParam(MmsContentParam &param, DataCursor &cursor)
{
    FillContentParamTextFields(param, cursor);
    AddContentParamNormalFields(param, cursor);
}

inline MmsContentType MakeContentType(DataCursor &cursor)
{
    MmsContentType contentType;
    MmsContentParam param;
    FillContentParam(param, cursor);
    contentType.SetContentType(PickContentType(cursor));
    contentType.SetContentParam(param);
    return contentType;
}

static_assert(sizeof(&MakeContentType) >= MIN_BUFFER_SIZE);

inline MmsAttachment MakeAttachment(DataCursor &cursor, bool forceSmil = false)
{
    MmsAttachment attachment;
    std::string fileName = NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), forceSmil ? "smil.xml" : "image.jpg");
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

inline void FillFallbackAttachmentCodec(MmsAttachment &fallback, bool forceSmil);

inline void FillFallbackAttachmentMetadata(MmsAttachment &fallback, bool forceSmil)
{
    fallback.SetFileName("fallback.dat");
    fallback.SetContentId("<fallback>");
    fallback.SetContentLocation("fallback.dat");
    fallback.SetContentDisposition("attachment");
    FillFallbackAttachmentCodec(fallback, forceSmil);
}

inline void FillFallbackAttachmentCodec(MmsAttachment &fallback, bool forceSmil)
{
    fallback.SetContentTransferEncoding("binary");
    fallback.SetContentType("text/plain");
    fallback.SetCharSet(static_cast<uint32_t>(MmsCharSets::UTF_8));
    fallback.SetIsSmilFile(forceSmil);
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

inline MmsBodyPart MakeBodyPart(DataCursor &cursor, bool forceSmil = false)
{
    MmsBodyPart bodyPart;
    MmsAttachment attachment = MakeAttachment(cursor, forceSmil);
    if (!bodyPart.SetAttachment(attachment)) {
        AttachFallbackBodyPart(bodyPart, forceSmil);
    }
    return bodyPart;
}

static_assert(sizeof(&MakeBodyPart) >= MIN_BUFFER_SIZE);

inline uint8_t PickMessageType(DataCursor &cursor)
{
    return MESSAGE_TYPES[cursor.ConsumeUint32(EMPTY_SIZE, LastIndex(MESSAGE_TYPES.size()))];
}

static_assert(sizeof(&PickMessageType) >= MIN_BUFFER_SIZE);

inline void AddMessageHeaderTextFields(MmsHeader &header, DataCursor &cursor);
inline void AddMessageHeaderSizeFields(MmsHeader &header, DataCursor &cursor);

inline void AddCommonHeaderFields(MmsHeader &header, DataCursor &cursor, uint8_t messageType)
{
    header.SetOctetValue(MMS_MESSAGE_TYPE, messageType);
    header.SetLongValue(MMS_MMS_VERSION, cursor.ConsumeUint32(MMS_VERSION_MIN_VALUE, MMS_VERSION_MAX_VALUE));
    header.SetTextValue(MMS_TRANSACTION_ID, NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "fuzz-transaction"));
    header.SetLongValue(MMS_DATE, static_cast<int64_t>(cursor.ConsumeUint64()));
    AddMessageHeaderTextFields(header, cursor);
    AddMessageHeaderSizeFields(header, cursor);
}

inline void AddMessageHeaderTextFields(MmsHeader &header, DataCursor &cursor)
{
    header.SetTextValue(MMS_MESSAGE_ID, NonEmpty(cursor.ConsumeText(SHORT_TEXT_SIZE), "fuzz-message"));
    header.SetTextValue(MMS_CONTENT_LOCATION, NonEmpty(cursor.ConsumeText(MEDIUM_TEXT_SIZE), "http://mmsc/fuzz"));
}

inline void AddMessageHeaderSizeFields(MmsHeader &header, DataCursor &cursor)
{
    header.SetLongValue(MMS_MESSAGE_SIZE, cursor.ConsumeUint32(MIN_BUFFER_SIZE, MAX_HEADER_VALUE));
    header.SetLongValue(MMS_EXPIRY, cursor.ConsumeUint32(MIN_BUFFER_SIZE, MAX_HEADER_VALUE));
    header.GetHeaderContentType().SetContentType("application/vnd.wap.multipart.related");
}

inline void AddAddressFields(MmsHeader &header, DataCursor &cursor)
{
    MmsAddress from = MakeAddress(cursor);
    MmsAddress to = MakeAddress(cursor);
    MmsAddress cc = MakeAddress(cursor);
    header.AddHeaderAddressValue(MMS_FROM, from);
    header.AddHeaderAddressValue(MMS_TO, to);
    header.AddHeaderAddressValue(MMS_CC, cc);
}

inline void AddClassOctetFields(MmsHeader &header);
inline void AddBoolOctetFields(MmsHeader &header, DataCursor &cursor);

inline void AddOctetFields(MmsHeader &header, DataCursor &cursor)
{
    header.SetOctetValue(MMS_RESPONSE_STATUS, static_cast<uint8_t>(MmsResponseStatus::MMS_OK));
    header.SetOctetValue(MMS_STATUS, static_cast<uint8_t>(MmsStatus::MMS_RETRIEVED));
    header.SetOctetValue(MMS_READ_STATUS, static_cast<uint8_t>(MmsReadStatus::MMS_READ));
    header.SetOctetValue(MMS_PRIORITY, static_cast<uint8_t>(MmsPriority::MMS_NORMAL));
    AddClassOctetFields(header);
    AddBoolOctetFields(header, cursor);
}

inline void AddClassOctetFields(MmsHeader &header)
{
    header.SetOctetValue(MMS_MESSAGE_CLASS, static_cast<uint8_t>(MmsMessageClass::PERSONAL));
    header.SetOctetValue(MMS_CONTENT_CLASS, static_cast<uint8_t>(MmsContentClass::MMS_TEXT));
}

inline void AddBoolOctetFields(MmsHeader &header, DataCursor &cursor)
{
    header.SetOctetValue(MMS_DELIVERY_REPORT, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsBoolType::MMS_YES) :
        static_cast<uint8_t>(MmsBoolType::MMS_NO));
    header.SetOctetValue(MMS_REPORT_ALLOWED, cursor.ConsumeBool() ? static_cast<uint8_t>(MmsBoolType::MMS_YES) :
        static_cast<uint8_t>(MmsBoolType::MMS_NO));
}

inline void FillMmsHeader(MmsHeader &header, DataCursor &cursor, uint8_t messageType)
{
    AddCommonHeaderFields(header, cursor, messageType);
    AddAddressFields(header, cursor);
    AddOctetFields(header, cursor);
}

inline void AddMsgAttachments(MmsMsg &msg, DataCursor &cursor)
{
    MmsAttachment smil = MakeAttachment(cursor, true);
    MmsAttachment media = MakeAttachment(cursor, false);
    msg.AddAttachment(smil);
    msg.AddAttachment(media);
}

inline void FillMmsMsg(MmsMsg &msg, DataCursor &cursor, uint8_t messageType)
{
    FillMmsHeader(msg.GetHeader(), cursor, messageType);
    msg.SetHeaderContentType("application/vnd.wap.multipart.related");
    if (messageType == MMS_MSGTYPE_SEND_REQ || messageType == MMS_MSGTYPE_RETRIEVE_CONF) {
        AddMsgAttachments(msg, cursor);
    }
}

static_assert(sizeof(&FillMmsMsg) >= MIN_BUFFER_SIZE);

inline std::vector<uint8_t> EncodeMmsMessageToBytes(MmsMsg &msg, uint32_t &len)
{
    auto encoded = msg.EncodeMsg(len);
    if (encoded == nullptr || len == EMPTY_SIZE) {
        return {};
    }
    return CopyRawBytes(encoded, len);
}

static_assert(sizeof(&EncodeMmsMessageToBytes) >= MIN_BUFFER_SIZE);
} // namespace SmsMmsFuzz
} // namespace Telephony
} // namespace OHOS
#endif // MMS_FUZZER_UTILS_H
