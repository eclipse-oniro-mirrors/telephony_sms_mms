/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "mms_persist_helper.h"

#include "gsm_pdu_hex_value.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
const std::string SMS_PROFILE_URI = "datashare:///com.ohos.smsmmsability";
const std::string SMS_PROFILE_MMS_PDU_URI = "datashare:///com.ohos.smsmmsability/sms_mms/mms_pdu";
static constexpr const char *PDU_CONTENT = "pdu_content";
static constexpr const char *ID = "id";
static constexpr uint8_t SLIDE_STEP = 2;
static constexpr uint32_t SPLIT_PDU_LENGTH = 195 * 1024;

std::shared_ptr<DataShare::DataShareHelper> MmsPersistHelper::CreateSmsHelper()
{
    if (mmsPduDataShareHelper_ == nullptr) {
        mmsPduDataShareHelper_ = CreateDataAHelper(TELEPHONY_SMS_MMS_SYS_ABILITY_ID, SMS_PROFILE_URI);
    }
    return mmsPduDataShareHelper_;
}

std::shared_ptr<DataShare::DataShareHelper> MmsPersistHelper::CreateDataAHelper(
    int32_t systemAbilityId, const std::string &dataAbilityUri) const
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        TELEPHONY_LOGE("Get system ability mgr failed");
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        TELEPHONY_LOGE("GetSystemAbility Service Failed");
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(remoteObj, dataAbilityUri);
}

void MmsPersistHelper::DeleteMmsPdu(const std::string &dbUrl)
{
    std::shared_ptr<DataShare::DataShareHelper> helper = CreateSmsHelper();
    if (helper == nullptr) {
        TELEPHONY_LOGE("helper is nullptr");
        return;
    }

    Uri uri(SMS_PROFILE_MMS_PDU_URI);
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> dbUrls = SplitUrl(dbUrl);
    int32_t result = -1;
    for (std::string url : dbUrls) {
        predicates.EqualTo("id", url);
        result = helper->Delete(uri, predicates);
        if (result < 0) {
            TELEPHONY_LOGE("delete mms pdu fail");
            helper->Release();
            return;
        }
    }
    helper->Release();
    mmsPdu_ = "";
    TELEPHONY_LOGI("result:%{public}d", result);
}

bool MmsPersistHelper::InsertMmsPdu(const std::string &mmsPdu, std::string &dbUrl)
{
    std::shared_ptr<DataShare::DataShareHelper> helper = CreateSmsHelper();
    if (helper == nullptr) {
        TELEPHONY_LOGE("helper is nullptr");
        return false;
    }
    Uri uri(SMS_PROFILE_MMS_PDU_URI);

    std::vector<std::string> mmsPdus = SplitPdu(mmsPdu);
    for (std::string mmsPdu : mmsPdus) {
        DataShare::DataShareValuesBucket bucket;
        bucket.Put(PDU_CONTENT, mmsPdu);
        int32_t result = helper->Insert(uri, bucket);
        if (result < 0) {
            TELEPHONY_LOGE("mms pdu insert fail");
            return false;
        }
        dbUrl += std::to_string(result) + ',';
    }
    helper->Release();
    TELEPHONY_LOGI("download mms insert db, dbUrl:%{public}s", dbUrl.c_str());
    return dbUrl.empty() ? false : true;
}

std::vector<std::string> MmsPersistHelper::SplitPdu(const std::string &mmsPdu)
{
    std::string targetMmsPdu;
    for (size_t i = 0; i < mmsPdu.size(); i++) {
        targetMmsPdu += static_cast<char>((mmsPdu[i] & HEX_VALUE_0F) | HEX_VALUE_F0);
        targetMmsPdu += static_cast<char>((mmsPdu[i] & HEX_VALUE_F0) | HEX_VALUE_0F);
    }

    std::vector<std::string> mmsPdus;
    for (uint32_t locate = 0; locate * SPLIT_PDU_LENGTH < targetMmsPdu.size(); locate++) {
        std::string mmsPduData;
        if ((locate + 1) * SPLIT_PDU_LENGTH < targetMmsPdu.size()) {
            mmsPduData = targetMmsPdu.substr(locate * SPLIT_PDU_LENGTH, SPLIT_PDU_LENGTH);
            mmsPdus.push_back(mmsPduData);
        } else {
            mmsPduData = targetMmsPdu.substr(locate * SPLIT_PDU_LENGTH);
            mmsPdus.push_back(mmsPduData);
            break;
        }
    }
    TELEPHONY_LOGI("pduLen:%{public}zu,target:%{public}zu,pageLen:%{public}zu", mmsPdu.size(), targetMmsPdu.size(),
        mmsPdus.size());
    return mmsPdus;
}

std::string MmsPersistHelper::GetMmsPdu(const std::string &dbUrl)
{
    if (!QueryMmsPdu(dbUrl)) {
        return "";
    }
    return mmsPdu_;
}

void MmsPersistHelper::SetMmsPdu(const std::string &mmsPdu)
{
    mmsPdu_ = mmsPdu;
}

std::vector<std::string> MmsPersistHelper::SplitUrl(std::string url)
{
    std::vector<std::string> dbUrls;
    while (url.size() > 0) {
        size_t locate = url.find_first_of(',');
        if (locate == 0 || locate == std::string::npos) {
            break;
        }
        dbUrls.push_back(url.substr(0, locate));
        url = url.substr(locate + 1);
    }
    return dbUrls;
}

bool MmsPersistHelper::QueryMmsPdu(const std::string &dbUrl)
{
    std::shared_ptr<DataShare::DataShareHelper> helper = CreateSmsHelper();
    if (helper == nullptr) {
        TELEPHONY_LOGE("helper is nullptr");
        return false;
    }

    std::vector<std::string> dbUrls = SplitUrl(dbUrl);
    std::string mmsPdu;
    for (std::string url : dbUrls) {
        Uri uri(SMS_PROFILE_MMS_PDU_URI);
        std::vector<std::string> colume;
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(ID, url);
        auto resultSet = helper->Query(uri, predicates, colume);
        if (resultSet == nullptr) {
            TELEPHONY_LOGE("Query Result Set nullptr Failed.");
            helper->Release();
            return false;
        }
        int count = 0;
        resultSet->GetRowCount(count);
        if (count <= 0) {
            TELEPHONY_LOGE("MmsPdu count: %{public}d error", count);
            resultSet->Close();
            helper->Release();
            return false;
        }
        int columnIndex;
        std::vector<uint8_t> blobValue;
        resultSet->GoToFirstRow();
        resultSet->GetColumnIndex(PDU_CONTENT, columnIndex);
        resultSet->GetBlob(columnIndex, blobValue);
        // LCOV_EXCL_START
        if (blobValue.empty()) {
            TELEPHONY_LOGE("blobValue is empty, pdu_content may be NULL");
            resultSet->Close();
            helper->Release();
            return false;
        }
        blobValue.pop_back();
        if (blobValue.size() < SLIDE_STEP) {
            TELEPHONY_LOGE("blobValue too small after pop_back");
            resultSet->Close();
            helper->Release();
            return false;
        }
        // LCOV_EXCL_STOP
        for (size_t i = 0; i + 1 < blobValue.size(); i = i + SLIDE_STEP) {
            char pduChar = (blobValue[i] & HEX_VALUE_0F) | (blobValue[i + 1] & HEX_VALUE_F0);
            mmsPdu += static_cast<char>(pduChar);
        }
        TELEPHONY_LOGI("blob len:%{public}zu, ", blobValue.size());
        resultSet->Close();
    }
    helper->Release();
    TELEPHONY_LOGI("mmsPdu len:%{public}zu", mmsPdu.size());
    SetMmsPdu(mmsPdu);
    return true;
}
} // namespace Telephony
} // namespace OHOS