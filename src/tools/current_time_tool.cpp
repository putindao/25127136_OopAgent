#include "tools/current_time_tool.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace agent {

std::string CurrentTimeTool::name() const {
    return "current_time";
}

std::string CurrentTimeTool::description() const {
    return "Returns the current local date and time. Args may be empty.";
}

ToolResult CurrentTimeTool::execute(const std::string& args) {
    (void)args;

    const auto now = std::chrono::system_clock::now();
    const std::time_t value = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

#ifdef _WIN32
    if (localtime_s(&local_time, &value) != 0) {
        return std::unexpected(
            ToolError{"current_time: failed to obtain local time"});
    }
#else
    if (localtime_r(&value, &local_time) == nullptr) {
        return std::unexpected(
            ToolError{"current_time: failed to obtain local time"});
    }
#endif

    std::ostringstream output;
    output << "Current local time: "
           << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    return output.str();
}

}  // namespace agent