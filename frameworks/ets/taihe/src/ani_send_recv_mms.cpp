/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ani_send_recv_mms.h"
#include "ability.h"
#include "ani_base_context.h"
#include "ani_mms_pdu.h"
#include "sms_constants_utils.h"
#include "sms_service_manager_client.h"
#include "taihe/runtime.hpp"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"
#include <chrono>
#include <string>
#include "napi_util.h"

using namespace taihe;

namespace OHOS {
namespace Telephony {
namespace {
const std::string SMS_PROFILE_URI = "datashare:///com.ohos.smsmmsability";
const bool STORE_MMS_PDU_TO_FILE = false;
constexpr static uint32_t WAIT_PDN_TOGGLE_TIME = 3000;
} // namespace
std::mutex AniSendRecvMms::downloadCtx_;
std::mutex AniSendRecvMms::countCtx_;
int32_t AniSendRecvMms::reqCount_ = 0;
bool AniSendRecvMms::waitFlag = false;

enum class ContextMode { INIT = -1, FA = 0, STAGE = 1 };
static std::atomic<ContextMode> gContextNode = ContextMode::INIT;
static ContextMode GetContextMode(ani_env *env, ani_object context)
{
    if (gContextNode.load() == ContextMode::INIT) {
        ani_boolean isStageMode;
        ani_status status = OHOS::AbilityRuntime::IsStageContext(env, context, isStageMode);
        TELEPHONY_LOGD("isStageMode:%{public}d, status:%{public}d", static_cast<bool>(isStageMode),
            static_cast<int32_t>(status));
        if (status == ANI_OK) {
            gContextNode.store(isStageMode ? ContextMode::STAGE : ContextMode::FA);
        }
    }
    return gContextNode.load();
}

static std::shared_ptr<OHOS::DataShare::DataShareHelper> CreateDataShareHelper(ani_object context)
{
    auto env = ::taihe::get_env();
    if (env == nullptr) {
        return nullptr;
    }

    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = nullptr;
    if (GetContextMode(env, context) == ContextMode::STAGE) {
        auto stageContext = OHOS::AbilityRuntime::GetStageModeContext(env, context);
        if (stageContext == nullptr) {
            return nullptr;
        }
        dataShareHelper = DataShare::DataShareHelper::Creator(stageContext->GetToken(), SMS_PROFILE_URI);
    }
    return dataShareHelper;
}

static bool GetMmsPduFromFile(const std::string &fileName, std::string &mmsPdu)
{
    char realPath[PATH_MAX] = {0};
    if (fileName.empty() || realpath(fileName.c_str(), realPath) == nullptr) {
        return false;
    }

    FILE *pFile = fopen(realPath, "rb");
    if (pFile == nullptr) {
        return false;
    }

    if (fseek(pFile, 0, SEEK_END) != 0) {
        (void)fclose(pFile);
        return false;
    }
    long fileLen = ftell(pFile);
    if (fileLen <= 0 || fileLen > static_cast<long>(MMS_PDU_MAX_SIZE)) {
        (void)fclose(pFile);
        return false;
    }

    std::unique_ptr<char[]> pduBuffer = std::make_unique<char[]>(fileLen);
    if (!pduBuffer) {
        (void)fclose(pFile);
        return false;
    }
    if (fseek(pFile, 0, SEEK_SET) != 0) {
        (void)fclose(pFile);
        return false;
    }
    int32_t totolLength = static_cast<int32_t>(fread(pduBuffer.get(), 1, MMS_PDU_MAX_SIZE, pFile));
    TELEPHONY_LOGI("fread totolLength%{private}d", totolLength);

    long i = 0;
    while (i < fileLen) {
        mmsPdu += pduBuffer[i];
        i++;
    }
    (void)fclose(pFile);
    return true;
}

static void StoreSendMmsPduToDataBase(AniMmsPduHelper &helper) __attribute__((no_sanitize("cfi")))
{
    std::shared_ptr<AniMmsPdu> mmsPduObj = std::make_shared<AniMmsPdu>();
    if (mmsPduObj == nullptr) {
        helper.NotifyAll();
        return;
    }
    std::string mmsPdu;
    if (!GetMmsPduFromFile(helper.GetPduFileName(), mmsPdu)) {
        helper.NotifyAll();
        return;
    }
    mmsPduObj->InsertMmsPdu(helper, mmsPdu);
}

static void UpdateTimeStamp(const int64_t &timeStamp, MmsContext &context)
{
    context.timeStamp = timeStamp;
}

static int32_t NativeSendMms(uintptr_t context, MmsContext &mmsContext)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!STORE_MMS_PDU_TO_FILE) {
        std::string pduFileName = NapiUtil::ToUtf8(mmsContext.data);
        if (pduFileName.empty()) {
            TELEPHONY_LOGE("pduFileName empty");
            return TELEPHONY_ERR_ARGUMENT_INVALID;
        }

        auto datashareHelper = CreateDataShareHelper(reinterpret_cast<ani_object>(context));
        if (datashareHelper == nullptr) {
            TELEPHONY_LOGE("datashareHelper is nullptr");
            return TELEPHONY_ERR_LOCAL_PTR_NULL;
        }
        AniMmsPduHelper helper;
        helper.SetDataShareHelper(datashareHelper);
        helper.SetPduFileName(pduFileName);
        if (!helper.Run(StoreSendMmsPduToDataBase, helper)) {
            TELEPHONY_LOGE("StoreMmsPdu fail");
            return TELEPHONY_ERR_LOCAL_PTR_NULL;
        }
        mmsContext.data = NapiUtil::ToUtf16(helper.GetDbUrl());
    }
    auto errorCode = Singleton<SmsServiceManagerClient>::GetInstance().SendMms(mmsContext.slotId, mmsContext.mmsc,
        mmsContext.data, mmsContext.mmsConfig.userAgent, mmsContext.mmsConfig.userAgentProfile, mmsContext.timeStamp);
    if (errorCode != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SendMms fail");
    }

    return errorCode;
}

int32_t AniSendRecvMms::SendMms(uintptr_t context, struct MmsContext &mmsContext)
{
    int64_t timeStamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    UpdateTimeStamp(timeStamp, mmsContext);
    return NativeSendMms(context, mmsContext);
}

bool WriteBufferToFile(const std::unique_ptr<char[]> &buff, uint32_t len, const std::string &strPathName)
{
    if (buff == nullptr) {
        TELEPHONY_LOGE("buff nullptr");
        return false;
    }

    char realPath[PATH_MAX] = {0};
    if (strPathName.empty() || realpath(strPathName.c_str(), realPath) == nullptr) {
        TELEPHONY_LOGE("path or realPath is nullptr");
        return false;
    }

    FILE *pFile = fopen(realPath, "wb");
    if (pFile == nullptr) {
        TELEPHONY_LOGE("openFile Error");
        return false;
    }
    uint32_t fileLen = fwrite(buff.get(), len, 1, pFile);
    (void)fclose(pFile);
    if (fileLen > 0) {
        TELEPHONY_LOGI("write mms buffer to file success");
        return true;
    } else {
        TELEPHONY_LOGI("write mms buffer to file error");
        return false;
    }
}

bool StoreMmsPduToFile(const std::string &fileName, const std::string &mmsPdu)
{
    uint32_t len = static_cast<uint32_t>(mmsPdu.size());
    if (len > MMS_PDU_MAX_SIZE || len == 0) {
        TELEPHONY_LOGE("MMS pdu length invalid");
        return false;
    }

    std::unique_ptr<char[]> resultResponse = std::make_unique<char[]>(len);
    if (memset_s(resultResponse.get(), len, 0x00, len) != EOK) {
        TELEPHONY_LOGE("memset_s err");
        return false;
    }
    if (memcpy_s(resultResponse.get(), len, &mmsPdu[0], len) != EOK) {
        TELEPHONY_LOGE("memcpy_s error");
        return false;
    }

    TELEPHONY_LOGI("len:%{public}d", len);
    if (!WriteBufferToFile(std::move(resultResponse), len, fileName)) {
        TELEPHONY_LOGE("write to file error");
        return false;
    }
    return true;
}

void GetMmsPduFromDataBase(AniMmsPduHelper &helper) __attribute__((no_sanitize("cfi")))
{
    AniMmsPdu mmsPduObj;
    std::string mmsPdu = mmsPduObj.GetMmsPdu(helper);
    if (mmsPdu.empty()) {
        TELEPHONY_LOGE("from dataBase empty");
        return;
    }

    mmsPduObj.DeleteMmsPdu(helper);
    if (!StoreMmsPduToFile(helper.GetStoreFileName(), mmsPdu)) {
        TELEPHONY_LOGE("store mmsPdu fail");
    }
    helper.NotifyAll();
}

static int32_t DownloadExceptionCase(MmsContext &context,
    std::shared_ptr<OHOS::DataShare::DataShareHelper> datashareHelper)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (datashareHelper == nullptr) {
        TELEPHONY_LOGE("datashareHelper is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::string fileName = NapiUtil::ToUtf8(context.data);
    char realPath[PATH_MAX] = {0};
    if (fileName.empty() || realpath(fileName.c_str(), realPath) == nullptr) {
        TELEPHONY_LOGE("path or realPath is nullptr");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    FILE *pFile = fopen(realPath, "wb");
    if (pFile == nullptr) {
        TELEPHONY_LOGE("openFile Error");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    (void)fclose(pFile);
    return TELEPHONY_ERR_SUCCESS;
}

void UpdateReqCount()
{
    std::unique_lock<std::mutex> lck(AniSendRecvMms::countCtx_);
    AniSendRecvMms::reqCount_++;
    TELEPHONY_LOGI("reqCount_:%{public}d", AniSendRecvMms::reqCount_);
}

void DecreaseReqCount()
{
    AniSendRecvMms::reqCount_--;
    if (AniSendRecvMms::reqCount_ > 0) {
        AniSendRecvMms::waitFlag = true;
    } else {
        AniSendRecvMms::waitFlag = false;
    }
}

static int32_t NativeDownloadMms(uintptr_t context, struct MmsContext &mmsContext)
{
    if (!STORE_MMS_PDU_TO_FILE) {
        auto datashareHelper = CreateDataShareHelper(reinterpret_cast<ani_object>(context));
        if (!datashareHelper) {
            TELEPHONY_LOGE("datashareHelper is null");
            return TELEPHONY_ERR_LOCAL_PTR_NULL;
        }
    }

    auto errorCode = DownloadExceptionCase(mmsContext, datashareHelper);
    if (!DownloadExceptionCase(mmsContext, datashareHelper)) {
        TELEPHONY_LOGE("Exception case");
        return errorCode;
    }

    TELEPHONY_LOGI("native download mms");
    UpdateReqCount();
    std::unique_lock<std::mutex> lck(AniSendRecvMms::downloadCtx_);
    if (AniSendRecvMms::waitFlag) {
        TELEPHONY_LOGI("down multiple mms at once wait");
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_PDN_TOGGLE_TIME));
    }
    std::u16string dbUrls;
    errorCode = Singleton<SmsServiceManagerClient>::GetInstance().DownloadMms(mmsContext.slotId, mmsContext.mmsc,
        dbUrls, mmsContext.mmsConfig.userAgent, mmsContext.mmsConfig.userAgentProfile);
    TELEPHONY_LOGI("NativeDownloadMms dbUrls:%{public}s", NapiUtil::ToUtf8(dbUrls).c_str());
    if (errorCode == TELEPHONY_ERR_SUCCESS) {
        if (!STORE_MMS_PDU_TO_FILE) {
            AniMmsPduHelper helper;
            helper.SetDataShareHelper(datashareHelper);
            helper.SetDbUrl(NapiUtil::ToUtf8(dbUrls));
            helper.SetStoreFileName(NapiUtil::ToUtf8(mmsContext.data));
            if (!helper.Run(GetMmsPduFromDataBase, helper)) {
                TELEPHONY_LOGE("StoreMmsPdu fail");
                return TELEPHONY_ERR_LOCAL_PTR_NULL;
            }
        }
    }
    DecreaseReqCount();
    return errorCode;
}

int32_t AniSendRecvMms::DownloadMms(uintptr_t context, struct MmsContext &mmsContext)
{
    return NativeDownloadMms(context, mmsContext);
}
} // namespace Telephony
} // namespace OHOS
