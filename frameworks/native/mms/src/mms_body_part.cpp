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
#include "mms_body_part.h"

#include <ctime>

#include "securec.h"
#include "sms_constants_utils.h"
#include "telephony_log_wrapper.h"
#include "utils/mms_base64.h"
#include "utils/mms_quoted_printable.h"

namespace OHOS {
namespace Telephony {
const std::string ENCODE_BINARY = "binary";
const std::string ENCODE_BASE64 = "base64";
const std::string ENCODE_QUOTED_PRINTABLE = "quoted-printable";
using namespace std;
static constexpr const char *APP_SAND_ABSOLUTE_DIR = "/data/app/";
static constexpr const char *APP_SAND_RELATIVE_DIR = "/data/storage/";

MmsBodyPart::MmsBodyPart() : headerLen_(0), bodyLen_(0) {}

MmsBodyPart::MmsBodyPart(const MmsBodyPart &srcBodyPart) : headerLen_(0), bodyLen_(0)
{
    *this = srcBodyPart;
}

MmsBodyPart::~MmsBodyPart()
{
    if (pbodyPartBuffer_ != nullptr) {
        pbodyPartBuffer_.reset();
    }
}

void MmsBodyPart::DumpMmsBodyPart()
{
    std::string isSmil = isSmilFile_ ? "ture" : "false";
    TELEPHONY_LOGI("isSmilFile : %{private}s", isSmil.c_str());
    TELEPHONY_LOGI("strFileName : %{private}s", strFileName_.c_str());
    TELEPHONY_LOGI("headerLen : %{private}u", headerLen_);
    TELEPHONY_LOGI("bodyPartLen : %{private}u", bodyLen_);
    bodyPartContentType_.DumpMmsContentType();
    mmsBodyPartHeader_.DumpBodyPartHeader();
}

MmsBodyPart &MmsBodyPart::operator=(const MmsBodyPart &srcBodyPart)
{
    if (this == &srcBodyPart) {
        return *this;
    }

    if (srcBodyPart.bodyLen_ > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("srcBodyPart.bodyLen_ over size error");
        return *this;
    }
    bodyLen_ = srcBodyPart.bodyLen_;
    pbodyPartBuffer_ = std::make_unique<char[]>(bodyLen_);
    if (pbodyPartBuffer_ == nullptr || srcBodyPart.pbodyPartBuffer_ == nullptr) {
        bodyLen_ = 0;
        AssignBodyPart(srcBodyPart);
        TELEPHONY_LOGE("Assignment Buffer Nullptr Error.");
        return *this;
    }

    if (memcpy_s(pbodyPartBuffer_.get(), bodyLen_, srcBodyPart.pbodyPartBuffer_.get(), bodyLen_) != EOK) {
        bodyLen_ = 0;
        TELEPHONY_LOGE("Copy BodyPart Buffer Memory Error.");
    }
    AssignBodyPart(srcBodyPart);
    return *this;
}

void MmsBodyPart::AssignBodyPart(const MmsBodyPart &obj)
{
    headerLen_ = obj.headerLen_;
    mmsBodyPartHeader_ = obj.mmsBodyPartHeader_;
    strFileName_ = obj.strFileName_;
    bodyPartContentType_ = obj.bodyPartContentType_;
}

/**
 * @brief DecodePart
 * wap-230-wsp-20010705-a   section:8.5.3 Multipart Entry
 * HeadersLen   Uintvar
 * DataLen      Uintvar
 * ContentType  Multiple octets
 * Headers      (HeadersLen – length of ContentType) octets
 * Data         DataLen octets
 * @param decodeBuffer
 * @return true
 * @return false
 */
bool MmsBodyPart::DecodePart(MmsDecodeBuffer &decodeBuffer, uint32_t nEntries)
{
    uint32_t headerLength = 0;
    uint32_t bodyLength = 0;
    uint32_t length = 0;
    if (!decodeBuffer.DecodeUintvar(headerLength, length)) {
        TELEPHONY_LOGE("Decode Body Part Header Uintvar Error.");
        return false;
    }
    TELEPHONY_LOGE("header length: %{public}d", headerLength);
    if (!decodeBuffer.DecodeUintvar(bodyLength, length)) {
        TELEPHONY_LOGE("Decode Body Part Body Lenght Uintvar Error.");
        return false;
    }
    TELEPHONY_LOGE("body length: %{public}d", bodyLength);
    int32_t contentLength = 0;
    if (!bodyPartContentType_.DecodeMmsContentType(decodeBuffer, contentLength)) {
        TELEPHONY_LOGE("Decode Body Part ContentType Error.");
        return false;
    }
    TELEPHONY_LOGE("content length: %{public}d", contentLength);
    headerLen_ = headerLength;
    bodyLen_ = bodyLength;
    if (headerLen_ < static_cast<uint32_t>(contentLength)) {
        TELEPHONY_LOGE("Decode Body Part HeaderLen Less Than ContentLength Error.");
        return false;
    }
    if (!DecodePartHeader(decodeBuffer, headerLen_ - static_cast<uint32_t>(contentLength))) {
        TELEPHONY_LOGE("Decode Body Part Header Error.");
        return false;
    }
    if (!DecodePartBody(decodeBuffer, bodyLen_)) {
        TELEPHONY_LOGE("Decode Body Part Body Error.");
        return false;
    }
    DecodeSetFileName(nEntries);
    return true;
}

/**
 * @brief DecodePartHeader
 * wap-230-wsp-20010705-a   section:8.4.2.6 Header
 * Message-header			  = Well-known-header | Application-header
 * Application-header		  = Token-text Application-specific-value
 * Well-known-field-name	  = Short-integer
 * Application-specific-value = Text-string
 * @param decodeBuffer
 * @param headerLen
 * @return true
 * @return false
 */
bool MmsBodyPart::DecodePartHeader(MmsDecodeBuffer &decodeBuffer, uint32_t headerLen)
{
    const uint8_t headerAccept = 0x80;
    const uint8_t headerCacheControl = 0xC7;
    const uint8_t textMin = 32;
    const uint8_t textMax = 127;

    uint8_t oneByte = 0;
    while (headerLen > 0) {
        if (!decodeBuffer.PeekOneByte(oneByte)) {
            TELEPHONY_LOGE("Decode Body Part PeekOneByte Error.");
            return false;
        }
        TELEPHONY_LOGE("part header onebyte:%{public}u", oneByte);
        if (headerAccept <= oneByte && headerCacheControl >= oneByte) {
            if (!mmsBodyPartHeader_.DecodeWellKnownHeader(decodeBuffer, headerLen)) {
                TELEPHONY_LOGE("Decode Body Part DecodeWellKnownHeader Error.");
                return false;
            }
        } else if ((oneByte >= textMin) && (oneByte <= textMax)) {
            if (!mmsBodyPartHeader_.DecodeApplicationHeader(decodeBuffer, headerLen)) {
                TELEPHONY_LOGE("Decode Body Part DecodeApplicationHeader Error.");
                return false;
            }
        } else {
            TELEPHONY_LOGE("Header Field[%{public}02X] is not support.", oneByte);
            return false;
        }
    }
    return true;
}

bool MmsBodyPart::DecodePartBody(MmsDecodeBuffer &decodeBuffer, uint32_t bodyLength)
{
    uint32_t offset = decodeBuffer.GetCurPosition();
    if (offset + bodyLength > decodeBuffer.GetSize() || bodyLength > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("Decode Body Part buffer size err.");
        return false;
    }

    std::unique_ptr<char[]> bodyPartBuffer = decodeBuffer.ReadDataBuffer(offset, bodyLength);
    if (bodyPartBuffer == nullptr) {
        TELEPHONY_LOGE("Decode Body Part buffer is null.");
        return false;
    }

    std::string transferEncoding;
    if (!mmsBodyPartHeader_.GetContentTransferEncoding(transferEncoding)) {
        TELEPHONY_LOGE("bodyPartHeader GetContentTransferEncoding Error");
        return false;
    }

    std::string encodebuffer = "";
    std::string encodeString(bodyPartBuffer.get(), bodyLength);
    if (transferEncoding == ENCODE_BASE64) {
        encodebuffer = MmsBase64::Decode(encodeString);
    } else if (transferEncoding == ENCODE_QUOTED_PRINTABLE) {
        MmsQuotedPrintable::Decode(encodeString, encodebuffer);
    }

    if (encodebuffer.length()) {
        bodyLen_ = 0;
        uint32_t tempLen = encodebuffer.length();
        if (tempLen > MMS_PDU_MAX_SIZE) {
            TELEPHONY_LOGE("tempLen over size error");
            return false;
        }
        pbodyPartBuffer_ = std::make_unique<char[]>(tempLen);
        if (pbodyPartBuffer_ == nullptr) {
            TELEPHONY_LOGE("pbodyPartBuffer_ nullptr Error");
            return false;
        }
        if (memcpy_s(pbodyPartBuffer_.get(), tempLen, encodebuffer.data(), tempLen) != EOK) {
            TELEPHONY_LOGE("Memcpy_s pbodyPartBuffer_ Error");
            return false;
        }
        bodyLen_ = tempLen;
    } else {
        pbodyPartBuffer_ = std::move(bodyPartBuffer);
    }
    if (!decodeBuffer.IncreasePointer(bodyLength)) {
        TELEPHONY_LOGE("Decode Body Part IncreasePointer err.");
        return false;
    }
    return true;
}

bool MmsBodyPart::SetAttachment(MmsAttachment &attachment)
{
    std::string filePathName = attachment.GetAttachmentFilePath();
    bool readFileRes = WriteBodyFromFile(filePathName);
    if (readFileRes) {
        std::string tempFileName = attachment.GetFileName();
        if (tempFileName.empty()) {
            std::size_t pos = filePathName.find_last_of('/');
            if (pos != std::string::npos) {
                tempFileName = filePathName.substr(pos + 1);
            }
        }
        SetFileName(tempFileName);
    }

    /** If Read Attatemt Body Buffer From File Error Will Temp Read From Buffer **/
    if (!readFileRes) {
        if (!WriteBodyFromAttachmentBuffer(attachment)) {
            TELEPHONY_LOGE("Attachment Not Any Body Data Error.");
            return false;
        }
        SetFileName(attachment.GetFileName());
    }

    if (strFileName_.empty()) {
        TELEPHONY_LOGE("Get Attachment FileName Invalid error!");
        return false;
    }
    if (!SetContentType(attachment.GetContentType())) {
        TELEPHONY_LOGE("Mms BodyPart SetContentType is fail!");
        return false;
    }
    if (!SetContentId(attachment.GetContentId())) {
        TELEPHONY_LOGE("Mms BodyPart GetContentId is fail!");
        return false;
    }
    if (!SetContentLocation(attachment.GetContentLocation())) {
        TELEPHONY_LOGE("Mms BodyPart SetContentLocation is fail!");
        return false;
    }
    if (!mmsBodyPartHeader_.SetContentTransferEncoding(attachment.GetContentTransferEncoding())) {
        TELEPHONY_LOGE("Mms BodyPartHeader SetContentTransferEncoding is fail!");
        return false;
    }
    SetSmilFile(attachment.IsSmilFile());
    SetContentDisposition(attachment.GetContentDisposition());
    GetContentType().GetContentParam().SetFileName(strFileName_);
    GetContentType().GetContentParam().SetCharSet(attachment.GetCharSet());
    return true;
}

bool MmsBodyPart::IsSmilFile()
{
    return isSmilFile_;
}

void MmsBodyPart::SetSmilFile(bool isSmil)
{
    isSmilFile_ = isSmil;
}

bool MmsBodyPart::SetContentType(std::string strContentType)
{
    return bodyPartContentType_.SetContentType(strContentType);
}

bool MmsBodyPart::GetContentType(std::string &strContentType)
{
    return bodyPartContentType_.GetContentType(strContentType);
}

bool MmsBodyPart::SetContentId(std::string contentId)
{
    return mmsBodyPartHeader_.SetContentId(contentId);
}

bool MmsBodyPart::GetContentId(std::string &contentId)
{
    return mmsBodyPartHeader_.GetContentId(contentId);
}

bool MmsBodyPart::SetContentLocation(std::string contentLocation)
{
    return mmsBodyPartHeader_.SetContentLocation(contentLocation);
}

bool MmsBodyPart::GetContentLocation(std::string &contentLocation)
{
    return mmsBodyPartHeader_.GetContentLocation(contentLocation);
}

bool MmsBodyPart::SetContentDisposition(std::string contentDisposition)
{
    return mmsBodyPartHeader_.SetContentDisposition(contentDisposition);
}

bool MmsBodyPart::GetContentDisposition(std::string &contentDisposition)
{
    return mmsBodyPartHeader_.GetContentDisposition(contentDisposition);
}

/**
 * @brief EncodeMmsBodyPart
 * wap-230-wsp-20010705-a   section:8.5.3 Multipart Entry
 * HeadersLen   Uintvar
 * DataLen      Uintvar
 * ContentType  Multiple octets
 * Headers      (HeadersLen – length of ContentType) octets
 * Data         DataLen octets
 * @param encodeBuffer
 * @return true
 * @return false
 */
bool MmsBodyPart::EncodeMmsBodyPart(MmsEncodeBuffer &encodeBuffer)
{
    MmsEncodeBuffer tmpEncodeBuffer;
    if (!bodyPartContentType_.EncodeMmsBodyPartContentType(tmpEncodeBuffer)) {
        TELEPHONY_LOGE("Encode MmsBodyPart ContentType Error.");
        return false;
    }
    if (!mmsBodyPartHeader_.EncodeMmsBodyPartHeader(tmpEncodeBuffer)) {
        TELEPHONY_LOGE("Encode MmsBodyPart Header Error.");
        return false;
    }
    if (!encodeBuffer.EncodeUintvar(tmpEncodeBuffer.GetCurPosition())) {
        TELEPHONY_LOGE("Encode MmsBodyPart Body Uintvar Error.");
        return false;
    }
    if (!encodeBuffer.EncodeUintvar(bodyLen_)) {
        TELEPHONY_LOGE("Encode MmsBodyPart Body Len Uintvar Error.");
        return false;
    }
    if (!encodeBuffer.WriteBuffer(tmpEncodeBuffer)) {
        TELEPHONY_LOGE("Encode MmsBodyPart WriteBuffer Error.");
        return false;
    }
    uint32_t bodyLen = 0;
    std::unique_ptr<char[]> bodyBuff = ReadBodyPartBuffer(bodyLen);
    if (bodyBuff == nullptr) {
        TELEPHONY_LOGE("bodyBuff nullptr Error.");
        return false;
    }
    if (!encodeBuffer.WriteBuffer(std::move(bodyBuff), bodyLen)) {
        return false;
    }
    return true;
}

void MmsBodyPart::DecodeSetFileName(uint32_t nEntries)
{
    std::string fileName = "";
    GetContentType().GetContentParam().GetFileName(fileName);
    if (fileName.length() > 0) {
        strFileName_ = fileName;
        return;
    }
    std::string contentLocation = "";
    GetPartHeader().GetContentLocation(contentLocation);
    if (contentLocation.length() > 0) {
        strFileName_ = contentLocation;
        return;
    }
    std::string contentId = "";
    GetPartHeader().GetContentId(contentId);
    if (contentId.length() > 0) {
        strFileName_ = contentId;
        return;
    }

    const unsigned char timeBufferLen = 64;
    time_t currentTime = time(nullptr);
    if (currentTime == -1) {
        return;
    }
    char chCurrentTime[timeBufferLen] = {0};
    struct tm tmInfo;
    if (memset_s(&tmInfo, sizeof(struct tm), 0x00, sizeof(tm)) != EOK) {
        TELEPHONY_LOGE("DisplayTime memset fail.");
        return;
    }

    tm *timeptr = localtime_r(&currentTime, &tmInfo);
    if (currentTime == static_cast<time_t>(-1) || timeptr == nullptr) {
        TELEPHONY_LOGI("obtain current time Error.");
    }
    if (timeptr != nullptr) {
        (void)strftime(chCurrentTime, sizeof(chCurrentTime), "%Y%m%d%H%M%S", timeptr);
    }
    std::string finalFileName(chCurrentTime);
    finalFileName += '_';
    finalFileName += '0' + nEntries;
    strFileName_ = finalFileName;
    return;
}

bool MmsBodyPart::WriteBodyFromFile(std::string path)
{
    FILE *pFile = nullptr;
    char realPath[PATH_MAX] = { 0 };
    if (path.empty() || realpath(path.c_str(), realPath) == NULL) {
        TELEPHONY_LOGE("path or realPath is NULL");
        return false;
    }

    std::string filePath = realPath;
    std::string absDir = APP_SAND_ABSOLUTE_DIR;
    std::string relDir = APP_SAND_RELATIVE_DIR;
    if ((absDir.compare(filePath.substr(0, absDir.size())) != 0) &&
        (relDir.compare(filePath.substr(0, relDir.size())) != 0)) {
        TELEPHONY_LOGE("filePath no app sand box.");
        return false;
    }
    pFile = fopen(realPath, "rb");
    if (pFile == nullptr) {
        TELEPHONY_LOGI("Write Body Part from File notFind, try to use buffer");
        return false;
    }
    (void)fseek(pFile, 0, SEEK_END);
    long fileLen = ftell(pFile);
    if (fileLen <= 0 || fileLen > static_cast<long>(MMS_PDU_MAX_SIZE)) {
        (void)fclose(pFile);
        TELEPHONY_LOGE("fileLen is invalid [%{public}ld]", fileLen);
        return false;
    }
    if (pbodyPartBuffer_) {
        pbodyPartBuffer_.reset();
    }
    pbodyPartBuffer_ = std::make_unique<char[]>(fileLen);
    if (!pbodyPartBuffer_) {
        (void)fclose(pFile);
        TELEPHONY_LOGE("Buffer initialize fail!");
        return false;
    }

    (void)fseek(pFile, 0, SEEK_SET);
    bodyLen_ = fread(pbodyPartBuffer_.get(), 1, fileLen, pFile);
    (void)fclose(pFile);
    return true;
}

bool MmsBodyPart::WriteBodyFromAttachmentBuffer(MmsAttachment &attachment)
{
    if (attachment.GetFileName().empty()) {
        TELEPHONY_LOGE("Attachment must set fileName, else error!");
        return false;
    }

    uint32_t dataLen = 0;
    std::unique_ptr<char[]> tempBuffer = nullptr;
    tempBuffer = attachment.GetDataBuffer(dataLen);
    if (tempBuffer == nullptr) {
        TELEPHONY_LOGE("Read Attachment Data Buffer nullptr error.");
        return false;
    }

    if (dataLen == 0 || dataLen > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("Attachment DataLen is invalid Error");
        return false;
    }

    if (pbodyPartBuffer_) {
        pbodyPartBuffer_.reset();
    }
    pbodyPartBuffer_ = std::make_unique<char[]>(dataLen);
    if (!pbodyPartBuffer_) {
        TELEPHONY_LOGE("Buffer initialize fail!");
        return false;
    }

    if (memcpy_s(pbodyPartBuffer_.get(), dataLen, tempBuffer.get(), dataLen) != EOK) {
        TELEPHONY_LOGE("Attachment Buffer MemCopy Error.");
        bodyLen_ = 0;
        return false;
    }
    bodyLen_ = dataLen;
    return true;
}

std::string MmsBodyPart::GetPartFileName()
{
    return strFileName_;
}

void MmsBodyPart::SetFileName(std::string fileName)
{
    strFileName_ = fileName;
}

MmsContentType &MmsBodyPart::GetContentType()
{
    return bodyPartContentType_;
}

MmsBodyPartHeader &MmsBodyPart::GetPartHeader()
{
    return mmsBodyPartHeader_;
}

std::unique_ptr<char[]> MmsBodyPart::ReadBodyPartBuffer(uint32_t &len)
{
    if (bodyLen_ > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("srcBodyPart.bodyLen_ over size error");
        return nullptr;
    }
    std::unique_ptr<char[]> result = std::make_unique<char[]>(bodyLen_);
    if (result == nullptr) {
        TELEPHONY_LOGE("Read BodyPart Buffer MakeUnique Error.");
        return nullptr;
    }
    if (memcpy_s(result.get(), bodyLen_, pbodyPartBuffer_.get(), bodyLen_) != EOK) {
        TELEPHONY_LOGE("Read BodyPart Buffer Memcpy_s Error.");
        return nullptr;
    }
    len = bodyLen_;
    return result;
}
} // namespace Telephony
} // namespace OHOS
