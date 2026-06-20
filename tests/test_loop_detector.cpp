// =============================================================================
//  tests/test_loop_detector.cpp — unit test for LoopDetector (no network).
// =============================================================================
#include <cstdio>

#include "agent/loop_detector.h"

using agent::LoopDetector;
using Severity = agent::LoopDetector::Severity;

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
    std::puts("LoopDetector tests");

    // Generic repeat with the default threshold of 3.
    {
        LoopDetector d;
        CHECK(d.observe("calculator|1+1").severity == Severity::None,     "1st repeat -> None");
        CHECK(d.observe("calculator|1+1").severity == Severity::Warning,  "2nd repeat -> Warning");
        CHECK(d.observe("calculator|1+1").severity == Severity::Critical, "3rd repeat -> Critical");
    }

    // Ping-pong A,B,A,B with the default threshold of 2 (span 4).
    {
        LoopDetector d;
        d.observe("read_file|a");
        d.observe("write_file|a");
        d.observe("read_file|a");
        CHECK(d.observe("write_file|a").severity == Severity::Critical, "A,B,A,B -> ping-pong Critical");
    }

    // Varied actions never trip the detector.
    {
        LoopDetector d;
        d.observe("calculator|1");
        d.observe("web_search|x");
        CHECK(d.observe("memory_save|y").severity == Severity::None, "varied actions -> None");
    }

    // A custom threshold of 2 makes the second identical action Critical.
    {
        LoopDetector d{LoopDetector::Config{.repeat_threshold = 2, .pingpong_threshold = 3}};
        d.observe("exec|ls");
        CHECK(d.observe("exec|ls").severity == Severity::Critical, "custom threshold=2 -> Critical on 2nd");
    }

    std::printf(g_failures == 0 ? "\nALL PASS\n" : "\n%d CHECK(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
