/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "setgetcbconfig_fuzzer.h"

#ifdef GTEST_API_
#define private public
#endif

#include "addsmstoken_fuzzer.h"
#include "core_manager_inner.h"
#include "gsm_cb_gsm_codec.h"
#include "gsm_cb_umts_codec.h"
#include "sms_service.h"
#include "string_utils.h"

using namespace OHOS::Telephony;
namespace OHOS {
static bool g_isInited = false;
static int32_t SIM_COUNT = 2;
static int32_t CB_CHANNEL_DIVISOR = 2;
static int32_t NET_COUNT = 3;
constexpr int32_t SLEEP_TIME_SECONDS = 3;

bool IsServiceInited()
{
    if (!g_isInited) {
        CoreManagerInner::GetInstance().isInitAllObj_ = true;
        DelayedSingleton<SmsService>::GetInstance()->registerToService_ = true;
        DelayedSingleton<SmsService>::GetInstance()->WaitCoreServiceToInit();
        DelayedSingleton<SmsService>::GetInstance()->OnStart();
        if (DelayedSingleton<SmsService>::GetInstance()->GetServiceRunningState() ==
            static_cast<int32_t>(Telephony::ServiceRunningState::STATE_RUNNING)) {
            g_isInited = true;
            atexit([] {
                DelayedSingleton<SmsService>::GetInstance()->OnStop();
                _exit(0);
            });
        }
    }
    return g_isInited;
}

void SetCBConfigFuzz(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t slotId = static_cast<int32_t>(size % SIM_COUNT);
    bool enable = slotId == 1 ? true : false;
    uint32_t fromMsgId = static_cast<uint32_t>(size / CB_CHANNEL_DIVISOR);
    uint32_t toMsgId = static_cast<uint32_t>(size);
    int32_t netType = static_cast<int32_t>(size % NET_COUNT);

    dataParcel.WriteInt32(slotId);
    dataParcel.WriteBool(enable);
    dataParcel.WriteUint32(fromMsgId);
    dataParcel.WriteUint32(toMsgId);
    dataParcel.WriteUint8(netType);

    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnSetCBConfig(dataParcel, replyParcel, option);

    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr error");
        return;
    }
    interfaceManager->InitInterfaceManager();
    interfaceManager->SetCBConfig(enable, fromMsgId, toMsgId, netType);

    std::shared_ptr<SmsMiscManager> smsMiscManager = std::make_shared<SmsMiscManager>(slotId);
    if (smsMiscManager == nullptr) {
        TELEPHONY_LOGE("smsMiscManager nullptr error");
        return;
    }
    smsMiscManager->SetCBConfig(enable, fromMsgId, toMsgId, netType);
}

void SetImsSmsConfigFuzz(const uint8_t *data, size_t size)
{
    if (!IsServiceInited()) {
        return;
    }

    MessageParcel dataParcel;
    MessageParcel replyParcel;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t slotId = static_cast<int32_t>(size % SIM_COUNT);
    int32_t enable = slotId == 1 ? true : false;
    dataParcel.WriteInt32(slotId);
    dataParcel.WriteInt32(enable);
    dataParcel.WriteBuffer(data, size);
    dataParcel.RewindRead(0);
    DelayedSingleton<SmsService>::GetInstance()->OnSetImsSmsConfig(dataParcel, replyParcel, option);

    std::shared_ptr<SmsInterfaceManager> interfaceManager = std::make_shared<SmsInterfaceManager>(slotId);
    if (interfaceManager == nullptr) {
        TELEPHONY_LOGE("interfaceManager nullptr error");
        return;
    }
    interfaceManager->InitInterfaceManager();
    interfaceManager->SetImsSmsConfig(slotId, enable);

    auto smsSendManager = std::make_shared<SmsSendManager>(slotId);
    if (smsSendManager == nullptr) {
        return;
    }
    smsSendManager->Init();
    smsSendManager->SetImsSmsConfig(slotId, enable);
}

void UpdataCBMessage(const uint8_t *data, size_t size)
{
    std::string pdu(reinterpret_cast<const char *>(data), size);
    std::string gsmData("C00000324811006800610072006D006F006E00790020006F00730020005500"
                        "630073003200200065006E0063006F0064006500200064006100740061");
    auto cbMessage = GsmCbCodec::CreateCbMessage(gsmData);
    if (cbMessage == nullptr) {
        return;
    }
    cbMessage->GetCbHeader();
    cbMessage->PduAnalysis(StringUtils::HexToByteVector(pdu));
    std::string umtsData("01a41f51101102ea3030a830ea30a230e130fc30eb914d4fe130c630b930c8000"
                         "d000a3053308c306f8a669a137528306e30e130c330bb30fc30b8306730593002"
                         "000d000aff080032003000310033002f00310031002f003252ea3000370020003"
                         "10035003a00340034ff09000d000aff0830a830ea30a25e02ff09000000000000"
                         "00000000000000000000000000000000000000000000000000000000000000000"
                         "000000000000000000022");
    auto umtsCbMessage = GsmCbCodec::CreateCbMessage(umtsData);
    if (umtsCbMessage == nullptr) {
        return;
    }
    umtsCbMessage->GetCbHeader();
    umtsCbMessage->PduAnalysis(StringUtils::HexToByteVector(pdu));
    auto cbMessageByVectorInit = GsmCbCodec::CreateCbMessage(StringUtils::HexToByteVector(pdu));
    if (cbMessageByVectorInit == nullptr) {
        return;
    }
    cbMessageByVectorInit->GetCbMessageRaw();
    cbMessageByVectorInit->IsSinglePageMsg();

    auto gsmCodec = std::make_shared<GsmCbGsmCodec>(cbMessage->cbHeader_, cbMessage->cbPduBuffer_, cbMessage);
    auto umtsCodec =
        std::make_shared<GsmCbUmtsCodec>(umtsCbMessage->cbHeader_, umtsCbMessage->cbPduBuffer_, umtsCbMessage);
    if (gsmCodec == nullptr || umtsCodec == nullptr) {
        return;
    }
    gsmCodec->Decode2gHeader();
    umtsCodec->Decode3gHeader();

    gsmCodec->Decode2gCbMsg();
    umtsCodec->Decode3gCbMsg();
    umtsCodec->Decode3g7Bit();
    umtsCodec->Decode3gUCS2();
    gsmCodec->DecodeEtwsMsg();

    std::string raw(reinterpret_cast<const char *>(data), size);
    std::string message(reinterpret_cast<const char *>(data), size);
    cbMessage->ConvertToUTF8(raw, message);
}

void DoCBConfigWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    SetCBConfigFuzz(data, size);
    SetImsSmsConfigFuzz(data, size);
    UpdataCBMessage(data, size);
    DelayedSingleton<SmsService>::DestroyInstance();
    sleep(SLEEP_TIME_SECONDS);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoCBConfigWithMyAPI(data, size);
    return 0;
}
