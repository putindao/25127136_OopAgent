#include "tools/text_stats_tool.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <sstream>
#include <string>

namespace agent {

std::string TextStatsTool::name() const {
    return "text_stats";
}

std::string TextStatsTool::description() const {
    return "Counts the characters, words, and lines in the supplied text. "
           "Args: the plain text to analyze.";
}

ToolResult TextStatsTool::execute(const std::string& args) {
    if (args.empty()) {
        return std::unexpected(
            ToolError{"text_stats: input text cannot be empty"});
    }

    std::istringstream input(args);
    std::string word;
    std::size_t word_count = 0;

    while (input >> word) {
        ++word_count;
    }

    const std::size_t character_count = args.size();
    const std::size_t line_count =
        1 + static_cast<std::size_t>(std::ranges::count(args, '\n'));

    std::ostringstream output;
    output << "characters=" << character_count
           << ", words=" << word_count
           << ", lines=" << line_count;

    return output.str();
}

}  // namespace agent