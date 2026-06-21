// =============================================================================
//  harness/functional_evaluator.cpp — run the task's shell eval_script via bash.
//  The script is written to a temp file (avoids double-shell quoting) and run
//  through bash, so Unix-style checks (test/grep) work on Windows + Linux alike.
// =============================================================================
#include "harness/functional_evaluator.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace agent {
namespace {
namespace fs = std::filesystem;

// Locate a bash interpreter. An explicit AGENT_BASH override wins; otherwise we
// probe the usual Windows (Git/MSYS2) and POSIX locations before falling back to
// whatever "bash" resolves to on PATH.
std::string find_bash() {
    if (const char* env = std::getenv("AGENT_BASH"); env && *env) return env;
    static const char* const candidates[] = {
#ifdef _WIN32
        "C:/Program Files/Git/bin/bash.exe",
        "C:/msys64/usr/bin/bash.exe",
#endif
        "/bin/bash", "/usr/bin/bash", "/bin/sh",
    };
    std::error_code ec;
    for (const char* c : candidates) {
        if (fs::exists(c, ec)) return c;
    }
    return "bash";
}

struct ShellResult {
    int         exit_code = -1;
    std::string output;
};

ShellResult run_script(const std::string& script) {
    const fs::path tmp = fs::temp_directory_path() / "agent_eval_script.sh";
    {
        std::ofstream out(tmp, std::ios::binary);
        out << script << "\n";
    }

    const std::string bash = find_bash();
    std::string command = "\"" + bash + "\" \"" + tmp.generic_string() + "\" 2>&1";
#ifdef _WIN32
    // _popen runs via `cmd /c`, which strips one outer quote pair; wrap the whole
    // command in an extra pair so the quotes around the paths survive.
    command = "\"" + command + "\"";
#endif

    std::string output;
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) {
        std::error_code ec;
        fs::remove(tmp, ec);
        return {-1, "failed to start shell"};
    }

    std::array<char, 4096> buffer;
    std::size_t n = 0;
    while ((n = std::fread(buffer.data(), 1, buffer.size(), pipe)) > 0) {
        output.append(buffer.data(), n);
    }

#ifdef _WIN32
    const int exit_code = _pclose(pipe);
#else
    const int raw       = pclose(pipe);
    const int exit_code = WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
#endif

    std::error_code ec;
    fs::remove(tmp, ec);
    return {exit_code, output};
}

}  // namespace

EvalResult FunctionalEvaluator::evaluate(const Task& task, const Trajectory& /*trajectory*/) {
    if (task.eval_script.empty()) {
        return {false, 0.0, "functional eval: no eval_script provided"};
    }
    const ShellResult r = run_script(task.eval_script);

    // Success = shell exit code 0, or an explicit PASS marker in the output.
    const bool passed = (r.exit_code == 0) || (r.output.find("PASS") != std::string::npos);
    return {passed, passed ? 1.0 : 0.0,
            "exit=" + std::to_string(r.exit_code) + " output=" + r.output};
}

}  // namespace agent
