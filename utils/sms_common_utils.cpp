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

#include "sms_common_utils.h"

#include <ctime>
#include "securec.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
static constexpr uint8_t SMS_ENCODE_GSM_BIT = 7;
static constexpr uint8_t MAX_GSM_7BIT_DATA_LEN = 160;
static constexpr uint8_t SMS_BYTE_BIT = 8;
static constexpr uint16_t SEC_PER_HOUR = 3600;
static constexpr uint8_t BASE_GSM_YEAR = 100;
static constexpr uint8_t MAX_ABS_TIME_LEN = 32;
static constexpr uint8_t HEX_NUM_A = 0x0A;
static constexpr uint8_t HEX_NUM_B = 0x0B;
static constexpr uint8_t HEX_NUM_C = 0x0C;
static constexpr uint8_t MIN_PRINTABLE_CHAR = 32;
static constexpr uint8_t MAX_PRINTABLE_CHAR = 127;

uint16_t SmsCommonUtils::Pack7bitChar(
    const uint8_t *userData, uint16_t dataLen, uint8_t fillBits, uint8_t *packData, uint16_t packLen)
{
    uint16_t dstIdx = 0;
    if (userData == nullptr || packData == nullptr || dataLen > MAX_GSM_7BIT_DATA_LEN) {
        TELEPHONY_LOGE("userData error.");
        return dstIdx;
    }

    auto shift = fillBits;
    if (shift > 0) {
        dstIdx = 1;
    }
    uint16_t srcIdx = 0;
    while (srcIdx < dataLen && dstIdx < packLen) {
        if (shift == 0) {
            packData[dstIdx] = userData[srcIdx];
            shift = SMS_ENCODE_GSM_BIT;
            srcIdx++;
            dstIdx++;
            if (srcIdx >= dataLen) {
                break;
            }
        }
        if (shift > 1) {
            packData[dstIdx - 1] |= userData[srcIdx] << shift;
            packData[dstIdx] = userData[srcIdx] >> (SMS_BYTE_BIT - shift);
            srcIdx++;
            dstIdx++;
            shift--;
        } else if (shift == 1) {
            packData[dstIdx - 1] |= userData[srcIdx] << shift;
            shift--;
            srcIdx++;
        }
    }
    return dstIdx;
}

uint16_t SmsCommonUtils::Unpack7bitChar(
    const uint8_t *tpdu, uint16_t dataLen, uint8_t fillBits, uint8_t *unpackData, uint16_t unpackDataLen)
{
    uint16_t srcIdx = 0;
    uint16_t dstIdx = 0;
    auto shift = fillBits;
    if (unpackData == nullptr || tpdu == nullptr || dataLen == 0 || unpackDataLen == 0 || dataLen > unpackDataLen) {
        TELEPHONY_LOGE("userData error.");
        return dstIdx;
    }
    if (shift > 0) {
        srcIdx = 1;
    }
    for (; srcIdx < dataLen && dstIdx < unpackDataLen; dstIdx++) {
        if (shift == 0) {
            unpackData[dstIdx] = tpdu[srcIdx] & 0x7F;
            shift = SMS_ENCODE_GSM_BIT;
            srcIdx++;
            dstIdx++;
            if (dstIdx >= dataLen) {
                dstIdx--;
                break;
            }
        }
        if (shift > 0 && srcIdx < dataLen && dstIdx < unpackDataLen) {
            unpackData[dstIdx] =
                (static_cast<unsigned int>(tpdu[srcIdx - 1]) >> shift) + (tpdu[srcIdx] << (SMS_BYTE_BIT - shift));
            unpackData[dstIdx] &= 0x7F;
            shift--;
            if (shift > 0) {
                srcIdx++;
            }
        }
    }
    return dstIdx;
}

uint16_t SmsCommonUtils::Unpack7bitCharForCBPdu(
    const uint8_t *tpdu, uint16_t dataLen, uint8_t fillBits, uint8_t *unpackData, uint16_t unpackDataLen)
{
    uint16_t srcIdx = 0;
    uint16_t dstIdx = 0;
    auto shift = fillBits;
    if (unpackData == nullptr || tpdu == nullptr || dataLen == 0 || unpackDataLen == 0 || dataLen > unpackDataLen) {
        TELEPHONY_LOGE("userData error.");
        return dstIdx;
    }
    if (shift > 0) {
        srcIdx = 1;
    }
    for (; srcIdx < dataLen && dstIdx < unpackDataLen;) {
        if (shift == 0) {
            unpackData[dstIdx] = tpdu[srcIdx] & 0x7F;
            shift = SMS_ENCODE_GSM_BIT;
            srcIdx++;
            dstIdx++;
        }
        if (shift > 0 && srcIdx < dataLen && dstIdx < unpackDataLen) {
            unpackData[dstIdx] =
                (static_cast<unsigned int>(tpdu[srcIdx - 1]) >> shift) + (tpdu[srcIdx] << (SMS_BYTE_BIT - shift));
            unpackData[dstIdx] &= 0x7F;
            shift--;
            if (shift > 0) {
                srcIdx++;
            }
            dstIdx++;
        }
    }
    if (dstIdx >= unpackDataLen) {
        TELEPHONY_LOGE("dstIdx:%{public}d", dstIdx);
        return 0;
    }
    uint8_t value = 0;
    if (shift == 0) {
        value = tpdu[srcIdx] >> shift;
    } else if (srcIdx > 1) {
        value = tpdu[srcIdx - 1] >> shift;
    }
    if (value >= MIN_PRINTABLE_CHAR && value <= MAX_PRINTABLE_CHAR) {
        unpackData[dstIdx] = value;
        dstIdx++;
    }
    TELEPHONY_LOGI("dstIdx:%{public}d", dstIdx);
    return dstIdx;
}

uint8_t SmsCommonUtils::DigitToDtmfChar(const uint8_t c)
{
    if (c == '0') {
        return HEX_NUM_A;
    } else if (c == '*') {
        return HEX_NUM_B;
    } else if (c == '#') {
        return HEX_NUM_C;
    } else {
        return (c - '0');
    }
}

uint8_t SmsCommonUtils::DtmfCharToDigit(const uint8_t c)
{
    switch (c) {
        case HEX_NUM_B:
            return '*';
        case HEX_NUM_C:
            return '#';
        case HEX_NUM_A:
            return '0';
        default:
            return (c + '0');
    }
}

int64_t SmsCommonUtils::ConvertTime(const struct SmsTimeAbs &timeAbs)
{
    time_t rawtime;
    struct tm tmObj;
    if (memset_s(&tmObj, sizeof(struct tm), 0x00, sizeof(tm)) != EOK) {
        return time(nullptr);
    }
    tmObj.tm_year = (timeAbs.year + BASE_GSM_YEAR);
    tmObj.tm_mon = (timeAbs.month - 0x01);
    tmObj.tm_mday = timeAbs.day;
    tmObj.tm_hour = timeAbs.hour;
    tmObj.tm_min = timeAbs.minute;
    tmObj.tm_sec = timeAbs.second;
    tmObj.tm_isdst = 0;
    rawtime = mktime(&tmObj);
    GetDisplayTime(rawtime);
    rawtime -= (timeAbs.timeZone * (SEC_PER_HOUR / 0x04));
    GetDisplayTime(rawtime);
    /* timezone value is tiemzone + daylight. So should not add daylight */
    rawtime -= timezone;
    GetDisplayTime(rawtime);
    return rawtime;
}

void SmsCommonUtils::GetDisplayTime(const time_t &rawtime)
{
    struct tm tmObj;
    char displayTime[MAX_ABS_TIME_LEN];
    if (memset_s(&tmObj, sizeof(struct tm), 0x00, sizeof(tm)) != EOK) {
        TELEPHONY_LOGE("GetDisplayTime memset fail.");
        return;
    }

    if (memset_s(displayTime, sizeof(displayTime), 0x00, sizeof(displayTime)) != EOK) {
        TELEPHONY_LOGE("GetDisplayTime memset fail.");
        return;
    }

    localtime_r(&rawtime, &tmObj);
    if (strftime(displayTime, MAX_ABS_TIME_LEN, "%Y-%02m-%02d %T %z", &tmObj) <= 0) {
        TELEPHONY_LOGE("strftime error.");
        return;
    }
}
} // namespace Telephony
} // namespace OHOS
