param(
    [string]$QtRoot = "",
    [string]$BuildDir = "build",
    [switch]$Deploy
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$PythonScripts = Join-Path $env:APPDATA "Python\Python314\Scripts"
$CMake = Join-Path $PythonScripts "cmake.exe"
$Ninja = Join-Path $PythonScripts "ninja.exe"
$VcVars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if (-not $QtRoot) {
    $LocalQt = Join-Path $ProjectRoot ".qt\6.7.3\msvc2019_64"
    if (Test-Path -LiteralPath $LocalQt -PathType Container) {
        $QtRoot = $LocalQt
    }
}

if (-not (Test-Path -LiteralPath $QtRoot -PathType Container)) {
    throw "QtRoot not found. Pass -QtRoot C:\Qt\6.x.x\msvc2019_64 or install local Qt under .qt."
}
if (-not (Test-Path -LiteralPath $CMake -PathType Leaf)) {
    throw "cmake.exe not found at $CMake. Install with: py -m pip install --user cmake ninja"
}
if (-not (Test-Path -LiteralPath $Ninja -PathType Leaf)) {
    throw "ninja.exe not found at $Ninja. Install with: py -m pip install --user ninja"
}
if (-not (Test-Path -LiteralPath $VcVars -PathType Leaf)) {
    throw "Visual Studio vcvars64.bat not found: $VcVars"
}

$BuildPath = Join-Path $ProjectRoot $BuildDir
$TempBat = Join-Path ([System.IO.Path]::GetTempPath()) "build-mt7987a-downloadtools.bat"
@"
@echo off
call "$VcVars"
if errorlevel 1 exit /b %errorlevel%
"$CMake" -S "$ProjectRoot" -B "$BuildPath" -G Ninja -DCMAKE_PREFIX_PATH="$QtRoot" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="$Ninja"
if errorlevel 1 exit /b %errorlevel%
"$CMake" --build "$BuildPath" --config Release
exit /b %errorlevel%
"@ | Set-Content -LiteralPath $TempBat -Encoding ASCII

cmd.exe /c "`"$TempBat`""
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Deploy) {
    $DeployDir = Join-Path $ProjectRoot "deploy\windows"
    if (Test-Path -LiteralPath $DeployDir) {
        Remove-Item -LiteralPath $DeployDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $DeployDir | Out-Null
    Copy-Item -LiteralPath (Join-Path $BuildPath "MT7987ADownloadTools.exe") -Destination $DeployDir
    & (Join-Path $QtRoot "bin\windeployqt.exe") --release --no-translations --no-compiler-runtime (Join-Path $DeployDir "MT7987ADownloadTools.exe")
    $AssetsDir = Join-Path $ProjectRoot "assets"
    if (Test-Path -LiteralPath $AssetsDir -PathType Container) {
        Copy-Item -LiteralPath $AssetsDir -Destination (Join-Path $DeployDir "assets") -Recurse -Force
    }
}
