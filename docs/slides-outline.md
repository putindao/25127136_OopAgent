# Presentation Slides — Outline

~14 slides, ~12 minutes + live demo. Keep one idea per slide.

1. **Title** — AI Agent Framework in C++ over Ollama. Team / MSSV. One-line: "LangChain's
   ideas, rebuilt from scratch in modern C++."

2. **Problem & goal** — not "call API, print result"; build a *layered* agent system where each
   layer is an independent OOP exercise that composes into a working agent.

3. **Architecture at a glance** — the layer diagram (client → tools → skills → agent loop →
   harness). State the one rule: dependencies point strictly downward.

4. **Layer 1 — LLM client** — `LLMClient` abstract → `OllamaClient`. libcurl + nlohmann/json,
   `std::expected` error handling. "Swap Ollama for OpenAI = one subclass."

5. **Layer 2 — Tools + Registry** — the 5 tools; `ToolRegistry` = Registry/Factory, runtime
   registration, allow/deny. Highlight: SQLite memory uses prepared statements.

6. **Layer 3 — Skills** — Markdown skills, keyword selection, injected into the system prompt.

7. **Layer 4 — The ReAct loop** — `AgentLoop::run()` as Template Method; the
   `{thought, action, action_input}` protocol we parse ourselves; `Action` = `std::variant`.

8. **Loop detection** — generic repeat + ping-pong, Warning/Critical. Why an agent needs it.

9. **Layer 5 — Harness & evaluation** — `Trajectory` JSON, `Evaluator` (Strategy:
   keyword/functional), `HarnessRunner` records via a step hook (Observer).

10. **The 4 design patterns** — one slide mapping Registry/Factory · Template Method · Strategy ·
    Observer to exactly where they live. (Examiner will ask — be ready to open the file.)

11. **C++ techniques** — the 17/20/23/26 feature table; call out `std::expected`, `std::variant`
    + `std::visit`, RAII, and the C++26 `= delete("reason")`.

12. **Benchmark results** — 10 tasks, 100% success rate, step-count-by-difficulty chart; honest
    caveats (sampling, suite-specific).

13. **Difficulties solved** — the four real bugs (std::println link, SSL CA bundle, cmd/c quoting,
    nested-aggregate default arg) — shows real engineering, not generated code.

14. **Live demo** (per brief §9.3):
    - start Ollama, run `./agent "..."` end-to-end;
    - add a new tool the examiner names, register it, run again;
    - run `./run_eval`, open a `trajectory_*.json`;
    - point to one design pattern in the code and explain it.

## Demo cheat-sheet

```bash
ollama serve &
cd build
./agent "Compute 15*17, save it to result.txt, then report it."
ctest --output-on-failure
./run_eval && cat trajectories/summary.json
```

To add a tool live: create `src/tools/<x>_tool.{h,cpp}` subclassing `Tool`, then
`registry.register_tool(std::make_unique<XTool>())` in `main.cpp` / `run_eval.cpp`; rebuild.
