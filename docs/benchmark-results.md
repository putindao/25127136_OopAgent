# Benchmark Results

Model: **gemma4** (Ollama, local) · temperature 0.1 · ReAct agent, JSON tool-call protocol
Harness: `run_eval` over `benchmark/tasks.json` · evaluators: keyword + functional (bash)

## Summary

| Metric | Value |
|--------|-------|
| Tasks | 10 (4 easy · 4 medium · 2 hard) |
| **Success rate** | **10 / 10 = 100%** |
| Total steps | 33 |
| Total tokens | ~30,577 |

## Per-task results

| Task | Difficulty | Eval | Tools exercised | Steps | Tokens | Result |
|------|-----------|------|-----------------|-------|--------|--------|
| task_001 | easy | keyword | calculator | 2 | 1550 | PASS |
| task_002 | easy | functional | calculator → write_file | 3 | 2471 | PASS |
| task_003 | easy | keyword | calculator (parentheses) | 2 | 1543 | PASS |
| task_004 | easy | functional | write_file → read_file | 3 | 2464 | PASS |
| task_005 | medium | keyword | calculator → exec | 3 | 2353 | PASS |
| task_006 | medium | functional | calculator → write_file → read_file | 4 | 3432 | PASS |
| task_007 | medium | keyword | memory_save → memory_search | 3 | 3294 | PASS |
| task_008 | medium | keyword | memory_save ×2 → memory_search | 4 | 4300 | PASS |
| task_009 | hard | functional | calculator ×N → write_file | 5 | 4707 | PASS |
| task_010 | hard | keyword | calculator → (conditional) memory_save → memory_search | 4 | 4463 | PASS |

## Analysis

- **Step count scales with task complexity** (easy ~2.5, medium ~3.5, hard ~4.5 steps),
  which confirms the agent is genuinely decomposing tasks rather than guessing in one shot.
- **Hard tasks passed**: task_009 required the model to compute two sub-products and sum
  them before writing the file; task_010 required a *conditional* decision based on a
  computed value. Both succeeded, exercising multi-step planning and tool sequencing.
- **All five tool classes were exercised** across the suite (calculator, file r/w, exec,
  memory save/search), plus both evaluator strategies (keyword + functional/bash).
- **Token cost grows with steps** (1.5k for a one-tool task up to ~4.7k for the hardest),
  dominated by re-sending the growing conversation history each step.

## Caveats / honesty notes

- 100% reflects this 10-task suite with a low temperature (0.1) and tasks whose tool
  sequence is discoverable; harder open-ended tasks would lower the rate. The number is a
  property of *this* benchmark, not a general capability claim.
- Results are non-deterministic (LLM sampling); re-running may vary by a task or two.

## Reproduce

```bash
cmake --build build
cd build && ./run_eval.exe          # uses ../benchmark/tasks.json
# per-task trajectory_<id>.json + summary.json land in build/trajectories/
```

A captured example run lives in [benchmark/sample_output/](../benchmark/sample_output/).
