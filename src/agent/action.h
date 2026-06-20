// =============================================================================
//  agent/action.h
//  The decision the model makes on each ReAct step, modelled as a std::variant
//  so the agent loop can dispatch with std::visit / if constexpr instead of tag
//  enums and casts. Two cases for the core loop: call a tool, or finish.
//  (The GUI bonus later adds Click / TypeText / KeyPress to the same variant.)
// =============================================================================
#pragma once

#include <expected>
#include <string>
#include <variant>

namespace agent {

// The agent wants to invoke a tool.
struct ToolCall {
    std::string thought;  // the reasoning the model gave for this step
    std::string tool;     // tool name (must match a ToolRegistry entry)
    std::string args;     // argument string passed to Tool::execute
};

// The agent is done and returns its final answer.
struct FinalAnswer {
    std::string thought;
    std::string text;
};

// One step's decision.
using Action = std::variant<ToolCall, FinalAnswer>;

// Parse one raw LLM response into an Action. The model is asked to emit a single
// JSON object {"thought","action","action_input"}; real models often wrap it in
// prose or ``` fences, so we extract the first balanced {...} block first.
// Returns an error string when no usable action can be recovered.
std::expected<Action, std::string> parse_action(const std::string& llm_text);

}  // namespace agent
