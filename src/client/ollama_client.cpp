// =============================================================================
//  client/ollama_client.cpp
//  HTTP + JSON plumbing for the Ollama backend. All libcurl handles are wrapped
//  in RAII (std::unique_ptr with custom deleters) so an early return or thrown
//  exception can never leak a handle.
// =============================================================================
#include "client/ollama_client.h"

#include <chrono>
#include <memory>
#include <utility>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace agent {
namespace {

// Initialize libcurl exactly once per process. The function-local static is
// thread-safe to construct since C++11, and the destructor runs at exit.
void ensure_curl_global() {
    struct Global {
        Global()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
        ~Global() { curl_global_cleanup(); }
    };
    static Global g;
}

// RAII: a CURL* easy handle is freed automatically.
struct CurlEasyDeleter {
    void operator()(CURL* h) const noexcept { if (h) curl_easy_cleanup(h); }
};
using CurlEasy = std::unique_ptr<CURL, CurlEasyDeleter>;

// RAII: a curl_slist (header list) is freed automatically.
struct CurlSlistDeleter {
    void operator()(curl_slist* l) const noexcept { if (l) curl_slist_free_all(l); }
};
using CurlSlist = std::unique_ptr<curl_slist, CurlSlistDeleter>;

// libcurl write callback: append received bytes to the std::string in userdata.
std::size_t write_to_string(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    const std::size_t total = size * nmemb;
    out->append(ptr, total);
    return total;
}

}  // namespace

OllamaClient::OllamaClient(LLMConfig config) : config_(std::move(config)) {
    ensure_curl_global();
}

std::expected<std::string, LLMError>
OllamaClient::http_post(const std::string& url, const std::string& json_body) {
    CurlEasy curl{curl_easy_init()};
    if (!curl) {
        return std::unexpected(LLMError{LLMErrorKind::Unknown, "curl_easy_init failed", 0});
    }

    std::string response;
    CurlSlist headers{curl_slist_append(nullptr, "Content-Type: application/json")};

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(config_.timeout_seconds));

    const CURLcode rc = curl_easy_perform(curl.get());
    if (rc != CURLE_OK) {
        LLMErrorKind kind = LLMErrorKind::Unknown;
        if (rc == CURLE_OPERATION_TIMEDOUT)    kind = LLMErrorKind::Timeout;
        else if (rc == CURLE_COULDNT_CONNECT)  kind = LLMErrorKind::ConnectionFailed;
        return std::unexpected(LLMError{kind, curl_easy_strerror(rc), 0});
    }

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) {
        return std::unexpected(LLMError{
            LLMErrorKind::HttpError, "HTTP " + std::to_string(status) + ": " + response, status});
    }
    return response;
}

LLMResult OllamaClient::chat(const std::vector<ChatMessage>& messages) {
    // ---- Build the /api/chat request body -----------------------------------
    nlohmann::json body;
    body["model"]   = config_.model;
    body["stream"]  = false;  // we want one complete JSON object, not a stream
    body["options"] = {
        {"temperature", config_.temperature},
        {"num_predict", config_.max_tokens},  // Ollama's name for max output tokens
    };

    nlohmann::json jmsgs = nlohmann::json::array();
    for (const auto& m : messages) {
        nlohmann::json jm = {{"role", m.role}, {"content", m.content}};
        if (!m.images.empty()) jm["images"] = m.images;  // multimodal path
        jmsgs.push_back(std::move(jm));
    }
    body["messages"] = std::move(jmsgs);

    // ---- Send it, timing the round-trip -------------------------------------
    const std::string url = config_.base_url + "/api/chat";
    const auto start = std::chrono::steady_clock::now();
    auto raw = http_post(url, body.dump());
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count();

    if (!raw) return std::unexpected(raw.error());  // propagate transport error

    // ---- Parse defensively: malformed JSON is a real, expected failure ------
    try {
        const nlohmann::json j = nlohmann::json::parse(*raw);
        LLMResponse out;
        out.content           = j.at("message").at("content").get<std::string>();
        out.prompt_tokens     = j.value("prompt_eval_count", 0);
        out.completion_tokens = j.value("eval_count", 0);
        out.latency_ms        = static_cast<long>(elapsed_ms);
        return out;
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(LLMError{LLMErrorKind::MalformedResponse, e.what(), 0});
    }
}

std::string to_string(LLMErrorKind kind) {
    switch (kind) {
        case LLMErrorKind::ConnectionFailed:  return "ConnectionFailed";
        case LLMErrorKind::Timeout:           return "Timeout";
        case LLMErrorKind::HttpError:         return "HttpError";
        case LLMErrorKind::MalformedResponse: return "MalformedResponse";
        case LLMErrorKind::Unknown:           return "Unknown";
    }
    return "Unknown";
}

}  // namespace agent
