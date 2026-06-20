# Project Template

A **standalone** project template, separate from older projects. Every source
change is **automatically logged to Obsidian** per project and per day, with
standard working rules (plan first, keep CLAUDE.md under 200 lines, English-only
source).

## What's inside

| Component | Role |
|-----------|------|
| `CLAUDE.md` | Project rules for Claude Code (< 200 lines) |
| `.claude/settings.json` | Registers the `PostToolUse` logging hook |
| `.claude/hooks/log-to-obsidian.ps1` | PowerShell script that writes logs |
| `src/` | Your source code |
| `docs/plans/` | Saved plans per task |

## Language policy

- **Source code, comments, docs, commits, logs: 100% English.**
- **Chat replies: Vietnamese.**

## Obsidian logging

- **Vault:** `D:\obsidian\putindao` (change via the `$VaultPath` variable in the hook)
- **Log path:** `<Vault>\<project-name>\logs\YYYY-MM-DD.md`
- **Trigger:** automatically after each `Edit` / `Write` / `MultiEdit` / `NotebookEdit`.
- **Each row:** time · action type · relative file path.

> The first time you open the project, Claude Code may ask you to approve the
> hook (it is code that runs on your machine). Accept it to enable logging.

## Use for a new project

1. Copy this folder -> rename the root folder (that name is the project name in logs).
2. Open with Claude Code, approve the hook.
3. (Optional) edit `$VaultPath` for a different vault.
4. Code in `src/`. Logs appear in Obsidian automatically.

## Verify the hook works

Create/edit any file under `src/`, then open
`D:\obsidian\putindao\New-project\logs\` and check today's file.

## Requirements

- Windows + PowerShell (5.1 or newer — ships with Windows 11).
- Obsidian (only the vault folder needs to exist; the app need not be open to log).
