// =============================================================================
//  agent/loop_detector.h
//  Detects when the agent is stuck. Two kinds of loop are recognised:
//    1. generic repeat  — the same action fired several times in a row;
//    2. ping-pong       — two actions alternating A,B,A,B,...
//  Thresholds are configurable and a verdict escalates None -> Warning ->
//  Critical. The AgentLoop logs a Warning but keeps going, and stops on Critical.
//  Self-contained: it only sees opaque action "signatures", never tools or LLM.
// =============================================================================
#pragma once

#include <string>
#include <vector>

namespace agent {

class LoopDetector {
public:
    enum class Severity { None, Warning, Critical };

    struct Verdict {
        Severity    severity = Severity::None;
        std::string reason;
    };

    struct Config {
        int repeat_threshold   = 3;  // N identical actions in a row => Critical
        int pingpong_threshold = 2;  // N A/B cycles (2*N actions) => Critical
    };

    LoopDetector() = default;                              // default thresholds
    explicit LoopDetector(Config config) : config_(config) {}

    // Feed the next action signature (e.g. "calculator|15*17"); get a verdict.
    Verdict observe(const std::string& signature);

    // Forget all history (e.g. when starting a new task).
    void reset() noexcept { history_.clear(); }

private:
    Config                   config_;
    std::vector<std::string> history_;
};

}  // namespace agent
