// =============================================================================
//  agent/skill_loader.cpp — directory scan, front-matter parsing, and keyword
//  selection. Selection scores each skill by how many of its keywords appear in
//  the (lower-cased) task and ranks them with std::ranges::sort.
// =============================================================================
#include "agent/skill_loader.h"

#include <algorithm>   // std::ranges::sort
#include <cctype>
#include <fstream>
#include <ranges>
#include <sstream>
#include <utility>

namespace agent {
namespace fs = std::filesystem;
namespace {

std::string to_lower(std::string s) {
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(std::string_view sv) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!sv.empty() && is_space(sv.front())) sv.remove_prefix(1);
    while (!sv.empty() && is_space(sv.back()))  sv.remove_suffix(1);
    return std::string(sv);
}

// Split "a, b , c" (optionally wrapped in [ ]) into trimmed, non-empty pieces.
std::vector<std::string> split_csv(std::string_view value) {
    value = trim(value);
    if (value.starts_with('[') && value.ends_with(']')) {
        value.remove_prefix(1);
        value.remove_suffix(1);
    }
    std::vector<std::string> out;
    for (const auto part : std::views::split(value, ',')) {   // C++20 ranges
        std::string token = trim(std::string_view(part.begin(), part.end()));
        if (!token.empty()) out.push_back(std::move(token));
    }
    return out;
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Parse one skill file: optional "---" front-matter (name/description/keywords)
// followed by the Markdown body that becomes the prompt guidance.
Skill parse_skill(const fs::path& path, const std::string& text) {
    Skill skill;
    skill.source = path;
    skill.name = path.stem().string();  // sensible default if no name: field

    std::istringstream stream(text);
    std::string line;

    // Front-matter only when the very first line is exactly "---".
    if (std::getline(stream, line) && trim(line) == "---") {
        while (std::getline(stream, line)) {
            if (trim(line) == "---") break;  // end of front-matter
            const auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            const std::string key = trim(line.substr(0, colon));
            const std::string val = trim(line.substr(colon + 1));
            if (key == "name" && !val.empty())        skill.name = val;
            else if (key == "description")            skill.description = val;
            else if (key == "keywords")               skill.keywords = split_csv(val);
        }
        // The remainder of the stream is the body.
        std::ostringstream body;
        body << stream.rdbuf();
        skill.content = trim(body.str());
    } else {
        skill.content = trim(text);  // no front-matter: whole file is the body
    }
    return skill;
}

// How many of a skill's keywords occur in the already-lower-cased task.
int score(const Skill& skill, const std::string& task_lower) {
    int hits = 0;
    for (const std::string& kw : skill.keywords) {
        const std::string needle = to_lower(kw);
        if (!needle.empty() && task_lower.find(needle) != std::string::npos) ++hits;
    }
    return hits;
}

}  // namespace

std::expected<std::size_t, std::string>
SkillLoader::load_directory(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return std::unexpected("skills path is not a directory: " + dir.string());
    }
    skills_.clear();
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".md") continue;
        skills_.push_back(parse_skill(entry.path(), read_file(entry.path())));
    }
    return skills_.size();
}

std::vector<const Skill*> SkillLoader::select_top(std::string_view task, std::size_t k) const {
    const std::string task_lower = to_lower(std::string(task));

    std::vector<std::pair<int, const Skill*>> scored;
    for (const Skill& s : skills_) {
        if (const int sc = score(s, task_lower); sc > 0) scored.emplace_back(sc, &s);
    }
    // Highest keyword overlap first (stable so ties keep load order).
    std::ranges::stable_sort(scored, [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<const Skill*> out;
    for (const auto& [sc, ptr] : scored) {
        if (out.size() >= k) break;
        out.push_back(ptr);
    }
    return out;
}

const Skill* SkillLoader::select(std::string_view task) const {
    const auto top = select_top(task, 1);
    return top.empty() ? nullptr : top.front();
}

std::string SkillLoader::build_system_prompt(std::string_view task, std::size_t k) const {
    std::string out;
    for (const Skill* s : select_top(task, k)) {
        out += "## Skill: " + s->name + "\n";
        out += s->content;
        out += "\n\n";
    }
    return out;
}

}  // namespace agent
