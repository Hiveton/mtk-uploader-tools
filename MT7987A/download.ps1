param(
    [string]$Model,
    [string]$SerialPort,
    [string]$DeviceIp,
    [ValidateSet("BL2", "GPT", "FIP", "FIRMWARE")]
    [string]$StartAt,
    [int]$WaitDeviceSeconds,
    [switch]$SkipUartBoot,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$models = [ordered]@{
    "1" = "H87Pro"
    "2" = "H87AM"
    "3" = "H5MIFI"
    "4" = "H5000M"
    "5" = "H5000W"
    "6" = "E87N"
}

function Write-Menu {
    Write-Host "MT7987A download entry"
    Write-Host ""
    foreach ($item in $models.GetEnumerator()) {
        Write-Host ("  {0}. {1}" -f $item.Key, $item.Value)
    }
    Write-Host ""
}

function Resolve-Model {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        Write-Menu
        $Value = Read-Host "Select model"
    }

    if ($models.Contains($Value)) {
        return $models[$Value]
    }

    foreach ($knownModel in $models.Values) {
        if ($knownModel.Equals($Value, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $knownModel
        }
    }

    throw "Unknown model: $Value. Use one of: $($models.Values -join ', ')"
}

$selectedModel = Resolve-Model -Value $Model
$modelDir = Join-Path $PSScriptRoot $selectedModel
$targetScript = Join-Path $modelDir "auto-download.ps1"

if (-not (Test-Path -LiteralPath $targetScript -PathType Leaf)) {
    throw "Missing target script: $targetScript"
}

$arguments = @(
    "-ExecutionPolicy", "Bypass",
    "-NoProfile",
    "-File", $targetScript
)

if (-not [string]::IsNullOrWhiteSpace($SerialPort)) {
    $arguments += @("-SerialPort", $SerialPort)
}
if (-not [string]::IsNullOrWhiteSpace($DeviceIp)) {
    $arguments += @("-DeviceIp", $DeviceIp)
}
if (-not [string]::IsNullOrWhiteSpace($StartAt)) {
    $arguments += @("-StartAt", $StartAt)
}
if ($WaitDeviceSeconds -gt 0) {
    $arguments += @("-WaitDeviceSeconds", $WaitDeviceSeconds)
}
if ($SkipUartBoot) {
    $arguments += "-SkipUartBoot"
}

Write-Output "Model: $selectedModel"
Write-Output "Directory: $modelDir"
Write-Output "Script: $targetScript"

if ($DryRun) {
    Write-Output "Dry-run command:"
    Write-Output ("powershell.exe " + (($arguments | ForEach-Object {
        if ($_ -match '\s') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join " "))
    exit 0
}

Push-Location -LiteralPath $modelDir
try {
    & powershell.exe @arguments
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
