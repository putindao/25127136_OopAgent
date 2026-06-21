// =============================================================================
//  harness/evaluator.h
//  STRATEGY pattern (the fourth required pattern). Every way of judging a run is
//  an Evaluator subclass sharing one interface; the HarnessRunner picks the
//  concrete strategy by the task's eval_type. An Evaluator depends only on the
//  finished Trajectory + Task — never on HOW the agent produced it.
// =============================================================================
#pragma once

#include <string>

#include "harness/task.h"
#include "harness/trajectory.h"

namespace agent {

struct EvalResult {
    bool        passed = false;
    double      score  = 0.0;   // 0..1 (supports partial credit)
    std::string detail;
};

class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual std::string name() const = 0;
    virtual EvalResult  evaluate(const Task& task, const Trajectory& trajectory) = 0;
};

}  // namespace agent
