// =============================================================================
//  tests/test_evaluators.cpp — unit tests for the evaluators + trajectory JSON.
//  Deterministic: no Ollama needed (the functional test runs a trivial script).
// =============================================================================
#include <cstdio>

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

    std::printf(g_failures == 0 ? "\nALL PASS\n" : "\n%d CHECK(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
