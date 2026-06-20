// =============================================================================
//  main.cpp — temporary driver. Phase 2 turns it into a self-test for the tool
//  layer: register every tool (some eagerly, some via a lazy factory), list
//  them, run each once, and demonstrate the allow/deny policy. Later phases
//  replace this with the real ReAct agent CLI.
// =============================================================================
#include <filesystem>
#include <memory>
#include <print>   // C++23
#include <string>

#include "agent/skill_loader.h"
#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include "tools/web_search_tool.h"
#include "tools/memory_tool.h"

using namespace agent;
namespace fs = std::filesystem;

// Call one tool by name and pretty-print its ToolResult. Construction of a
// factory-built tool may throw (e.g. MemoryTool on a SQLite error), so guard it.
static void run(ToolRegistry& registry, const std::string& name, const std::string& args) {
    try {
        Tool* tool = registry.get(name);
        if (!tool) {
            std::println("  [{:<13}] DENIED / not found", name);
            return;
        }
        if (const ToolResult result = tool->execute(args); result.has_value()) {
            std::println("  [{:<13}] OK : {}", name, *result);
        } else {
            std::println("  [{:<13}] ERR: {}", name, result.error().message);
        }
    } catch (const std::exception& e) {
        std::println("  [{:<13}] EXCEPTION: {}", name, e.what());
    }
}

int main() {
    ToolRegistry registry;

    // Eagerly registered instances.
    registry.register_tool(std::make_unique<CalculatorTool>());
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Read));
    registry.register_tool(std::make_unique<FileTool>(FileTool::Mode::Write));
    registry.register_tool(std::make_unique<ExecTool>());

    // Lazily registered via factories (built on first use — Factory pattern).
    registry.register_factory("web_search",
                              [] { return std::make_unique<WebSearchTool>(); });
    registry.register_factory("memory_save",
                              [] { return std::make_unique<MemoryTool>(MemoryTool::Mode::Save); });
    registry.register_factory("memory_search",
                              [] { return std::make_unique<MemoryTool>(MemoryTool::Mode::Search); });

    std::println("=== Eagerly registered tools ===");
    for (const Tool* tool : registry.list()) {
        std::println("  - {:<13}: {}", tool->name(), tool->description());
    }

    std::println("\n=== Tool self-tests ===");
    run(registry, "calculator",    "15*17 + (3-1)*4");
    run(registry, "write_file",    R"({"path":"build/_selftest/hello.txt","content":"hi from agent"})");
    run(registry, "read_file",     "build/_selftest/hello.txt");
    run(registry, "exec",          "echo hello-from-exec");
    run(registry, "memory_save",   "Project deadline is week 12");
    run(registry, "memory_search", "deadline");
    run(registry, "web_search",    "Alan Turing");   // needs internet

    std::println("\n=== Policy demo: deny 'exec' ===");
    registry.deny("exec");
    run(registry, "exec", "echo should-not-run");

    // ---- Skill system (Phase 3) --------------------------------------------
    std::println("\n=== Skill system ===");
    SkillLoader skills;
    fs::path skills_dir;
    for (const char* candidate : {"skills", "../skills", "../../skills"}) {
        if (fs::is_directory(candidate)) { skills_dir = candidate; break; }
    }
    if (const auto loaded = skills.load_directory(skills_dir); loaded.has_value()) {
        std::println("Loaded {} skills from '{}':", *loaded, skills_dir.string());
        for (const Skill& s : skills.skills()) {
            std::println("  - {:<14}: {}", s.name, s.description);
        }
        std::println("Skill selection by keyword:");
        for (const char* task : {"please plan the steps to refactor this module",
                                 "the build failed with an error, fix it",
                                 "find out who Alan Turing was"}) {
            const Skill* picked = skills.select(task);
            std::println("  task=\"{}\"\n      -> {}", task, picked ? picked->name : "(no match)");
        }
    } else {
        std::println("Skill load error: {}", loaded.error());
    }

    return 0;
}
