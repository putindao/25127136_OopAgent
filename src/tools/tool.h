// =============================================================================
//  tools/tool.h
//  The Tool abstraction. Every capability the agent can invoke (run a shell
//  command, read a file, search the web, do arithmetic, remember things) is a
//  Tool subclass. Tools know NOTHING about the AgentLoop — they only take a
//  string of arguments and return a result. This keeps the dependency arrow
//  pointing one way (AgentLoop -> Tool, never the reverse), a graded invariant.
// =============================================================================
#pragma once

#include <expected>
#include <string>

namespace agent {

// A tool failure carries a human-readable message (surfaced to the LLM so it
// can recover on the next ReAct step).
struct ToolError {
    std::string message;
};

// Success carries the tool's textual output; failure carries a ToolError.
// Same std::expected discipline as the LLM layer for a uniform error style.
using ToolResult = std::expected<std::string, ToolError>;

// Abstract base for every tool (pure virtual = cannot be instantiated).
class Tool {
public:
    virtual ~Tool() = default;

    // Unique identifier the LLM uses to call this tool (e.g. "calculator").
    virtual std::string name() const = 0;

    // One-line capability description + expected argument format. This text is
    // injected into the prompt, so the model relies on it to call correctly.
    virtual std::string description() const = 0;

    // Run the tool. `args` is the raw argument string produced by the model
    // (each tool documents whether it expects plain text or a JSON object).
    virtual ToolResult execute(const std::string& args) = 0;
};

}  // namespace agent
