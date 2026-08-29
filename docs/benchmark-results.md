# Benchmark Results

Model: **gemma4** (Ollama, local) · temperature 0.1 · ReAct agent with JSON tool-call protocol  
Harness: `run_eval` over `benchmark/tasks.json` · evaluators: keyword and functional

## Summary

| Metric | Value |
|---|---|
| Tasks | 10 (4 easy · 4 medium · 2 hard) |
| **Success rate** | **10 / 10 = 100%** |
| Total steps | 31 |
| Total tokens | 32,394 |
| Total execution time | 88,299 ms (~88.3 seconds) |

## Per-task results

| Task | Difficulty | Evaluator | Main tools exercised | Steps | Tokens | Result |
|---|---|---|---|---:|---:|---|
| task_001 | Easy | Keyword | `calculator` | 2 | 1,707 | PASS |
| task_002 | Easy | Functional | `calculator` → `write_file` | 3 | 2,754 | PASS |
| task_003 | Easy | Keyword | `calculator` | 2 | 1,732 | PASS |
| task_004 | Easy | Functional | `write_file` → `read_file` | 3 | 2,767 | PASS |
| task_005 | Medium | Keyword | `calculator` → `exec` | 3 | 2,685 | PASS |
| task_006 | Medium | Keyword | `web_search` | 2 | 2,505 | PASS |
| task_007 | Medium | Keyword | `memory_save` → `memory_search` | 3 | 3,417 | PASS |
| task_008 | Medium | Keyword | `memory_save` ×2 → `memory_search` | 4 | 4,741 | PASS |
| task_009 | Hard | Functional | `calculator` ×N → `write_file` | 5 | 5,251 | PASS |
| task_010 | Hard | Keyword | `calculator` → conditional memory operations | 4 | 4,835 | PASS |

## Difficulty analysis

| Difficulty | Tasks | Total steps | Average steps |
|---|---:|---:|---:|
| Easy | 4 | 10 | 2.5 |
| Medium | 4 | 12 | 3.0 |
| Hard | 2 | 9 | 4.5 |

The average step count increases with task difficulty. Easy tasks generally require
one tool call followed by a final answer, while hard tasks require multi-step planning,
multiple tool calls, and conditional decisions.

## Tool coverage

The benchmark exercises all five base tool classes:

- `CalculatorTool`
- `FileTool`
- `ExecTool`
- `WebSearchTool`
- `MemoryTool`

It also exercises both evaluator strategies:

- `KeywordEvaluator`
- `FunctionalEvaluator`

The three extended tools—`CurrentTimeTool`, `TextStatsTool`, and `JsonQueryTool`—are
verified separately by deterministic unit tests. The benchmark does not claim to exercise
every registered tool name.

## Observations

- All 10 tasks completed with `stop_reason` equal to `final`.
- Hard tasks required up to five ReAct steps.
- Token usage increased with the number of steps because the growing conversation and
  tool observations are sent back to the model on each iteration.
- `task_006` explicitly exercised `web_search`.
- The final source version contains 8 tool classes and 10 registered tool names.

## Caveats

- The 100% result applies to this specific 10-task benchmark.
- LLM output is non-deterministic, so repeated runs may produce different token counts,
  execution times, or occasional task failures.
- The benchmark was executed locally with temperature 0.1.
- The extended tools are covered by deterministic unit tests rather than benchmark tasks.

## Reproduce

From the repository root on Windows:

```bat
build\run_eval.exe benchmark\tasks.json benchmark\sample_output_final
```

On Linux, macOS, Git Bash, or MSYS2:

```bash
./build/run_eval benchmark/tasks.json benchmark/sample_output_final
```

Each run exports `summary.json` and one JSON trajectory for every task.

The verified run is stored in [benchmark/sample_output/](../benchmark/sample_output/).