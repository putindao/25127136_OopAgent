// =============================================================================
//  benchmark/run_eval.cpp — load tasks.json, run the batch through the Harness,
//  print a per-task table + overall success rate, export trajectories.
//  Usage: run_eval [tasks.json] [output_dir]
// =============================================================================
#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "agent/skill_loader.h"
#include "client/ollama_client.h"
#include "harness/environment.h"
#include "harness/harness_runner.h"
#include "harness/task.h"
#include "tools/calculator_tool.h"
#include "tools/exec_tool.h"
#include "tools/file_tool.h"
#include "tools/memory_tool.h"
#include "tools/tool_registry.h"
#include "tools/web_search_tool.h"

using namespace agent;
namespace fs = std::filesystem;

namespace {

void register_all_tools(ToolRegistry& registry, const Environment& env) {
    registry.register_tool(std::make_unique<CalculatorTool>());
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Read));
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Write));

    // The exec tool honours the environment's command policy.
    auto exec = std::make_unique<ExecTool>();
    exec->set_command_policy([&env](const std::string& cmd) { return env.allows_command(cmd); });
    registry.register_tool(std::move(exec));

    registry.register_tool(std::make_unique<WebSearchTool>());
    try {
        registry.register_tool(std::make_unique<MemoryTool>(MemoryTool::Mode::Save));
        registry.register_tool(std::make_unique<MemoryTool>(MemoryTool::Mode::Search));
    } catch (const std::exception& e) {
        std::println("warning: memory tools unavailable: {}", e.what());
    }
}

// Resolve a path that may live at the repo root or one level up (build/).
fs::path resolve(const std::string& p) {
    if (fs::exists(p)) return p;
    if (fs::exists(fs::path("..") / p)) return fs::path("..") / p;
    return p;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string tasks_path = resolve(argc > 1 ? argv[1] : "benchmark/tasks.json").string();
    const fs::path    out_dir    = (argc > 2) ? argv[2] : "trajectories";

    // ---- Load tasks ---------------------------------------------------------
    std::ifstream in(tasks_path, std::ios::binary);
    if (!in) {
        std::println("error: cannot open tasks file '{}'", tasks_path);
        return 2;
    }
    nlohmann::json arr;
    try {
        in >> arr;
    } catch (const nlohmann::json::exception& e) {
        std::println("error: invalid tasks JSON: {}", e.what());
        return 2;
    }
    std::vector<Task> tasks;
    for (const auto& j : arr) tasks.push_back(Task::from_json(j));
    std::println("Loaded {} tasks from {}\n", tasks.size(), tasks_path);

    // ---- Assemble the agent stack -------------------------------------------
    // NativeEnvironment keeps behaviour identical to running in the current
    // directory; swap in a SandboxEnvironment to isolate + sandbox each task.
    NativeEnvironment env(fs::current_path());

    ToolRegistry registry;
    register_all_tools(registry, env);

    SkillLoader skills;
    for (const char* d : {"skills", "../skills"}) {
        if (fs::is_directory(d)) { (void)skills.load_directory(d); break; }
    }

    OllamaClient client(LLMConfig{.model = "gemma4", .temperature = 0.1,
                                  .max_tokens = 512, .timeout_seconds = 180});

    HarnessRunner harness(client, registry, env, &skills, AgentConfig{.max_steps = 8, .verbose = false});

    // ---- Run the batch ------------------------------------------------------
    const auto report = harness.run_batch(tasks, out_dir);

    // ---- Report -------------------------------------------------------------
    std::println("{:<10} {:<7} {:<14} {:>6} {:>8}  detail", "task", "result", "stop_reason", "steps", "tokens");
    std::println("{}", std::string(72, '-'));
    for (const auto& o : report.outcomes) {
        std::println("{:<10} {:<7} {:<14} {:>6} {:>8}  {}",
                     o.trajectory.task_id,
                     o.eval.passed ? "PASS" : "FAIL",
                     o.trajectory.stop_reason,
                     o.trajectory.steps.size(),
                     o.trajectory.total_tokens,
                     o.eval.passed ? "" : o.eval.detail.substr(0, 60));
    }
    std::println("{}", std::string(72, '-'));
    std::println("Success rate: {}/{} = {:.1f}%   (trajectories in '{}')",
                 report.passed, report.total, report.success_rate * 100.0, out_dir.string());
    return 0;
}
