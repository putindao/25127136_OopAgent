// =============================================================================
//  tools/memory_tool.cpp
//  SQLite plumbing for the persistent memory tool. Every sqlite3* connection
//  and sqlite3_stmt* statement is wrapped in RAII (std::unique_ptr with a
//  custom deleter) so an early return or a thrown exception can never leak a
//  handle. User input is ALWAYS bound as a parameter via sqlite3_bind_text —
//  it is never concatenated into the SQL, so there is no injection surface.
// =============================================================================
#include "tools/memory_tool.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <sqlite3.h>

namespace agent {
namespace {

// The single backing file shared by the save and search instances.
constexpr const char* kDatabaseFile = "agent_memory.db";

// RAII: a prepared sqlite3_stmt* is finalized automatically. Kept local to the
// .cpp because nothing outside this translation unit handles statements.
struct Sqlite3StmtDeleter {
    void operator()(sqlite3_stmt* stmt) const noexcept {
        if (stmt) sqlite3_finalize(stmt);
    }
};
using Sqlite3Stmt = std::unique_ptr<sqlite3_stmt, Sqlite3StmtDeleter>;

}  // namespace

// ---- RAII deleter for the connection ----------------------------------------
void MemoryTool::Sqlite3Deleter::operator()(sqlite3* db) const noexcept {
    if (db) sqlite3_close(db);
}

MemoryTool::MemoryTool(Mode mode) : mode_(mode) {
    // Open the database. sqlite3_open returns a handle even on failure, so we
    // adopt it first (RAII) and only then inspect the result code — this way a
    // failed open is still cleaned up by the unique_ptr.
    sqlite3* raw = nullptr;
    const int open_rc = sqlite3_open(kDatabaseFile, &raw);
    db_.reset(raw);
    if (open_rc != SQLITE_OK) {
        throw std::runtime_error(std::string("failed to open memory database: ") +
                                 sqlite3_errmsg(db_.get()));
    }

    // Ensure the schema. CREATE TABLE IF NOT EXISTS is idempotent, so both the
    // save and search instances can run it harmlessly.
    static constexpr const char* kCreateSql =
        "CREATE TABLE IF NOT EXISTS memory("
        "id INTEGER PRIMARY KEY, "
        "content TEXT NOT NULL, "
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP)";

    char* err_msg = nullptr;
    const int exec_rc = sqlite3_exec(db_.get(), kCreateSql, nullptr, nullptr, &err_msg);
    if (exec_rc != SQLITE_OK) {
        // sqlite3_exec allocates err_msg with sqlite3_malloc; copy then free it.
        std::string message = err_msg ? err_msg : "unknown error";
        sqlite3_free(err_msg);
        throw std::runtime_error("failed to create memory schema: " + message);
    }
}

std::string MemoryTool::name() const {
    return mode_ == Mode::Save ? "memory_save" : "memory_search";
}

std::string MemoryTool::description() const {
    if (mode_ == Mode::Save) {
        return "Persist a note into long-term memory. "
               "args: the plain-text content to remember (the whole argument "
               "string is stored verbatim). "
               "Returns a confirmation with the new record id.";
    }
    return "Search long-term memory for previously saved notes. "
           "args: a plain-text keyword to look for (matched as a substring, "
           "case-insensitively per SQLite LIKE). "
           "Returns up to 10 matching notes, newest first, one per line, "
           "or \"no matches\" when nothing is found.";
}

ToolResult MemoryTool::execute(const std::string& args) {
    return mode_ == Mode::Save ? save(args) : search(args);
}

ToolResult MemoryTool::save(const std::string& args) {
    // Parameterized INSERT: the note is bound, never concatenated.
    static constexpr const char* kInsertSql =
        "INSERT INTO memory(content) VALUES(?)";

    sqlite3_stmt* raw_stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), kInsertSql, -1, &raw_stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(ToolError{std::string("failed to prepare insert: ") +
                                         sqlite3_errmsg(db_.get())});
    }
    Sqlite3Stmt stmt{raw_stmt};  // RAII from here: finalized on every exit.

    // SQLITE_TRANSIENT: SQLite copies the bytes, so `args` need not outlive this.
    if (sqlite3_bind_text(stmt.get(), 1, args.data(),
                          static_cast<int>(args.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        return std::unexpected(ToolError{std::string("failed to bind content: ") +
                                         sqlite3_errmsg(db_.get())});
    }

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        return std::unexpected(ToolError{std::string("failed to store note: ") +
                                         sqlite3_errmsg(db_.get())});
    }

    const sqlite3_int64 new_id = sqlite3_last_insert_rowid(db_.get());
    return "Saved note as memory #" + std::to_string(new_id);
}

ToolResult MemoryTool::search(const std::string& args) {
    // Parameterized SELECT: the keyword is bound inside a LIKE pattern, never
    // spliced into the SQL string.
    static constexpr const char* kSelectSql =
        "SELECT content FROM memory WHERE content LIKE ? ORDER BY id DESC LIMIT 10";

    sqlite3_stmt* raw_stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), kSelectSql, -1, &raw_stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(ToolError{std::string("failed to prepare search: ") +
                                         sqlite3_errmsg(db_.get())});
    }
    Sqlite3Stmt stmt{raw_stmt};  // RAII from here: finalized on every exit.

    // Build the LIKE pattern in C++ (NOT in SQL): "%keyword%".
    const std::string pattern = "%" + args + "%";
    if (sqlite3_bind_text(stmt.get(), 1, pattern.data(),
                          static_cast<int>(pattern.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        return std::unexpected(ToolError{std::string("failed to bind keyword: ") +
                                         sqlite3_errmsg(db_.get())});
    }

    std::string matches;
    int row_count = 0;
    for (;;) {
        const int step_rc = sqlite3_step(stmt.get());
        if (step_rc == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmt.get(), 0);
            const int bytes = sqlite3_column_bytes(stmt.get(), 0);
            if (row_count > 0) matches += '\n';
            if (text) matches.append(reinterpret_cast<const char*>(text),
                                     static_cast<std::size_t>(bytes));
            ++row_count;
        } else if (step_rc == SQLITE_DONE) {
            break;
        } else {
            return std::unexpected(ToolError{std::string("failed to read matches: ") +
                                             sqlite3_errmsg(db_.get())});
        }
    }

    return row_count == 0 ? std::string("no matches") : matches;
}

}  // namespace agent
