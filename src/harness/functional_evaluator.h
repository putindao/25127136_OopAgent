// =============================================================================
//  harness/functional_evaluator.h
//  Passes when the task's eval_script (a POSIX shell command) succeeds — e.g.
//  "test -f result.txt && grep -q 255 result.txt && echo PASS". Concrete
//  Strategy #2. The script is run through bash so Unix-style checks work on any
//  platform that has bash available.
// =============================================================================
#pragma once

#include "harness/evaluator.h"

namespace agent {

class FunctionalEvaluator final : public Evaluator {
public:
    std::string name() const override { return "functional"; }
    EvalResult  evaluate(const Task& task, const Trajectory& trajectory) override;
};

}  // namespace agent
