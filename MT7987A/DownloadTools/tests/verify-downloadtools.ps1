param()

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")

function Assert-File {
    param([string]$RelativePath)
    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing file: $RelativePath"
    }
}

function Assert-Contains {
    param(
        [string]$RelativePath,
        [string]$Expected
    )
    $path = Join-Path $Root $RelativePath
    $text = Get-Content -LiteralPath $path -Raw -Encoding UTF8
    if (-not $text.Contains($Expected)) {
        throw "Expected '$Expected' in $RelativePath"
    }
}

foreach ($file in @(
    "CMakeLists.txt",
    "DownloadTools.pro",
    ".gitignore",
    "README.md",
    "scripts/build-windows.ps1",
    "assets/board-preview.png",
    "src/main.cpp",
    "src/MainWindow.h",
    "src/MainWindow.cpp",
    "src/BoardModel.h",
    "src/BoardModel.cpp",
    "src/BoardConfigDialog.h",
    "src/BoardConfigDialog.cpp",
    "src/DownloadCommand.h",
    "src/DownloadCommand.cpp",
    "src/DownloadProcess.h",
    "src/DownloadProcess.cpp"
)) {
    Assert-File $file
}

Assert-Contains "CMakeLists.txt" "Qt6"
Assert-Contains "CMakeLists.txt" "Widgets"
Assert-Contains "CMakeLists.txt" "SerialPort"
Assert-Contains "CMakeLists.txt" "BoardConfigDialog.cpp"
Assert-Contains "CMakeLists.txt" "/utf-8"
Assert-Contains "DownloadTools.pro" "serialport"
Assert-Contains "DownloadTools.pro" "BoardConfigDialog.cpp"
Assert-Contains "DownloadTools.pro" "/utf-8"
Assert-Contains ".gitignore" ".qt/"
Assert-Contains ".gitignore" "deploy/"
Assert-Contains "scripts/build-windows.ps1" "windeployqt.exe"
Assert-Contains "scripts/build-windows.ps1" "assets"
Assert-Contains "src/BoardModel.cpp" "H87Pro"
Assert-Contains "src/BoardModel.cpp" "H87AM"
Assert-Contains "src/BoardModel.cpp" "H5MIFI"
Assert-Contains "src/BoardModel.cpp" "H5000M"
Assert-Contains "src/BoardModel.cpp" "H5000W"
Assert-Contains "src/BoardModel.cpp" "E87N"
Assert-Contains "src/BoardModel.cpp" "boards.json"
Assert-Contains "src/BoardModel.cpp" "QJsonDocument"
Assert-Contains "src/BoardModel.cpp" "firmwareComponents"
Assert-Contains "src/BoardModel.cpp" "boardImage"
Assert-Contains "src/BoardModel.h" "FirmwareComponentInfo"
Assert-Contains "src/BoardModel.h" "save"
Assert-Contains "src/BoardModel.h" "validate"
Assert-Contains "src/BoardModel.h" "exportConfig"
Assert-Contains "src/BoardModel.h" "importConfig"
Assert-Contains "src/BoardModel.cpp" "backup"
Assert-Contains "src/BoardConfigDialog.cpp" "板卡配置"
Assert-Contains "src/BoardConfigDialog.cpp" "添加板卡"
Assert-Contains "src/BoardConfigDialog.cpp" "添加组件"
Assert-Contains "src/BoardConfigDialog.cpp" "导入配置"
Assert-Contains "src/BoardConfigDialog.cpp" "导出配置"
Assert-Contains "src/BoardConfigDialog.cpp" "校验配置"
Assert-Contains "src/BoardConfigDialog.cpp" "板卡照片"
Assert-Contains "src/BoardConfigDialog.cpp" "选择照片"
Assert-Contains "src/BoardConfigDialog.cpp" "QTableWidget"
Assert-Contains "src/MainWindow.cpp" "板卡管理"
Assert-Contains "src/MainWindow.cpp" "BoardConfigDialog"
Assert-Contains "src/MainWindow.cpp" "缺少必需组件"
Assert-Contains "src/MainWindow.cpp" "boards.json"
Assert-Contains "..\download.ps1" "boards.json"
Assert-Contains "..\download.ps1" "ConvertFrom-Json"
Assert-Contains "..\download-mac.sh" "boards.json"
Assert-Contains "..\download-mac.sh" "python3"
Assert-Contains "..\H87AM\auto-download.ps1" "Invoke-UbootWebMode"
Assert-Contains "..\H87AM\auto-download.ps1" "UbootBaudrate = 115200"
Assert-Contains "..\H87AM\auto-download.ps1" 'UbootWebUiKey = "a"'
Assert-Contains "..\H87AM\mac\auto-download-mac.sh" "enter_uboot_web_mode"
Assert-Contains "..\H87AM\mac\auto-download-mac.sh" "UBOOT_BAUDRATE"
Assert-Contains "src/DownloadCommand.cpp" "download.ps1"
Assert-Contains "src/DownloadCommand.cpp" "download-mac.sh"
Assert-Contains "src/DownloadCommand.cpp" "-SkipUartBoot"
Assert-Contains "src/DownloadCommand.cpp" "-DryRun"
Assert-Contains "src/DownloadCommand.cpp" "--dry-run"
Assert-Contains "src/DownloadProcess.cpp" "decodeProcessOutput"
Assert-Contains "src/DownloadProcess.cpp" "PYTHONIOENCODING"
Assert-Contains "src/DownloadProcess.cpp" "POWERSHELL_OUTPUT_ENCODING"
Assert-Contains "src/main.cpp" "Hiveton MTK Downloader Tools"
Assert-Contains "src/MainWindow.cpp" "Hiveton MTK Downloader Tools"
Assert-Contains "src/MainWindow.cpp" "版本号：www.hiveton.com"
Assert-Contains "src/MainWindow.cpp" "微信：qiqistudio"
Assert-Contains "src/MainWindow.cpp" "开始下载"
Assert-Contains "src/MainWindow.cpp" "下载进行中..."
Assert-Contains "src/MainWindow.cpp" "正在停止..."
Assert-Contains "src/MainWindow.cpp" "下载已成功完成。"
Assert-Contains "src/MainWindow.cpp" "刷新串口"
Assert-Contains "src/MainWindow.cpp" "扫描设备"
Assert-Contains "src/MainWindow.cpp" "导入固件"
Assert-Contains "src/MainWindow.cpp" "烧录进度"
Assert-Contains "src/MainWindow.cpp" "applyTheme"
Assert-Contains "src/MainWindow.cpp" "isDarkTheme"
Assert-Contains "src/MainWindow.cpp" "ApplicationPaletteChange"
Assert-Contains "src/MainWindow.cpp" "PaletteChange"
Assert-Contains "src/MainWindow.cpp" "darkTheme"
Assert-Contains "src/MainWindow.cpp" "setStyleSheet"
Assert-Contains "src/MainWindow.cpp" "m_applyingTheme"
Assert-Contains "src/MainWindow.cpp" "校验 (SHA256)"
Assert-Contains "src/MainWindow.cpp" "进行中"
Assert-Contains "src/MainWindow.cpp" "QFileDialog::getOpenFileName"
Assert-Contains "src/MainWindow.cpp" "QFileDialog::getSaveFileName"
Assert-Contains "src/MainWindow.cpp" "QInputDialog::getItem"
Assert-Contains "src/MainWindow.cpp" "componentPath"
Assert-Contains "src/MainWindow.cpp" "board-preview.png"
Assert-Contains "src/MainWindow.cpp" "setEditable(false)"
Assert-Contains "src/MainWindow.cpp" "未发现串口"
Assert-Contains "src/MainWindow.cpp" "清空"
Assert-Contains "src/MainWindow.cpp" "复制"
Assert-Contains "src/MainWindow.cpp" "保存"
Assert-Contains "src/MainWindow.cpp" "QSerialPortInfo::availablePorts"
Assert-Contains "src/MainWindow.cpp" "正在等待 U-Boot WebUI"
Assert-Contains "src/MainWindow.cpp" "HiGoROS-H5000AM-1-26-05-29-02.bin"
Assert-Contains "src/main.cpp" "--smoke-test"
Assert-Contains "src/main.cpp" "DOWNLOADTOOLS_SMOKE_TEST"
Assert-Contains "src/MainWindow.cpp" "QStatusBar"
Assert-Contains "src/MainWindow.cpp" 'font-family: "Segoe UI"'

$mainTextForButtons = Get-Content -LiteralPath (Join-Path $Root "src/MainWindow.cpp") -Raw -Encoding UTF8
if ($mainTextForButtons.Contains("Dry Run")) {
    throw "Dry Run button text remains in MainWindow.cpp"
}

foreach ($badToken in @("[APP]", "[MODELS]", "[BOARD]", "[PORT]", "[IP]", "[STEP]", "[WAIT]", "[PKG]", "[FLOW]", "[LOG]", "[DRY]", "[START]", "[STOP]")) {
    $mainText = Get-Content -LiteralPath (Join-Path $Root "src/MainWindow.cpp") -Raw -Encoding UTF8
    if ($mainText.Contains($badToken)) {
        throw "Placeholder UI token remains in MainWindow.cpp: $badToken"
    }
}

Write-Host "DownloadTools project verification passed"
