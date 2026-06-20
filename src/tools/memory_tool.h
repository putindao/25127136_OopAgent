// =============================================================================
//  tools/memory_tool.h
//  A persistent memory Tool backed by SQLite. The same class serves two roles
//  selected by a Mode enum: Mode::Save exposes the "memory_save" tool (store a
//  note), Mode::Search exposes "memory_search" (recall notes by keyword). Both
//  modes share one DB file ("agent_memory.db"); two instances over the same
//  file is fine. All SQLite handles are wrapped in RAII so no error path leaks.
// =============================================================================
#pragma once

#include <memory>
#include <string>

#include "tools/tool.h"

// Forward declarations keep <sqlite3.h> out of this header (it is an opaque
// implementation detail used only by memory_tool.cpp).
struct sqlite3;

namespace agent {

class MemoryTool : public Tool {
public:
    // Which capability this instance presents to the agent.
    enum class Mode { Save, Search };

    // Open "agent_memory.db" and ensure the schema exists. Either step failing
    // throws std::runtime_error (construction is the one place we use an
    // exception; execute() never lets one escape — see the .cpp).
    explicit MemoryTool(Mode mode);

    // ---- Tool contract ------------------------------------------------------
    std::string name() const override;
    std::string description() const override;
    ToolResult  execute(const std::string& args) override;

private:
    // RAII: an open sqlite3* connection is closed automatically.
    struct Sqlite3Deleter {
        void operator()(sqlite3* db) const noexcept;
    };
    using Sqlite3Handle = std::unique_ptr<sqlite3, Sqlite3Deleter>;

    // Mode-specific execute() bodies; both bind user input as a parameter and
    // never splice it into the SQL text.
    ToolResult save(const std::string& args);
    ToolResult search(const std::string& args);

    Mode          mode_;
    Sqlite3Handle db_;
};

}  // namespace agent
