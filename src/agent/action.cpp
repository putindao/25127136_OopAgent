// =============================================================================
//  agent/action.cpp — recover a structured Action from a free-form LLM reply.
// =============================================================================
#include "agent/action.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <nlohmann/json.hpp>

namespace agent {
namespace {

std::string to_lower(std::string s) {
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Extract the first complete top-level {...} object from arbitrary text. Braces
// inside double-quoted JSON strings are ignored, so prose or ``` fences around
// the object do not confuse the scan.
std::expected<std::string, std::string> extract_json_object(const std::string& text) {
    const std::size_t start = text.find('{');
    if (start == std::string::npos) {
        return std::unexpected("no JSON object found in model response");
    }
    int  depth     = 0;
    bool in_string = false;
    bool escaped   = false;
    for (std::size_t i = start; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            if (escaped)            escaped = false;
            else if (c == '\\')     escaped = true;
            else if (c == '"')      in_string = false;
        } else if (c == '"') {
            in_string = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            if (--depth == 0) return text.substr(start, i - start + 1);
        }
    }
    return std::unexpected("unbalanced JSON braces in model response");
}

}  // namespace

std::expected<Action, std::string> parse_action(const std::string& llm_text) {
    const auto json_str = extract_json_object(llm_text);
    if (!json_str) return std::unexpected(json_str.error());

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(*json_str);
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(std::string("invalid JSON action: ") + e.what());
    }

    const std::string thought = j.value("thought", std::string{});
    const std::string action  = j.value("action", std::string{});
    if (action.empty()) {
        return std::unexpected("action JSON is missing the 'action' field");
    }

    // action_input may legitimately be a string, a number, or a nested object;
    // normalise everything to a string the tool can consume.
    std::string action_input;
    if (j.contains("action_input")) {
        const auto& ai = j.at("action_input");
        action_input = ai.is_string() ? ai.get<std::string>() : ai.dump();
    }

    // A small set of synonyms all mean "stop and answer".
    const std::string verb = to_lower(action);
    if (verb == "final" || verb == "final_answer" || verb == "answer" ||
        verb == "finish" || verb == "done") {
        return Action{FinalAnswer{thought, action_input}};
    }
    return Action{ToolCall{thought, action, action_input}};
}

}  // namespace agent
