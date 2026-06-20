// =============================================================================
//  tools/exec_tool.h
//  The "exec" tool: runs a shell command and returns its combined stdout+stderr
//  plus the process exit code. This is the agent's own sandboxed shell hatch —
//  the ToolRegistry's allow/deny policy decides whether it may be called, so the
//  tool itself never second-guesses the command, it only stays robust against a
//  null pipe and unbounded output.
// =============================================================================
#pragma once

#include <string>

#include "tools/tool.h"

namespace agent {

// Executes an arbitrary shell command via popen/_pclose and reports what it
// printed alongside its exit status.
class ExecTool : public Tool {
public:
    // Identifier the LLM uses to call this tool.
    std::string name() const override;

    // Capability + argument-format documentation injected into the prompt.
    std::string description() const override;

    // Run `args` as a shell command line; return combined output + exit code.
    ToolResult execute(const std::string& args) override;
};

}  // namespace agent
