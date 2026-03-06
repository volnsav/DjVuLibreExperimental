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

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$solution = Join-Path $repoRoot "windows\\djvulibre\\djvulibre.sln"
$outputDir = Join-Path $repoRoot ("windows\\build\\{0}\\{1}" -f $Configuration, $Platform)
$gtestExe = Join-Path $outputDir "libdjvu_gtest.exe"

function Get-VsInstallPath {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\\Installer\\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    return $null
  }
  $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
  if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installPath)) {
    return $null
  }
  return $installPath.Trim()
}

function Resolve-Toolset {
  param(
    [string]$RequestedToolset,
    [string]$VsInstallPath
  )

  if ($RequestedToolset -ne "auto") {
    return $RequestedToolset
  }

  if ([string]::IsNullOrWhiteSpace($VsInstallPath)) {
    throw "Visual Studio installation was not detected. Install Visual Studio Build Tools 2022."
  }

  $vcRoot = Join-Path $VsInstallPath "MSBuild\\Microsoft\\VC"
  $toolsetBases = @()

  if (Test-Path $vcRoot) {
    $toolsetBases = Get-ChildItem -Path $vcRoot -Directory |
      Where-Object { $_.Name -match '^v\d+$' } |
      Sort-Object Name -Descending |
      ForEach-Object { Join-Path $_.FullName "Platforms\\x64\\PlatformToolsets" }
  }

  foreach ($base in $toolsetBases) {
    if (Test-Path (Join-Path $base "v145\\Toolset.props")) {
      return "v145"
    }
  }

  foreach ($base in $toolsetBases) {
    if (Test-Path (Join-Path $base "v143\\Toolset.props")) {
      return "v143"
    }
  }

  throw "Unable to auto-detect PlatformToolset. Expected v145 or v143 under: $vcRoot"
}

function Invoke-MsBuild {
  param(
    [string[]]$Arguments
  )

  $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
  if ($msbuild) {
    Write-Host "MSBuild: $($msbuild.Source)"
    & $msbuild.Source @Arguments
    $script:MsBuildExitCode = [int]$LASTEXITCODE
    return
  }

  $vsInstallPath = Get-VsInstallPath
  if (-not $vsInstallPath) {
    throw "msbuild not found and Visual Studio installation was not detected. Install Visual Studio Build Tools 2022."
  }

  $msbuildExe = Join-Path $vsInstallPath "MSBuild\\Current\\Bin\\MSBuild.exe"
  if (-not (Test-Path $msbuildExe)) {
    throw "MSBuild.exe not found: $msbuildExe"
  }

  Write-Host "MSBuild: $msbuildExe"
  & $msbuildExe @Arguments
  $script:MsBuildExitCode = [int]$LASTEXITCODE
  return
}

function Get-VcpkgRoot {
  $candidates = @()
  if (-not [string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
    $candidates += $env:VCPKG_ROOT
  }
  $candidates += (Join-Path $env:USERPROFILE "vcpkg")
  $candidates += "C:\\vcpkg"
  $candidates += "C:\\tools\\vcpkg"

  foreach ($root in $candidates) {
    if ([string]::IsNullOrWhiteSpace($root)) {
      continue
    }
    if (Test-Path (Join-Path $root "vcpkg.exe")) {
      return (Resolve-Path $root).Path
    }
  }
  return $null
}

function Get-VcpkgTriplet {
  if ($Platform -eq "x64") {
    return "x64-windows"
  }
  return "x86-windows"
}

function Get-GTestRoot {
  $candidates = @()
  if (-not [string]::IsNullOrWhiteSpace($env:GTEST_ROOT)) {
    $candidates += $env:GTEST_ROOT
  }
  $candidates += (Join-Path $repoRoot "third_party\\googletest")

  foreach ($root in $candidates) {
    if ([string]::IsNullOrWhiteSpace($root)) {
      continue
    }
    $direct = Join-Path $root "src\\gtest-all.cc"
    $nested = Join-Path $root "googletest\\src\\gtest-all.cc"
    if ((Test-Path $direct) -or (Test-Path $nested)) {
      return (Resolve-Path $root).Path
    }
  }

  return $null
}

function Ensure-GTestDependency {
  param(
    [string]$VcpkgRoot,
    [string]$Triplet
  )

  $includeFile = Join-Path $VcpkgRoot ("installed\\{0}\\include\\gtest\\gtest.h" -f $Triplet)
  if (Test-Path $includeFile) {
    return
  }

  $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
  Write-Host "Installing gtest via vcpkg ($Triplet)..."
  & $vcpkgExe install ("gtest:{0}" -f $Triplet)
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to install gtest via vcpkg (exit code $LASTEXITCODE)."
  }

  if (-not (Test-Path $includeFile)) {
    throw "gtest headers not found after installation: $includeFile"
  }
}

function Add-GTestRuntimePath {
  param(
    [string]$VcpkgRoot,
    [string]$Triplet
  )

  $runtimeDir = if ($Configuration -eq "Debug") {
    Join-Path $VcpkgRoot ("installed\\{0}\\debug\\bin" -f $Triplet)
  } else {
    Join-Path $VcpkgRoot ("installed\\{0}\\bin" -f $Triplet)
  }

  if (-not (Test-Path $runtimeDir)) {
    throw "gtest runtime directory not found: $runtimeDir"
  }

  $env:PATH = "$runtimeDir;$env:PATH"
}

$resolvedToolset = $Toolset
if ($Toolset -eq "auto") {
  $resolvedToolset = Resolve-Toolset -RequestedToolset $Toolset -VsInstallPath (Get-VsInstallPath)
}

$gtestRoot = Get-GTestRoot
$vcpkgRoot = $null
$vcpkgTriplet = $null

if ($gtestRoot) {
  Write-Host "Using local GoogleTest sources: $gtestRoot"
} else {
  $vcpkgRoot = Get-VcpkgRoot
  if (-not $vcpkgRoot) {
    throw "GoogleTest sources were not found and vcpkg was not found. Set GTEST_ROOT, add third_party\\googletest, or install vcpkg."
  }
  $vcpkgTriplet = Get-VcpkgTriplet
  Ensure-GTestDependency -VcpkgRoot $vcpkgRoot -Triplet $vcpkgTriplet
  Add-GTestRuntimePath -VcpkgRoot $vcpkgRoot -Triplet $vcpkgTriplet
}

if (-not $NoBuild) {
  Write-Host "Building libdjvu_gtest ($Configuration|$Platform, $resolvedToolset)..."
  $buildArgs = @(
    $solution,
    "/nologo",
    "/verbosity:minimal",
    "/m",
    "/t:libdjvu_gtest",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/p:PlatformToolset=$resolvedToolset",
    "/p:DjvuPlatformToolset=$resolvedToolset"
  )
  if ($gtestRoot) {
    $buildArgs += "/p:GTestRoot=$gtestRoot"
  } else {
    $buildArgs += "/p:VcpkgRoot=$vcpkgRoot"
    $buildArgs += "/p:VcpkgTriplet=$vcpkgTriplet"
  }
  $script:MsBuildExitCode = 1
  Invoke-MsBuild -Arguments $buildArgs
  $exitCode = [int]$script:MsBuildExitCode
  Write-Host "Build finished with exit code $exitCode"
  if ($exitCode -ne 0) {
    throw "Build failed with exit code $exitCode"
  }
}

if (-not (Test-Path $gtestExe)) {
  throw "GoogleTest executable not found: $gtestExe"
}

$env:PATH = "$outputDir;$env:PATH"

Write-Host "Running $gtestExe"
& $gtestExe --gtest_color=yes
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

Write-Host "Windows gtest suite passed."
