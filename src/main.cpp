// =============================================================================
//  main.cpp — the agent CLI (Phase 4).
//  Usage:  agent [task words...]
//  Wires every layer together: tools (ToolRegistry) + skills (SkillLoader) +
//  model (OllamaClient) + the ReAct loop (AgentLoop) guarded by a LoopDetector,
//  then runs one task and prints the resulting trajectory.
// =============================================================================
#include <filesystem>
#include <memory>
#include <print>
#include <string>
#include <vector>

#include "agent/agent_loop.h"
#include "agent/loop_detector.h"
#include "agent/skill_loader.h"
#include "client/ollama_client.h"
#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include "tools/web_search_tool.h"
#include "tools/memory_tool.h"

using namespace agent;
namespace fs = std::filesystem;

namespace {

// Register all five tool classes (seven names) as ready instances.
void register_all_tools(ToolRegistry& registry) {
    registry.register_tool(std::make_unique<CalculatorTool>());
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Read));
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Write));
    registry.register_tool(std::make_unique<ExecTool>());
    registry.register_tool(std::make_unique<WebSearchTool>());
    try {  // MemoryTool opens SQLite in its ctor and may throw.
        registry.register_tool(std::make_unique<MemoryTool>(MemoryTool::Mode::Save));
        registry.register_tool(std::make_unique<MemoryTool>(MemoryTool::Mode::Search));
    } catch (const std::exception& e) {
        std::println("warning: memory tools unavailable: {}", e.what());
    }
}

// Find the skills/ directory whether we run from the repo root or build/.
fs::path find_skills_dir() {
    for (const char* candidate : {"skills", "../skills", "../../skills"}) {
        if (fs::is_directory(candidate)) return candidate;
    }
    return {};
}

}  // namespace

int main(int argc, char** argv) {
    // Task comes from the command line; fall back to a multi-step default.
    std::string task;
    for (int i = 1; i < argc; ++i) {
        if (!task.empty()) task += ' ';
        task += argv[i];
    }
    if (task.empty()) {
        task = "Use the calculator to compute 23 * 19, then give the result as the final answer.";
    }

    // ---- Assemble the layers ------------------------------------------------
    ToolRegistry registry;
    register_all_tools(registry);

    SkillLoader skills;
    if (const auto dir = find_skills_dir(); !dir.empty()) {
        (void)skills.load_directory(dir);
    }

    OllamaClient client(LLMConfig{
        .model           = "gemma4",
        .temperature     = 0.1,
        .max_tokens      = 512,
        .timeout_seconds = 180,
    });

    LoopDetector detector;  // default thresholds (repeat=3, ping-pong=2)

    AgentLoop agent(client, registry, AgentConfig{.max_steps = 6, .verbose = true});
    agent.set_loop_detector(&detector);

    // System prompt = persona + the single most relevant skill for this task.
    std::string persona = "You are a precise, tool-using task-solving agent.";
    if (const std::string guidance = skills.build_system_prompt(task, 1); !guidance.empty()) {
        persona += "\n\n" + guidance;
    }
    agent.set_system_prompt(persona);

    // ---- Run ----------------------------------------------------------------
    std::println("Task: {}\n", task);
    const AgentResult result = agent.run(task);

    // ---- Report -------------------------------------------------------------
    std::println("\n=== Result ===");
    std::println("stop_reason : {}", result.stop_reason);
    std::println("success     : {}", result.success);
    std::println("final answer: {}", result.final_answer);
    std::println("steps       : {}", result.steps.size());
    std::println("tokens      : {}", result.total_tokens);
    std::println("time        : {} ms", result.total_time_ms);
    return result.success ? 0 : 1;
}
