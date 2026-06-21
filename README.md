# AI Agent Framework (C++ over Ollama)

An object-oriented **AI agent framework** built from scratch in modern C++ (C++17→C++26),
talking to a local **Ollama** LLM. It implements, as separable OOP layers, what frameworks
like LangChain / OpenClaw do: an LLM client, a tool registry, a skill system, a ReAct agent
loop with loop detection, and an evaluation harness.

```
LLM client  →  tools (5)  →  skills  →  ReAct agent loop  →  harness + evaluators
```

See [docs/uml.md](docs/uml.md) for class/sequence/component diagrams and
[docs/cpp-features.md](docs/cpp-features.md) for the C++ feature map.

## Layers

| Layer | Key types |
|-------|-----------|
| Client | `LLMClient` (abstract) → `OllamaClient` (libcurl + nlohmann/json) |
| Tools | `Tool` → `CalculatorTool`, `FileTool`, `ExecTool`, `WebSearchTool`, `MemoryTool`; `ToolRegistry` (Registry/Factory + allow/deny) |
| Skills | `SkillLoader` — Markdown skills with keyword selection |
| Agent | `AgentLoop` (ReAct, Template Method) + `LoopDetector` + `Action` variant |
| Harness | `Trajectory`, `Evaluator` → `Keyword`/`Functional` (Strategy), `Environment` → `Native`/`Sandbox`, `HarnessRunner` (Observer step hook) |

**Design patterns:** Registry/Factory · Template Method · Strategy · Observer/Hook.

## Requirements

- A C++26-capable compiler — **g++ 16+** (this project was built with MSYS2 UCRT64 g++ 16.1.0).
- **CMake ≥ 3.25** and **Ninja**.
- **libcurl**, **nlohmann/json**, **sqlite3** (dev headers).
- **Ollama** with a tool-capable model (default: `gemma4`).
- **bash** on PATH (Git Bash or MSYS2) for the functional evaluator on Windows.

### Installing the libraries (MSYS2 UCRT64)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-sqlite3
```

On Debian/Ubuntu: `sudo apt install cmake ninja-build libcurl4-openssl-dev nlohmann-json3-dev libsqlite3-dev`.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

This produces `build/agent` (CLI), `build/run_eval` (benchmark), and the test executables.

## Ollama setup

```bash
ollama pull gemma4        # ~9.6 GB; any tool-capable model works
ollama serve              # starts the API on http://localhost:11434
```

The model/host/temperature are configured via `LLMConfig` (see `src/main.cpp`).

## Run the agent

```bash
cd build
./agent "Use the calculator to compute 23 * 19, then give the result."
./agent "Compute 15*17, save the result to result.txt, then report it."
```

The agent prints each ReAct step (action + observation) and a final result block.

## Run the benchmark

```bash
cd build
./run_eval                       # uses ../benchmark/tasks.json
./run_eval ../benchmark/tasks.json my_out
```

Prints a per-task PASS/FAIL table + overall success rate, and writes
`trajectory_<id>.json` + `summary.json` under the output directory. Latest results:
**10/10 = 100%** — see [docs/benchmark-results.md](docs/benchmark-results.md).

## Run the tests

```bash
cd build && ctest --output-on-failure
```

Deterministic unit tests (no Ollama needed) for the action parser, loop detector,
evaluators + trajectory, and environments.

## Project layout

```
src/client/      LLMClient + OllamaClient
src/tools/       Tool, ToolRegistry, the 5 tools
src/agent/       AgentLoop, LoopDetector, SkillLoader, Action
src/harness/     Trajectory, Evaluator(s), Environment(s), HarnessRunner
skills/          Markdown skill files
benchmark/       tasks.json + run_eval + sample_output
tests/           unit tests (CTest)
docs/            uml.md, cpp-features.md, benchmark-results.md, report.md, plans/
```

## Documentation

- [docs/report.md](docs/report.md) — design, difficulties, results.
- [docs/uml.md](docs/uml.md) — UML diagrams (mermaid).
- [docs/cpp-features.md](docs/cpp-features.md) — C++17/20/23/26 feature map.
- [docs/benchmark-results.md](docs/benchmark-results.md) — benchmark analysis.
