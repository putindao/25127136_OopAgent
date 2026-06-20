---
name: error_recovery
description: Diagnose and recover from a failed tool call without looping
keywords: [error, fail, failed, failure, exception, retry, recover, broken, not working, debug, fix, cannot]
---

# Error Recovery

A tool returned an error. Treat the error text as information, not a dead end.

1. **Read the actual error message.** It usually names the cause (file not found,
   division by zero, network error, malformed arguments).
2. **Form a hypothesis** about why it failed before doing anything else.
3. **Change something meaningful** on the next attempt. Never repeat the exact same
   tool call with the exact same arguments — that is a loop and it will fail again.
   - File not found -> check the path, or create the file first with `write_file`.
   - Bad arguments -> re-read the tool's argument format and fix the JSON shape.
   - Network/web error -> try a shorter or differently worded query, or proceed
     with what you already know.
4. **Bound your attempts.** If two or three varied attempts still fail, stop and
   report what you tried and what the obstacle is. Do not keep trying forever.
5. **Preserve progress.** If earlier steps succeeded, keep their results; only redo
   the step that failed.

Failing gracefully with a clear explanation is a correct outcome. Looping is not.
