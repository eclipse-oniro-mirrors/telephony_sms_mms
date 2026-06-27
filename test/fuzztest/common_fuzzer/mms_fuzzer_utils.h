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

class DataCursor {
public:
    DataCursor(const uint8_t *data, size_t size) : data_(data), size_(size) {}

    uint8_t ConsumeByte()
    {
        if (data_ == nullptr || offset_ >= size_) {
            return static_cast<uint8_t>((size_ + offset_) & 0xffU);
        }
        return data_[offset_++];
    }

    bool ConsumeBool()
    {
        return (ConsumeByte() & 1U) != 0;
    }

    uint32_t ConsumeUint32(uint32_t minValue, uint32_t maxValue)
    {
        uint32_t value = 0;
        for (uint32_t i = 0; i < sizeof(uint32_t); ++i) {
            value = (value << BYTE_BITS) | ConsumeByte();
        }
        if (maxValue <= minValue) {
            return minValue;
        }
        return minValue + (value % (maxValue - minValue + 1));
    }

    uint64_t ConsumeUint64()
    {
        uint64_t value = 0;
        for (uint32_t i = 0; i < sizeof(uint64_t); ++i) {
            value = (value << BYTE_BITS) | ConsumeByte();
        }
        return value;
    }

    std::string ConsumeText(uint32_t maxLen)
    {
        uint32_t len = ConsumeUint32(0, maxLen);
        std::string result;
        result.reserve(len);
        for (uint32_t i = 0; i < len; ++i) {
            result.push_back(static_cast<char>(' ' + (ConsumeByte() % 95)));
        }
        return result;
    }

    std::vector<uint8_t> ConsumeBytes(uint32_t maxLen)
    {
        uint32_t len = ConsumeUint32(0, maxLen);
        std::vector<uint8_t> result;
        result.reserve(len);
        for (uint32_t i = 0; i < len; ++i) {
            result.push_back(ConsumeByte());
        }
        return result;
    }

    template<typename T, size_t N>
    T Pick(const std::array<T, N> &values)
    {
        return values[ConsumeUint32(0, static_cast<uint32_t>(N - 1))];
    }

private:
    static constexpr uint32_t BYTE_BITS = 8;
    const uint8_t *data_ = nullptr;
    size_t size_ = 0;
    size_t offset_ = 0;
};

inline std::string NonEmpty(std::string value, const std::string &fallback)
{
    return value.empty() ? fallback : value;
}

inline std::string PickContentType(DataCursor &cursor)
{
    static constexpr std::array<const char *, 8> CONTENT_TYPES = {
        "text/plain",
        "text/html",
        "image/jpeg",
        "image/png",
        "application/smil",
        "application/vnd.wap.multipart.mixed",
        "application/vnd.wap.multipart.related",
        "application/vnd.wap.mms-message",
    };
    if (cursor.ConsumeBool()) {
        return CONTENT_TYPES[cursor.ConsumeUint32(0, static_cast<uint32_t>(CONTENT_TYPES.size() - 1))];
    }
    return NonEmpty(cursor.ConsumeText(MAX_TEXT_SIZE), "application/octet-stream");
}

inline std::string PickDisposition(DataCursor &cursor)
{
    static constexpr std::array<const char *, 4> DISPOSITIONS = {
        "from-data",
        "attachment",
        "inline",
        "x-fuzz-disposition",
    };
    return DISPOSITIONS[cursor.ConsumeUint32(0, static_cast<uint32_t>(DISPOSITIONS.size() - 1))];
}

inline std::string PickTransferEncoding(DataCursor &cursor)
{
    static constexpr std::array<const char *, 4> ENCODINGS = {
        "binary",
        "base64",
        "quoted-printable",
        "x-fuzz-encoding",
    };
    return ENCODINGS[cursor.ConsumeUint32(0, static_cast<uint32_t>(ENCODINGS.size() - 1))];
}

inline uint32_t PickCharset(DataCursor &cursor)
{
    static constexpr std::array<uint32_t, 7> CHARSETS = {
        static_cast<uint32_t>(MmsCharSets::UTF_8),
        static_cast<uint32_t>(MmsCharSets::US_ASCII),
        static_cast<uint32_t>(MmsCharSets::ISO_8859_1),
        static_cast<uint32_t>(MmsCharSets::ISO_10646_UCS_2),
        static_cast<uint32_t>(MmsCharSets::SHIFT_JIS),
        static_cast<uint32_t>(MmsCharSets::BIG5),
        0,
    };
    return CHARSETS[cursor.ConsumeUint32(0, static_cast<uint32_t>(CHARSETS.size() - 1))];
}

inline MmsAddress MakeAddress(DataCursor &cursor)
{
    static constexpr std::array<const char *, 5> TYPES = {
        "PLMN",
        "IPv4",
        "IPv6",
        "EMAIL",
        "UNKNOWN",
    };
    std::string localPart = NonEmpty(cursor.ConsumeText(32), "+8613812345678");
    std::string address = localPart + "/TYPE=" + TYPES[cursor.ConsumeUint32(0, static_cast<uint32_t>(
        TYPES.size() - 1))];
    MmsAddress result;
    result.SetMmsAddressString(address, static_cast<MmsCharSets>(PickCharset(cursor)));
    return result;
}

inline std::unique_ptr<char[]> MakeCharBuffer(const std::vector<uint8_t> &bytes)
{
    uint32_t len = std::max<uint32_t>(MIN_BUFFER_SIZE, static_cast<uint32_t>(bytes.size()));
    auto buffer = std::make_unique<char[]>(len);
    for (uint32_t i = 0; i < len; ++i) {
        uint8_t value = i < bytes.size() ? bytes[i] : static_cast<uint8_t>('F');
        buffer[i] = static_cast<char>(value);
    }
    return buffer;
}

inline std::vector<uint8_t> ReadEncodedBytes(MmsEncodeBuffer &encodeBuffer)
{
    uint32_t len = encodeBuffer.GetCurPosition();
    std::vector<uint8_t> bytes;
    if (len == 0) {
        return bytes;
    }
    auto raw = encodeBuffer.ReadDataBuffer(0, len);
    if (raw == nullptr) {
        return bytes;
    }
    bytes.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(raw[i]));
    }
    return bytes;
}

inline std::unique_ptr<MmsDecodeBuffer> MakeDecodeBuffer(const std::vector<uint8_t> &bytes)
{
    std::vector<uint8_t> payload = bytes;
    if (payload.empty()) {
        payload.push_back(0);
    }
    auto decodeBuffer = std::make_unique<MmsDecodeBuffer>();
    decodeBuffer->WriteDataBuffer(MakeCharBuffer(payload), static_cast<uint32_t>(payload.size()));
    return decodeBuffer;
}

inline void FillContentParam(MmsContentParam &param, DataCursor &cursor)
{
    static constexpr std::array<uint8_t, 8> TEXT_FIELDS = {
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME),
        static_cast<uint8_t>(ContentParam::CT_P_FILENAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_START_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_START),
        static_cast<uint8_t>(ContentParam::CT_P_NAME),
        static_cast<uint8_t>(ContentParam::CT_P_NAME_VALUE),
        static_cast<uint8_t>(ContentParam::CT_P_START_INFO),
        static_cast<uint8_t>(ContentParam::CT_P_START_INFO_VALUE),
    };
    param.SetCharSet(PickCharset(cursor));
    param.SetType(PickContentType(cursor));
    param.SetFileName(NonEmpty(cursor.ConsumeText(32), "part.txt"));
    param.SetStart(NonEmpty(cursor.ConsumeText(32), "<smil>"));
    uint32_t count = cursor.ConsumeUint32(1, 4);
    for (uint32_t i = 0; i < count; ++i) {
        uint8_t field = TEXT_FIELDS[cursor.ConsumeUint32(0, static_cast<uint32_t>(TEXT_FIELDS.size() - 1))];
        param.AddNormalField(field, NonEmpty(cursor.ConsumeText(32), "field"));
    }
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

inline MmsAttachment MakeAttachment(DataCursor &cursor, bool forceSmil = false)
{
    MmsAttachment attachment;
    std::string fileName = NonEmpty(cursor.ConsumeText(32), forceSmil ? "smil.xml" : "image.jpg");
    bool isSmil = forceSmil || cursor.ConsumeBool();
    attachment.SetAttachmentFilePath("/not_in_app_sandbox/" + fileName, isSmil);
    attachment.SetFileName(fileName);
    attachment.SetContentId(NonEmpty(cursor.ConsumeText(32), "part-1"));
    attachment.SetContentLocation(NonEmpty(cursor.ConsumeText(32), fileName));
    attachment.SetContentDisposition(PickDisposition(cursor));
    attachment.SetContentTransferEncoding(PickTransferEncoding(cursor));
    attachment.SetContentType(PickContentType(cursor));
    attachment.SetCharSet(PickCharset(cursor));
    std::vector<uint8_t> bytes = cursor.ConsumeBytes(MAX_BINARY_SIZE);
    if (bytes.empty()) {
        bytes.push_back(static_cast<uint8_t>('A'));
    }
    attachment.SetDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
    return attachment;
}

inline MmsBodyPart MakeBodyPart(DataCursor &cursor, bool forceSmil = false)
{
    MmsAttachment attachment = MakeAttachment(cursor, forceSmil);
    MmsBodyPart bodyPart;
    if (!bodyPart.SetAttachment(attachment)) {
        MmsAttachment fallback;
        fallback.SetFileName("fallback.dat");
        fallback.SetContentId("<fallback>");
        fallback.SetContentLocation("fallback.dat");
        fallback.SetContentDisposition("attachment");
        fallback.SetContentTransferEncoding("binary");
        fallback.SetContentType("text/plain");
        fallback.SetCharSet(static_cast<uint32_t>(MmsCharSets::UTF_8));
        fallback.SetIsSmilFile(forceSmil);
        std::vector<uint8_t> bytes = { static_cast<uint8_t>('F'), static_cast<uint8_t>('U'),
            static_cast<uint8_t>('Z'), static_cast<uint8_t>('Z') };
        fallback.SetDataBuffer(MakeCharBuffer(bytes), static_cast<uint32_t>(bytes.size()));
        if (!bodyPart.SetAttachment(fallback)) {
            bodyPart.SetFileName("fallback.dat");
            bodyPart.SetContentType("text/plain");
            bodyPart.SetContentId("<fallback>");
            bodyPart.SetContentLocation("fallback.dat");
            bodyPart.SetContentDisposition("attachment");
            bodyPart.SetSmilFile(forceSmil);
        }
    }
    return bodyPart;
}

inline uint8_t PickMessageType(DataCursor &cursor)
{
    static constexpr std::array<uint8_t, 9> MESSAGE_TYPES = {
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
    return MESSAGE_TYPES[cursor.ConsumeUint32(0, static_cast<uint32_t>(MESSAGE_TYPES.size() - 1))];
}

inline void AddCommonHeaderFields(MmsHeader &header, DataCursor &cursor, uint8_t messageType)
{
    header.SetOctetValue(MMS_MESSAGE_TYPE, messageType);
    header.SetLongValue(MMS_MMS_VERSION, cursor.ConsumeUint32(0x10, 0x13));
    header.SetTextValue(MMS_TRANSACTION_ID, NonEmpty(cursor.ConsumeText(32), "fuzz-transaction"));
    header.SetLongValue(MMS_DATE, static_cast<int64_t>(cursor.ConsumeUint64()));
    header.SetTextValue(MMS_MESSAGE_ID, NonEmpty(cursor.ConsumeText(32), "fuzz-message"));
    header.SetTextValue(MMS_CONTENT_LOCATION, NonEmpty(cursor.ConsumeText(48), "http://mmsc/fuzz"));
    header.SetLongValue(MMS_MESSAGE_SIZE, cursor.ConsumeUint32(1, 4096));
    header.SetLongValue(MMS_EXPIRY, cursor.ConsumeUint32(1, 4096));
    header.SetEncodeStringValue(MMS_SUBJECT, PickCharset(cursor), NonEmpty(cursor.ConsumeText(48), "fuzz subject"));
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

inline void AddOctetFields(MmsHeader &header, DataCursor &cursor)
{
    header.SetOctetValue(MMS_RESPONSE_STATUS, static_cast<uint8_t>(MmsResponseStatus::MMS_OK));
    header.SetOctetValue(MMS_STATUS, static_cast<uint8_t>(MmsStatus::MMS_RETRIEVED));
    header.SetOctetValue(MMS_READ_STATUS, static_cast<uint8_t>(MmsReadStatus::MMS_READ));
    header.SetOctetValue(MMS_PRIORITY, static_cast<uint8_t>(MmsPriority::MMS_NORMAL));
    header.SetOctetValue(MMS_MESSAGE_CLASS, static_cast<uint8_t>(MmsMessageClass::PERSONAL));
    header.SetOctetValue(MMS_CONTENT_CLASS, static_cast<uint8_t>(MmsContentClass::MMS_TEXT));
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

inline void FillMmsMsg(MmsMsg &msg, DataCursor &cursor, uint8_t messageType)
{
    FillMmsHeader(msg.GetHeader(), cursor, messageType);
    msg.SetHeaderContentType("application/vnd.wap.multipart.related");
    if (messageType == MMS_MSGTYPE_SEND_REQ || messageType == MMS_MSGTYPE_RETRIEVE_CONF) {
        MmsAttachment smil = MakeAttachment(cursor, true);
        MmsAttachment media = MakeAttachment(cursor, false);
        msg.AddAttachment(smil);
        msg.AddAttachment(media);
    }
}

inline std::vector<uint8_t> EncodeBodyPart(DataCursor &cursor)
{
    MmsBodyPart bodyPart = MakeBodyPart(cursor);
    MmsEncodeBuffer encodeBuffer;
    bodyPart.EncodeMmsBodyPart(encodeBuffer);
    return ReadEncodedBytes(encodeBuffer);
}

inline std::vector<uint8_t> EncodeContentType(DataCursor &cursor)
{
    MmsContentType contentType = MakeContentType(cursor);
    MmsEncodeBuffer encodeBuffer;
    contentType.EncodeMmsBodyPartContentType(encodeBuffer);
    return ReadEncodedBytes(encodeBuffer);
}

inline std::vector<uint8_t> EncodeMmsMessage(DataCursor &cursor, uint8_t messageType)
{
    MmsMsg msg;
    FillMmsMsg(msg, cursor, messageType);
    uint32_t len = 0;
    auto encoded = msg.EncodeMsg(len);
    std::vector<uint8_t> bytes;
    if (encoded == nullptr || len == 0) {
        return bytes;
    }
    bytes.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(encoded[i]));
    }
    return bytes;
}
} // namespace SmsMmsFuzz
} // namespace Telephony
} // namespace OHOS
#endif // MMS_FUZZER_UTILS_H
