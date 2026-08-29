#include <iostream>
#include <string>

#include "tools/json_query_tool.h"

int main() {
    agent::JsonQueryTool tool;

    if (tool.name() != "json_query") {
        std::cerr << "FAIL: incorrect tool name\n";
        return 1;
    }

    const auto number_result = tool.execute(
        R"({"data":{"student":{"id":25127136,"name":"Dao"}},"key":"student.id"})");

    if (!number_result || *number_result != "25127136") {
        std::cerr << "FAIL: could not extract numeric value\n";
        return 1;
    }

    const auto string_result = tool.execute(
        R"({"data":{"student":{"id":25127136,"name":"Dao"}},"key":"student.name"})");

    if (!string_result || *string_result != "Dao") {
        std::cerr << "FAIL: could not extract string value\n";
        return 1;
    }

    const auto missing_result = tool.execute(
        R"({"data":{"student":{"id":25127136}},"key":"student.age"})");

    if (missing_result) {
        std::cerr << "FAIL: missing key should return an error\n";
        return 1;
    }

    const auto invalid_result = tool.execute("not valid JSON");

    if (invalid_result) {
        std::cerr << "FAIL: invalid JSON should return an error\n";
        return 1;
    }

    std::cout << "json_query tool tests\nALL PASS\n";
    return 0;
}