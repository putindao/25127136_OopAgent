// =============================================================================
//  tools/calculator_tool.cpp
//  The recursive-descent parser behind CalculatorTool. The grammar (lowest to
//  highest precedence) is:
//      expression := term   (('+' | '-') term)*
//      term       := factor (('*' | '/') factor)*
//      factor     := ('+' | '-') factor | primary
//      primary    := number | '(' expression ')'
//  Parse errors are reported through std::expected (never thrown across the
//  execute() boundary), matching the ToolError discipline used elsewhere.
// =============================================================================
#include "tools/calculator_tool.h"

#include <cmath>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

namespace agent {
namespace {

// A single-pass recursive-descent parser/evaluator over a string_view. It walks
// the input left to right; on the first malformed token it short-circuits by
// returning std::unexpected, so no exception ever leaves the public entry point.
class Parser {
public:
    explicit Parser(std::string_view src) : src_(src) {}

    // Evaluate the whole input and verify nothing is left over after the top
    // expression — trailing junk (e.g. "1 2") is a real, reportable error.
    std::expected<double, ToolError> evaluate() {
        auto value = parse_expression();
        if (!value) return value;
        skip_spaces();
        if (pos_ != src_.size()) {
            return std::unexpected(ToolError{
                "unexpected trailing characters at position " + std::to_string(pos_)});
        }
        return value;
    }

private:
    // expression := term (('+' | '-') term)*
    std::expected<double, ToolError> parse_expression() {
        auto left = parse_term();
        if (!left) return left;
        for (;;) {
            skip_spaces();
            const char op = peek();
            if (op != '+' && op != '-') break;
            ++pos_;
            auto right = parse_term();
            if (!right) return right;
            *left = (op == '+') ? (*left + *right) : (*left - *right);
        }
        return left;
    }

    // term := factor (('*' | '/') factor)*
    std::expected<double, ToolError> parse_term() {
        auto left = parse_factor();
        if (!left) return left;
        for (;;) {
            skip_spaces();
            const char op = peek();
            if (op != '*' && op != '/') break;
            ++pos_;
            auto right = parse_factor();
            if (!right) return right;
            if (op == '*') {
                *left *= *right;
            } else {
                if (*right == 0.0) {
                    return std::unexpected(ToolError{"division by zero"});
                }
                *left /= *right;
            }
        }
        return left;
    }

    // factor := ('+' | '-') factor | primary  (right-associative unary signs)
    std::expected<double, ToolError> parse_factor() {
        skip_spaces();
        const char op = peek();
        if (op == '+' || op == '-') {
            ++pos_;
            auto operand = parse_factor();
            if (!operand) return operand;
            return (op == '-') ? -*operand : *operand;
        }
        return parse_primary();
    }

    // primary := number | '(' expression ')'
    std::expected<double, ToolError> parse_primary() {
        skip_spaces();
        if (peek() == '(') {
            ++pos_;
            auto inner = parse_expression();
            if (!inner) return inner;
            skip_spaces();
            if (peek() != ')') {
                return std::unexpected(ToolError{
                    "expected ')' at position " + std::to_string(pos_)});
            }
            ++pos_;
            return inner;
        }
        return parse_number();
    }

    // number := digits ['.' digits] — at least one digit total is required.
    std::expected<double, ToolError> parse_number() {
        skip_spaces();
        const std::size_t start = pos_;
        bool seen_digit = false;
        while (pos_ < src_.size() && is_digit(src_[pos_])) {
            ++pos_;
            seen_digit = true;
        }
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && is_digit(src_[pos_])) {
                ++pos_;
                seen_digit = true;
            }
        }
        if (!seen_digit) {
            return std::unexpected(ToolError{
                "expected a number at position " + std::to_string(pos_)});
        }
        // Manual base-10 parse keeps the tool dependency-free and locale-stable.
        const std::string_view tok = src_.substr(start, pos_ - start);
        return to_double(tok);
    }

    // Convert a validated "digits[.digits]" token to double without exceptions.
    static std::expected<double, ToolError> to_double(std::string_view tok) {
        double integral = 0.0;
        std::size_t i = 0;
        for (; i < tok.size() && is_digit(tok[i]); ++i) {
            integral = integral * 10.0 + static_cast<double>(tok[i] - '0');
        }
        double result = integral;
        if (i < tok.size() && tok[i] == '.') {
            ++i;
            double scale = 0.1;
            for (; i < tok.size() && is_digit(tok[i]); ++i) {
                result += static_cast<double>(tok[i] - '0') * scale;
                scale *= 0.1;
            }
        }
        return result;
    }

    static bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

    void skip_spaces() noexcept {
        while (pos_ < src_.size() &&
               (src_[pos_] == ' ' || src_[pos_] == '\t' ||
                src_[pos_] == '\n' || src_[pos_] == '\r')) {
            ++pos_;
        }
    }

    // Look at the current byte without consuming it; '\0' marks end of input.
    char peek() const noexcept {
        return pos_ < src_.size() ? src_[pos_] : '\0';
    }

    std::string_view src_;
    std::size_t      pos_ = 0;
};

// Render a numeric result, trimming trailing zeros so whole numbers come back
// as "42" rather than "42.000000" while decimals keep their meaningful digits.
std::string format_number(double value) {
    // Whole-number fast path keeps integer results free of any decimal point.
    if (std::isfinite(value) && value == std::floor(value) &&
        std::abs(value) < 1e15) {
        return std::to_string(static_cast<long long>(value));
    }
    std::string s = std::to_string(value);  // fixed notation, e.g. "1.500000"
    const std::size_t dot = s.find('.');
    if (dot != std::string::npos) {
        std::size_t last = s.size() - 1;
        while (last > dot && s[last] == '0') --last;
        if (last == dot) --last;  // drop the now-dangling '.'
        s.erase(last + 1);
    }
    return s;
}

}  // namespace

std::string CalculatorTool::name() const {
    return "calculator";
}

std::string CalculatorTool::description() const {
    return "Evaluates an arithmetic expression passed as plain text and returns "
           "the numeric result. Supports + - * / , unary minus, parentheses, and "
           "integer or decimal numbers with standard operator precedence. "
           "Examples: \"15*17\" or \"(3+4)*2-1\".";
}

ToolResult CalculatorTool::execute(const std::string& args) {
    Parser parser{args};
    auto value = parser.evaluate();
    if (!value) {
        return std::unexpected(value.error());
    }
    if (!std::isfinite(*value)) {
        return std::unexpected(ToolError{"result is not a finite number"});
    }
    return format_number(*value);
}

}  // namespace agent
