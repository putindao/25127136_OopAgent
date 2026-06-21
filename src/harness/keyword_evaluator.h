// =============================================================================
//  harness/keyword_evaluator.h
//  Passes when every expected keyword appears in the agent's output. Concrete
//  Strategy #1.
// =============================================================================
#pragma once

#include "harness/evaluator.h"

namespace agent {

class KeywordEvaluator final : public Evaluator {
public:
    std::string name() const override { return "keyword"; }
    EvalResult  evaluate(const Task& task, const Trajectory& trajectory) override;
};

}  // namespace agent
