param(
  [ValidateSet("5", "6")]
  [string]$QtMajor = "6",
  [switch]$NoInstallDeps,
  [switch]$EnableGtest,
  [int]$Jobs = 0
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$repoRootLinux = (& wsl wslpath -a $repoRoot).Trim()
if (-not $repoRootLinux) {
  throw "Failed to convert repository path to WSL path."
}

$args = @("--qt-major", $QtMajor)
if ($NoInstallDeps) {
  $args += "--no-install-deps"
}
if ($EnableGtest) {
  $args += "--enable-gtest"
}
if ($Jobs -gt 0) {
  $args += @("--jobs", "$Jobs")
}

$argLine = [string]::Join(" ", $args)
$cmd = "cd '$repoRootLinux' && bash ./scripts/bootstrap_wsl.sh $argLine"

Write-Host "Running in WSL: $cmd"
& wsl bash -lc $cmd
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
