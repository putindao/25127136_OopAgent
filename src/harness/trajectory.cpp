// =============================================================================
//  harness/trajectory.cpp — trajectory JSON serialisation (brief section 7.1).
// =============================================================================
#include "harness/trajectory.h"

#include <fstream>

namespace agent {
namespace {

// Map one Step to the nested action shape from the brief.
nlohmann::json step_to_json(const Step& s) {
    nlohmann::json action;
    if (s.action == "final") {
        action = {{"type", "final"}, {"answer", s.action_input}};
    } else if (s.action == "error") {
        action = {{"type", "error"}, {"detail", s.observation}};
    } else {
        action = {{"type", "tool_call"}, {"tool", s.action}, {"args", s.action_input}};
    }

    nlohmann::json j = {
        {"step_id", s.step_id},
        {"thought", s.thought},
        {"action", action},
        {"tokens_used", s.tokens_used},
        {"latency_ms", s.latency_ms},
    };
    if (s.action != "final") {
        j["tool_result"] = s.observation;
    }
    return j;
}

}  // namespace

nlohmann::json Trajectory::to_json() const {
    nlohmann::json steps_json = nlohmann::json::array();
    for (const Step& s : steps) {
        steps_json.push_back(step_to_json(s));
    }
    return {
        {"task_id", task_id},
        {"model", model},
        {"success", success},
        {"stop_reason", stop_reason},
        {"final_answer", final_answer},
        {"total_tokens", total_tokens},
        {"total_time_ms", total_time_ms},
        {"steps", steps_json},
    };
}

bool Trajectory::save(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << to_json().dump(2);
    return static_cast<bool>(out);
}

}  // namespace agent
