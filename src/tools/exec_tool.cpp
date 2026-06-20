// =============================================================================
//  tools/exec_tool.cpp
//  popen/pclose plumbing for the "exec" tool. The pipe handle is wrapped in RAII
//  (std::unique_ptr with a custom deleter) so an early return or a thrown
//  exception can never leak it. stderr is folded into stdout via "2>&1" and the
//  pipe is drained in fixed-size chunks so arbitrarily large output is safe.
// =============================================================================
#include "tools/exec_tool.h"

#include <array>
#include <cstdio>
#include <memory>
#include <string>

// popen/pclose are spelled with a leading underscore in the MSVC CRT.
#ifdef _WIN32
#define AGENT_POPEN  _popen
#define AGENT_PCLOSE _pclose
#else
#define AGENT_POPEN  popen
#define AGENT_PCLOSE pclose
#endif

namespace agent {
namespace {

// RAII: a FILE* pipe opened by popen is closed automatically via pclose. The
// pclose return value (which carries the child exit status) is not needed here
// because execute() closes the pipe explicitly to read that status; this deleter
// only guards the leak on exception/early-return paths where it still owns it.
struct PipeDeleter {
    void operator()(std::FILE* p) const noexcept {
        if (p) AGENT_PCLOSE(p);
    }
};
using Pipe = std::unique_ptr<std::FILE, PipeDeleter>;

}  // namespace

std::string ExecTool::name() const {
    return "exec";
}

std::string ExecTool::description() const {
    return "Runs a shell command and returns its combined stdout+stderr plus the "
           "exit code. Args: the command line as plain text (e.g. \"ls -la /tmp\"). "
           "stderr is captured alongside stdout; the final line reports the process "
           "exit code.";
}

ToolResult ExecTool::execute(const std::string& args) {
    // Fold stderr into stdout so a single pipe carries the full output.
    const std::string command = args + " 2>&1";

    // Open the pipe; a null result means the command could not be started.
    Pipe pipe{AGENT_POPEN(command.c_str(), "r")};
    if (!pipe) {
        return std::unexpected(ToolError{"exec: failed to start command (popen returned null)"});
    }

    // Drain the pipe fully in fixed-size chunks so large output is handled.
    std::string output;
    std::array<char, 4096> buffer{};
    std::size_t read = 0;
    while ((read = std::fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) {
        output.append(buffer.data(), read);
    }

    // Close the pipe explicitly to obtain the child's exit status, then release
    // ownership so the RAII deleter does not pclose a second time.
    const int status = AGENT_PCLOSE(pipe.release());

    output += "\n[exec] exit code: " + std::to_string(status) + "\n";
    return output;
}

}  // namespace agent
