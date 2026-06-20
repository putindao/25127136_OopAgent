// =============================================================================
//  tools/web_search_tool.h
//  The "web_search" tool: queries the keyless DuckDuckGo Instant Answer API and
//  returns a short textual answer. Like every Tool it knows nothing about the
//  AgentLoop — it takes a plain-text query and returns a result string. All HTTP
//  work happens behind execute(); a failure (network, HTTP, malformed JSON) maps
//  to a clear ToolError instead of an exception escaping execute().
// =============================================================================
#pragma once

#include <string>

#include "tools/tool.h"

namespace agent {

// Searches the web via DuckDuckGo's Instant Answer API and returns the instant
// answer (or a stitched summary of related topics). Stateless and reusable.
class WebSearchTool : public Tool {
public:
    // Identifier the LLM uses to call this tool.
    std::string name() const override;

    // Capability + argument-format documentation injected into the prompt.
    std::string description() const override;

    // Run the search for the query in `args`; return the instant answer text.
    ToolResult execute(const std::string& args) override;
};

}  // namespace agent
