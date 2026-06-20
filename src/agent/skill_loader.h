// =============================================================================
//  agent/skill_loader.h
//  Skills are Markdown instruction files in skills/. Each carries a small YAML
//  front-matter (name / description / keywords) followed by free-form guidance.
//  SkillLoader scans the directory, then, given a task, selects the best-matching
//  skill(s) by keyword overlap and builds the text to inject into the system
//  prompt. Like every layer it is self-contained: it knows nothing about the
//  AgentLoop or any Tool, it just turns a task string into prompt guidance.
// =============================================================================
#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace agent {

// One parsed skill file.
struct Skill {
    std::string              name;         // defaults to the file stem
    std::string              description;  // one-line summary (front-matter)
    std::vector<std::string> keywords;     // selection triggers (front-matter)
    std::string              content;      // Markdown body = the actual guidance
    std::filesystem::path    source;       // file it came from
};

class SkillLoader {
public:
    // Load every *.md skill under `dir`. Returns how many were loaded, or an
    // error string if the directory is missing / not a directory.
    std::expected<std::size_t, std::string> load_directory(const std::filesystem::path& dir);

    // All loaded skills, in load order.
    const std::vector<Skill>& skills() const noexcept { return skills_; }

    // The single best-matching skill for `task`, or nullptr if none scores > 0.
    const Skill* select(std::string_view task) const;

    // Up to `k` best-matching skills, highest keyword overlap first (score > 0).
    std::vector<const Skill*> select_top(std::string_view task, std::size_t k) const;

    // The system-prompt fragment built from the top-`k` skills for `task`
    // (empty when nothing matches).
    std::string build_system_prompt(std::string_view task, std::size_t k = 1) const;

private:
    std::vector<Skill> skills_;
};

}  // namespace agent
