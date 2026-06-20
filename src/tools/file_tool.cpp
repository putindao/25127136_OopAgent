// =============================================================================
//  tools/file_tool.cpp
//  Filesystem read/write plumbing for FileTool. Every OS file handle is owned
//  by an std::ifstream/std::ofstream whose RAII closes it on scope exit, so an
//  early return or a thrown exception can never leak a handle. All failures are
//  converted to a ToolError — nothing escapes execute() as an exception.
// =============================================================================
#include "tools/file_tool.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

namespace agent {
namespace {

// Extract a target path from the raw argument string. We accept either a bare
// path as plain text, or a JSON object of the shape {"path": "..."}. If the
// text is not valid JSON we treat the whole string as the path itself, which
// keeps the common "just give me a filename" case ergonomic for the model.
std::expected<std::string, ToolError> parse_path(const std::string& args) {
    std::string trimmed = args;
    // Trim surrounding whitespace so a stray newline never becomes the path.
    const auto first = trimmed.find_first_not_of(" \t\r\n");
    const auto last  = trimmed.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::unexpected(ToolError{"empty arguments: expected a file path"});
    }
    trimmed = trimmed.substr(first, last - first + 1);

    // Only attempt a JSON parse when it actually looks like a JSON object;
    // otherwise a plain path like "notes.txt" would needlessly fail parsing.
    if (trimmed.front() == '{') {
        nlohmann::json j = nlohmann::json::parse(trimmed, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded()) {
            return std::unexpected(ToolError{"invalid JSON arguments for read_file"});
        }
        if (!j.contains("path") || !j["path"].is_string()) {
            return std::unexpected(ToolError{"JSON arguments must contain a string \"path\" field"});
        }
        return j["path"].get<std::string>();
    }
    return trimmed;
}

}  // namespace

std::string FileTool::name() const {
    return mode_ == Mode::Read ? "read_file" : "write_file";
}

std::string FileTool::description() const {
    if (mode_ == Mode::Read) {
        return "Read a UTF-8 text file and return its full contents. "
               "Args: a file path as plain text (e.g. notes/todo.txt), or a JSON "
               "object {\"path\": \"<file path>\"}. Returns the file contents, or "
               "an error if the file is missing or unreadable.";
    }
    return "Write text to a file, creating any missing parent directories. "
           "Args: a JSON object {\"path\": \"<file path>\", \"content\": \"<text>\"}. "
           "Overwrites the file if it already exists. Returns a short confirmation "
           "like \"wrote N bytes to <path>\".";
}

ToolResult FileTool::execute(const std::string& args) {
    return mode_ == Mode::Read ? read(args) : write(args);
}

ToolResult FileTool::read(const std::string& args) const {
    auto path = parse_path(args);
    if (!path) return std::unexpected(path.error());

    std::error_code ec;
    if (!std::filesystem::exists(*path, ec) || ec) {
        return std::unexpected(ToolError{"file not found: " + *path});
    }
    if (std::filesystem::is_directory(*path, ec)) {
        return std::unexpected(ToolError{"path is a directory, not a file: " + *path});
    }

    // std::ifstream's underlying FILE* is owned by the stream itself (RAII):
    // it is closed when `in` leaves scope, on every path including exceptions.
    std::ifstream in(*path, std::ios::binary);
    if (!in) {
        return std::unexpected(ToolError{"failed to open file for reading: " + *path});
    }

    // Read the whole file in one shot via stream iterators.
    std::string contents{std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>()};
    if (in.bad()) {
        return std::unexpected(ToolError{"IO error while reading file: " + *path});
    }
    return contents;
}

ToolResult FileTool::write(const std::string& args) const {
    // write_file always requires a JSON object — both path and content.
    nlohmann::json j = nlohmann::json::parse(args, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        return std::unexpected(ToolError{"invalid JSON arguments for write_file"});
    }
    if (!j.contains("path") || !j["path"].is_string()) {
        return std::unexpected(ToolError{"JSON arguments must contain a string \"path\" field"});
    }
    if (!j.contains("content") || !j["content"].is_string()) {
        return std::unexpected(ToolError{"JSON arguments must contain a string \"content\" field"});
    }

    const auto path    = j["path"].get<std::string>();
    const auto content = j["content"].get<std::string>();

    // Create parent directories if the path has any (e.g. "out/logs/a.txt").
    const std::filesystem::path fs_path(path);
    if (fs_path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(fs_path.parent_path(), ec);
        if (ec) {
            return std::unexpected(
                ToolError{"failed to create parent directories for: " + path});
        }
    }

    // std::ofstream owns and closes its FILE* (RAII), even on early return.
    std::ofstream out(fs_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return std::unexpected(ToolError{"failed to open file for writing: " + path});
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.flush();
    if (!out) {
        return std::unexpected(ToolError{"IO error while writing file: " + path});
    }

    return "wrote " + std::to_string(content.size()) + " bytes to " + path;
}

}  // namespace agent
