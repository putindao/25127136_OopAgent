// =============================================================================
//  tests/test_environment.cpp — unit test for the Environment strategies.
// =============================================================================
#include <cstdio>
#include <filesystem>

#include "harness/environment.h"

using namespace agent;
namespace fs = std::filesystem;

static int g_failures = 0;

#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (cond) {                                       \
            std::printf("  ok   : %s\n", (msg));          \
        } else {                                          \
            std::printf("  FAIL : %s\n", (msg));          \
            ++g_failures;                                 \
        }                                                 \
    } while (0)

int main() {
    std::puts("Environment tests");

    // NativeEnvironment: permissive, working_dir is what we asked for.
    {
        NativeEnvironment native(fs::temp_directory_path());
        CHECK(native.name() == "native", "native name");
        CHECK(native.working_dir() == fs::temp_directory_path(), "native working_dir");
        CHECK(native.allows_command("rm -rf /"), "native allows any command");
    }

    // SandboxEnvironment: isolates a directory and blocks destructive commands.
    {
        SandboxEnvironment sb(fs::temp_directory_path() / "agent_sandbox_test");
        sb.setup();
        CHECK(fs::is_directory(sb.working_dir()), "sandbox setup creates the dir");

        CHECK(!sb.allows_command("rm -rf /tmp/x"),  "sandbox blocks rm -rf");
        CHECK(!sb.allows_command("shutdown now"),   "sandbox blocks shutdown");
        CHECK(!sb.allows_command("mkfs.ext4 /dev/sda"), "sandbox blocks mkfs");
        CHECK(sb.allows_command("echo hello"),      "sandbox allows echo");
        CHECK(sb.allows_command("ls -la"),          "sandbox allows ls");

        sb.teardown();
        CHECK(!fs::exists(sb.working_dir()), "sandbox teardown removes the dir");
    }

    std::printf(g_failures == 0 ? "\nALL PASS\n" : "\n%d CHECK(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
