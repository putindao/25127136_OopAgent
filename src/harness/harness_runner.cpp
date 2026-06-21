// =============================================================================
//  harness/harness_runner.cpp — single-task and batch orchestration.
// =============================================================================
#include "harness/harness_runner.h"

#include <fstream>

namespace agent {

HarnessRunner::HarnessRunner(LLMClient& client, ToolRegistry& registry,
                             SkillLoader* skills, AgentConfig base_config)
    : client_(client), registry_(registry), skills_(skills), base_config_(base_config) {}

Evaluator& HarnessRunner::pick_evaluator(const std::string& eval_type) {
    if (eval_type == "functional") return functional_eval_;
    return keyword_eval_;  // default strategy
}

HarnessRunner::RunOutcome HarnessRunner::run_task(const Task& task) {
    // Fresh agent + loop detector per task.
    AgentConfig config = base_config_;
    config.max_steps   = task.max_steps;

    AgentLoop    agent(client_, registry_, config);
    LoopDetector detector;
    agent.set_loop_detector(&detector);

    // System prompt: persona + the most relevant skill for this instruction.
    std::string persona = "You are a precise, tool-using task-solving agent.";
    if (skills_) {
        if (const std::string g = skills_->build_system_prompt(task.instruction, 1); !g.empty()) {
            persona += "\n\n" + g;
        }
    }
    agent.set_system_prompt(persona);

    // OBSERVER: record every step into the trajectory as the agent runs.
    Trajectory trajectory;
    trajectory.task_id = task.id;
    trajectory.model   = client_.config().model;
    agent.set_step_hook([&trajectory](const Step& s) { trajectory.steps.push_back(s); });

    const AgentResult result = agent.run(task.instruction);
    trajectory.stop_reason   = result.stop_reason;
    trajectory.final_answer  = result.final_answer;
    trajectory.total_tokens  = result.total_tokens;
    trajectory.total_time_ms = result.total_time_ms;

    // STRATEGY: evaluate with the strategy named by the task.
    Evaluator&       evaluator = pick_evaluator(task.eval_type);
    const EvalResult eval      = evaluator.evaluate(task, trajectory);

    // The trajectory's success reflects the evaluation verdict.
    trajectory.success = eval.passed;
    return {std::move(trajectory), eval};
}

HarnessRunner::BatchReport
HarnessRunner::run_batch(const std::vector<Task>& tasks, const std::filesystem::path& out_dir) {
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);

    BatchReport report;
    report.total = static_cast<int>(tasks.size());

    for (const Task& task : tasks) {
        RunOutcome outcome = run_task(task);
        if (outcome.eval.passed) ++report.passed;

        const std::string id = task.id.empty() ? "task" : task.id;
        outcome.trajectory.save(out_dir / ("trajectory_" + id + ".json"));
        report.outcomes.push_back(std::move(outcome));
    }
    report.success_rate =
        report.total > 0 ? static_cast<double>(report.passed) / report.total : 0.0;

    // Write an aggregate summary alongside the per-task trajectories.
    if (std::ofstream summary(out_dir / "summary.json", std::ios::binary); summary) {
        summary << report.to_json().dump(2);
    }
    return report;
}

nlohmann::json HarnessRunner::BatchReport::to_json() const {
    nlohmann::json results = nlohmann::json::array();
    for (const RunOutcome& o : outcomes) {
        results.push_back({
            {"task_id", o.trajectory.task_id},
            {"passed", o.eval.passed},
            {"score", o.eval.score},
            {"stop_reason", o.trajectory.stop_reason},
            {"steps", o.trajectory.steps.size()},
            {"total_tokens", o.trajectory.total_tokens},
            {"total_time_ms", o.trajectory.total_time_ms},
            {"detail", o.eval.detail},
        });
    }
    return {
        {"total", total},
        {"passed", passed},
        {"success_rate", success_rate},
        {"results", results},
    };
}

}  // namespace agent
