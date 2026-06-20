# Plan — OOP AI Agent Framework (Ollama, C++)

> Course project: build an AI Agent framework in C++17+ on top of the Ollama API.
> Team size: 1 (>= 6 git commits, gap between commits <= 7 days).
> Build location: this repo (`D:\New-project`). Toolchain: MSYS2 UCRT64, g++ 16.1.0.
> Backend: Ollama local, model `gemma4:latest` (tool calling + vision capable).

## Grading map (100 pts) — what each phase earns

| Area | Pts | Covered by phase |
|------|-----|------------------|
| OOP design (class diagram, patterns, layering) | 25 | P1–P7, P9 |
| C++ technique (C++17/20/23/26, smart ptrs, exceptions) | 20 | all + P8 |
| Functionality (5 tools, agent loop, loop detect, skills, harness) | 25 | P2–P6 |
| Benchmark (10 tasks, evaluator, JSON, success rate) | 15 | P5–P6 |
| Documentation (README, report, slides) | 15 | P10 |
| Bonus (GUI / vector memory / multi-agent) | +15 | P11 |

## Architecture (layers — strict dependency direction)

```
HarnessRunner ──► AgentLoop ──► LLMClient (abstract) ──► OllamaClient
      │               │     └──► ToolRegistry ──► Tool (abstract) ──► [5 tools]
      │               └──► SkillLoader, LoopDetector
      └──► Evaluator (abstract) ──► Keyword / Functional / VLM
      └──► Trajectory + Step (data)
      └──► Environment (abstract) ──► Native / Sandbox
```

Design invariants (graded — violating loses points):
- AgentLoop does NOT know Harness exists → only exposes a `step_hook` interface.
- Tool implementations do NOT depend on AgentLoop.
- Evaluator does NOT depend on how the agent executes → only sees the result.
- LLMClient is generic → swapping Ollama for OpenAI = one new subclass.

## Required design patterns (>= 4)

| Pattern | Where |
|---------|-------|
| Strategy | `Evaluator` family shares `evaluate()` |
| Template Method | `AgentLoop::run()` defines skeleton; `observe()/act()` overridable |
| Registry / Factory | `ToolRegistry` registers + creates tools by name |
| Observer / Hook | `HarnessRunner` injects `step_hook` into `AgentLoop` to record |
| (bonus) Builder / Command / Decorator | optional extra credit |

## C++ feature budget (>= 4×C++17, >= 2×C++20, >= 2×C++23, >= 1×C++26)

- C++17: `unique_ptr/shared_ptr`, `std::function`+lambda, `std::variant`, `std::filesystem`,
  `if constexpr`/`std::visit`, structured bindings, `std::optional`, range-based for, pure virtual, template `Registry<T>`.
- C++20: concepts, ranges, `std::format`, `std::span`, designated initializers, `<=>`.
- C++23: `std::expected`, `std::print`, deducing `this`, `if consteval`, ranges `to<>()`.
- C++26: pack indexing `T...[i]`, `_` placeholder, `#embed`, or `= delete("reason")`.
- Document the exact features + file:line in the report (table V of the brief).

## Phases (each phase = at least one git commit)

- **P0 Setup**: install cmake/ninja/libcurl/nlohmann-json/sqlite3; git init; folder skeleton;
  CMake + hello-world build; verify Ollama `/api/chat` with gemma4.
- **P1 LLM Client**: `LLMClient` abstract + `OllamaClient` (libcurl POST, json, error handling, config).
- **P2 Tools**: `Tool` abstract, `ToolRegistry`, 5 tools (calculator, file r/w, exec, web_search, memory/SQLite), allow/deny policy.
- **P3 Skills**: `SkillLoader` (filesystem scan, keyword selection), 3 skill `.md` files.
- **P4 Agent loop**: `AgentLoop` ReAct (Template Method), tool-call parsing, history, max_steps; `LoopDetector` (generic repeat + ping-pong).
- **P5 Harness**: `Trajectory`/`Step`, `Evaluator`→Keyword/Functional, `HarnessRunner` + step_hook, batch eval, JSON export.
- **P6 Benchmark**: `tasks.json` (4 easy + 4 medium + 2 hard), `run_eval.cpp`, run + record success rate.
- **P7 Environment**: `Environment` abstract → Native / Sandbox.
- **P8 C++ polish**: ensure feature budget met + documented; smart-pointer audit; exception safety.
- **P9 UML (mermaid)**: class, sequence (agent run), sequence (batch eval), component.
- **P10 Docs**: README (build/run/Ollama), report (design/difficulties/results), slides outline.
- **P11 Bonus**: VLM evaluator + capture_screenshot; vector memory (nomic-embed-text + cosine); multi-agent (threads + queue).
- **P12 Git hygiene**: >= 6 commits spread across phases; PAT (read-only) instructions for grading.

## Deadlines (from brief)

- Week 11 (Sun 21:00): submit design = class diagram + sequence diagram.
- Week 12 (Sun 21:00): submit source code + full report.
- Week 13: live presentation + demo (random code Q&A by instructor).

## Academic-integrity stance

AI use allowed for learning/debug, NOT for blind code dumps. Every phase ships with an
explanation of the design + C++ idioms so the author can defend any line during the demo.
