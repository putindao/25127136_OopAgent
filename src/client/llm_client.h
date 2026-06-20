// =============================================================================
//  client/llm_client.h
//  Provider-agnostic LLM interface. This is the most important abstraction in
//  the system: everything above it (AgentLoop, Harness) depends ONLY on this
//  interface, never on Ollama specifics. Swapping Ollama for OpenAI is then a
//  single new subclass with zero changes upstream (a graded design invariant).
// =============================================================================
#pragma once

#include <expected>   // C++23: chat() returns a value OR a structured error
#include <string>
#include <vector>

namespace agent {

// One turn in a conversation, in the vocabulary Ollama/OpenAI expect.
struct ChatMessage {
    std::string role{};                 // "system" | "user" | "assistant" | "tool"
    std::string content{};
    std::vector<std::string> images{};  // optional base64 images (multimodal)
};

// Everything needed to reach and steer a model. Intentionally generic.
struct LLMConfig {
    std::string base_url        = "http://localhost:11434";
    std::string model           = "gemma4";
    double      temperature     = 0.7;
    int         max_tokens      = 2048;
    int         timeout_seconds = 120;
};

// A successful reply plus the telemetry the Harness later records per step.
struct LLMResponse {
    std::string content;
    int  prompt_tokens     = 0;
    int  completion_tokens = 0;
    long latency_ms        = 0;
};

// Why a call failed — callers react differently per cause (retry vs abort).
enum class LLMErrorKind { ConnectionFailed, Timeout, HttpError, MalformedResponse, Unknown };

struct LLMError {
    LLMErrorKind kind = LLMErrorKind::Unknown;
    std::string  message;
    long         http_status = 0;
};

// C++23 std::expected: success carries LLMResponse, failure carries LLMError.
// No exceptions on the hot path; the caller must look at the error explicitly.
using LLMResult = std::expected<LLMResponse, LLMError>;

// Abstract base (pure virtual) — the provider-agnostic contract.
class LLMClient {
public:
    virtual ~LLMClient() = default;

    // Send the whole conversation; receive exactly one assistant turn back.
    virtual LLMResult chat(const std::vector<ChatMessage>& messages) = 0;

    // Read-only view of the active configuration.
    virtual const LLMConfig& config() const noexcept = 0;
};

// Human-readable error kind, used in logs and trajectory JSON.
std::string to_string(LLMErrorKind kind);

}  // namespace agent
