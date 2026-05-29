param(
    [string]$SerialPort = "COM3",
    [string]$DeviceIp = "192.168.1.1",
    [string]$UartTool = ".\mtk_uartboot.exe",
    [string]$RamBl2 = ".\bl2ram.bin",
    [string]$UartFip = ".\fip-h5s.bin",
    [string]$WebBl2 = ".\bl2.img",
    [string]$WebGpt = ".\gpt.bin",
    [string]$WebFip = ".\fip.bin",
    [string]$Firmware = ".\HiGoROS-H5000AM-1-26-05-29-02.bin",
    [string]$Bl2Page = "bl2.html",
    [string]$GptPage = "gpt.html",
    [string]$FipPage = "uboot.html",
    [string]$FirmwarePage = "",
    [string]$UploadPath = "/upload",
    [string]$UpdatePath = "/flashing.html",
    [string]$ResultPath = "/result",
    [int]$BromBaudrate = 921600,
    [int]$Bl2Baudrate = 1500000,
    [int]$UbootBaudrate = 115200,
    [string]$UbootWebUiKey = "a",
    [int]$UbootWebUiKeySeconds = 8,
    [int]$WaitDeviceSeconds = 120,
    [int]$HttpTimeoutSeconds = 60,
    [int]$UpdateTimeoutSeconds = 1800,
    [int]$AfterUploadDelaySeconds = 8,
    [ValidateSet("BL2", "GPT", "FIP", "FIRMWARE")]
    [string]$StartAt = "BL2",
    [switch]$SkipUartBoot
)

$ErrorActionPreference = "Stop"

function Write-Info {
    param([string]$Message)
    Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

function Resolve-ExistingFile {
    param(
        [string]$Primary,
        [string[]]$Fallbacks = @()
    )

    $candidates = @($Primary) + $Fallbacks
    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "File not found: $($candidates -join ', ')"
}

function Join-Url {
    param(
        [string]$BaseUrl,
        [string]$MaybeRelative
    )

    if ([string]::IsNullOrWhiteSpace($MaybeRelative)) {
        return $BaseUrl
    }

    $base = [Uri]::new($BaseUrl)
    return ([Uri]::new($base, $MaybeRelative)).AbsoluteUri
}

function Get-FormInfo {
    param(
        [string]$Html,
        [string]$PageUrl
    )

    $formHtml = $Html
    $formMatch = [regex]::Match($Html, '<form\b(?<attrs>[^>]*)>(?<body>.*?)</form>', 'IgnoreCase,Singleline')
    if ($formMatch.Success) {
        $formHtml = $formMatch.Value
    }

    $action = Join-Url -BaseUrl $PageUrl -MaybeRelative $UploadPath
    $actionMatch = [regex]::Match($formHtml, '\baction\s*=\s*(?:"(?<v>[^"]*)"|''(?<v>[^'']*)''|(?<v>[^\s>]+))', 'IgnoreCase')
    if ($actionMatch.Success) {
        $action = Join-Url -BaseUrl $PageUrl -MaybeRelative $actionMatch.Groups["v"].Value
    }

    $fileField = "file"
    $fileInputMatch = [regex]::Match($formHtml, '<input\b(?=[^>]*\btype\s*=\s*(?:"file"|''file''|file))[^>]*>', 'IgnoreCase')
    if ($fileInputMatch.Success) {
        $nameMatch = [regex]::Match($fileInputMatch.Value, '\bname\s*=\s*(?:"(?<v>[^"]+)"|''(?<v>[^'']+)''|(?<v>[^\s>]+))', 'IgnoreCase')
        if ($nameMatch.Success) {
            $fileField = $nameMatch.Groups["v"].Value
        }
    }

    $hidden = @{}
    foreach ($match in [regex]::Matches($formHtml, '<input\b(?=[^>]*\btype\s*=\s*(?:"hidden"|''hidden''|hidden))[^>]*>', 'IgnoreCase')) {
        $nameMatch = [regex]::Match($match.Value, '\bname\s*=\s*(?:"(?<v>[^"]+)"|''(?<v>[^'']+)''|(?<v>[^\s>]+))', 'IgnoreCase')
        if (-not $nameMatch.Success) {
            continue
        }
        $valueMatch = [regex]::Match($match.Value, '\bvalue\s*=\s*(?:"(?<v>[^"]*)"|''(?<v>[^'']*)''|(?<v>[^\s>]+))', 'IgnoreCase')
        $hidden[$nameMatch.Groups["v"].Value] = $(if ($valueMatch.Success) { $valueMatch.Groups["v"].Value } else { "" })
    }

    return [pscustomobject]@{
        Action = $action
        FileField = $fileField
        Hidden = $hidden
    }
}

function Wait-DeviceWeb {
    param([string]$BaseUrl, [int]$TimeoutSeconds)

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            Invoke-WebRequest -Uri $BaseUrl -UseBasicParsing -TimeoutSec 3 | Out-Null
            return
        } catch {
            Start-Sleep -Seconds 2
        }
    }

    throw "Device web UI is not reachable: $BaseUrl"
}

function Invoke-UbootWebMode {
    param(
        [string]$PortName,
        [int]$Baudrate,
        [string]$Key,
        [int]$DurationSeconds
    )

    if ([string]::IsNullOrWhiteSpace($PortName)) {
        Write-Info "Skip U-Boot web UI key: no serial port"
        return
    }

    Write-Info "Entering U-Boot web UI via $PortName at ${Baudrate}: send '$Key'"
    $deadline = (Get-Date).AddSeconds($DurationSeconds)
    $serial = $null
    $opened = $false
    try {
        while ((Get-Date) -lt $deadline -and -not $opened) {
            try {
                $serial = [System.IO.Ports.SerialPort]::new($PortName, $Baudrate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
                $serial.ReadTimeout = 500
                $serial.WriteTimeout = 500
                $serial.DtrEnable = $true
                $serial.RtsEnable = $true
                $serial.Open()
                $opened = $true
            } catch {
                if ($serial) {
                    $serial.Dispose()
                    $serial = $null
                }
                Start-Sleep -Milliseconds 250
            }
        }

        if (-not $opened) {
            Write-Info "WARN U-Boot web UI key failed: unable to open $PortName"
            return
        }

        while ((Get-Date) -lt $deadline) {
            $serial.Write($Key)
            Start-Sleep -Milliseconds 250
        }
        Write-Info "U-Boot web UI key sent"
    } catch {
        Write-Info "WARN U-Boot web UI key failed: $($_.Exception.Message)"
    } finally {
        if ($serial -and $serial.IsOpen) {
            $serial.Close()
        }
        if ($serial) {
            $serial.Dispose()
        }
    }
}

function Invoke-UbootUpload {
    param(
        [string]$Name,
        [string]$Page,
        [string]$FilePath
    )

    $pageUrl = "http://$DeviceIp/$Page"
    $fileInfo = Get-Item -LiteralPath $FilePath
    $started = Get-Date

    Write-Info "START $Name"
    Write-Info "Page: $pageUrl"
    Write-Info "File: $($fileInfo.FullName) ($($fileInfo.Length) bytes)"

    $pageResponse = Invoke-WebRequest -Uri $pageUrl -UseBasicParsing -TimeoutSec $HttpTimeoutSeconds
    $form = Get-FormInfo -Html $pageResponse.Content -PageUrl $pageUrl
    Write-Info "Upload: $($form.Action), field: $($form.FileField)"

    $formArg = "$($form.FileField)=@$($fileInfo.FullName)"
    $curlOutput = & curl.exe --noproxy "*" -sS --max-time $HttpTimeoutSeconds -w "`nHTTP_CODE=%{http_code}`n" -F $formArg $form.Action 2>&1
    $curlExit = $LASTEXITCODE
    $elapsed = [int]((Get-Date) - $started).TotalSeconds

    foreach ($line in $curlOutput) {
        if ($line -match '^HTTP_CODE=(\d+)$') {
            Write-Info "Upload HTTP: $($Matches[1])"
        } elseif (-not [string]::IsNullOrWhiteSpace($line)) {
            Write-Info "Reply: $line"
        }
    }

    if ($curlExit -ne 0) {
        throw "$Name upload failed, curl exit code: $curlExit"
    }
    if (($curlOutput -join "`n") -match '(^|\s)fail(\s|$)') {
        throw "$Name upload failed: device replied fail"
    }

    Write-Info "UPLOAD DONE $Name in ${elapsed}s"
}

function Invoke-UbootUpdate {
    param([string]$Name)

    $updateUrl = "http://$DeviceIp$UpdatePath"
    $resultUrl = "http://$DeviceIp$ResultPath"
    $started = Get-Date

    Write-Info "UPDATE START $Name"
    Write-Info "Update page: $updateUrl"
    Invoke-WebRequest -Uri $updateUrl -UseBasicParsing -TimeoutSec $HttpTimeoutSeconds | Out-Null

    Write-Info "Waiting result: $resultUrl"
    try {
        $result = Invoke-WebRequest -Uri $resultUrl -UseBasicParsing -TimeoutSec $UpdateTimeoutSeconds
    } catch {
        throw "$Name update result request failed: $($_.Exception.Message)"
    }

    $text = ($result.Content -replace '\s+', ' ').Trim()
    $elapsed = [int]((Get-Date) - $started).TotalSeconds
    Write-Info "Update result HTTP: $($result.StatusCode)"
    if ($text) {
        Write-Info "Update result: $text"
    }

    if ($text -eq "failed") {
        throw "$Name update failed"
    }

    Write-Info "UPDATE DONE $Name in ${elapsed}s"
}

$webBl2Path = Resolve-ExistingFile -Primary $WebBl2
$webGptPath = Resolve-ExistingFile -Primary $WebGpt
$webFipPath = Resolve-ExistingFile -Primary $WebFip
$firmwarePath = Resolve-ExistingFile -Primary $Firmware

Write-Info "MT7987A auto download started"

if (-not $SkipUartBoot) {
    $uartToolPath = Resolve-ExistingFile -Primary $UartTool
    $ramBl2Path = Resolve-ExistingFile -Primary $RamBl2
    $uartFipPath = Resolve-ExistingFile -Primary $UartFip -Fallbacks @(".\fip.bin")

    Write-Info "UART boot via $SerialPort"
    & $uartToolPath -s $SerialPort -p $ramBl2Path -a -f $uartFipPath --brom-load-baudrate $BromBaudrate --bl2-load-baudrate $Bl2Baudrate
    if ($LASTEXITCODE -ne 0) {
        throw "mtk_uartboot failed, exit code: $LASTEXITCODE"
    }
    Write-Info "UART boot finished"
    Invoke-UbootWebMode -PortName $SerialPort -Baudrate $UbootBaudrate -Key $UbootWebUiKey -DurationSeconds $UbootWebUiKeySeconds
}

$baseUrl = "http://$DeviceIp/"
Write-Info "Waiting for U-Boot web UI: $baseUrl"
Wait-DeviceWeb -BaseUrl $baseUrl -TimeoutSeconds $WaitDeviceSeconds
Write-Info "U-Boot web UI is reachable"

$steps = @(
    @{ Name = "BL2"; Page = $Bl2Page; File = $webBl2Path },
    @{ Name = "GPT"; Page = $GptPage; File = $webGptPath },
    @{ Name = "FIP"; Page = $FipPage; File = $webFipPath },
    @{ Name = "FIRMWARE"; Page = $FirmwarePage; File = $firmwarePath }
)

$startIndex = 0
for ($i = 0; $i -lt $steps.Count; $i++) {
    if ($steps[$i].Name -eq $StartAt) {
        $startIndex = $i
        break
    }
}

foreach ($step in $steps[$startIndex..($steps.Count - 1)]) {
    Invoke-UbootUpload -Name $step.Name -Page $step.Page -FilePath $step.File
    Invoke-UbootUpdate -Name $step.Name
    Start-Sleep -Seconds $AfterUploadDelaySeconds
    Wait-DeviceWeb -BaseUrl $baseUrl -TimeoutSeconds $WaitDeviceSeconds
}

Write-Info "All download steps finished"
