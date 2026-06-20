---
name: task_planner
description: Break a complex request into an ordered, verifiable plan before acting
keywords: [plan, planning, steps, step by step, break down, multi-step, organize, strategy, complex, workflow]
---

# Task Planner

When a request needs more than one action, plan before you act.

1. **Restate the goal** in one sentence so the success condition is explicit.
2. **Decompose** the goal into the smallest ordered steps. Each step should map to a
   single tool call when possible (calculate, read a file, search, remember, run a command).
3. **Identify dependencies**: if step B needs the output of step A, do A first and keep
   its result in working memory.
4. **Execute one step at a time.** After each tool result, check it against what you
   expected before moving on. Do not batch unrelated actions into one step.
5. **Verify the end state.** Before declaring success, confirm the goal's success
   condition actually holds (for example, re-read the file you were asked to write).

Prefer the smallest plan that satisfies the goal. If a step fails, switch to the
`error_recovery` guidance rather than repeating the same failing action.
