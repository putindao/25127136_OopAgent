// =============================================================================
//  main.cpp — temporary driver to exercise the LLM Client layer (Phase 1).
//  Later phases turn this into the real agent CLI; for now it proves that
//  OllamaClient can hold a real conversation and report telemetry + errors.
// =============================================================================
#include <print>   // C++23: std::println
#include <vector>

#include "client/ollama_client.h"

int main() {
    using namespace agent;

    // C++20 designated initializers: the config reads like a spec sheet.
    OllamaClient client(LLMConfig{
        .model           = "gemma4",
        .temperature     = 0.2,
        .max_tokens      = 128,
        .timeout_seconds = 180,
    });

    const std::vector<ChatMessage> conversation = {
        {.role = "system", .content = "You are terse. Answer in one short sentence."},
        {.role = "user",   .content = "What is the capital of Vietnam?"},
    };

    std::println("Calling Ollama model '{}' ...", client.config().model);

    // The whole point of std::expected: handle both outcomes explicitly.
    if (const LLMResult result = client.chat(conversation); result.has_value()) {
        std::println("Reply   : {}", result->content);
        std::println("Tokens  : prompt={} completion={}",
                     result->prompt_tokens, result->completion_tokens);
        std::println("Latency : {} ms", result->latency_ms);
        return 0;
    } else {
        const LLMError& err = result.error();
        std::println("ERROR [{}]: {}", to_string(err.kind), err.message);
        return 1;
    }
}
