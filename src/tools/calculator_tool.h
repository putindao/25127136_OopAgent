// =============================================================================
//  tools/calculator_tool.h
//  A pure-C++ arithmetic evaluator tool. Given an expression as plain text it
//  parses and evaluates it with correct operator precedence — no shelling out,
//  no system eval. Implemented as a recursive-descent parser so the grammar is
//  readable and every failure (syntax, division by zero, leftover tokens) maps
//  to a clear ToolError instead of an exception escaping execute().
// =============================================================================
#pragma once

#include <string>

#include "tools/tool.h"

namespace agent {

// Evaluates arithmetic expressions: + - * / , unary minus, parentheses,
// integers and decimals, with standard precedence. Stateless and reusable.
class CalculatorTool : public Tool {
public:
    std::string name() const override;
    std::string description() const override;
    ToolResult  execute(const std::string& args) override;
};

}  // namespace agent
