/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SMS_FUZZER_UTILS_H
#define SMS_FUZZER_UTILS_H

#include <cstddef>
#include <cstdint>
#include <string>

#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace SmsFuzz {
inline constexpr size_t MIN_INPUT_SIZE = 1;
inline constexpr size_t MAX_STRING_SIZE = 256;
inline constexpr size_t MAX_DATA_SIZE = 256;
inline constexpr int32_t MAX_SLOT_ID = 2;
inline constexpr int32_t INVALID_SLOT_ID = -1;
inline constexpr uint32_t MAX_SIM_STATUS = 8;
inline constexpr uint32_t MAX_MESSAGE_CLASS = 4;
inline constexpr uint32_t MAX_CHARSET = 0x800;
inline constexpr uint32_t CHARSET_BASE = 0x3A;
inline constexpr uint32_t MAX_IPC_ERROR = 8;
inline constexpr uint32_t MAX_RESULT_ERROR = 4;
inline constexpr int32_t MAX_SIM_MESSAGE_COUNT = 2;
inline constexpr int32_t MAX_SPLIT_COUNT = 4;
inline constexpr int32_t MAX_SEGMENT_COUNT = 8;
inline constexpr int32_t MAX_LIST_COUNT = 4;
inline constexpr int32_t REMOTE_CALL_SUCCESS = 0;
inline constexpr int32_t REMOTE_CALL_FAILURE = -1;
inline constexpr int32_t REPLY_RESULT_SUCCESS = 0;
inline constexpr int32_t REPLY_RESULT_FAILURE = -1;

// Raw bytes are returned on purpose: control characters, NUL and high-bit bytes must reach
// Base64, Quoted-Printable, URL, Hex and MMS header text parsers instead of being normalised
// into the printable ASCII range.
inline std::string ConsumeString(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeRandomLengthString(MAX_STRING_SIZE);
}

inline std::u16string ConsumeText(FuzzedDataProvider &fdp)
{
    const std::string raw = ConsumeString(fdp);
    std::u16string text;
    text.reserve(raw.size());
    for (char item : raw) {
        text.push_back(static_cast<char16_t>(static_cast<uint8_t>(item)));
    }
    return text;
}

inline int32_t ConsumeSlotId(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeIntegralInRange<int32_t>(INVALID_SLOT_ID, MAX_SLOT_ID);
}

inline uint32_t ConsumeCharset(FuzzedDataProvider &fdp)
{
    return CHARSET_BASE + fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_CHARSET);
}
} // namespace SmsFuzz
} // namespace OHOS
#endif // SMS_FUZZER_UTILS_H
