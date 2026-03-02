param(
  [ValidateSet("Debug", "Release")]
  [string]$Configuration = "Debug",
  [ValidateSet("Win32", "x64")]
  [string]$Platform = "x64",
  [ValidateSet("auto", "v143", "v145")]
  [string]$Toolset = "auto",
  [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$scriptPath = Join-Path $PSScriptRoot "scripts\\run_windows_tests.ps1"

if (-not (Test-Path $scriptPath)) {
  throw "Script not found: $scriptPath"
}

& $scriptPath -Configuration $Configuration -Platform $Platform -Toolset $Toolset -NoBuild:$NoBuild
exit $LASTEXITCODE
