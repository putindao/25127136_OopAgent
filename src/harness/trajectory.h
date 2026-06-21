// =============================================================================
//  harness/trajectory.h
//  A full record of one agent run, serialisable to the trajectory JSON format
//  from the brief (section 7.1). The Harness fills it via the AgentLoop step
//  hook (Observer pattern) and dumps it for offline inspection.
// =============================================================================
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "agent/agent_loop.h"  // agent::Step

namespace agent {

struct Trajectory {
    std::string       task_id;
    std::string       model;
    bool              success = false;     // did the task pass evaluation
    std::string       stop_reason;         // from the agent loop
    std::string       final_answer;
    int               total_tokens  = 0;
    long              total_time_ms = 0;
    std::vector<Step> steps;               // recorded live via the step hook

    // Serialise to the trajectory_{task_id}.json shape.
    nlohmann::json to_json() const;

    // Write to_json() (pretty) to `path`. Returns false on IO failure.
    bool save(const std::filesystem::path& path) const;
};

}  // namespace agent
