param()

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Contains {
    param(
        [string]$Text,
        [string]$Expected
    )

    Assert-True -Condition $Text.Contains($Expected) -Message "Expected text not found: $Expected"
}

$downloadPs1 = Join-Path $Root "download.ps1"
$downloadBat = Join-Path $Root "download.bat"
$downloadMac = Join-Path $Root "download-mac.sh"

Assert-True -Condition (Test-Path -LiteralPath $downloadPs1 -PathType Leaf) -Message "Missing download.ps1"
Assert-True -Condition (Test-Path -LiteralPath $downloadBat -PathType Leaf) -Message "Missing download.bat"
Assert-True -Condition (Test-Path -LiteralPath $downloadMac -PathType Leaf) -Message "Missing download-mac.sh"

$entry = Get-Content -LiteralPath $downloadPs1 -Raw
foreach ($model in @("H87Pro", "H87AM", "H5MIFI", "H5000M", "H5000W", "E87N")) {
    Assert-Contains -Text $entry -Expected $model
}

$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($downloadPs1, [ref]$tokens, [ref]$errors) | Out-Null
Assert-True -Condition ($errors.Count -eq 0) -Message "download.ps1 has parse errors: $($errors | ForEach-Object Message)"

$dryRun = & powershell.exe -ExecutionPolicy Bypass -NoProfile -File $downloadPs1 -Model H5000M -SerialPort COM9 -DeviceIp 192.168.9.1 -StartAt FIRMWARE -WaitDeviceSeconds 3 -SkipUartBoot -DryRun 2>&1
Assert-True -Condition ($LASTEXITCODE -eq 0) -Message "Dry-run command failed: $dryRun"
$dryRunText = $dryRun -join "`n"
Assert-Contains -Text $dryRunText -Expected "Model: H5000M"
Assert-Contains -Text $dryRunText -Expected "Directory:"
Assert-Contains -Text $dryRunText -Expected "auto-download.ps1"
Assert-Contains -Text $dryRunText -Expected "-SerialPort COM9"
Assert-Contains -Text $dryRunText -Expected "-DeviceIp 192.168.9.1"
Assert-Contains -Text $dryRunText -Expected "-StartAt FIRMWARE"
Assert-Contains -Text $dryRunText -Expected "-WaitDeviceSeconds 3"
Assert-Contains -Text $dryRunText -Expected "-SkipUartBoot"

$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$badModel = & powershell.exe -ExecutionPolicy Bypass -NoProfile -File $downloadPs1 -Model BADMODEL -DryRun 2>&1
$badModelExitCode = $LASTEXITCODE
$ErrorActionPreference = $previousErrorActionPreference
Assert-True -Condition ($badModelExitCode -ne 0) -Message "Invalid model should fail"
Assert-Contains -Text ($badModel -join "`n") -Expected "Unknown model"

foreach ($model in @("H87Pro", "H87AM", "H5MIFI", "H5000M", "H5000W", "E87N")) {
    $boardScript = Join-Path $Root "$model\auto-download.ps1"
    $boardText = Get-Content -LiteralPath $boardScript -Raw
    $skipIndex = $boardText.IndexOf('if (-not $SkipUartBoot)')
    $ramBl2Index = $boardText.IndexOf('$ramBl2Path = Resolve-ExistingFile')
    Assert-True -Condition ($skipIndex -ge 0) -Message "$model missing SkipUartBoot guard"
    Assert-True -Condition ($ramBl2Index -gt $skipIndex) -Message "$model resolves RamBl2 before SkipUartBoot guard"

    $windowsBl2Ram = Join-Path $Root "$model\bl2ram.bin"
    $macBl2Ram = Join-Path $Root "$model\mac\bl2ram.bin"
    Assert-True -Condition (Test-Path -LiteralPath $windowsBl2Ram -PathType Leaf) -Message "$model missing Windows bl2ram.bin"
    Assert-True -Condition (Test-Path -LiteralPath $macBl2Ram -PathType Leaf) -Message "$model missing mac/bl2ram.bin"
    $windowsHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $windowsBl2Ram).Hash
    $macHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $macBl2Ram).Hash
    Assert-True -Condition ($windowsHash -eq $macHash) -Message "$model bl2ram.bin hash mismatch between Windows and mac directories"
}

Write-Host "download entry tests passed"
