# =============================================================================
#  log-to-obsidian.ps1
#  PostToolUse hook - auto-logs every source change into the Obsidian vault.
#  Layout: <Vault>\<project-name>\logs\YYYY-MM-DD.md  (one folder per project,
#  one file per day; changes within a day are appended to that file).
#
#  Receives the JSON payload from Claude Code via stdin. All errors are
#  swallowed (exit 0) so logging NEVER blocks the work in progress.
#
#  NOTE: every string WRITTEN to the log here is ASCII to avoid the Windows
#  PowerShell 5.1 encoding trap (a .ps1 without a BOM is read as ANSI). Dynamic
#  data (file paths) is read/written via .NET UTF-8, so non-ASCII folder names
#  still render correctly.
# =============================================================================

# ============================ CONFIG ========================================
# Path to your Obsidian vault. Change this line if the vault moves.
$VaultPath = 'D:\obsidian\putindao'
# ============================================================================

try {
    # --- Read the JSON payload from stdin (decode UTF-8 for Unicode safety) ---
    $stdin  = [System.Console]::OpenStandardInput()
    $reader = New-Object System.IO.StreamReader($stdin, [System.Text.Encoding]::UTF8)
    $raw    = $reader.ReadToEnd()
    $reader.Close()
    if ([string]::IsNullOrWhiteSpace($raw)) { exit 0 }
    $payload = $raw | ConvertFrom-Json

    $toolName = $payload.tool_name
    $filePath = $payload.tool_input.file_path
    if ([string]::IsNullOrWhiteSpace($filePath)) {
        # NotebookEdit uses notebook_path instead of file_path
        $filePath = $payload.tool_input.notebook_path
    }
    if ([string]::IsNullOrWhiteSpace($filePath)) { exit 0 }

    # --- Resolve project root + name ---
    $projectDir = $env:CLAUDE_PROJECT_DIR
    if ([string]::IsNullOrWhiteSpace($projectDir)) { $projectDir = $payload.cwd }
    if ([string]::IsNullOrWhiteSpace($projectDir)) { exit 0 }
    $projectName = Split-Path $projectDir -Leaf

    # --- Normalize the file path ---
    $fullFile = $filePath
    try { $fullFile = (Resolve-Path -LiteralPath $filePath -ErrorAction Stop).Path } catch {}

    # Skip changes inside the vault itself (avoid logging the log)
    if ($fullFile.ToLower().StartsWith($VaultPath.ToLower())) { exit 0 }

    # --- Compute the path relative to the project ---
    $projFull = $projectDir
    try { $projFull = (Resolve-Path -LiteralPath $projectDir -ErrorAction Stop).Path } catch {}
    $relPath = $fullFile
    if ($fullFile.ToLower().StartsWith($projFull.ToLower())) {
        $relPath = $fullFile.Substring($projFull.Length).TrimStart('\', '/')
    }
    if ([string]::IsNullOrWhiteSpace($relPath)) { $relPath = Split-Path $fullFile -Leaf }
    $relPath = $relPath -replace '\\', '/'

    # --- Build the log file path: <Vault>\<project>\logs\YYYY-MM-DD.md ---
    $now  = Get-Date
    $day  = $now.ToString('yyyy-MM-dd')
    $time = $now.ToString('HH:mm:ss')
    $logDir = Join-Path (Join-Path $VaultPath $projectName) 'logs'
    if (-not (Test-Path -LiteralPath $logDir)) {
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    }
    $logFile = Join-Path $logDir "$day.md"

    # --- Write with .NET UTF-8 WITHOUT BOM (clean for Obsidian) ---
    $enc = New-Object System.Text.UTF8Encoding($false)

    # Create the header (frontmatter + table) on the first write of the day
    if (-not (Test-Path -LiteralPath $logFile)) {
        $nl = [Environment]::NewLine
        $header =
            "---$nl" +
            "project: $projectName$nl" +
            "date: $day$nl" +
            "tags: [changelog, $projectName]$nl" +
            "---$nl$nl" +
            "# Change Log - $projectName - $day$nl$nl" +
            "| Time | Action | File |$nl" +
            "|------|--------|------|$nl"
        [System.IO.File]::WriteAllText($logFile, $header, $enc)
    }

    # Append one row for this change
    $entry = "| $time | ``$toolName`` | ``$relPath`` |" + [Environment]::NewLine
    [System.IO.File]::AppendAllText($logFile, $entry, $enc)
}
catch {
    # Never let a logging error interrupt the work.
}
exit 0
