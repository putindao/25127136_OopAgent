// =============================================================================
//  client/ollama_client.h
//  Concrete LLMClient that talks to a local Ollama server via /api/chat.
//  Marked `final`: it is a leaf in the hierarchy, not meant to be subclassed.
// =============================================================================
#pragma once

#include "client/llm_client.h"

namespace agent {

class OllamaClient final : public LLMClient {
public:
    explicit OllamaClient(LLMConfig config);

    // Implement the LLMClient contract.
    LLMResult chat(const std::vector<ChatMessage>& messages) override;
    const LLMConfig& config() const noexcept override { return config_; }

private:
    // Low-level HTTP POST. Returns the raw response body on 2xx, or an LLMError
    // describing a transport failure (connection refused / timeout / non-2xx).
    std::expected<std::string, LLMError>
    http_post(const std::string& url, const std::string& json_body);

    LLMConfig config_;
};

}  // namespace agent
