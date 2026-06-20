// =============================================================================
//  agent/agent_loop.cpp — the ReAct skeleton and its default step behaviours.
// =============================================================================
#include "agent/agent_loop.h"

#include <chrono>
#include <exception>
#include <print>
#include <type_traits>
#include <utility>
#include <variant>

namespace agent {
namespace {

// The response protocol the model must follow on every step. Kept terse and
// explicit because compliance is what makes the parser's job easy.
constexpr const char* kProtocol =
    "You solve the task step by step, using tools when needed.\n"
    "On EACH step reply with EXACTLY ONE JSON object and NOTHING else:\n"
    "{\"thought\": \"<brief reasoning>\", \"action\": \"<a tool name or 'final'>\", "
    "\"action_input\": \"<tool argument, or your final answer>\"}\n"
    "Rules:\n"
    "- To use a tool, set \"action\" to its exact name and \"action_input\" to its argument.\n"
    "- You then receive an \"Observation\"; use it to decide the next step.\n"
    "- When you can answer the task, set \"action\" to \"final\" and put the full answer "
    "in \"action_input\".\n"
    "- Output only one JSON object. Do not wrap it in code fences or extra prose.";

long ms_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start)
        .count();
}

}  // namespace

AgentLoop::AgentLoop(LLMClient& client, ToolRegistry& registry, AgentConfig config)
    : client_(client), registry_(registry), config_(config) {}

std::string AgentLoop::tool_catalog() const {
    std::string out;
    for (const Tool* tool : registry_.list()) {
        out += "- " + tool->name() + ": " + tool->description() + "\n";
    }
    return out;
}

std::vector<ChatMessage> AgentLoop::build_initial_messages(const std::string& task) {
    std::string system = system_prompt_.empty() ? "You are a capable autonomous agent."
                                                : system_prompt_;
    system += "\n\n";
    system += kProtocol;
    system += "\n\n## Available tools\n";
    system += tool_catalog();

    return {
        {.role = "system", .content = std::move(system)},
        {.role = "user",   .content = task},
    };
}

std::expected<Action, std::string>
AgentLoop::think(std::vector<ChatMessage>& history, Step& step) {
    const LLMResult resp = client_.chat(history);
    if (!resp) {
        // Transport/HTTP/parse failure from the model server itself.
        return std::unexpected("LLM error: " + resp.error().message);
    }
    step.tokens_used = resp->prompt_tokens + resp->completion_tokens;
    step.latency_ms  = resp->latency_ms;

    // Record the model's raw turn so the conversation stays well-formed.
    history.push_back({.role = "assistant", .content = resp->content});

    auto action = parse_action(resp->content);
    if (action) {
        std::visit([&](const auto& a) { step.thought = a.thought; }, *action);
    }
    return action;
}

std::string AgentLoop::act(const ToolCall& call) {
    Tool* tool = registry_.get(call.tool);
    if (!tool) {
        return "Error: unknown or denied tool '" + call.tool + "'";
    }
    try {
        if (const ToolResult result = tool->execute(call.args); result.has_value()) {
            return *result;
        } else {
            return "Error: " + result.error().message;
        }
    } catch (const std::exception& e) {
        return std::string("Error: tool threw an exception: ") + e.what();
    }
}

void AgentLoop::observe(std::vector<ChatMessage>& history,
                        const ToolCall& call, const std::string& observation) {
    std::string obs = observation;
    if (obs.size() > config_.max_observation_chars) {
        obs.resize(config_.max_observation_chars);
        obs += "\n...[truncated]";
    }
    history.push_back({.role    = "user",
                       .content = "Observation from " + call.tool + ": " + obs});
}

AgentResult AgentLoop::run(const std::string& task) {
    AgentResult result;
    std::vector<ChatMessage> history = build_initial_messages(task);
    const auto t_start = std::chrono::steady_clock::now();

    int consecutive_failures = 0;

    for (int step_id = 0; step_id < config_.max_steps; ++step_id) {
        Step step;
        step.step_id = step_id;

        // ---- THINK ----------------------------------------------------------
        auto decision = think(history, step);
        if (!decision) {
            step.action      = "error";
            step.observation = decision.error();
            if (++consecutive_failures >= 2) {
                result.stop_reason = "error";
                if (step_hook_) step_hook_(step);
                result.steps.push_back(step);
                result.total_tokens += step.tokens_used;
                break;
            }
            // Nudge the model back to the required format and retry.
            history.push_back({.role = "user",
                               .content = "Your previous reply was not a single valid JSON "
                                          "action. Reply with exactly one JSON object "
                                          "{\"thought\",\"action\",\"action_input\"}."});
            if (step_hook_) step_hook_(step);
            result.steps.push_back(step);
            result.total_tokens += step.tokens_used;
            if (config_.verbose) std::println("[step {}] parse error: {}", step_id, decision.error());
            continue;
        }
        consecutive_failures = 0;

        // ---- ACT / FINISH (dispatch over the variant) -----------------------
        bool finished = false;
        std::visit(
            [&](const auto& a) {
                using T = std::decay_t<decltype(a)>;
                if constexpr (std::is_same_v<T, FinalAnswer>) {
                    step.action       = "final";
                    step.action_input = a.text;
                    result.success      = true;
                    result.final_answer = a.text;
                    result.stop_reason  = "final";
                    finished = true;
                } else if constexpr (std::is_same_v<T, ToolCall>) {
                    step.action       = a.tool;
                    step.action_input = a.args;

                    // Loop guard before spending a tool call.
                    if (loop_detector_) {
                        const auto verdict = loop_detector_->observe(a.tool + "|" + a.args);
                        if (verdict.severity == LoopDetector::Severity::Critical) {
                            step.observation   = "[loop detected] " + verdict.reason;
                            result.stop_reason = "loop_detected";
                            finished = true;
                        } else if (verdict.severity == LoopDetector::Severity::Warning &&
                                   config_.verbose) {
                            std::println("[step {}] warning: {}", step.step_id, verdict.reason);
                        }
                    }
                    if (!finished) {
                        step.observation = act(a);
                        observe(history, a, step.observation);
                    }
                }
            },
            *decision);

        if (config_.verbose) {
            std::println("[step {}] action={} input={}", step.step_id, step.action, step.action_input);
            if (!step.observation.empty()) {
                std::println("           observation: {}", step.observation);
            }
        }
        if (step_hook_) step_hook_(step);
        result.steps.push_back(step);
        result.total_tokens += step.tokens_used;

        if (finished) break;
    }

    if (result.stop_reason.empty()) result.stop_reason = "max_steps";
    result.total_time_ms = ms_since(t_start);
    return result;
}

}  // namespace agent
