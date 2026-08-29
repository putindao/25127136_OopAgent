#include "tools/json_query_tool.h"

#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

namespace agent {

std::string JsonQueryTool::name() const {
    return "json_query";
}

std::string JsonQueryTool::description() const {
    return "Extracts a value from JSON using a dot-separated key path. "
           "Args: a JSON object with fields 'data' and 'key'. "
           "Example: {\"data\":{\"user\":{\"name\":\"An\"}},"
           "\"key\":\"user.name\"}.";
}

ToolResult JsonQueryTool::execute(const std::string& args) {
    if (args.empty()) {
        return std::unexpected(
            ToolError{"json_query: arguments cannot be empty"});
    }

    try {
        const nlohmann::json request = nlohmann::json::parse(args);

        if (!request.is_object() ||
            !request.contains("data") ||
            !request.contains("key") ||
            !request["key"].is_string()) {
            return std::unexpected(
                ToolError{
                    "json_query: expected an object containing 'data' "
                    "and a string 'key'"});
        }

        const std::string key_path = request["key"].get<std::string>();

        if (key_path.empty()) {
            return std::unexpected(
                ToolError{"json_query: key cannot be empty"});
        }

        const nlohmann::json* current = &request["data"];
        std::istringstream path_stream(key_path);
        std::string segment;

        while (std::getline(path_stream, segment, '.')) {
            if (segment.empty() ||
                !current->is_object() ||
                !current->contains(segment)) {
                return std::unexpected(
                    ToolError{
                        "json_query: key path not found: " + key_path});
            }

            current = &current->at(segment);
        }

        if (current->is_string()) {
            return current->get<std::string>();
        }

        return current->dump();

    } catch (const nlohmann::json::exception& error) {
        return std::unexpected(
            ToolError{
                std::string{"json_query: invalid JSON: "} + error.what()});
    }
}

}  // namespace agent