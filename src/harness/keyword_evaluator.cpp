// =============================================================================
//  harness/keyword_evaluator.cpp — pass iff every expected keyword is present.
// =============================================================================
#include "harness/keyword_evaluator.h"

#include <algorithm>
#include <cctype>

namespace agent {
namespace {

std::string to_lower(std::string s) {
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

EvalResult KeywordEvaluator::evaluate(const Task& task, const Trajectory& trajectory) {
    // Look in the final answer plus every observation, so an answer that ends up
    // in a tool result (e.g. a file's content) still counts.
    std::string haystack = trajectory.final_answer;
    for (const Step& s : trajectory.steps) {
        haystack += '\n';
        haystack += s.observation;
    }
    haystack = to_lower(haystack);

    if (task.expected_keywords.empty()) {
        const bool ok = trajectory.stop_reason == "final";
        return {ok, ok ? 1.0 : 0.0,
                "no keywords given; judged by stop_reason=" + trajectory.stop_reason};
    }

    int         matched = 0;
    std::string missing;
    for (const std::string& kw : task.expected_keywords) {
        if (haystack.find(to_lower(kw)) != std::string::npos) {
            ++matched;
        } else {
            if (!missing.empty()) missing += ", ";
            missing += kw;
        }
    }

    const auto   total  = static_cast<int>(task.expected_keywords.size());
    const double score  = static_cast<double>(matched) / total;
    const bool   passed = (matched == total);
    return {passed, score,
            passed ? "all keywords present"
                   : ("matched " + std::to_string(matched) + "/" + std::to_string(total) +
                      ", missing: " + missing)};
}

}  // namespace agent
