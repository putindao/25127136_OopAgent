// =============================================================================
//  tools/tool_registry.cpp — Registry/Factory implementation + policy logic.
// =============================================================================
#include "tools/tool_registry.h"

#include <utility>

namespace agent {

void ToolRegistry::register_tool(std::unique_ptr<Tool> tool) {
    if (!tool) return;
    tools_[tool->name()] = std::move(tool);
}

void ToolRegistry::register_factory(std::string name, Factory factory) {
    factories_[std::move(name)] = std::move(factory);
}

Tool* ToolRegistry::get(const std::string& name) {
    if (!is_allowed(name)) return nullptr;

    // Already built?
    if (auto it = tools_.find(name); it != tools_.end()) {
        return it->second.get();
    }
    // Build it from a registered factory (lazy instantiation).
    if (auto it = factories_.find(name); it != factories_.end()) {
        std::unique_ptr<Tool> created = it->second();
        Tool* raw = created.get();
        tools_[name] = std::move(created);
        return raw;
    }
    return nullptr;
}

std::vector<Tool*> ToolRegistry::list() const {
    std::vector<Tool*> out;
    out.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {     // C++17 structured bindings
        if (is_allowed(name)) out.push_back(tool.get());
    }
    return out;
}

void ToolRegistry::allow(const std::string& name) { allowed_.insert(name); }
void ToolRegistry::deny(const std::string& name)  { denied_.insert(name); }

bool ToolRegistry::is_allowed(const std::string& name) const {
    if (denied_.contains(name)) return false;                 // deny wins (C++20)
    if (!allowed_.empty() && !allowed_.contains(name)) return false;  // whitelist
    return true;
}

}  // namespace agent
