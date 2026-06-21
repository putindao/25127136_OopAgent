// =============================================================================
//  harness/harness_runner.h
//  Orchestrates a run: setup -> run agent -> evaluate -> record. It injects a
//  step hook into the AgentLoop (Observer) to capture the trajectory, then picks
//  an Evaluator by eval_type (Strategy). Batch mode runs many tasks, exports
//  each trajectory, and reports the success rate.
//
//  This is the ONLY class that knows about both the agent and the evaluators;
//  the AgentLoop itself remains unaware the Harness exists.
// =============================================================================
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "agent/agent_loop.h"
#include "agent/skill_loader.h"
#include "client/llm_client.h"
#include "harness/environment.h"
#include "harness/evaluator.h"
#include "harness/functional_evaluator.h"
#include "harness/keyword_evaluator.h"
#include "harness/task.h"
#include "harness/trajectory.h"
#include "tools/tool_registry.h"

namespace agent {

class HarnessRunner {
public:
    struct RunOutcome {
        Trajectory trajectory;
        EvalResult eval;
    };

    struct BatchReport {
        int                     total        = 0;
        int                     passed       = 0;
        double                  success_rate = 0.0;
        std::vector<RunOutcome> outcomes;

        nlohmann::json to_json() const;
    };

    HarnessRunner(LLMClient& client, ToolRegistry& registry, Environment& environment,
                  SkillLoader* skills = nullptr, AgentConfig base_config = AgentConfig{});

    // HarnessRunner aliases its client, registry and environment by reference;
    // copying it would alias them. Forbid with a C++26 delete-with-reason.
    HarnessRunner(const HarnessRunner&)            = delete("HarnessRunner aliases its dependencies by reference");
    HarnessRunner& operator=(const HarnessRunner&) = delete("HarnessRunner aliases its dependencies by reference");

    // Run + evaluate a single task.
    RunOutcome run_task(const Task& task);

    // Run a batch, save trajectory_{id}.json + summary.json under out_dir,
    // and return the aggregate report.
    BatchReport run_batch(const std::vector<Task>& tasks, const std::filesystem::path& out_dir);

private:
    // Strategy selection by eval_type.
    Evaluator& pick_evaluator(const std::string& eval_type);

    LLMClient&    client_;
    ToolRegistry& registry_;
    Environment&  env_;
    SkillLoader*  skills_;
    AgentConfig   base_config_;

    KeywordEvaluator    keyword_eval_;
    FunctionalEvaluator functional_eval_;
};

}  // namespace agent
