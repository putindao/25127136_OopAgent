# CLAUDE.md — Project Template

> This is a **standalone project template**, fully separate from older projects.
> Any new project can copy this folder as a starting point.
> **This file MUST NOT exceed 200 lines.** Keep it short and to the point.

---

## 1. Core rules (MANDATORY)

### Rule 1 — Always plan before doing
- Before any non-trivial code/change/refactor, THINK and present a plan first
  (steps, affected files, risks).
- For multi-step tasks, save the plan under `docs/plans/` (one `.md` per task).
- Only start coding once the plan is clear. No "think-while-typing".

### Rule 2 — Auto-log every change to Obsidian
- On every `Edit` / `Write` / `MultiEdit` / `NotebookEdit` on a file, an
  **automatic hook** records it into the Obsidian vault.
- Logs are organized **per project and per day**:
  `D:\obsidian\putindao\<project-name>\logs\YYYY-MM-DD.md`
- This is automatic (see section 4) — it does not rely on remembering to log.

### Rule 3 — Keep CLAUDE.md under 200 lines
- This file is the project's short "contract". When more detail is needed, move
  it to `README.md` or `docs/`, do **not** bloat CLAUDE.md.
- Before adding new content, consider removing/merging old content.

### Rule 4 — Language policy
- **Source code, comments, docs, commits, identifiers, and logs: 100% English.**
- **Chat replies to the user: Vietnamese.**
- No Vietnamese inside any file in the repository.

---

## 2. Folder structure

```
.
├── CLAUDE.md              # Project rules (this file, < 200 lines)
├── README.md             # How to use / reuse this template
├── .gitignore
├── .claude/
│   ├── settings.json     # Registers PostToolUse hook -> Obsidian log
│   └── hooks/
│       └── log-to-obsidian.ps1   # Auto-logging script
├── src/                  # Your source code
└── docs/
    └── plans/            # Saved plans per task (Rule 1)
```

> You are free to add subfolders under `src/` per each project's needs.

---

## 3. Working conventions

- **Chat language:** Vietnamese. **Code/docs language:** English (Rule 4).
- **File/folder naming:** `kebab-case`, descriptive.
- **Commits (if using git):** short imperative, describe "what was done".
- **New code** must match the surrounding style (naming, comments, idioms).
- Before deleting/overwriting an existing file: read it first, never delete blind.

---

## 4. How Obsidian logging works

The hook lives in `.claude/settings.json` on the **PostToolUse** event and runs
AFTER each file edit. It calls `.claude/hooks/log-to-obsidian.ps1`, which:

1. Reads the JSON payload (tool name + file path) from Claude Code via stdin.
2. Resolves the **project name** = the root folder name (via `$CLAUDE_PROJECT_DIR`).
3. Appends one row to `<Vault>\<project-name>\logs\<date>.md` as a table:
   `| time | action | relative-file-path |`
4. Creates the folder/file + frontmatter if missing. All errors are swallowed
   (never blocks your work).

**Change the vault path:** edit the `$VaultPath` variable at the top of
`log-to-obsidian.ps1`. Current vault: `D:\obsidian\putindao`.

> WARNING: The hook is code that runs on your machine. The first time you open
> the project, Claude Code may ask you to **approve the hook** for security —
> accept it so logging works.

---

## 5. Reusing for a new project

1. Copy the whole template folder and **rename the root folder**
   (the folder name is the project name used in logs).
2. Open the new project with Claude Code, approve the hook when prompted.
3. (Optional) adjust `$VaultPath` for a different vault.
4. Start coding in `src/`. Logs appear in Obsidian automatically.

---

## 6. Quick checklist per task

- [ ] Understood the request? If unclear -> ask.
- [ ] Made a plan (and saved to `docs/plans/` if multi-step)?
- [ ] Code matches surrounding style and is 100% English?
- [ ] Logging hook active (check today's file in the vault)?
- [ ] CLAUDE.md still < 200 lines?
