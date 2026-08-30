# AI Agent Framework (C++ over Ollama)

An object-oriented AI agent framework built from scratch in modern C++ (C++17–C++26) and connected to a local Ollama language model.

The project implements the main layers commonly found in agent frameworks such as LangChain, OpenClaw, and Hermes:

```text
LLM client → tools (8 classes / 10 names) → skills → ReAct agent loop
           → loop detection → harness → evaluators
```

## Student information

- Student ID: `25127136`
- Full name: `Đào Nhật Tân`
- Project: OOP AI Agent Framework

## Architecture layers

| Layer | Key types and responsibilities |
|---|---|
| LLM client | `LLMClient` abstraction and `OllamaClient` implementation using libcurl and nlohmann/json |
| Tools | Common `Tool` interface, 8 tool classes, 10 registered tool names, and `ToolRegistry` |
| Skills | `SkillLoader` loads Markdown skills and selects relevant skills by keywords |
| Agent | `AgentLoop` implements the ReAct cycle and uses `LoopDetector` |
| Harness | `Trajectory`, `HarnessRunner`, environments, and evaluators |
| Evaluation | `KeywordEvaluator` and `FunctionalEvaluator` evaluate agent results |

Design patterns used:

- Registry/Factory
- Template Method
- Strategy
- Observer/Hook

See [docs/uml.md](docs/uml.md) for the class, sequence, and component diagrams. See [docs/cpp-features.md](docs/cpp-features.md) for the C++ feature map.

## Tools

The framework registers 10 tool names from 8 concrete tool classes.

### Base tools

| Registered name | Class | Description |
|---|---|---|
| `calculator` | `CalculatorTool` | Evaluates arithmetic expressions |
| `read_file` | `FileTool` | Reads text files |
| `write_file` | `FileTool` | Writes text files |
| `exec` | `ExecTool` | Executes an allowed shell command |
| `web_search` | `WebSearchTool` | Queries the DuckDuckGo Instant Answer API |
| `memory_save` | `MemoryTool` | Saves information to SQLite memory |
| `memory_search` | `MemoryTool` | Searches previously stored memories |

### Extended tools

Besides the required base tools, the framework provides three additional tools from three different categories:

| Registered name | Category | Class | Description |
|---|---|---|---|
| `current_time` | System utility | `CurrentTimeTool` | Returns the current local date and time |
| `text_stats` | Text processing | `TextStatsTool` | Counts characters, words, and lines |
| `json_query` | Structured data processing | `JsonQueryTool` | Extracts a JSON value using a dot-separated key path |

All extended tools implement the common `Tool` interface, are registered through `ToolRegistry`, and have deterministic unit tests.

## Skills

The project includes at least three Markdown skills:

| Skill | Purpose |
|---|---|
| `task_planner.md` | Breaks a complex request into manageable steps |
| `error_recovery.md` | Helps the agent recover from tool or execution errors |
| `web_research.md` | Guides tasks that require web research |

## Requirements

- A C++26-capable compiler, such as MSYS2 UCRT64 g++ 16+
- CMake 3.25 or newer
- Ninja
- libcurl
- nlohmann/json
- SQLite3 development libraries
- Ollama with a tool-capable model
- Bash on `PATH` for functional evaluators on Windows

The default model is `gemma4`.

### Install dependencies with MSYS2 UCRT64

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-curl \
  mingw-w64-ucrt-x86_64-nlohmann-json \
  mingw-w64-ucrt-x86_64-sqlite3
```

## Build

From the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

The build produces:

```text
build/agent.exe
build/run_eval.exe
build/test_*.exe
```

Executable files do not include the `.exe` extension on Linux.

## Ollama setup

Pull the model:

```bash
ollama pull gemma4
```

Start the Ollama service if it is not already running:

```bash
ollama serve
```

The default Ollama API endpoint is:

```text
http://localhost:11434
```

The model, host, temperature, token limit, and timeout are configured through `LLMConfig`.

## Run the agent

### Windows CMD

From the repository root:

```bat
build\agent.exe "Use the calculator to compute 23 * 19, then give the result."
```

Test the system utility tool:

```bat
build\agent.exe "You MUST use current_time to obtain the current local date and time, then report it."
```

Test the text processing tool:

```bat
build\agent.exe "You MUST use text_stats to analyze the exact text 'hello world from agent', then report the statistics."
```

Test the structured data tool:

```bat
build\agent.exe "You MUST use json_query with this input: {\"data\":{\"student\":{\"id\":25127136}},\"key\":\"student.id\"}. Report the extracted value."
```

### Linux, macOS, Git Bash, or MSYS2

```bash
./build/agent "Use the calculator to compute 23 * 19, then give the result."
```

For each request, the agent prints its ReAct steps, tool observations, final answer, stop reason, token usage, and execution time.

## Run the tests

### Windows

```bash
ctest --test-dir build --output-on-failure
```

### Linux, macOS, Git Bash, or MSYS2

```bash
ctest --test-dir build --output-on-failure
```

If CMake is not available on PATH, invoke it using the installation path configured on the current machine.

The project currently contains seven deterministic test executables:

```text
test_action_parser
test_current_time_tool
test_environment
test_evaluators
test_json_query_tool
test_loop_detector
test_text_stats_tool
```

Latest result:

```text
7/7 tests passed — 100%
```

The deterministic tests do not require Ollama.

## Run the benchmark

Make sure Ollama is running, then execute the benchmark from the repository root.

### Windows

```bat
build\run_eval.exe benchmark\tasks.json trajectories
```

### Linux, macOS, Git Bash, or MSYS2

```bash
./build/run_eval benchmark/tasks.json trajectories
```

The benchmark contains 10 tasks:

- 4 easy tasks
- 4 medium tasks
- 2 hard tasks

It exports one JSON trajectory per task and a summary file.

Latest verified result:

```text
10/10 tasks passed — 100%
```

The committed benchmark artifacts are stored in `benchmark/sample_output/`:

```text
benchmark/sample_output/
├── summary.json
├── trajectory_task_001.json
├── trajectory_task_002.json
├── trajectory_task_003.json
├── trajectory_task_004.json
├── trajectory_task_005.json
├── trajectory_task_006.json
├── trajectory_task_007.json
├── trajectory_task_008.json
├── trajectory_task_009.json
└── trajectory_task_010.json
```

See [docs/benchmark-results.md](docs/benchmark-results.md) for the benchmark analysis.

## Project layout

```text
src/
├── agent/          AgentLoop, Action, LoopDetector, and SkillLoader
├── client/         LLMClient abstraction and OllamaClient
├── harness/        Trajectory, evaluators, environments, and HarnessRunner
└── tools/          Tool interface, ToolRegistry, and 8 tool classes

tests/              Seven deterministic unit tests
skills/             Markdown skill files
benchmark/          Benchmark tasks and batch evaluation runner
benchmark/sample_output/      Summary and 10 benchmark trajectories
docs/               Report, UML, C++ feature map, results, and slide
```

Generated directories and temporary runtime files such as `build/`, `*.db`, logs, and temporary result files are not committed.

## Documentation

- [Project report](docs/report.md)
- [UML diagrams](docs/uml.md)
- [C++17–C++26 feature map](docs/cpp-features.md)
- [Benchmark analysis](docs/benchmark-results.md)
- [Presentation slides](docs/Slide.pptx)

## Submission links

- [GitHub repository](https://github.com/putindao/25127136_OopAgent)
- [Demo video — YouTube Unlisted](https://youtu.be/1Xn5-6cR6_8)

The demonstration includes:

1. Running all seven tests.
2. Running an agent task with multiple tool calls.
3. Demonstrating an extended tool.
4. Running the 10-task benchmark.
5. Inspecting `summary.json` and a trajectory file.
6. Explaining the Registry/Factory design pattern.