// =============================================================================
//  tools/web_search_tool.cpp
//  HTTP + JSON plumbing for the "web_search" tool. The libcurl easy handle and
//  every curl-allocated buffer are wrapped in RAII (std::unique_ptr with custom
//  deleters) so an early return or a thrown exception can never leak them. The
//  query is URL-encoded with curl_easy_escape, the GET runs against DuckDuckGo's
//  keyless Instant Answer API, and the JSON reply is parsed defensively.
// =============================================================================
#include "tools/web_search_tool.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

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

// RAII: a string returned by curl_easy_escape is freed with curl_free.
struct CurlStrDeleter {
    void operator()(char* s) const noexcept { if (s) curl_free(s); }
};
using CurlStr = std::unique_ptr<char, CurlStrDeleter>;

// libcurl write callback: append received bytes to the std::string in userdata.
std::size_t write_to_string(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    const std::size_t total = size * nmemb;
    out->append(ptr, total);
    return total;
}

// How many RelatedTopics entries to stitch together when there is no
// AbstractText to return directly.
constexpr int kMaxRelatedTopics = 3;

// Request timeout in seconds (the spec asks for about 15).
constexpr long kTimeoutSeconds = 15;

// Locate a usable CA-certificate bundle for HTTPS verification. Returns an empty
// string if none is found (libcurl then keeps its built-in default). On
// MSYS2/Windows the compiled-in default is often a 0-byte broken symlink, which
// fails as CURLE_SSL_CACERT_BADFILE, so we probe known locations and accept only
// a non-empty regular file. Environment overrides win for portability.
std::string resolve_ca_bundle() {
    namespace fs = std::filesystem;
    const auto usable = [](const std::string& p) {
        std::error_code ec;
        return !p.empty() && fs::is_regular_file(p, ec) && fs::file_size(p, ec) > 0;
    };
    for (const char* var : {"CURL_CA_BUNDLE", "SSL_CERT_FILE"}) {
        if (const char* v = std::getenv(var); v && usable(v)) return v;
    }
    static const char* const kCandidates[] = {
        "C:/msys64/usr/ssl/certs/ca-bundle.crt",        // MSYS2 (the real bundle)
        "C:/msys64/ucrt64/ssl/certs/ca-bundle.crt",     // MSYS2 UCRT64 (if populated)
        "C:/msys64/mingw64/etc/ssl/certs/ca-bundle.crt",// MSYS2 MINGW64
        "/etc/ssl/certs/ca-certificates.crt",           // Debian/Ubuntu/Arch
        "/etc/pki/tls/certs/ca-bundle.crt",             // Fedora/RHEL
        "/usr/ssl/certs/ca-bundle.crt",                 // misc
    };
    for (const char* c : kCandidates) {
        if (usable(c)) return c;
    }
    return {};
}

}  // namespace

std::string WebSearchTool::name() const {
    return "web_search";
}

std::string WebSearchTool::description() const {
    return "Searches the web via the keyless DuckDuckGo Instant Answer API and "
           "returns a short textual answer. Args: the search query as plain text "
           "(e.g. \"capital of France\"). Returns the instant answer when one "
           "exists, otherwise a summary stitched from the first few related "
           "topics; returns a \"no instant answer found\" message when nothing "
           "matches.";
}

ToolResult WebSearchTool::execute(const std::string& args) {
    if (args.empty()) {
        return std::unexpected(ToolError{"web_search: empty query"});
    }

    ensure_curl_global();

    CurlEasy curl{curl_easy_init()};
    if (!curl) {
        return std::unexpected(ToolError{"web_search: curl_easy_init failed"});
    }

    // URL-encode the raw query so spaces and reserved characters are safe.
    CurlStr encoded{curl_easy_escape(curl.get(), args.c_str(), static_cast<int>(args.size()))};
    if (!encoded) {
        return std::unexpected(ToolError{"web_search: failed to URL-encode query"});
    }

    const std::string url = std::string("https://api.duckduckgo.com/?q=") + encoded.get() +
                            "&format=json&no_html=1&skip_disambig=1";

    std::string response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);  // the API may redirect
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, kTimeoutSeconds);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "agent-web-search/1.0");

    // Point libcurl at a usable CA bundle so HTTPS verification works even when
    // the platform default is missing or a broken 0-byte symlink (MSYS2).
    if (const std::string ca_bundle = resolve_ca_bundle(); !ca_bundle.empty()) {
        curl_easy_setopt(curl.get(), CURLOPT_CAINFO, ca_bundle.c_str());
    }

    const CURLcode rc = curl_easy_perform(curl.get());
    if (rc != CURLE_OK) {
        return std::unexpected(ToolError{
            std::string("web_search: network error: ") + curl_easy_strerror(rc)});
    }

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) {
        return std::unexpected(ToolError{"web_search: HTTP " + std::to_string(status)});
    }

    // ---- Parse defensively: malformed JSON is a real, expected failure ------
    try {
        const nlohmann::json j = nlohmann::json::parse(response);

        // Prefer the instant-answer abstract when it is present and non-empty.
        const std::string abstract = j.value("AbstractText", std::string{});
        if (!abstract.empty()) {
            return abstract;
        }

        // Otherwise stitch together the first few related-topic snippets.
        std::string stitched;
        int taken = 0;
        if (auto it = j.find("RelatedTopics"); it != j.end() && it->is_array()) {
            for (const auto& topic : *it) {
                if (taken >= kMaxRelatedTopics) break;
                const std::string text = topic.value("Text", std::string{});
                if (text.empty()) continue;  // skip nested category groups
                if (!stitched.empty()) stitched += "\n";
                stitched += "- " + text;
                ++taken;
            }
        }
        if (!stitched.empty()) {
            return stitched;
        }

        return std::string("web_search: no instant answer found");
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(ToolError{
            std::string("web_search: failed to parse JSON response: ") + e.what()});
    }
}

}  // namespace agent
