# Hiveton MTK Downloader Tools

Hiveton MTK Downloader Tools 是面向 Windows 和 macOS 的 Qt Widgets 桌面下载工具，用于 MT7987A 工厂固件烧录。软件封装 `MT7987A/download.ps1` 和 `MT7987A/download-mac.sh` 总入口脚本，提供统一的图形化操作界面。

## 功能

- 从 `MT7987A/boards.json` 读取板卡配置。
- 默认支持 `H87Pro`、`H87AM`、`H5MIFI`、`H5000M`、`H5000W`、`E87N`。
- 支持图形化板卡和组件管理，可修改板卡照片。
- 校验 `BL2`、`GPT`、`FIP`、`FIRMWARE` 固件组件。
- 支持串口下拉选择、设备 IP、起始步骤、等待时间和跳过串口启动。
- 固件导入时可选择组件，并自动复制到当前板卡目录。
- 开始/停止下载状态清晰展示，实时输出日志。
- BL2 下载完成后可通过 115200 串口发送 `a` 进入 U-Boot WebUI 模式。
- 命令行和图形界面共用同一份板卡配置。

## 板卡配置

软件使用 `MT7987A/boards.json` 作为统一板卡数据库。如果文件不存在，启动时会根据当前生产包自动生成。

每个板卡包含：

- `id`：命令行使用的板卡型号。
- `displayName`：界面显示名称。
- `directoryName`：`MT7987A` 下的板卡目录。
- `boardImage`：板卡照片路径。
- `defaultIp`、`defaultStartStep`、`waitDeviceSeconds`：默认下载参数。
- `firmwareComponents`：固件组件，例如 `BL2`、`GPT`、`FIP`、`FIRMWARE`。

点击工具栏中的 **板卡管理** 可以添加、删除或编辑板卡和组件。点击 **导入固件** 可以选择组件，并把固件复制到当前板卡目录。

板卡管理窗口支持：

- **校验配置**：检查重复板卡 ID、空字段、重复组件和缺失 `FIRMWARE` 组件。
- **导入配置**：从其他 JSON 文件替换当前板卡数据库。
- **导出配置**：导出当前板卡数据库备份。
- 自动备份：保存前会在同目录生成 `boards.json.backup`。

## 构建

需要 Qt 6 Widgets 模块和 C++17 编译器。

Windows 构建：

```powershell
cd D:\mtk-uploader-tools\MT7987A\DownloadTools
.\scripts\build-windows.ps1 -QtRoot .\.qt\6.7.3\msvc2019_64 -Deploy
```

Qt 安装在其他目录时，通过 `-QtRoot` 指定路径。

通用 CMake 构建：

```powershell
cd D:\mtk-uploader-tools\MT7987A\DownloadTools
cmake -S . -B build
cmake --build build --config Release
```

macOS 构建：

```bash
cd /path/to/MT7987A/DownloadTools
cmake -S . -B build
cmake --build build --config Release
```

## 运行

从构建输出目录运行软件。程序会自动向上查找 `MT7987A` 目录，并识别 `download.ps1` 和 `download-mac.sh`。

如果从特殊目录启动，请把当前工作目录设置到 `MT7987A`。

冒烟测试模式：

```powershell
.\build\MT7987ADownloadTools.exe --smoke-test
```

Windows 如果启用了应用控制策略，本地未签名 `.exe` 可能会被拦截，需要按本机策略签名后运行。

## 验证

静态项目验证：

```powershell
powershell.exe -ExecutionPolicy Bypass -NoProfile -File .\tests\verify-downloadtools.ps1
```

期望输出：

```text
DownloadTools project verification passed
```
