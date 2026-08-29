#include <iostream>
#include <string>

#include "tools/text_stats_tool.h"

int main() {
    agent::TextStatsTool tool;

    if (tool.name() != "text_stats") {
        std::cerr << "FAIL: incorrect tool name\n";
        return 1;
    }

    const auto result = tool.execute("hello world\nfrom agent");

    if (!result) {
        std::cerr << "FAIL: " << result.error().message << '\n';
        return 1;
    }

    if (result->find("characters=22") == std::string::npos ||
        result->find("words=4") == std::string::npos ||
        result->find("lines=2") == std::string::npos) {
        std::cerr << "FAIL: unexpected result: " << *result << '\n';
        return 1;
    }

    const auto empty_result = tool.execute("");

    if (empty_result) {
        std::cerr << "FAIL: empty input should fail\n";
        return 1;
    }

    std::cout << "text_stats tool tests\nALL PASS\n";
    return 0;
}