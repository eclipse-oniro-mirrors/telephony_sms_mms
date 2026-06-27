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

#include "wappushbuffer_fuzzer.h"

#ifdef GTEST_API_
#define private public
#endif

#include "addsmstoken_fuzzer.h"
#include "sms_wap_push_handler.h"

using namespace OHOS::Telephony;
namespace OHOS {
static int32_t SIM_COUNT = 2;

void WapPushBuffer(const uint8_t *data, size_t size)
{
    int32_t slotId = static_cast<int32_t>(size % SIM_COUNT);
    std::string strValue(reinterpret_cast<const char *>(data), size);
    std::unique_ptr<SmsWapPushHandler> smsWapPushHandler = std::make_unique<SmsWapPushHandler>(slotId);
    auto indexer = std::make_shared<SmsReceiveIndexer>();
    smsWapPushHandler->DecodeWapPushPdu(indexer, strValue);

    auto decodeBuffer = std::make_shared<SmsWapPushBuffer>();
    if (decodeBuffer == nullptr) {
        return;
    }
    uint32_t desLen = static_cast<uint32_t>(size);
    decodeBuffer->ReadDataBuffer(desLen);

    std::unique_ptr<char[]> inBuff = std::make_unique<char[]>(desLen);
    decodeBuffer->WriteDataBuffer(std::move(inBuff), desLen);
    decodeBuffer->GetCurPosition();
    decodeBuffer->GetSize();

    uint8_t uint8tValue;
    decodeBuffer->PeekOneByte(uint8tValue);
    decodeBuffer->IncreasePointer(desLen);
    decodeBuffer->DecreasePointer(desLen);
    decodeBuffer->DecodeUintvar(desLen, desLen);

    decodeBuffer->DecodeShortLength(uint8tValue);
    decodeBuffer->DecodeValueLength(desLen);
    decodeBuffer->CharIsToken(uint8tValue);
    decodeBuffer->DecodeTokenText(strValue, desLen);
    decodeBuffer->DecodeText(strValue, desLen);

    decodeBuffer->DecodeQuotedText(strValue, desLen);
    decodeBuffer->DecodeShortInteger(uint8tValue);

    uint64_t uint64tValue;
    decodeBuffer->DecodeLongInteger(uint64tValue);
    decodeBuffer->DecodeInteger(uint64tValue);

    decodeBuffer->DecodeIsShortInt();
    decodeBuffer->DecodeIsString();
    decodeBuffer->DecodeIsValueLength();
    decodeBuffer->DecodeExtensionMedia();
    decodeBuffer->DecodeConstrainedEncoding();

    bool isNoValue = slotId == 0 ? true : false;
    decodeBuffer->DecodeTextValue(strValue, isNoValue);
    decodeBuffer->DecodeNoValue(isNoValue);
    decodeBuffer->MarkPosition();
    decodeBuffer->UnMarkPosition();
}

void DoWapPushBufferWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    WapPushBuffer(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AddSmsTokenFuzzer token;
    OHOS::DoWapPushBufferWithMyAPI(data, size);
    return 0;
}
