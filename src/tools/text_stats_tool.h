#pragma once

#include <string>

#include "tools/tool.h"

namespace agent {

class TextStatsTool final : public Tool {
public:
    std::string name() const override;
    std::string description() const override;
    ToolResult execute(const std::string& args) override;
};

}  // namespace agent