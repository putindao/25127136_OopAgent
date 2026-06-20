// =============================================================================
//  tools/file_tool.h
//  A filesystem Tool: read a file's contents or write content to a file. One
//  class serves two registered names via a Mode enum chosen at construction
//  (Mode::Read -> "read_file", Mode::Write -> "write_file"). Like every Tool it
//  knows nothing about the AgentLoop — it takes a string of arguments and hands
//  back a ToolResult (the std::expected error discipline shared system-wide).
// =============================================================================
#pragma once

#include <string>

#include "tools/tool.h"

namespace agent {

// One class, two behaviours. The mode is fixed at construction so the same
// implementation can be registered under both "read_file" and "write_file".
class FileTool : public Tool {
public:
    enum class Mode { Read, Write };

    explicit FileTool(Mode mode) : mode_(mode) {}

    // "read_file" when Mode::Read, "write_file" when Mode::Write.
    std::string name() const override;

    // Documents the exact argument format the model must produce (per mode).
    std::string description() const override;

    // Read or write a file depending on the configured mode. Never throws:
    // missing files, bad JSON, and IO errors come back as a ToolError.
    ToolResult execute(const std::string& args) override;

private:
    // Implementation halves, one per mode (keeps execute() a thin dispatcher).
    ToolResult read(const std::string& args) const;
    ToolResult write(const std::string& args) const;

    Mode mode_;
};

}  // namespace agent
