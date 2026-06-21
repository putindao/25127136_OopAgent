// =============================================================================
//  harness/environment.h
//  Abstracts WHERE a run happens. The HarnessRunner asks an Environment for a
//  working directory, lets it set up / tear down a clean workspace around each
//  task, and consults its command policy. NativeEnvironment runs directly on the
//  host; SandboxEnvironment confines the run to a disposable directory and
//  blocks a denylist of destructive commands.
//
//  Like the Evaluator, the Environment is an interchangeable Strategy: swapping
//  Native for Sandbox changes nothing in the agent or the tools.
// =============================================================================
#pragma once

#include <filesystem>
#include <string>

namespace agent {

class Environment {
public:
    virtual ~Environment() = default;

    virtual std::string name() const = 0;

    // Directory the agent's file/exec operations should resolve against.
    virtual std::filesystem::path working_dir() const = 0;

    // Prepare / clean the workspace around a single run.
    virtual void setup()    = 0;
    virtual void teardown() = 0;

    // Whether a shell command is permitted by this environment's policy.
    virtual bool allows_command(const std::string& command) const = 0;
};

// Runs directly on the host: the working directory is wherever you point it,
// nothing is created or removed, and every command is permitted.
class NativeEnvironment final : public Environment {
public:
    explicit NativeEnvironment(std::filesystem::path dir = std::filesystem::current_path());

    std::string           name() const override { return "native"; }
    std::filesystem::path  working_dir() const override { return dir_; }
    void                   setup() override {}
    void                   teardown() override {}
    bool                   allows_command(const std::string&) const override { return true; }

private:
    std::filesystem::path dir_;
};

// Confines a run to a fresh, disposable directory and refuses a denylist of
// destructive commands.
class SandboxEnvironment final : public Environment {
public:
    explicit SandboxEnvironment(std::filesystem::path root = {});

    std::string           name() const override { return "sandbox"; }
    std::filesystem::path  working_dir() const override { return root_; }
    void                   setup() override;     // (re)create an empty sandbox dir
    void                   teardown() override;  // remove the sandbox dir
    bool                   allows_command(const std::string& command) const override;

private:
    std::filesystem::path root_;
};

}  // namespace agent
