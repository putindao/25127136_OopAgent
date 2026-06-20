// =============================================================================
//  tests/test_action_parser.cpp — unit test for parse_action() (no network).
//  Verifies we recover a structured Action from realistic, messy model output.
// =============================================================================
#include <cstdio>
#include <variant>

#include "agent/action.h"

using namespace agent;

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
    std::puts("parse_action tests");

    // Clean tool call.
    {
        auto r = parse_action(R"({"thought":"need math","action":"calculator","action_input":"2+2"})");
        CHECK(r.has_value(), "clean JSON parses");
        CHECK(r && std::holds_alternative<ToolCall>(*r), "is a ToolCall");
        if (r && std::holds_alternative<ToolCall>(*r)) {
            const auto& tc = std::get<ToolCall>(*r);
            CHECK(tc.tool == "calculator", "tool name == calculator");
            CHECK(tc.args == "2+2", "args == 2+2");
        }
    }

    // JSON buried in prose and a ``` fence (typical model output).
    {
        auto r = parse_action("Sure!\n```json\n{\"action\":\"final\",\"action_input\":\"42\"}\n```\nHope that helps.");
        CHECK(r.has_value(), "extracts JSON from fence + prose");
        CHECK(r && std::holds_alternative<FinalAnswer>(*r), "is a FinalAnswer");
        if (r && std::holds_alternative<FinalAnswer>(*r)) {
            CHECK(std::get<FinalAnswer>(*r).text == "42", "final answer == 42");
        }
    }

    // Non-string action_input is coerced to a string.
    {
        auto r = parse_action(R"({"action":"calculator","action_input":255})");
        CHECK(r && std::holds_alternative<ToolCall>(*r) &&
                  std::get<ToolCall>(*r).args == "255",
              "numeric action_input coerced to \"255\"");
    }

    // "done" is a synonym for finishing.
    {
        auto r = parse_action(R"({"action":"done","action_input":"all set"})");
        CHECK(r && std::holds_alternative<FinalAnswer>(*r), "'done' maps to FinalAnswer");
    }

    // No JSON at all is a clean error, not a crash.
    {
        auto r = parse_action("I am not sure how to proceed.");
        CHECK(!r.has_value(), "no JSON -> error (no crash)");
    }

    std::printf(g_failures == 0 ? "\nALL PASS\n" : "\n%d CHECK(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
