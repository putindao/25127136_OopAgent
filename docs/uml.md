# UML Diagrams

Source diagrams in Mermaid. Render to PNG with the [Mermaid Live Editor](https://mermaid.live)
or the VS Code "Markdown Preview Mermaid" extension for the `docs/*.png` deliverables.

---

## 1. Class Diagram (whole system)

```mermaid
classDiagram
    direction LR

    class LLMClient {
        <<abstract>>
        +chat(messages) LLMResult*
        +config() LLMConfig*
    }
    class OllamaClient {
        +chat(messages) LLMResult
    }
    LLMClient <|-- OllamaClient

    class Tool {
        <<abstract>>
        +name() string*
        +description() string*
        +execute(args) ToolResult*
    }
    Tool <|-- CalculatorTool
    Tool <|-- FileTool
    Tool <|-- ExecTool
    Tool <|-- WebSearchTool
    Tool <|-- MemoryTool

    class ToolRegistry {
        <<Registry / Factory>>
        +register_tool(Tool)
        +register_factory(name, Factory)
        +get(name) Tool
        +list() Tool[]
        +allow(name) / deny(name)
    }
    ToolRegistry o-- "*" Tool : owns

    class SkillLoader {
        +load_directory(dir)
        +select(task) Skill
        +build_system_prompt(task) string
    }
    SkillLoader o-- "*" Skill

    class LoopDetector {
        +observe(signature) Verdict
    }

    class Action {
        <<variant>>
        +ToolCall
        +FinalAnswer
    }

    class AgentLoop {
        <<Template Method>>
        +run(task) AgentResult
        #build_initial_messages()
        #think() Action
        #act(ToolCall) string
        #observe()
        +set_step_hook(StepHook)
    }
    AgentLoop --> LLMClient : uses
    AgentLoop --> ToolRegistry : uses
    AgentLoop --> LoopDetector : uses
    AgentLoop ..> Action : produces
    AgentLoop --> StepHook : Observer

    class Evaluator {
        <<Strategy>>
        +evaluate(Task, Trajectory) EvalResult*
    }
    Evaluator <|-- KeywordEvaluator
    Evaluator <|-- FunctionalEvaluator

    class Environment {
        <<abstract>>
        +working_dir() path*
        +setup()* 
        +teardown()*
        +allows_command(cmd) bool*
    }
    Environment <|-- NativeEnvironment
    Environment <|-- SandboxEnvironment

    class Step
    class Trajectory {
        +to_json()
        +save(path)
    }
    Trajectory o-- "*" Step

    class HarnessRunner {
        +run_task(Task) RunOutcome
        +run_batch(tasks, dir) BatchReport
    }
    HarnessRunner ..> AgentLoop : creates
    HarnessRunner --> Evaluator : selects (Strategy)
    HarnessRunner --> Environment : uses
    HarnessRunner ..> Trajectory : records via step hook (Observer)
    HarnessRunner ..> Task
```

**Design invariants shown:** `AgentLoop` depends on `LLMClient` / `Tool` interfaces only — never
on `HarnessRunner`. The Harness reaches *down* into the agent via the `StepHook`, not the reverse.

---

## 2. Sequence — one complete agent run

```mermaid
sequenceDiagram
    actor User
    participant Main as main (CLI)
    participant Agent as AgentLoop
    participant LLM as OllamaClient
    participant Reg as ToolRegistry
    participant T as Tool
    participant LD as LoopDetector

    User->>Main: task string
    Main->>Agent: run(task)
    Agent->>Agent: build_initial_messages (persona + skill + tool catalog)

    loop until FinalAnswer or max_steps
        Agent->>LLM: chat(history)
        LLM-->>Agent: assistant JSON
        Agent->>Agent: parse_action() -> Action

        alt Action is ToolCall
            Agent->>LD: observe("tool|args")
            LD-->>Agent: Verdict (None/Warning/Critical)
            Agent->>Reg: get(tool)
            Reg-->>Agent: Tool*
            Agent->>T: execute(args)
            T-->>Agent: ToolResult
            Agent->>Agent: observe() -> append "Observation" to history
        else Action is FinalAnswer
            Agent-->>Main: AgentResult (success, final answer)
        end
    end

    Main-->>User: final answer + step trace
```

---

## 3. Sequence — HarnessRunner batch evaluation

```mermaid
sequenceDiagram
    participant R as run_eval
    participant H as HarnessRunner
    participant E as Environment
    participant A as AgentLoop
    participant V as Evaluator
    participant FS as filesystem

    R->>H: run_batch(tasks, out_dir)
    loop each Task
        H->>E: setup() + chdir(working_dir)
        H->>A: construct + set_step_hook(record into Trajectory)
        H->>A: run(task.instruction)
        Note over A,H: each Step is pushed into the Trajectory via the hook (Observer)
        A-->>H: AgentResult
        H->>V: pick_evaluator(eval_type).evaluate(task, trajectory)
        V-->>H: EvalResult (pass/score)
        H->>E: teardown()
        H->>FS: write trajectory_<id>.json
    end
    H->>FS: write summary.json
    H-->>R: BatchReport (success rate)
```

---

## 4. Component Diagram (modules + dependencies)

```mermaid
flowchart TD
    subgraph apps[applications]
        main[main / agent CLI]
        run_eval[run_eval / benchmark]
    end
    subgraph agent[agent/]
        AgentLoop
        SkillLoader
        LoopDetector
        Action
    end
    subgraph client[client/]
        LLMClient -. implements .-> OllamaClient
    end
    subgraph tools[tools/]
        Tool
        ToolRegistry
    end
    subgraph harness[harness/]
        HarnessRunner
        Evaluator
        Environment
        Trajectory
        Task
    end

    main --> AgentLoop
    run_eval --> HarnessRunner
    AgentLoop --> LLMClient
    AgentLoop --> ToolRegistry
    AgentLoop --> SkillLoader
    AgentLoop --> LoopDetector
    ToolRegistry --> Tool
    HarnessRunner --> AgentLoop
    HarnessRunner --> Evaluator
    HarnessRunner --> Environment
    HarnessRunner --> Trajectory
    HarnessRunner --> Task
```

**Dependency direction is strictly downward:** `apps → harness → agent → {client, tools}`.
No lower layer ever includes a higher one.
