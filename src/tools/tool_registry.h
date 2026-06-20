// =============================================================================
//  tools/tool_registry.h
//  REGISTRY / FACTORY pattern (one of the four required patterns).
//  Tools are registered at runtime (not hardcoded into the agent): either as a
//  ready instance, or as a name -> factory callback that builds the tool lazily
//  on first use. A per-name allow/deny policy gates what the agent may call.
// =============================================================================
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tools/tool.h"

namespace agent {

class ToolRegistry {
public:
    // A Factory builds a Tool on demand (the "Factory" half of the pattern).
    using Factory = std::function<std::unique_ptr<Tool>()>;

    // Register a fully-constructed tool. The registry takes ownership.
    void register_tool(std::unique_ptr<Tool> tool);

    // Register a factory; the tool is created lazily the first time get() asks.
    void register_factory(std::string name, Factory factory);

    // Look up a tool by name. Returns nullptr if unknown or denied by policy.
    // Lazily instantiates from a factory on first request (hence non-const).
    Tool* get(const std::string& name);

    // All currently-instantiated tools that policy allows (for the prompt).
    std::vector<Tool*> list() const;

    // ---- allow/deny policy --------------------------------------------------
    // Deny wins over allow. If the allow-list is non-empty it becomes a
    // whitelist (only listed names are usable); empty = allow all but denied.
    void allow(const std::string& name);
    void deny(const std::string& name);
    bool is_allowed(const std::string& name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
    std::unordered_map<std::string, Factory>               factories_;
    std::unordered_set<std::string>                        allowed_;  // whitelist
    std::unordered_set<std::string>                        denied_;   // blacklist
};

}  // namespace agent
