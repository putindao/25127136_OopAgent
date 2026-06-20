// =============================================================================
//  agent/loop_detector.cpp — repeat and ping-pong detection over the action log.
// =============================================================================
#include "agent/loop_detector.h"

namespace agent {

LoopDetector::Verdict LoopDetector::observe(const std::string& signature) {
    history_.push_back(signature);
    const std::size_t n = history_.size();

    // ---- 1) Generic consecutive repeat -------------------------------------
    std::size_t repeat = 1;
    while (repeat < n && history_[n - 1 - repeat] == history_[n - 1]) {
        ++repeat;
    }
    if (static_cast<int>(repeat) >= config_.repeat_threshold) {
        return {Severity::Critical,
                "action repeated " + std::to_string(repeat) + " times: " + history_[n - 1]};
    }
    if (config_.repeat_threshold > 1 &&
        static_cast<int>(repeat) == config_.repeat_threshold - 1) {
        return {Severity::Warning,
                "action repeating (" + std::to_string(repeat) + "x): " + history_[n - 1]};
    }

    // ---- 2) Ping-pong A,B,A,B,... ------------------------------------------
    const std::size_t span = static_cast<std::size_t>(2 * config_.pingpong_threshold);
    if (span >= 4 && n >= span) {
        const std::string& a = history_[n - 1];
        const std::string& b = history_[n - 2];
        if (a != b) {
            bool pingpong = true;
            for (std::size_t k = 0; k < span; ++k) {
                const std::string& want = (k % 2 == 0) ? a : b;
                if (history_[n - 1 - k] != want) {
                    pingpong = false;
                    break;
                }
            }
            if (pingpong) {
                return {Severity::Critical,
                        "ping-pong loop between '" + a + "' and '" + b + "'"};
            }
        }
    }

    return {Severity::None, ""};
}

}  // namespace agent
