// =============================================================================
//  harness/task.cpp — parse one task object from tasks.json.
// =============================================================================
#include "harness/task.h"

namespace agent {

Task Task::from_json(const nlohmann::json& j) {
    Task t;
    t.id          = j.value("id", std::string{});
    t.description = j.value("description", std::string{});
    // The agent receives "instruction"; fall back to "description" if absent.
    t.instruction = j.value("instruction", t.description);
    t.eval_type   = j.value("eval_type", std::string{"keyword"});
    t.eval_script = j.value("eval_script", std::string{});
    t.max_steps   = j.value("max_steps", 10);
    t.difficulty  = j.value("difficulty", std::string{});

    // Expected keywords accept either "expected" or "keywords".
    if (j.contains("expected") && j.at("expected").is_array()) {
        t.expected_keywords = j.at("expected").get<std::vector<std::string>>();
    } else if (j.contains("keywords") && j.at("keywords").is_array()) {
        t.expected_keywords = j.at("keywords").get<std::vector<std::string>>();
    }
    return t;
}

}  // namespace agent
