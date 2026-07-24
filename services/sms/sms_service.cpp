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

#include "sms_service.h"

#include <regex>

#include "cdma_sms_message.h"
#include "core_manager_inner.h"
#include "ims_sms_client.h"
#include "phonenumbers/phonenumber.pb.h"
#include "sms_dump_helper.h"
#include "sms_mms_common.h"
#include "sms_hisysevent.h"
#include "sms_policy_utils.h"
#include "string_utils.h"
#include "telephony_common_utils.h"
#include "telephony_ext_wrapper.h"
#include "telephony_log_wrapper.h"
#include "telephony_permission.h"
#include "text_coder.h"
#include "base64.h"

namespace OHOS {
namespace Telephony {
using namespace std;
using namespace AppExecFwk;
using namespace HiviewDFX;
constexpr static uint32_t CONNECT_SERVICE_WAIT_TIME = 2000;
constexpr static int32_t CB_RANGE_PAIR_SIZE = 2;
constexpr static size_t MIN_LEN = 1;
bool g_registerResult = SystemAbility::MakeAndRegisterAbility(DelayedSingleton<SmsService>::GetInstance().get());
const std::string INFO_MSG_TELEPHONE_REG = "^(0086|\\+?86|\\+)?(9|95|100|101|106|108|111|116|118|123|125|400|800|1212|"
    "000000|\\d{2,4}12123$|\\d{2,4}12329$|^02[0-9]|010|^05[34]|^07[56])";
const std::string ISO_COUNTRY_CODE = "CN";
SmsService::SmsService() : SystemAbility(TELEPHONY_SMS_MMS_SYS_ABILITY_ID, true) {}

SmsService::~SmsService() {}

void SmsService::OnStart()
{
    TELEPHONY_LOGI("SmsService::OnStart start service Enter.");
    bindTime_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        TELEPHONY_LOGE("msService has already started.");
        return;
    }
    if (!Init()) {
        TELEPHONY_LOGE("failed to init SmsService");
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    endTime_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    TELEPHONY_LOGI("SmsService::OnStart start service Exit.");
}

bool SmsService::Init()
{
#ifdef OHOS_BUILD_ENABLE_TELEPHONY_EXT
    TELEPHONY_EXT_WRAPPER.InitTelephonyExtWrapper();
#endif
    DelayedSingleton<ImsSmsClient>::GetInstance()->Init();
    if (!registerToService_) {
        bool ret = Publish(DelayedSingleton<SmsService>::GetInstance().get());
        if (!ret) {
            TELEPHONY_LOGE("SmsService::Init Publish failed!");
            return false;
        }
        registerToService_ = true;
        WaitCoreServiceToInit();
    }
    DelayedSingleton<SmsStateHandler>::GetInstance()->Init();
    return true;
}

void SmsService::OnStop()
{
    state_ = ServiceRunningState::STATE_NOT_START;
    registerToService_ = false;
    DelayedSingleton<ImsSmsClient>::GetInstance()->UnInit();
    DelayedSingleton<SmsStateHandler>::GetInstance()->UnInit();
    TELEPHONY_LOGI("SmsService::OnStop stop service.");
}

int32_t SmsService::Dump(std::int32_t fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        TELEPHONY_LOGE("dump fd invalid");
        return TELEPHONY_ERR_FAIL;
    }
    std::vector<std::string> argsInStr;
    for (const auto &arg : args) {
        TELEPHONY_LOGI("Dump args: %{public}s", Str16ToStr8(arg).c_str());
        argsInStr.emplace_back(Str16ToStr8(arg));
    }
    std::string result;
    SmsDumpHelper dumpHelper;
    if (dumpHelper.Dump(argsInStr, result)) {
        TELEPHONY_LOGI("%{public}s", result.c_str());
        std::int32_t ret = dprintf(fd, "%s", result.c_str());
        if (ret < 0) {
            TELEPHONY_LOGE("dprintf to dump fd failed");
            return TELEPHONY_ERR_FAIL;
        }
        return TELEPHONY_SUCCESS;
    }
    TELEPHONY_LOGW("dumpHelper failed");
    return TELEPHONY_ERR_FAIL;
}

void SmsService::WaitCoreServiceToInit()
{
    TelFFRTUtils::Submit([&]() {
        while (true) {
            if (CoreManagerInner::GetInstance().IsInitFinished()) {
                InitModule();
                TELEPHONY_LOGI("SmsService Connection successful");
                break;
            }
            TelFFRTUtils::SleepFor(CONNECT_SERVICE_WAIT_TIME);
        }
    });
}

std::string SmsService::GetBindTime()
{
    return std::to_string(bindTime_);
}

int32_t SmsService::SendMessage(int32_t slotId, const u16string desAddr, const u16string scAddr, const u16string text,
    const sptr<ISendShortMessageCallback> &sendCallback, const sptr<IDeliveryShortMessageCallback> &deliveryCallback,
    bool isMmsApp)
{
    if (SmsPolicyUtils::IsSmsPolicyDisable()) {
        TELEPHONY_LOGE("SmsService::SendMessage sms policy is disabled");
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    if (!CheckSmsPermission(sendCallback)) {
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_PERMISSION_ERROR, Permission::SEND_MESSAGES);
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (desAddr.empty()) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("SmsService::SendMessage desAddr empty");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_NULL_POINTER, "text sms interfaceManager is nullptr");
        TELEPHONY_LOGE("SmsService::SendMessage interfaceManager nullptr error.");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (!ValidDestinationAddress(StringUtils::ToUtf8(desAddr))) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("SmsService::SendMessage desAddr not conform to the regular specification");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }

    int32_t dataId = 0;
    if (!isMmsApp) {
        InsertSessionAndDetail(slotId, StringUtils::ToUtf8(desAddr), StringUtils::ToUtf8(text), dataId);
        TELEPHONY_LOGI("InsertSessionAndDetail write data to db. the id:%{public}d", dataId);
    } else {
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(SmsMmsInfo::MSG_TYPE, SmsMmsCommonData::SMS_MSM_TYPE_SMS);
        predicates.EqualTo(SmsMmsInfo::RECEIVER_NUMBER, StringUtils::ToUtf8(desAddr));
        predicates.EqualTo(SmsMmsInfo::SLOT_ID, slotId);
        predicates.EqualTo(SmsMmsInfo::MSG_CONTENT, StringUtils::ToUtf8(text));
        predicates.EqualTo(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MSM_STATUS_SENDING);
        predicates.OrderByDesc(SmsMmsInfo::START_TIME);
        DelayedSingleton<SmsPersistHelper>::GetInstance()->QuerySmsMmsForId(predicates, dataId);
    }
    bool ret = interfaceManager->TextBasedSmsDelivery(StringUtils::ToUtf8(desAddr), StringUtils::ToUtf8(scAddr),
        StringUtils::ToUtf8(text), sendCallback, deliveryCallback, dataId, isMmsApp);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateContact(StringUtils::ToUtf8(desAddr));
    return ret;
}

int32_t SmsService::SendMessageWithoutSave(int32_t slotId, const u16string desAddr, const u16string scAddr,
    const u16string text, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    return SendMessage(slotId, desAddr, scAddr, text, sendCallback, deliveryCallback);
}

void SmsService::InsertSessionAndDetail(int32_t slotId, const std::string &telephone, const std::string &text,
    int32_t &dataBaseId)
{
    uint16_t sessionId = 0;
    uint16_t messageCount = 0;
    bool ret = QuerySessionByTelephone(telephone, sessionId, messageCount);
    if (ret) {
        InsertSmsMmsInfo(slotId, sessionId, telephone, text, dataBaseId);
        messageCount++;
        InsertSession(false, messageCount, telephone, text);
    } else {
        ret = InsertSession(true, 0, telephone, text);
        if (ret) {
            QuerySessionByTelephone(telephone, sessionId, messageCount);
            InsertSmsMmsInfo(slotId, sessionId, telephone, text, dataBaseId);
        }
    }
}

bool SmsService::QuerySessionByTelephone(const std::string &telephone, uint16_t &sessionId, uint16_t &messageCount)
{
    DataShare::DataSharePredicates predicates;
    auto persistHelper = DelayedSingleton<SmsPersistHelper>::GetInstance();
    UpdatePredicatesByPhoneNum(predicates, telephone);
    return persistHelper->QueryOneSessionByPhoneNum(predicates, sessionId, messageCount, telephone);
}

void SmsService::InsertSmsMmsInfo(
    int32_t slotId, uint16_t sessionId, const std::string &number, const std::string &text, int32_t &dataBaseId)
{
    DataShare::DataSharePredicates predicates;
    uint16_t maxGroupId = 0;
    DelayedSingleton<SmsPersistHelper>::GetInstance()->QueryMaxGroupId(predicates, maxGroupId);
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    DataShare::DataShareValuesBucket smsMmsInfoBucket;
    smsMmsInfoBucket.Put(SmsMmsInfo::SLOT_ID, std::to_string(slotId));
    smsMmsInfoBucket.Put(SmsMmsInfo::RECEIVER_NUMBER, number);
    smsMmsInfoBucket.Put(SmsMmsInfo::SENDER_NUMBER, "");
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_SENDER, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::START_TIME, std::to_string(duration.count()));
    smsMmsInfoBucket.Put(SmsMmsInfo::END_TIME, std::to_string(duration.count()));
    smsMmsInfoBucket.Put(SmsMmsInfo::MSG_TYPE, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::SMS_TYPE, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::MSG_TITLE, text);
    smsMmsInfoBucket.Put(SmsMmsInfo::MSG_CONTENT, text);
    smsMmsInfoBucket.Put(SmsMmsInfo::MSG_STATE, SMS_MMS_INFO_MSG_STATE_SENDING);
    smsMmsInfoBucket.Put(SmsMmsInfo::MSG_CODE, "");
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_LOCK, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_READ, "1");
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_COLLECT, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::SESSION_TYPE, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::SESSION_ID, std::to_string(sessionId));
    smsMmsInfoBucket.Put(SmsMmsInfo::GROUP_ID, std::to_string(maxGroupId + 1));
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_SUBSECTION, "0");
    smsMmsInfoBucket.Put(SmsMmsInfo::IS_SEND_REPORT, "0");
    DelayedSingleton<SmsPersistHelper>::GetInstance()->Insert(SMS_MMS_INFO, smsMmsInfoBucket, dataBaseId);
}


bool SmsService::InsertSession(
    bool isNewSession, uint16_t messageCount, const std::string &number, const std::string &text)
{
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    DataShare::DataShareValuesBucket sessionBucket;
    sessionBucket.Put(Session::TIME, std::to_string(duration.count()));
    sessionBucket.Put(Session::TELEPHONE, number);
    sessionBucket.Put(Session::CONTENT, text);
    sessionBucket.Put(Session::CONTACTS_NUM, "1");
    sessionBucket.Put(Session::SMS_TYPE, "0");
    sessionBucket.Put(Session::UNREAD_COUNT, "0");
    sessionBucket.Put(Session::SENDING_STATUS, "0");
    sessionBucket.Put(Session::HAS_DRAFT, "0");
    sessionBucket.Put(Session::HAS_LOCK, "0");
    sessionBucket.Put(Session::HAS_MMS, "0");
    sessionBucket.Put(Session::HAS_ATTACHMENT, "0");
    if (!isNewSession) {
        sessionBucket.Put(Session::MESSAGE_COUNT, std::to_string(messageCount));
        DataShare::DataSharePredicates predicates;
        UpdatePredicatesByPhoneNum(predicates, number);
        uint16_t sessionId;
        DataShare::DataSharePredicates predicatesNew;
        if (DelayedSingleton<SmsPersistHelper>::GetInstance()->
            QueryOneSessionByPhoneNum(predicates, sessionId, messageCount, number)) {
            predicatesNew.EqualTo(Session::ID, std::to_string(sessionId));
            return DelayedSingleton<SmsPersistHelper>::GetInstance()->Update(predicatesNew, sessionBucket);
        }
    }
    sessionBucket.Put(Session::MESSAGE_COUNT, "1");
    return DelayedSingleton<SmsPersistHelper>::GetInstance()->Insert(SMS_SESSION, sessionBucket);
}

int32_t SmsService::SendMessage(int32_t slotId, const u16string desAddr, const u16string scAddr, uint16_t port,
    const uint8_t *data, uint16_t dataLen, const sptr<ISendShortMessageCallback> &sendCallback,
    const sptr<IDeliveryShortMessageCallback> &deliveryCallback)
{
    if (SmsPolicyUtils::IsSmsPolicyDisable()) {
        TELEPHONY_LOGE("SmsService::SendMessage sms policy is disabled");
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    if (!CheckSmsPermission(sendCallback)) {
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_PERMISSION_ERROR, Permission::SEND_MESSAGES);
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (desAddr.empty()) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("SmsService::SendMessage desAddr empty");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("SmsService::SendMessage interfaceManager nullptr error.");
        SmsHiSysEvent::WriteSmsSendFaultEvent(slotId, SmsMmsMessageType::SMS_SHORT_MESSAGE,
            SmsMmsErrorCode::SMS_ERROR_NULL_POINTER, "data sms interfaceManager is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (!ValidDestinationAddress(StringUtils::ToUtf8(desAddr))) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("SmsService::SendMessage desAddr not conform to the regular specification");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    bool ret = interfaceManager->DataBasedSmsDelivery(
        StringUtils::ToUtf8(desAddr), StringUtils::ToUtf8(scAddr), port, data, dataLen, sendCallback, deliveryCallback);
    DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateContact(StringUtils::ToUtf8(desAddr));
    return ret;
}

bool SmsService::CheckSmsPermission(const sptr<ISendShortMessageCallback> &sendCallback)
{
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        SmsSender::SendResultCallBack(sendCallback, ISendShortMessageCallback::SEND_SMS_FAILURE_UNKNOWN);
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return false;
    }
    return true;
}

int32_t SmsService::IsImsSmsSupported(int32_t slotId, bool &isSupported)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager is nullptr.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->IsImsSmsSupported(slotId, isSupported);
}

int32_t SmsService::GetImsShortMessageFormat(std::u16string &format)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::GET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Get State Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    int32_t slotId = GetDefaultSmsSlotId();
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager is nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->GetImsShortMessageFormat(format);
}

int32_t SmsService::GetSmsShortCodeType(int32_t slotId, const std::string &desAddr, int32_t &smsShortCodeType)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager is nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->GetSmsShortCodeType(slotId, desAddr, smsShortCodeType);
}

bool SmsService::HasSmsCapability()
{
    int32_t slotId = GetDefaultSmsSlotId();
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("Get SmsInterfaceManager is nullptr error.");
        return true;
    }
    return interfaceManager->HasSmsCapability();
}

int32_t SmsService::SetSmscAddr(int32_t slotId, const std::u16string &scAddr)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Set State Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager is nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    string sca = StringUtils::ToUtf8(scAddr);
    TrimSmscAddr(sca);
    if (sca.empty() || sca.length() == 0) {
        TELEPHONY_LOGE("sca is empty");
        return TELEPHONY_ERR_ARGUMENT_NULL;
    }
    return interfaceManager->SetSmscAddr(sca);
}

void SmsService::TrimSmscAddr(std::string &sca)
{
    if (sca.length() < MIN_LEN) {
        return;
    }
    while (sca.length() >= MIN_LEN && sca[0] == ' ') {
        sca.erase(0, 1);
    }
    while (sca.length() > MIN_LEN && sca[sca.length() - 1] == ' ') {
        sca.erase(sca.length() - 1, 1);
    }
}

int32_t SmsService::GetSmscAddr(int32_t slotId, std::u16string &smscAddress)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::GET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Get State Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::GetSmscAddr interfaceManager nullptr error");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->GetSmscAddr(smscAddress);
}

int32_t SmsService::AddSimMessage(
    int32_t slotId, const std::u16string &smsc, const std::u16string &pdu, SimMessageStatus status)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Receive Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::AddSimMessage interfaceManager nullptr error");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    std::string smscData = StringUtils::ToUtf8(smsc);
    std::string pduData = StringUtils::ToUtf8(pdu);
    if (pdu.empty() || pduData.empty()) {
        TELEPHONY_LOGE("SmsService::AddSimMessage pdu empty error");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return interfaceManager->AddSimMessage(smscData, pduData, status);
}

int32_t SmsService::DelSimMessage(int32_t slotId, uint32_t msgIndex)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Receive Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::DelSimMessage interfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    if (!CheckSimMessageIndexValid(slotId, msgIndex)) {
        TELEPHONY_LOGE("SmsService::DelSimMessage msgIndex inValid");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }

    return interfaceManager->DelSimMessage(msgIndex);
}

int32_t SmsService::UpdateSimMessage(int32_t slotId, uint32_t msgIndex, SimMessageStatus newStatus,
    const std::u16string &pdu, const std::u16string &smsc)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Receive Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::UpdateSimMessage interfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    std::string pduData = StringUtils::ToUtf8(pdu);
    std::string smscData = StringUtils::ToUtf8(smsc);
    if (!CheckSimMessageIndexValid(slotId, msgIndex)) {
        TELEPHONY_LOGE("SmsService::UpdateSimMessage msgIndex inValid");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (pdu.empty() || pduData.empty()) {
        TELEPHONY_LOGE("SmsService::UpdateSimMessage pdu empty error");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return interfaceManager->UpdateSimMessage(msgIndex, newStatus, pduData, smscData);
}

int32_t SmsService::GetAllSimMessages(int32_t slotId, std::vector<ShortMessage> &message)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Receive Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::GetAllSimMessages interfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->GetAllSimMessages(message);
}

bool SmsService::CheckSimMessageIndexValid(int32_t slotId, uint32_t msgIndex)
{
    std::vector<ShortMessage> totalMessages;
    int32_t result = GetAllSimMessages(slotId, totalMessages);
    if (result != TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGE("SmsService::CheckSimMessageIndexValid get result fail");
        return false;
    }

    if (msgIndex < 0 || totalMessages.size() == 0) {
        TELEPHONY_LOGE("SmsService::CheckSimMessageIndexValid msgIndex error");
        return false;
    }
    for (auto message : totalMessages) {
        if (message.GetIndexOnSim() == static_cast<int32_t>(msgIndex)) {
            return true;
        }
    }
    TELEPHONY_LOGI("SmsService::CheckSimMessageIndexValid msgIndex not founded");
    return false;
}

int32_t SmsService::SetCBConfig(int32_t slotId, bool enable, uint32_t fromMsgId, uint32_t toMsgId, uint8_t netType)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Receive Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::SetCBConfig interfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->SetCBConfig(enable, fromMsgId, toMsgId, netType);
}

int32_t SmsService::SetCBConfigList(int32_t slotId, const std::vector<int32_t>& messageIds, int32_t ranType)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (!IsValidCBRangeList(messageIds)) {
        TELEPHONY_LOGE("CB parameter is invalid.");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("InterfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    return interfaceManager->SetCBConfigList(messageIds, ranType);
}

bool SmsService::IsValidCBRangeList(const std::vector<int32_t>& messageIds)
{
    if (messageIds.empty() || messageIds.size() % CB_RANGE_PAIR_SIZE != 0) {
        return false;
    }
    for (size_t i = 0; i < messageIds.size(); i += CB_RANGE_PAIR_SIZE) {
        if (i + 1 < messageIds.size() && messageIds[i + 1] < messageIds[i]) {
            return false;
        }
    }
    return true;
}

bool SmsService::SetImsSmsConfig(int32_t slotId, int32_t enable)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return false;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check permission failed.");
        return false;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::SetImsSmsConfig interfaceManager nullptr error.");
        return false;
    }
    return interfaceManager->SetImsSmsConfig(slotId, enable);
}

int32_t SmsService::SetDefaultSmsSlotId(int32_t slotId)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Set State Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager();
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::SetDefaultSmsSlotId interfaceManager nullptr error.");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return interfaceManager->SetDefaultSmsSlotId(slotId);
}

int32_t SmsService::GetDefaultSmsSlotId()
{
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager();
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::GetDefaultSmsSlotId interfaceManager nullptr error.");
        return TELEPHONY_ERROR;
    }
    return interfaceManager->GetDefaultSmsSlotId();
}

int32_t SmsService::GetDefaultSmsSimId(int32_t &simId)
{
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager();
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr error.");
        return TELEPHONY_ERROR;
    }
    return interfaceManager->GetDefaultSmsSimId(simId);
}

int32_t SmsService::SplitMessage(const std::u16string &message, std::vector<std::u16string> &splitMessage)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Send Messages Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }

    if (message.empty()) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    int32_t slotId = GetDefaultSmsSlotId();
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("SmsService::SplitMessage interfaceManager nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    std::string messageData = StringUtils::ToUtf8(message);
    return interfaceManager->SplitMessage(messageData, splitMessage);
}

int32_t SmsService::GetSmsSegmentsInfo(
    int32_t slotId, const std::u16string &message, bool force7BitCode, ISmsServiceInterface::SmsSegmentsInfo &info)
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::GET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Check Permission Failed, No Has Telephony Get State Permisson.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (message.empty()) {
        return TELEPHONY_ERR_ARGUMENT_NULL;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("GetSmsSegmentsInfo interfaceManager is nullptr error.");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    std::string messageData = StringUtils::ToUtf8(message);
    LengthInfo result;
    int32_t ret = interfaceManager->GetSmsSegmentsInfo(messageData, force7BitCode, result);
    if (ret != TELEPHONY_ERR_SUCCESS) {
        return ret;
    }
    info.msgSegCount = static_cast<int32_t>(result.msgSegCount);
    info.msgEncodingCount = static_cast<int32_t>(result.msgEncodeCount);
    info.msgRemainCount = static_cast<int32_t>(result.msgRemainCount);
    SmsSegmentsInfo::SmsSegmentCodeScheme dcs = static_cast<SmsSegmentsInfo::SmsSegmentCodeScheme>(result.dcs);
    switch (dcs) {
        case SmsSegmentsInfo::SmsSegmentCodeScheme::SMS_ENCODING_7BIT:
        case SmsSegmentsInfo::SmsSegmentCodeScheme::SMS_ENCODING_8BIT:
        case SmsSegmentsInfo::SmsSegmentCodeScheme::SMS_ENCODING_16BIT:
            info.msgCodeScheme = dcs;
            break;
        default:
            info.msgCodeScheme = SmsSegmentsInfo::SmsSegmentCodeScheme::SMS_ENCODING_UNKNOWN;
            break;
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t SmsService::GetServiceRunningState()
{
    return static_cast<int32_t>(state_);
}

int64_t SmsService::GetEndTime()
{
    return endTime_;
}

bool SmsService::ValidDestinationAddress(std::string desAddr)
{
    // Allow address start with '+' and number, Address length range 3 to 20
    std::regex regexMode("^([0-9_+]{1})([0-9]{2,19})$");
    if (desAddr.empty()) {
        return false;
    }
    return std::regex_match(desAddr, regexMode);
}

int64_t SmsService::GetSpendTime()
{
    spendTime_ = endTime_ - bindTime_;
    return spendTime_;
}

int32_t SmsService::CreateMessage(std::string pdu, std::string specification, ShortMessage &message)
{
    std::shared_ptr<SmsBaseMessage> baseMessage;
    if (specification == "3gpp") {
        baseMessage = GsmSmsMessage::CreateMessage(pdu);
    } else if (specification == "3gpp2") {
        baseMessage = CdmaSmsMessage::CreateMessage(pdu);
    }

    if (baseMessage == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    message.visibleMessageBody_ = StringUtils::ToUtf16(baseMessage->GetVisibleMessageBody());
    message.visibleRawAddress_ = StringUtils::ToUtf16(baseMessage->GetVisibleOriginatingAddress());
    message.messageClass_ = static_cast<ShortMessage::SmsMessageClass>(baseMessage->GetMessageClass());
    message.scAddress_ = StringUtils::ToUtf16(baseMessage->GetSmscAddr());
    message.scTimestamp_ = baseMessage->GetScTimestamp();
    message.isReplaceMessage_ = baseMessage->IsReplaceMessage();
    message.status_ = baseMessage->GetStatus();
    message.isSmsStatusReportMessage_ = baseMessage->IsSmsStatusReportMessage();
    message.hasReplyPath_ = baseMessage->HasReplyPath();
    message.protocolId_ = baseMessage->GetProtocolId();
    message.pdu_ = baseMessage->GetRawPdu();
    message.rawUserData_ = baseMessage->GetRawUserData();
    return TELEPHONY_ERR_SUCCESS;
}

bool SmsService::GetBase64Encode(std::string src, std::string &dest)
{
    auto results = Base64::Encode(std::vector<unsigned char>(src.begin(), src.end()));
    if (results == nullptr) {
        TELEPHONY_LOGE("Base64 encoding failed: nullptr returned");
        return false;
    }
    dest = *results;
    return true;
}

bool SmsService::GetBase64Decode(std::string src, std::string &dest)
{
    auto results = Base64::Decode(src);
    if (results == nullptr) {
        TELEPHONY_LOGE("Base64 decoding failed: empty vector returned");
        return false;
    }
    dest = std::string(results->begin(), results->end());
    return true;
}

bool SmsService::GetEncodeStringFunc(
    std::string &encodeString, uint32_t charset, uint32_t valLength, std::string strEncodeString)
{
    TextCoder::Instance().GetEncodeString(encodeString, charset, valLength, strEncodeString);
    return true;
}

int32_t SmsService::SendMms(int32_t slotId, const std::u16string &mmsc, const std::u16string &data,
    const std::u16string &ua, const std::u16string &uaprof, int64_t &time, bool isMmsApp)
{
    const int32_t permissionCheck = CheckMmsPermissions();
    if (permissionCheck != TELEPHONY_ERR_SUCCESS) {
        return permissionCheck;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    if (mmsc.empty()) {
        TELEPHONY_LOGE("mmsc URL is empty");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (data.empty()) {
        TELEPHONY_LOGE("mms pdu file is empty");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    int32_t dataBaseId = 0;
    if (isMmsApp) {
        dataBaseId = QueryMmsDatabaseId(slotId, time);
    }
    DataShare::DataShareValuesBucket sessionBucket;
    int32_t ret = interfaceManager->SendMms(mmsc, data, ua, uaprof);
    std::string  sendStatus = SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_FAILED;
    if (ret == TELEPHONY_ERR_SUCCESS) {
        sessionBucket.Put(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MSM_STATUS_SUCCEED);
        sendStatus = SmsMmsCommonData::SMS_MMS_INFO_MSG_STATE_SUCCEED;
    } else {
        sessionBucket.Put(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MSM_STATUS_FAILED);
    }
    if (isMmsApp) {
        ServiceAfterSendMmsComplete(slotId, time, dataBaseId, sessionBucket, sendStatus);
    }
    return ret;
}

int32_t SmsService::CheckMmsPermissions()
{
    if (SmsPolicyUtils::IsMmsPolicyDisable()) {
        TELEPHONY_LOGE("mms policy is disabled");
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::SEND_MESSAGES)) {
        TELEPHONY_LOGE("check permission failed");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t SmsService::QueryMmsDatabaseId(int32_t slotId, int64_t time)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SmsMmsInfo::MSG_TYPE, SmsMmsCommonData::SMS_MSM_TYPE_MMS);
    predicates.EqualTo(SmsMmsInfo::MSG_STATE, SmsMmsCommonData::SMS_MSM_STATUS_SENDING);
    predicates.EqualTo(SmsMmsInfo::SLOT_ID, slotId);
    predicates.LessThanOrEqualTo(SmsMmsInfo::START_TIME, time);
    predicates.OrderByDesc(SmsMmsInfo::START_TIME);
    int32_t dataBaseId = 0;
    DelayedSingleton<SmsPersistHelper>::GetInstance()->QuerySmsMmsForId(predicates, dataBaseId);
    TELEPHONY_LOGI("slot:%{public}d;;time:%{public}s;id:%{public}d",
        slotId, std::to_string(time).c_str(), dataBaseId);
    return dataBaseId;
}

void SmsService::ServiceAfterSendMmsComplete(int32_t slotId, int64_t &time, int32_t &dataBaseId,
    DataShare::DataShareValuesBucket &sessionBucket, std::string  &sendStatus)
{
    if (0 >= dataBaseId) {
        TELEPHONY_LOGE("SmsService::SendMms. slot:%{public}d;time:%{public}s",
            slotId, std::to_string(time).c_str());
    } else {
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(SmsMmsInfo::MSG_ID, dataBaseId);
        if (!DelayedSingleton<SmsPersistHelper>::GetInstance()->UpdateSms(predicates, sessionBucket)) {
            TELEPHONY_LOGE("SmsService::SendMms. Failed UpdateSms;dataBaseId:%{public}d;",  dataBaseId);
        }
        TELEPHONY_LOGI("before send boradcast. SmsService::SendMms %{public}d", dataBaseId);
        DelayedSingleton<SmsMmsCommon>::GetInstance()->SendBroadcast(dataBaseId,
            SmsMmsCommonData::SMS_MMS_SENT_RESULT_NOTIFY, sendStatus,
            SmsMmsCommonData::SMS_MMS_INFO_MMS_TYPE);
    }
    return;
}

int32_t SmsService::DownloadMms(int32_t slotId, const std::u16string &mmsc, std::u16string &data,
    const std::u16string &ua, const std::u16string &uaprof)
{
    if (SmsPolicyUtils::IsMmsPolicyDisable()) {
        TELEPHONY_LOGE("SmsService::DownloadMms mms policy is disable");
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(Permission::RECEIVE_MMS)) {
        TELEPHONY_LOGE("check permission failed");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr error");
        return TELEPHONY_ERR_SLOTID_INVALID;
    }
    if (mmsc.empty()) {
        TELEPHONY_LOGE("mmsc URL is empty");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TELEPHONY_LOGI("download mms slotId:%{public}d", slotId);
    int32_t ret = interfaceManager->DownloadMms(mmsc, data, ua, uaprof);
    if (ret == TELEPHONY_ERR_SUCCESS) {
        TELEPHONY_LOGI("down mms successed");
        return TELEPHONY_ERR_SUCCESS;
    } else {
        TELEPHONY_LOGI("down mms failed");
        return ret;
    }
}

int32_t SmsService::OnRilAdapterHostDied(int32_t slotId)
{
    std::shared_ptr<SmsInterfaceManager> interfaceManager = GetSmsInterfaceManager(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager is nullptr error.");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    interfaceManager->OnRilAdapterHostDied();
    return TELEPHONY_ERR_SUCCESS;
}

bool SmsService::IsInfoMsg(const std::string &telephone)
{
    std::regex regex(INFO_MSG_TELEPHONE_REG);
    if (telephone.empty()) {
        return false;
    }
    return std::regex_match(telephone, regex);
}

void SmsService::UpdatePredicatesByPhoneNum(DataShare::DataSharePredicates &predicates, const std::string &phoneNum)
{
    // 如果尾数小于等于7位，直接全等对比；群聊也直接全等对比；通知消息也做全等对比
    if (phoneNum.size() <= 7 || phoneNum.find(',') != std::string::npos || IsInfoMsg(phoneNum)) {
        predicates.EqualTo(Session::TELEPHONE, phoneNum);
    } else {
        std::string formatNum;
        auto persistHelper = DelayedSingleton<SmsPersistHelper>::GetInstance();
        int32_t ret = persistHelper->FormatSmsNumber(
            phoneNum, ISO_COUNTRY_CODE, i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::NATIONAL, formatNum);
        if (ret != TELEPHONY_SUCCESS) {
            ret = persistHelper->FormatSmsNumber(
                phoneNum, ISO_COUNTRY_CODE, i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::E164, formatNum);
        }
        if (ret != TELEPHONY_SUCCESS) {
            formatNum = phoneNum;
        }
        // 增加contactsNum字段的判断，防止单聊通过endsWith匹配到群聊。
        std::string normalizedNum = formatNum;
        size_t firstNonZero = normalizedNum.find_first_not_of('0');
        if (firstNonZero != string::npos) {
            // 从第一个非0字符开始截取
            normalizedNum = normalizedNum.substr(firstNonZero);
        } else {
            // 如果全是0，则保留一个0
            normalizedNum = "0";
        }
        predicates.In(Session::CONTACTS_NUM, std::vector<string>({ "0", "1" }));
        predicates.And();
        predicates.EndsWith(Session::TELEPHONE, normalizedNum);
    }
}
} // namespace Telephony
} // namespace OHOS
