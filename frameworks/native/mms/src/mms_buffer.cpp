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
#include "mms_buffer.h"

#include "securec.h"
#include "sms_constants_utils.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
static constexpr const char *APP_SAND_ABSOLUTE_DIR = "/data/app/";
static constexpr const char *APP_SAND_RELATIVE_DIR = "/data/storage/";

MmsBuffer::MmsBuffer()
{
    pduBuffer_ = std::make_unique<char[]>(MMS_PDU_MAX_SIZE);
    if (pduBuffer_ == nullptr) {
        TELEPHONY_LOGE("MmsBuffer make_unique create pduBuffer error");
        totolLength_ = 0;
        return;
    }
    totolLength_ = MMS_PDU_MAX_SIZE;
}

MmsBuffer::~MmsBuffer()
{
    if (pduBuffer_ != nullptr) {
        pduBuffer_.reset();
    }
}

std::unique_ptr<char[]> MmsBuffer::ReadDataBuffer(uint32_t desLen)
{
    return ReadDataBuffer(curPosition_, desLen);
}

std::unique_ptr<char[]> MmsBuffer::ReadDataBuffer(uint32_t offset, uint32_t desLen)
{
    if (desLen > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("desLen over size error");
        return nullptr;
    }
    std::unique_ptr<char[]> result = std::make_unique<char[]>(desLen);
    if (result == nullptr) {
        TELEPHONY_LOGE("make unique result nullptr Error .");
        return nullptr;
    }

    if (offset + desLen > totolLength_) {
        TELEPHONY_LOGE("read over buffer error .");
        return nullptr;
    }

    if (memcpy_s(result.get(), desLen, &pduBuffer_[offset], desLen) != EOK) {
        TELEPHONY_LOGE("read memcpy_s error .");
        return nullptr;
    }
    return result;
}

bool MmsBuffer::WriteDataBuffer(std::unique_ptr<char[]> inBuff, uint32_t len)
{
    if (inBuff == nullptr) {
        TELEPHONY_LOGE("InBuffer nullptr Error .");
        return false;
    }
    if (len <= 0 || len > MMS_PDU_MAX_SIZE) {
        TELEPHONY_LOGE("Data Len Over Error .");
        return false;
    }

    if (pduBuffer_) {
        pduBuffer_.reset();
    }

    pduBuffer_ = std::make_unique<char[]>(len);
    if (!pduBuffer_) {
        TELEPHONY_LOGE("make unique pduBuffer nullptr Error .");
        return false;
    }
    if (memcpy_s(pduBuffer_.get(), len, inBuff.get(), len) != EOK) {
        TELEPHONY_LOGE("MemCpy_s Error .");
        return false;
    }
    totolLength_ = len;
    return true;
}

bool MmsBuffer::WriteBufferFromFile(std::string &strPathName)
{
    FILE *pFile = nullptr;
    char realPath[PATH_MAX] = {0};
    uint32_t fileLen = 0;
    if (strPathName.empty() || realpath(strPathName.c_str(), realPath) == NULL) {
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
        TELEPHONY_LOGE("Open File Error");
        return false;
    }

    if (fseek(pFile, 0, SEEK_END) != 0) {
        (void)fclose(pFile);
        return false;
    }
    fileLen = (uint32_t)ftell(pFile);
    if (fileLen == 0) {
        (void)fclose(pFile);
        totolLength_ = 0;
        TELEPHONY_LOGE("Mmms File Is Empty.");
        return true;
    }
    if (fileLen < 0 || fileLen > MMS_PDU_MAX_SIZE) {
        (void)fclose(pFile);
        TELEPHONY_LOGE("Mms Over Long Error .");
        return false;
    }
    if (!WriteBufferFromFile(pFile, fileLen)) {
        (void)fclose(pFile);
        return false;
    }
    (void)fclose(pFile);
    return true;
}

bool MmsBuffer::WriteBufferFromFile(FILE *pFile, uint32_t fileLen)
{
    if (pduBuffer_) {
        pduBuffer_.reset();
    }

    pduBuffer_ = std::make_unique<char[]>(fileLen);
    if (!pduBuffer_) {
        TELEPHONY_LOGE("make unique pduBuffer nullptr Error .");
        return false;
    }
    if (fseek(pFile, 0, SEEK_SET) != 0) {
        return false;
    }
    totolLength_ = fread(pduBuffer_.get(), 1, fileLen, pFile);
    return true;
}

uint32_t MmsBuffer::GetCurPosition() const
{
    return curPosition_;
}

uint32_t MmsBuffer::GetSize() const
{
    return totolLength_;
}
} // namespace Telephony
} // namespace OHOS
