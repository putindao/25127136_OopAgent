// =============================================================================
//  harness/task.h
//  A benchmark task definition (the tasks.json schema from the brief). The agent
//  is given `instruction`; the harness then judges the run with the evaluator
//  named by `eval_type`.
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace agent {

struct Task {
    std::string              id;
    std::string              description;
    std::string              instruction;        // what the agent actually receives
    std::string              eval_type = "keyword";   // "keyword" | "functional"
    std::string              eval_script;         // functional: a shell command
    std::vector<std::string> expected_keywords;   // keyword: all must appear
    int                      max_steps = 10;
    std::string              difficulty;          // easy | medium | hard (reporting)

    // Parse one task object; missing optional fields take their defaults.
    static Task from_json(const nlohmann::json& j);
};

}  // namespace agent
