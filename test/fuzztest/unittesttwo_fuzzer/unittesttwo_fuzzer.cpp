/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifdef FUZZTEST
#define private public
#define protected public
#endif

#include "unittesttwo_fuzzer.h"

#include "addsmstoken_fuzzer.h"
#include "gsm_cb_codec.h"
#include "mms_network_manager.h"
#include "mms_receive_manager.h"
#include "mms_send_manager.h"
#include "sms_interface_manager.h"
#include "string_utils.h"

namespace OHOS {
using namespace Telephony;

class Fuzzer {
public:
    Fuzzer(const uint8_t* data, size_t size):g_data(data), g_size(size) {}
    virtual ~Fuzzer() {};
    virtual void StartFuzzerTest() = 0;

protected:
    std::string GetString(size_t size = 0)
    {
        std::string str(reinterpret_cast<const char*>(g_data), g_size);
        if (size != 0 && size < str.size()) {
            str.resize(size);
        }
        return str;
    }
    size_t GetNumRandom(size_t minNum = 0, size_t RangeStart = 0)
    {
        return g_size % (++RangeStart) + minNum;
    }
    std::vector<unsigned char> GetPduVect()
    {
        std::string strData = GetString(50);
        std::vector<unsigned char> vectUnsignedChar(strData.begin(), strData.end());
        return vectUnsignedChar;
    }
    
protected:
    const uint8_t* g_data;
    const size_t  g_size;
    const int32_t g_slotId = 1;
};

class MmsNetworkManagerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~MmsNetworkManagerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestAcquireNetwork();
        TestGetOrCreateHttpClient();
        TestReleaseNetwork();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pMmsNetworkManager = std::make_unique<MmsNetworkManager>();
        return m_pMmsNetworkManager != nullptr;
    }
    void DestoryObject()
    {
        m_pMmsNetworkManager.reset();
    }
    void TestAcquireNetwork()
    {
        uint8_t requestId = static_cast<uint8_t>(GetNumRandom(0, 10));
        m_pMmsNetworkManager->AcquireNetwork(g_slotId, requestId);
    }
    void TestGetOrCreateHttpClient()
    {
        m_pMmsNetworkManager->GetOrCreateHttpClient(g_slotId);
    }
    void TestReleaseNetwork()
    {
        uint8_t requestId = static_cast<uint8_t>(GetNumRandom(0, 10));
        bool shouldRelease = false;
        m_pMmsNetworkManager->ReleaseNetwork(g_slotId, requestId, shouldRelease);
    }
private:
    std::unique_ptr<MmsNetworkManager> m_pMmsNetworkManager;
};

class MmsSendAndReceiveManagerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~MmsSendAndReceiveManagerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestSendMms();
        TestDownloadMms();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pMmsSendManager = std::make_unique<MmsSendManager>(g_slotId);
        m_pMmsReceiveManager = std::make_unique<MmsReceiveManager>(g_slotId);
        return m_pMmsSendManager != nullptr && m_pMmsReceiveManager != nullptr;
    }
    void DestoryObject()
    {
        m_pMmsSendManager.reset();
        m_pMmsReceiveManager.reset();
    }
    void TestSendMms()
    {
        std::u16string mmsc = StringUtils::ToUtf16(GetString(10));
        std::u16string data = StringUtils::ToUtf16(GetString(20));
        std::u16string ua = StringUtils::ToUtf16("ua");
        std::u16string uaprof = StringUtils::ToUtf16("uaprof");
        m_pMmsSendManager->SendMms(mmsc, data, ua, uaprof);
    }
    void TestDownloadMms()
    {
        std::u16string mmsc = StringUtils::ToUtf16(GetString(10));
        std::u16string data = StringUtils::ToUtf16(GetString(20));
        std::u16string ua = StringUtils::ToUtf16("ua");
        std::u16string uaprof = StringUtils::ToUtf16("uaprof");
        m_pMmsReceiveManager->DownloadMms(mmsc, data, ua, uaprof);
    }
private:
    std::unique_ptr<MmsSendManager> m_pMmsSendManager;
    std::unique_ptr<MmsReceiveManager> m_pMmsReceiveManager;
};

class GsmCbCodecFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~GsmCbCodecFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestOperator();
        TestEncodeCbSerialNum();
        TestCMASClass();
        TestGetSomething();
        TestISSomething();
        TestToString();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pGsmCbCodec = std::make_unique<GsmCbCodec>();
        return m_pGsmCbCodec != nullptr;
    }
    void DestoryObject()
    {
        m_pGsmCbCodec.reset();
    }
    void TestOperator()
    {
        GsmCbCodec first;
        GsmCbCodec second;
        first.ParamsCheck(GetPduVect());
        auto result = (second == first);
        second.ParamsCheck(GetPduVect());
        result = (second == first);
    }
    void TestCMASClass()
    {
        std::vector<GsmCbCodec::CmasMsgType> messageIds = {
            GsmCbCodec::CmasMsgType::PRESIDENTIAL_SPANISH,
            GsmCbCodec::CmasMsgType::EXTREME_LIKELY_SPANISH,
            GsmCbCodec::CmasMsgType::EXPECTED_LIKELY_SPANISH,
            GsmCbCodec::CmasMsgType::AMBER_ALERT_SPANISH,
            GsmCbCodec::CmasMsgType::MONTHLY_ALERT_SPANISH,
            GsmCbCodec::CmasMsgType::EXERCISE_ALERT_SPANISH,
            GsmCbCodec::CmasMsgType::OPERATOR_ALERT_SPANISH};
        for (auto messageId : messageIds) {
            m_pGsmCbCodec->CMASClass(static_cast<uint16_t>(messageId));
        }
    }
    void TestEncodeCbSerialNum()
    {
        GsmCbCodec::GsmCBMsgSerialNum serialNum;
        m_pGsmCbCodec->EncodeCbSerialNum(serialNum);
    }
    void TestGetSomething()
    {
        int8_t cbFormat = static_cast<int8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetFormat(cbFormat);
        int8_t cbPriority = static_cast<int8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetPriority(cbPriority);
        uint8_t geoScope = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetGeoScope(geoScope);
        uint16_t cbSerial = static_cast<uint16_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetSerialNum(cbSerial);
        uint16_t cbCategoty = static_cast<uint16_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetServiceCategory(cbCategoty);
        uint8_t severity = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasSeverity(severity);
        uint8_t urgency = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasUrgency(urgency);
        uint8_t certainty = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasCertainty(certainty);
        uint8_t cmasCategory = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasCategory(cmasCategory);
        uint8_t cmasRes = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasResponseType(cmasRes);
        uint16_t msgId = static_cast<uint16_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetMessageId(msgId);
        uint8_t cmasClass = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetCmasMessageClass(cmasClass);
        uint8_t msgType = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetMsgType(msgType);
        uint8_t lan = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetLangType(lan);
        uint8_t dcs = static_cast<uint8_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetDcs(dcs);
        int64_t receiveTime = static_cast<int64_t>(GetNumRandom(1, 10));
        m_pGsmCbCodec->GetReceiveTime(receiveTime);

        m_pGsmCbCodec->ParamsCheck(GetPduVect());

        m_pGsmCbCodec->GetPriority(cbPriority);
        m_pGsmCbCodec->GetGeoScope(geoScope);
        m_pGsmCbCodec->GetSerialNum(cbSerial);
        m_pGsmCbCodec->GetServiceCategory(cbCategoty);
        m_pGsmCbCodec->GetCmasSeverity(severity);
        m_pGsmCbCodec->GetCmasUrgency(urgency);
        m_pGsmCbCodec->GetCmasCertainty(certainty);
        m_pGsmCbCodec->GetMessageId(msgId);
        m_pGsmCbCodec->GetCmasMessageClass(cmasClass);
        m_pGsmCbCodec->GetMsgType(msgType);
        m_pGsmCbCodec->GetLangType(lan);
        m_pGsmCbCodec->GetDcs(dcs);
        m_pGsmCbCodec->GetReceiveTime(receiveTime);
    }
    void TestISSomething()
    {
        GsmCbCodec codec;
        bool result = false;
        codec.IsEtwsPrimary(result);
        codec.IsCmasMessage(result);
        codec.IsEtwsEmergencyUserAlert(result);
        codec.IsEtwsPopupAlert(result);

        codec.ParamsCheck(GetPduVect());

        codec.IsEtwsPrimary(result);
        codec.IsCmasMessage(result);
        codec.IsEtwsEmergencyUserAlert(result);
        codec.IsEtwsPopupAlert(result);
    }
    void TestToString()
    {
        GsmCbCodec codec;
        codec.ToString();

        codec.ParamsCheck(GetPduVect());

        codec.ToString();
    }

private:
    std::unique_ptr<GsmCbCodec> m_pGsmCbCodec;
};

class SmsInterfaceManagerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~SmsInterfaceManagerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestTextBasedSmsDelivery();
        TestDataBasedSmsDelivery();
        TestSimMessage();
        TestGetAndSet();
        TestSplitMessage();
        TestIsImsSmsSupported();
        TestOnRilAdapterHostDied();
        TestSendAndDownloadMms();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pSmsInterfaceManager = std::make_unique<SmsInterfaceManager>(g_slotId);
        return m_pSmsInterfaceManager != nullptr;
    }
    void DestoryObject()
    {
        m_pSmsInterfaceManager.reset();
    }
    void TestTextBasedSmsDelivery()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        std::string text = GetString(50);
        int32_t dataBaseId = static_cast<int32_t>(GetNumRandom(0, 10));
        m_pSmsInterfaceManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
    }
    void TestDataBasedSmsDelivery()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        uint16_t port = static_cast<uint16_t>(GetNumRandom(0, 10));
        uint16_t dataLen = static_cast<uint16_t>(g_size);
        m_pSmsInterfaceManager->DataBasedSmsDelivery(desAddr, scAddr, port, nullptr, dataLen, nullptr, nullptr);
        m_pSmsInterfaceManager->DataBasedSmsDelivery(desAddr, scAddr, port, g_data, dataLen, nullptr, nullptr);
    }
    void TestSimMessage()
    {
        std::string smsc = GetString(10);
        std::string pdu = GetString(50);
        ISmsServiceInterface::SimMessageStatus status =
            static_cast<ISmsServiceInterface::SimMessageStatus>(GetNumRandom(0, 3));
        m_pSmsInterfaceManager->AddSimMessage(smsc, pdu, status);
        uint32_t msgIndex = static_cast<uint32_t>(GetNumRandom(0, 5));
        m_pSmsInterfaceManager->DelSimMessage(msgIndex);
        m_pSmsInterfaceManager->UpdateSimMessage(msgIndex, status, pdu, smsc);
        std::vector<ShortMessage> message;
        m_pSmsInterfaceManager->GetAllSimMessages(message);
    }
    void TestGetAndSet()
    {
        std::string scAddr = GetString(10);
        std::u16string smscAddress = StringUtils::ToUtf16(GetString(10));
        m_pSmsInterfaceManager->SetSmscAddr(scAddr);
        m_pSmsInterfaceManager->GetSmscAddr(smscAddress);
        bool enable = static_cast<bool>(GetNumRandom(0, 1));
        uint32_t fromMsgId = static_cast<uint32_t>(GetNumRandom(0, 10));
        uint32_t toMsgId = static_cast<uint32_t>(GetNumRandom(0, 10));
        uint8_t netType = static_cast<uint8_t>(GetNumRandom(0, 10));
        m_pSmsInterfaceManager->SetCBConfig(enable, fromMsgId, toMsgId, netType);
        m_pSmsInterfaceManager->SetImsSmsConfig(g_slotId, fromMsgId);
        m_pSmsInterfaceManager->SetDefaultSmsSlotId(g_slotId);
        m_pSmsInterfaceManager->GetDefaultSmsSlotId();
        int32_t simId;
        m_pSmsInterfaceManager->GetDefaultSmsSimId(simId);
        std::string message = GetString();
        LengthInfo outInfo;
        m_pSmsInterfaceManager->GetSmsSegmentsInfo(message, enable, outInfo);
        std::u16string format = StringUtils::ToUtf16(GetString(100));
        m_pSmsInterfaceManager->GetImsShortMessageFormat(format);
    }

    void TestSplitMessage()
    {
        std::string message = GetString(100);
        std::vector<std::u16string> splitMessage;
        std::u16string uMessage = StringUtils::ToUtf16(GetString(10));
        splitMessage.emplace_back(uMessage);
        m_pSmsInterfaceManager->SplitMessage(message, splitMessage);
    }

    void TestIsImsSmsSupported()
    {
        bool isSupported = static_cast<bool>(GetNumRandom(0, 1));
        m_pSmsInterfaceManager->IsImsSmsSupported(g_slotId, isSupported);
    }

    void TestOnRilAdapterHostDied()
    {
        m_pSmsInterfaceManager->OnRilAdapterHostDied();
    }
    void TestSendAndDownloadMms()
    {
        std::u16string mmsc = StringUtils::ToUtf16(GetString(10));
        std::u16string data = StringUtils::ToUtf16(GetString(20));
        std::u16string ua = StringUtils::ToUtf16("ua");
        std::u16string uaprof = StringUtils::ToUtf16("uaprof");
        m_pSmsInterfaceManager->SendMms(mmsc, data, ua, uaprof);
        m_pSmsInterfaceManager->DownloadMms(mmsc, data, ua, uaprof);
    }
private:
    std::unique_ptr<SmsInterfaceManager> m_pSmsInterfaceManager;
};

class SmsMiscManagerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~SmsMiscManagerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestGetRangeInfo();
        TestSplit();
        TestIsEmpty();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pSmsMiscManager = std::make_unique<SmsMiscManager>(g_slotId);
        return m_pSmsMiscManager != nullptr;
    }
    void DestoryObject()
    {
        m_pSmsMiscManager.reset();
    }
    void TestGetRangeInfo()
    {
        m_pSmsMiscManager->GetRangeInfo();
    }
    void TestSplit()
    {
        uint32_t fromMsgId = static_cast<uint32_t>(GetNumRandom(0, 10));
        uint32_t toMsgId = static_cast<uint32_t>(GetNumRandom(0, 10));
        std::list<SmsMiscManager::gsmCBRangeInfo> listInfo;
        uint32_t fromId = static_cast<uint32_t>(GetNumRandom(0, 100));
        uint32_t toId = static_cast<uint32_t>(GetNumRandom(0, 100));
        SmsMiscManager::gsmCBRangeInfo info(fromId, toId);
        listInfo.emplace_back(info);
        m_pSmsMiscManager->SplitMsgId(fromMsgId, toMsgId, listInfo.begin());
        std::string src = GetString(100);
        std::vector<std::string> dest;
        std::string delimiter  = "FuzzerTest";
        src += delimiter;
        m_pSmsMiscManager->SplitMids(src, dest, delimiter);
        std::string value = GetString(100);
        std::string start;
        std::string end;
        m_pSmsMiscManager->SplitMidValue(value, start, end, delimiter);
        value = delimiter + value;
        m_pSmsMiscManager->SplitMidValue(value, start, end, delimiter);
        value.clear();
        m_pSmsMiscManager->SplitMidValue(value, start, end, delimiter);
    }
    void TestIsEmpty()
    {
        m_pSmsMiscManager->IsEmpty();
    }
private:
    std::unique_ptr<SmsMiscManager> m_pSmsMiscManager;
};

class SmsSendManagerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~SmsSendManagerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestTextBasedSmsDelivery();
        TestDataBasedSmsDelivery();
        TestRetriedSmsDelivery();
        TestSplitMessage();
        TestGetSmsSegmentsInfo();
        TestIsImsSmsSupported();
        TestSetImsSmsConfig();
        TestGetImsShortMessageFormat();
        TestOnRilAdapterHostDied();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        m_pSmsSendManager = std::make_unique<SmsSendManager>(g_slotId);
        return m_pSmsSendManager != nullptr;
    }
    void DestoryObject()
    {
        m_pSmsSendManager.reset();
    }
    void Reset(bool isNetwork)
    {
        if (isNetwork) {
            m_pSmsSendManager->networkManager_ = nullptr;
        }
        m_pSmsSendManager->gsmSmsSender_ = nullptr;
        m_pSmsSendManager->cdmaSmsSender_ = nullptr;
    }
    void TestTextBasedSmsDelivery()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        std::string text = GetString(50);
        int32_t dataBaseId = static_cast<int32_t>(GetNumRandom(0, 10));
        m_pSmsSendManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
        desAddr.clear();
        m_pSmsSendManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
        desAddr =  scAddr;
        m_pSmsSendManager->Init();
        m_pSmsSendManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
        Reset(false);
        m_pSmsSendManager->TextBasedSmsDelivery(desAddr, scAddr, text, nullptr, nullptr, dataBaseId);
        Reset(true);
    }
    void TestDataBasedSmsDelivery()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        uint16_t port = static_cast<uint16_t>(GetNumRandom(0, 10));
        uint16_t dataLen = static_cast<uint16_t>(g_size);
        m_pSmsSendManager->DataBasedSmsDelivery(desAddr, scAddr, port, g_data, dataLen, nullptr, nullptr);
        m_pSmsSendManager->DataBasedSmsDelivery(desAddr, scAddr, port, nullptr, dataLen, nullptr, nullptr);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->DataBasedSmsDelivery(desAddr, scAddr, port, g_data, dataLen, nullptr, nullptr);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->DataBasedSmsDelivery(desAddr, scAddr, port, g_data, dataLen, nullptr, nullptr);
        Reset(false);
        m_pSmsSendManager->DataBasedSmsDelivery(desAddr, scAddr, port, g_data, dataLen, nullptr, nullptr);
        Reset(true);
    }
    void TestRetriedSmsDelivery()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        std::string text = GetString(50);
        auto smsIndexer = std::make_shared<SmsSendIndexer>(desAddr, scAddr, text, nullptr, nullptr);
        m_pSmsSendManager->RetriedSmsDelivery(nullptr);
        m_pSmsSendManager->RetriedSmsDelivery(smsIndexer);
        m_pSmsSendManager->Init();
        smsIndexer->SetNetWorkType(NetWorkType::NET_TYPE_CDMA);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_GSM;
        m_pSmsSendManager->RetriedSmsDelivery(smsIndexer);
        smsIndexer->SetNetWorkType(NetWorkType::NET_TYPE_GSM);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->RetriedSmsDelivery(smsIndexer);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->RetriedSmsDelivery(smsIndexer);
        Reset(true);
    }
    void TestSplitMessage()
    {
        std::string message = GetString(100);
        std::vector<std::u16string> splitMessage;
        std::u16string uMessage = StringUtils::ToUtf16(GetString(10));
        splitMessage.emplace_back(uMessage);
        m_pSmsSendManager->SplitMessage(message, splitMessage);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->SplitMessage(message, splitMessage);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->SplitMessage(message, splitMessage);
        Reset(true);
    }
    void TestGetSmsSegmentsInfo()
    {
        std::string message = GetString();
        bool force7BitCode = static_cast<bool>(GetNumRandom(0, 1));
        LengthInfo outInfo;
        m_pSmsSendManager->GetSmsSegmentsInfo(message, force7BitCode, outInfo);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->GetSmsSegmentsInfo(message, force7BitCode, outInfo);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->GetSmsSegmentsInfo(message, force7BitCode, outInfo);
        Reset(true);
    }
    void TestIsImsSmsSupported()
    {
        bool isSupported = static_cast<bool>(GetNumRandom(0, 1));
        m_pSmsSendManager->IsImsSmsSupported(g_slotId, isSupported);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->IsImsSmsSupported(g_slotId, isSupported);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->IsImsSmsSupported(g_slotId, isSupported);
        Reset(true);
    }
    void TestSetImsSmsConfig()
    {
        int32_t enable = static_cast<int32_t>(GetNumRandom(0, 1));
        m_pSmsSendManager->SetImsSmsConfig(g_slotId, enable);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->SetImsSmsConfig(g_slotId, enable);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->SetImsSmsConfig(g_slotId, enable);
        Reset(true);
    }
    void TestGetImsShortMessageFormat()
    {
        std::u16string format = StringUtils::ToUtf16(GetString(10));
        m_pSmsSendManager->GetImsShortMessageFormat(format);
        m_pSmsSendManager->Init();
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_UNKNOWN;
        m_pSmsSendManager->GetImsShortMessageFormat(format);
        m_pSmsSendManager->networkManager_->netWorkType_ = NetWorkType::NET_TYPE_CDMA;
        m_pSmsSendManager->GetImsShortMessageFormat(format);
        Reset(true);
    }
    void TestOnRilAdapterHostDied()
    {
        m_pSmsSendManager->Init();
        m_pSmsSendManager->OnRilAdapterHostDied();
        Reset(true);
    }
private:
    std::unique_ptr<SmsSendManager> m_pSmsSendManager;
};

class SmsSendIndexerFuzzer final : public Fuzzer {
public:
    using Fuzzer::Fuzzer;
    ~SmsSendIndexerFuzzer() final {}
public:
    void StartFuzzerTest() override
    {
        if (!CreateObject()) {
            return;
        }
        TestSetAndGet();
        TestIsImsSmsForCdma();
        TestUpdatePduForResend();
        DestoryObject();
    }
protected:
    bool CreateObject()
    {
        std::string desAddr = GetString(10);
        std::string scAddr = GetString(10);
        std::string text = GetString(50);
        m_pSmsSendIndexer = std::make_unique<SmsSendIndexer>(desAddr, scAddr, text, nullptr, nullptr);
        return m_pSmsSendIndexer != nullptr;
    }
    void DestoryObject()
    {
        m_pSmsSendIndexer.reset();
    }
    void TestSetAndGet()
    {
        auto smca = static_cast<uint8_t>(GetNumRandom(0, 1));
        std::vector<uint8_t> smcas;
        smcas.emplace_back(smca);
        m_pSmsSendIndexer->SetEncodeSmca(smcas);
        bool isImsSms = static_cast<bool>(GetNumRandom(0, 1));
        m_pSmsSendIndexer->SetImsSmsForCdma(isImsSms);
        std::string destAddr = GetString(10);
        m_pSmsSendIndexer->SetDestAddr(destAddr);
        bool isFailure = static_cast<bool>(GetNumRandom(0, 1));
        m_pSmsSendIndexer->SetIsFailure(isFailure);
        std::string text = GetString(10);
        m_pSmsSendIndexer->SetText(text);
        m_pSmsSendIndexer->SetDeliveryCallback(nullptr);
        m_pSmsSendIndexer->SetSendCallback(nullptr);
        int16_t destPort = static_cast<int16_t>(GetNumRandom(0, 10));
        m_pSmsSendIndexer->SetDestPort(destPort);
        std::string smcaAddr = GetString(10);
        m_pSmsSendIndexer->SetSmcaAddr(smcaAddr);
        auto errorCode = static_cast<uint8_t>(GetNumRandom(0, 1));
        m_pSmsSendIndexer->SetErrorCode(errorCode);
        auto data = static_cast<uint8_t>(GetNumRandom(0, 100));
        std::vector<uint8_t> datum;
        datum.emplace_back(data);
        m_pSmsSendIndexer->SetData(datum);
        m_pSmsSendIndexer->SetData(std::move(datum));
        auto ackPdu = static_cast<uint8_t>(GetNumRandom(0, 100));
        std::vector<uint8_t> ackPduData;
        ackPduData.emplace_back(ackPdu);
        m_pSmsSendIndexer->SetAckPdu(ackPduData);
        m_pSmsSendIndexer->SetAckPdu(std::move(ackPduData));
        auto msgRefId64Bit = static_cast<int64_t>(GetNumRandom(0, 100));
        m_pSmsSendIndexer->SetMsgRefId64Bit(msgRefId64Bit);

        m_pSmsSendIndexer->GetPsResendCount();
        m_pSmsSendIndexer->GetMsgId();
        m_pSmsSendIndexer->GetIsText();
        m_pSmsSendIndexer->GetAckPdu();
        m_pSmsSendIndexer->GetTimeStamp();
    }
    void TestIsImsSmsForCdma()
    {
        m_pSmsSendIndexer->IsImsSmsForCdma();
    }
    void TestUpdatePduForResend()
    {
        m_pSmsSendIndexer->UpdatePduForResend();
        auto pdu = static_cast<uint8_t>(g_slotId);
        std::vector<uint8_t> pduData;
        pduData.emplace_back(pdu++);
        pduData.emplace_back(pdu);
        m_pSmsSendIndexer->SetEncodePdu(pduData);
        m_pSmsSendIndexer->UpdatePduForResend();
    }
private:
    std::unique_ptr<SmsSendIndexer> m_pSmsSendIndexer;
};
}  // namespace OHOS

template<typename TY>
void RunFuzzer(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    std::unique_ptr<OHOS::Fuzzer> p = std::make_unique<TY>(data, size);
    p->StartFuzzerTest();
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;

    RunFuzzer<OHOS::MmsNetworkManagerFuzzer>(data, size);
    RunFuzzer<OHOS::MmsSendAndReceiveManagerFuzzer>(data, size);
    RunFuzzer<OHOS::GsmCbCodecFuzzer>(data, size);
    RunFuzzer<OHOS::SmsInterfaceManagerFuzzer>(data, size);
    RunFuzzer<OHOS::SmsMiscManagerFuzzer>(data, size);
    RunFuzzer<OHOS::SmsSendManagerFuzzer>(data, size);
    RunFuzzer<OHOS::SmsSendIndexerFuzzer>(data, size);
    return 0;
}