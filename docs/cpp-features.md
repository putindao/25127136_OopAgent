# C++ Standard Features Used

Requirement (brief §V): at least **4× C++17**, **2× C++20**, **2× C++23**, **1× C++26**.
Built with g++ 16.1.0, `-std=c++26 -Wall -Wextra -Wpedantic`.

## C++17 (used: 8 — requirement ≥4)

| Feature | Where |
|---------|-------|
| `std::unique_ptr` + custom deleter (RAII) | [ollama_client.cpp](../src/client/ollama_client.cpp) (CURL/slist), [exec_tool.cpp](../src/tools/exec_tool.cpp) (FILE*), [memory_tool.cpp](../src/tools/memory_tool.cpp) (sqlite3/stmt), [web_search_tool.cpp](../src/tools/web_search_tool.cpp) |
| Abstract class / pure virtual | `LLMClient`, `Tool`, `Evaluator`, `Environment` |
| `std::function` + lambda | `ToolRegistry::Factory`, `AgentLoop::StepHook`, `ExecTool::CommandPolicy` |
| `std::variant` | `Action` in [action.h](../src/agent/action.h) |
| `std::visit` + `if constexpr` | dispatch in [agent_loop.cpp](../src/agent/agent_loop.cpp) `run()` |
| `std::filesystem` | [skill_loader.cpp](../src/agent/skill_loader.cpp), [file_tool.cpp](../src/tools/file_tool.cpp), [environment.cpp](../src/harness/environment.cpp) |
| Structured bindings | `for (const auto& [name, tool] : ...)` in [tool_registry.cpp](../src/tools/tool_registry.cpp) |
| `std::string_view` | [calculator_tool.cpp](../src/tools/calculator_tool.cpp) parser, [skill_loader.cpp](../src/agent/skill_loader.cpp) |

## C++20 (used: 3 — requirement ≥2)

| Feature | Where |
|---------|-------|
| Designated initializers | `LLMConfig{.model=...}`, `AgentConfig{.max_steps=...}` ([main.cpp](../src/main.cpp), [run_eval.cpp](../benchmark/run_eval.cpp)) |
| `.contains()` on associative containers | `ToolRegistry::is_allowed` ([tool_registry.cpp](../src/tools/tool_registry.cpp)) |
| Ranges (`std::ranges::transform`/`stable_sort`, `std::views::split`) | [skill_loader.cpp](../src/agent/skill_loader.cpp), [keyword_evaluator.cpp](../src/harness/keyword_evaluator.cpp), [action.cpp](../src/agent/action.cpp) |

## C++23 (used: 2 — requirement ≥2)

| Feature | Where |
|---------|-------|
| `std::expected` / `std::unexpected` | `LLMResult`, `ToolResult`, parser/eval returns — pervasive error discipline |
| `std::print` / `std::println` | [main.cpp](../src/main.cpp), [agent_loop.cpp](../src/agent/agent_loop.cpp), [run_eval.cpp](../benchmark/run_eval.cpp) (links `stdc++exp`) |

## C++26 (used: 1 — requirement ≥1)

| Feature | Where |
|---------|-------|
| Deleted function with reason `= delete("...")` (P2573) | `AgentLoop` and `HarnessRunner` copy ops ([agent_loop.h](../src/agent/agent_loop.h), [harness_runner.h](../src/harness/harness_runner.h)) — forbids reference-aliasing copies with a diagnostic |

## Other modern-C++ discipline

- No raw `new`/`delete`; ownership via `std::unique_ptr`, non-owning access via references/raw pointers.
- Exceptions used only where meaningful (`MemoryTool` ctor on a SQLite failure); `execute()` never lets one escape.
- `const`-correctness and `noexcept` on read-only / deleter paths.
