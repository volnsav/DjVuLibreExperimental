param(
  [ValidateSet("Debug", "Release")]
  [string]$Configuration = "Debug",
  [ValidateSet("Win32", "x64")]
  [string]$Platform = "x64",
  [ValidateSet("auto", "v142", "v143")]
  [string]$Toolset = "auto",
  [string]$BuildDir = "build-windows",
  [switch]$NoBuild,
  [switch]$Package
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRoot = Join-Path $repoRoot $BuildDir

function Get-VsWhere {
  $path = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $path) {
    return $path
  }
  return $null
}

function Get-VisualStudioInfo {
  $vswhere = Get-VsWhere
  if (-not $vswhere) {
    throw "vswhere.exe was not found. Install Visual Studio 2019 or newer."
  }

  $raw = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
  if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($raw)) {
    throw "Visual Studio 2019 or newer with C++ tools was not detected."
  }

  $items = $raw | ConvertFrom-Json
  if ($items -isnot [System.Array]) {
    $items = @($items)
  }
  if ($items.Count -lt 1) {
    throw "Visual Studio 2019 or newer with C++ tools was not detected."
  }

  $item = $items[0]
  $major = [int]($item.installationVersion.Split(".")[0])
  switch ($major) {
    17 {
      return @{
        InstallPath = $item.installationPath
        Generator = "Visual Studio 17 2022"
        DefaultToolset = "v143"
      }
    }
    16 {
      return @{
        InstallPath = $item.installationPath
        Generator = "Visual Studio 16 2019"
        DefaultToolset = "v142"
      }
    }
    default {
      throw "Unsupported Visual Studio version: $($item.installationVersion). Require Visual Studio 2019 or newer."
    }
  }
}

function Get-Triplet {
  if ($Platform -eq "x64") {
    return "x64-windows"
  }
  return "x86-windows"
}

function Get-VcpkgRoot {
  if (-not [string]::IsNullOrWhiteSpace($env:VCPKG_ROOT) -and (Test-Path (Join-Path $env:VCPKG_ROOT "vcpkg.exe"))) {
    return (Resolve-Path $env:VCPKG_ROOT).Path
  }

  return (Join-Path $repoRoot "third_party\vcpkg")
}

function Ensure-Vcpkg {
  param([string]$Root)

  if (-not (Test-Path $Root)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $Root -Parent) | Out-Null
    git clone https://github.com/microsoft/vcpkg.git $Root
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to clone vcpkg into $Root"
    }
  }

  $vcpkgExe = Join-Path $Root "vcpkg.exe"
  if (-not (Test-Path $vcpkgExe)) {
    $bootstrap = Join-Path $Root "bootstrap-vcpkg.bat"
    if (-not (Test-Path $bootstrap)) {
      throw "vcpkg bootstrap script not found: $bootstrap"
    }
    & $bootstrap -disableMetrics
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to bootstrap vcpkg."
    }
  }

  return (Resolve-Path $Root).Path
}

function Ensure-VcpkgPackages {
  param(
    [string]$Root,
    [string]$Triplet
  )

  $vcpkgExe = Join-Path $Root "vcpkg.exe"
  $packages = @(
    "libjpeg-turbo:$Triplet"
    "tiff:$Triplet"
    "gtest:$Triplet"
  )

  & $vcpkgExe install @packages
  if ($LASTEXITCODE -ne 0) {
    throw "vcpkg install failed."
  }
}

function Invoke-CMake {
  param([string[]]$Arguments)

  Write-Host ("cmake " + ($Arguments -join " "))
  & cmake @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "cmake failed with exit code $LASTEXITCODE"
  }
}

$vsInfo = Get-VisualStudioInfo
$resolvedToolset = if ($Toolset -eq "auto") { $vsInfo.DefaultToolset } else { $Toolset }
$triplet = Get-Triplet
$vcpkgRoot = Ensure-Vcpkg -Root (Get-VcpkgRoot)
Ensure-VcpkgPackages -Root $vcpkgRoot -Triplet $triplet

$toolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchain)) {
  throw "vcpkg toolchain file not found: $toolchain"
}

$cmakeArgs = @(
  "-S", $repoRoot,
  "-B", $buildRoot,
  "-G", $vsInfo.Generator,
  "-A", $Platform,
  "-T", $resolvedToolset,
  "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
  "-DVCPKG_TARGET_TRIPLET=$triplet",
  "-DDJVULIBRE_ENABLE_GTEST=ON",
  "-DDJVULIBRE_ENABLE_XMLTOOLS=ON",
  "-DDJVULIBRE_ENABLE_DESKTOPFILES=OFF"
)

Invoke-CMake -Arguments $cmakeArgs

if (-not $NoBuild) {
  Invoke-CMake -Arguments @("--build", $buildRoot, "--config", $Configuration, "--parallel")
}

$runtimeDir = Join-Path $buildRoot ("bin\{0}" -f $Configuration)
$vcpkgBin = Join-Path $vcpkgRoot ("installed\{0}\bin" -f $triplet)
$vcpkgDebugBin = Join-Path $vcpkgRoot ("installed\{0}\debug\bin" -f $triplet)
$env:PATH = "$runtimeDir;$vcpkgBin;$vcpkgDebugBin;$env:PATH"

& ctest --test-dir $buildRoot -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

if ($Package) {
  $cpackConfig = Join-Path $buildRoot "CPackConfig.cmake"
  if (-not (Test-Path $cpackConfig)) {
    throw "CPackConfig.cmake not found: $cpackConfig"
  }
  & cpack --config $cpackConfig -C $Configuration -G ZIP
  if ($LASTEXITCODE -ne 0) {
    throw "CPack ZIP packaging failed."
  }

  $makensis = Get-Command makensis -ErrorAction SilentlyContinue
  if ($makensis) {
    & cpack --config $cpackConfig -C $Configuration -G NSIS
    if ($LASTEXITCODE -ne 0) {
      throw "CPack NSIS packaging failed."
    }
  } else {
    Write-Host "makensis not found, skipping NSIS packaging."
  }
}

Write-Host "Windows CMake build and tests passed."
