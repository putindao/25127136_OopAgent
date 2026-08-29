#include <iostream>
#include <string>

#include "tools/current_time_tool.h"

int main() {
    agent::CurrentTimeTool tool;

    if (tool.name() != "current_time") {
        std::cerr << "FAIL: incorrect tool name\n";
        return 1;
    }

    const auto result = tool.execute("");

    if (!result) {
        std::cerr << "FAIL: " << result.error().message << '\n';
        return 1;
    }

    if (result->find("Current local time:") == std::string::npos) {
        std::cerr << "FAIL: invalid result\n";
        return 1;
    }

    std::cout << "current_time tool tests\nALL PASS\n";
    return 0;
}