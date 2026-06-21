// =============================================================================
//  harness/environment.cpp — Native + Sandbox environment implementations.
// =============================================================================
#include "harness/environment.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace agent {
namespace fs = std::filesystem;

NativeEnvironment::NativeEnvironment(fs::path dir) : dir_(std::move(dir)) {}

SandboxEnvironment::SandboxEnvironment(fs::path root) : root_(std::move(root)) {
    if (root_.empty()) {
        root_ = fs::temp_directory_path() / "agent_sandbox";
    }
}

void SandboxEnvironment::setup() {
    std::error_code ec;
    fs::remove_all(root_, ec);          // start from a clean slate
    fs::create_directories(root_, ec);
}

void SandboxEnvironment::teardown() {
    std::error_code ec;
    fs::remove_all(root_, ec);
}

bool SandboxEnvironment::allows_command(const std::string& command) const {
    // Substrings that mark a destructive command the sandbox refuses to run.
    static const char* const kDenied[] = {
        "rm -rf", "rm -fr", "rmdir /s", "del /", "format ",
        "mkfs",   "shutdown", "reboot", ":(){",  "> /dev/sd",
    };
    std::string lc = command;
    std::ranges::transform(lc, lc.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const char* needle : kDenied) {
        if (lc.find(needle) != std::string::npos) return false;
    }
    return true;
}

}  // namespace agent
