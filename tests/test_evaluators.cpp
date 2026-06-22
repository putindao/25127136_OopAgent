// =============================================================================
//  tests/test_evaluators.cpp — unit tests for the evaluators + trajectory JSON.
//  Deterministic: no Ollama needed (the functional test runs a trivial script).
// =============================================================================
#include <cstdio>
#include <filesystem>
#include <string>

#include "harness/functional_evaluator.h"
#include "harness/keyword_evaluator.h"
#include "harness/trajectory.h"

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

static Trajectory make_traj(const std::string& final_answer) {
    Trajectory t;
    t.task_id      = "t";
    t.model        = "gemma4";
    t.stop_reason  = "final";
    t.final_answer = final_answer;
    Step s;
    s.step_id = 0;
    s.action  = "calculator";
    s.action_input = "15*17";
    s.observation  = "255";
    t.steps.push_back(s);
    return t;
}

int main() {
    std::puts("evaluator + trajectory tests");

    // ---- KeywordEvaluator ---------------------------------------------------
    {
        KeywordEvaluator ev;
        Task task;
        task.expected_keywords = {"255"};
        CHECK(ev.evaluate(task, make_traj("The result is 255.")).passed, "keyword present -> pass");

        task.expected_keywords = {"999"};
        CHECK(!ev.evaluate(task, make_traj("The result is 255.")).passed, "keyword absent -> fail");

        // The answer can live in an observation, not just the final answer.
        task.expected_keywords = {"255"};
        CHECK(ev.evaluate(task, make_traj("done")).passed, "keyword found in observation -> pass");
    }

    // ---- FunctionalEvaluator ------------------------------------------------
    {
        FunctionalEvaluator ev;
        Task pass_task;
        pass_task.eval_type   = "functional";
        pass_task.eval_script = "echo PASS";
        CHECK(ev.evaluate(pass_task, make_traj("")).passed, "exit 0 / PASS -> pass");

        Task fail_task;
        fail_task.eval_type   = "functional";
        fail_task.eval_script = "exit 1";
        CHECK(!ev.evaluate(fail_task, make_traj("")).passed, "exit 1 -> fail");
    }

    // ---- Trajectory JSON ----------------------------------------------------
    {
        const auto j = make_traj("255").to_json();
        CHECK(j.at("task_id") == "t", "trajectory json has task_id");
        CHECK(j.at("steps").is_array() && j.at("steps").size() == 1, "trajectory json has steps[]");
        CHECK(j.at("steps")[0].at("action").at("type") == "tool_call", "step action type == tool_call");
    }

    // ---- Non-UTF-8 output must not crash serialization ----------------------
    // Reproduces the runtime crash where exec/file output in a non-UTF-8 codepage
    // reached json::dump and threw type_error.316. With the replace handler the
    // bytes become U+FFFD and save() succeeds instead of terminating.
    {
        Trajectory t = make_traj("ok");
        // Adjacent literals keep the trailing letters out of the \x escapes.
        t.steps[0].observation = std::string("bad\xC3\x6F\xFF\x80" "bytes");  // invalid UTF-8
        const auto path = std::filesystem::temp_directory_path() / "agent_traj_utf8_test.json";
        const bool saved = t.save(path);
        CHECK(saved, "trajectory with invalid UTF-8 saves without throwing");
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::printf(g_failures == 0 ? "\nALL PASS\n" : "\n%d CHECK(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
