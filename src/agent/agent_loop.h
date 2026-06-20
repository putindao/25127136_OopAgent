// =============================================================================
//  agent/agent_loop.h
//  The ReAct loop — the heart of the framework. run() is a TEMPLATE METHOD: it
//  fixes the skeleton (build messages -> think -> act -> observe, bounded by
//  max_steps, guarded by a LoopDetector) while the per-step behaviours think(),
//  act(), observe() are protected virtuals a subclass may override.
//
//  Crucially, AgentLoop does NOT know the Harness exists. It only exposes a
//  StepHook (Observer/Hook pattern); the Harness subscribes to record a
//  trajectory. Dependencies point downward only: AgentLoop -> LLMClient, Tool,
//  Skill — never upward.
// =============================================================================
#pragma once

#include <cstddef>
#include <expected>
#include <functional>
#include <string>
#include <vector>

#include "agent/action.h"
#include "agent/loop_detector.h"
#include "client/llm_client.h"
#include "tools/tool_registry.h"

namespace agent {

// One recorded step of a run (also the unit the Harness turns into trajectory
// JSON in Phase 5).
struct Step {
    int         step_id = 0;
    std::string thought;
    std::string action;        // tool name, or "final", or "error"
    std::string action_input;
    std::string observation;   // tool result (empty for a final answer)
    int         tokens_used = 0;
    long        latency_ms  = 0;
};

// The outcome of a full run.
struct AgentResult {
    bool              success = false;
    std::string       final_answer;
    std::string       stop_reason;   // final | max_steps | loop_detected | error
    std::vector<Step> steps;
    int               total_tokens  = 0;
    long              total_time_ms = 0;
};

struct AgentConfig {
    int         max_steps             = 8;
    bool        verbose               = true;   // print each step to stdout
    std::size_t max_observation_chars = 2000;   // truncate big tool output in history
};

class AgentLoop {
public:
    // Called after every step (Observer/Hook). Never owns the AgentLoop.
    using StepHook = std::function<void(const Step&)>;

    AgentLoop(LLMClient& client, ToolRegistry& registry, AgentConfig config = AgentConfig{});
    virtual ~AgentLoop() = default;

    // Persona + injected skill guidance (the protocol + tool list are appended
    // automatically by build_initial_messages).
    void set_system_prompt(std::string system_prompt) { system_prompt_ = std::move(system_prompt); }
    void set_step_hook(StepHook hook) { step_hook_ = std::move(hook); }
    void set_loop_detector(LoopDetector* detector) { loop_detector_ = detector; }

    // TEMPLATE METHOD: the fixed skeleton. Do not override; override the steps.
    AgentResult run(const std::string& task);

protected:
    // ---- overridable skeleton steps ----------------------------------------
    virtual std::vector<ChatMessage> build_initial_messages(const std::string& task);
    // Think: ask the model, parse one Action. Fills telemetry into `step`.
    virtual std::expected<Action, std::string> think(std::vector<ChatMessage>& history, Step& step);
    // Act: execute a tool call and return its observation text.
    virtual std::string act(const ToolCall& call);
    // Observe: fold an observation back into the conversation history.
    virtual void observe(std::vector<ChatMessage>& history,
                         const ToolCall& call, const std::string& observation);

    // Formats the registry's allowed tools for the system prompt.
    std::string tool_catalog() const;

    LLMClient&    client_;
    ToolRegistry& registry_;
    AgentConfig   config_;
    std::string   system_prompt_;
    StepHook      step_hook_;
    LoopDetector* loop_detector_ = nullptr;  // optional, not owned
};

}  // namespace agent
