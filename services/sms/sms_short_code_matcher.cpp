/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "sms_short_code_matcher.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>

#include "core_service_client.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"
#include "sms_service.h"

namespace fs = std::filesystem;
namespace OHOS {
namespace Telephony {
const char *SMS_SHORT_CODE_RULES_JSON_PATH = "/etc/telephony/sms_short_code_rules.json";

SmsShortCodeMatcher::SmsShortCodeMatcher()
{
    if (LoadShortCodeRulesFromJson(SMS_SHORT_CODE_RULES_JSON_PATH)) {
        loadFileSuccess_ = true;
        return;
    }
    TELEPHONY_LOGE("Failed to load short code rules");
}

std::string SmsShortCodeMatcher::RemovePlusSign(const std::string &addr)
{
    if (!addr.empty() && addr[0] == '+') {
        return addr.substr(1);
    }
    return addr;
}

bool SmsShortCodeMatcher::MatchesRegexList(const std::string &str, const std::vector<std::string> &regexList)
{
    for (const auto &regexStr : regexList) {
        std::regex pattern(regexStr);
        if (!str.empty() && std::regex_match(str, pattern)) {
            return true;
        }
    }
    return false;
}

bool SmsShortCodeMatcher::LoadShortCodeRulesFromJson(const std::string &jsonPath)
{
    fs::path absolutePath = fs::absolute(jsonPath);
    if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
        TELEPHONY_LOGE("Invalid file path: %s", absolutePath.c_str());
        return false;
    }
    std::ifstream file(absolutePath);
    if (!file.is_open()) {
        TELEPHONY_LOGE("Failed to open rules file: %s", absolutePath.c_str());
        return false;
    }
    nlohmann::json jsonFile;
    file >> jsonFile;
    if (file.fail()) {
        TELEPHONY_LOGE("Failed to read JSON file: %s", absolutePath.c_str());
        return false;
    }
    const bool hasValidRules = jsonFile.contains("countryShortCodeRules") &&
        jsonFile["countryShortCodeRules"].is_array();
    if (!hasValidRules) {
        return false;
    }
    for (const auto& ruleJson : jsonFile["countryShortCodeRules"]) {
        if (!ruleJson.contains("countryCode") || !ruleJson["countryCode"].is_string()) {
            TELEPHONY_LOGE("Missing countryCode in rule or countryCode is not a string type");
            continue;
        }
        std::string countryCode = ruleJson["countryCode"].get<std::string>();
        std::transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        ShortCodeRule rule;
        rule.countryCode = countryCode;
        rule.pattern = ruleJson.value("pattern", "");
        ParseCodeArray(ruleJson.value("premiumCodes", nlohmann::json::array()), rule.premiumCodes);
        ParseCodeArray(ruleJson.value("standardCodes", nlohmann::json::array()), rule.standardCodes);
        ParseCodeArray(ruleJson.value("freeCodes", nlohmann::json::array()), rule.freeCodes);
        countryShortCodeRules_[countryCode] = rule;
    }
    TELEPHONY_LOGI("Loaded %zu short code rules", countryShortCodeRules_.size());
    return true;
}

void SmsShortCodeMatcher::ParseCodeArray(const nlohmann::json &jsonArray, std::vector<std::string> &result)
{
    if (!jsonArray.is_array()) {
        return;
    }
    for (const auto &item : jsonArray) {
        if (item.is_string()) {
            result.push_back(item.get<std::string>());
        } else {
            std::string itemStr = item.dump();
            TELEPHONY_LOGE("Non-string element found in JSON array: %s", itemStr.c_str());
        }
    }
}

SmsShortCodeType SmsShortCodeMatcher::GetSmsShortCodeType(const int32_t slotId, const std::string &desAddr)
{
    if (desAddr.empty() || !loadFileSuccess_) {
        TELEPHONY_LOGE("Invalid destination address or failed to load short code rules");
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN;
    }
    std::string countryCode = "";
    GetCountryCode(slotId, countryCode);
    if (countryCode.empty()) {
        TELEPHONY_LOGE("Invalid country code");
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN;
    }

    SmsShortCodeType smsShortCodeType = MatchShortCodeType(countryCode, desAddr);
    TELEPHONY_LOGI("smsShortCodeType: %d", static_cast<int>(smsShortCodeType));
    return smsShortCodeType;
}

SmsShortCodeType SmsShortCodeMatcher::MatchShortCodeType(const std::string &countryCode, const std::string &desAddr)
{
    std::string processedAddr = RemovePlusSign(desAddr);
    if (processedAddr.empty()) {
        TELEPHONY_LOGE("Invalid address after processing");
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN;
    }

    auto it = countryShortCodeRules_.find(countryCode);
    if (it == countryShortCodeRules_.end()) {
        TELEPHONY_LOGE("No rules found for country code: %s", countryCode.c_str());
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_UNKNOWN;
    }

    const ShortCodeRule& rule = it->second;

    if (MatchesRegexList(processedAddr, rule.freeCodes) || MatchesRegexList(processedAddr, rule.standardCodes)) {
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_NOT_PREMIUM;
    }
    if (MatchesRegexList(processedAddr, rule.premiumCodes)) {
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_POSSIBLE_PREMIUM;
    }
    if (!MatchesRegexList(processedAddr, std::vector<std::string>{rule.pattern})) {
        return SmsShortCodeType::SMS_SHORT_CODE_TYPE_NOT_PREMIUM;
    }
    return SmsShortCodeType::SMS_SHORT_CODE_TYPE_POSSIBLE_PREMIUM;
}

bool SmsShortCodeMatcher::GetCountryCode(const int32_t &slotId, std::string &countryCode)
{
    if (slotId < 0 || slotId >= SIM_SLOT_COUNT_MD) {
        TELEPHONY_LOGE("Invalid slotId");
        countryCode = "";
        return false;
    }
    std::u16string countryCode16 = u"";
    GetCountryCodeFromNetwork(countryCode16);
    if (countryCode16.empty()) {
        TELEPHONY_LOGI("Network country code is empty");
        CoreServiceClient::GetInstance().GetISOCountryCodeForSim(slotId, countryCode16);
    }
    if (countryCode16.empty()) {
        TELEPHONY_LOGE("Both network and SIM country codes are empty, get country code failed");
        countryCode = "";
        return false;
    }

    countryCode = Str16ToStr8(countryCode16);
    std::transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
    TELEPHONY_LOGI("Got country code: %s", countryCode.c_str());
    return true;
}

bool SmsShortCodeMatcher::GetCountryCodeFromNetwork(std::u16string &countryCode16)
{
    int32_t primarySlotId = 0;
    CoreServiceClient::GetInstance().GetPrimarySlotId(primarySlotId);
    CoreServiceClient::GetInstance().GetIsoCountryCodeForNetwork(primarySlotId, countryCode16);
    if (countryCode16.empty()) {
        return false;
    }
    return true;
}
} // namespace Telephony
} // namespace OHOS