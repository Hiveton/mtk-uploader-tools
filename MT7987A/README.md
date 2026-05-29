# MT7987A Production Download Guide

This directory contains MT7987A board download packages for multiple models.
Use the root entry script first, then select the target model.

## Supported Models

| Model | Firmware |
| --- | --- |
| H87Pro | `87pro.bin` |
| H87AM | `HiGoROS-H5000AM-1-26-05-29-02.bin` |
| H5MIFI | `87pro.bin` |
| H5000M | `HiGoROS-H5000M-1-26-04-29-09.bin` |
| H5000W | `HiGoROS-H5000M-1-26-04-29-09.bin` |
| E87N | `HiGoROS-E87N-1-26-05-09-02.bin` |

## Windows Operation

Open PowerShell or Command Prompt, then enter the `MT7987A` directory:

```powershell
cd D:\mtk-uploader-tools\MT7987A
```

Run the menu entry:

```powershell
.\download.bat
```

Select the board model number when prompted:

```text
1. H87Pro
2. H87AM
3. H5MIFI
4. H5000M
5. H5000W
6. E87N
```

The entry script changes into the selected model directory and runs that
model's `auto-download.ps1`.

## Direct Windows Commands

Run a specific model directly:

```powershell
.\download.ps1 -Model H87AM
```

Specify a serial port:

```powershell
.\download.ps1 -Model H5000M -SerialPort COM5
```

Specify the device IP:

```powershell
.\download.ps1 -Model E87N -DeviceIp 192.168.1.1
```

Start from a later step:

```powershell
.\download.ps1 -Model H87AM -StartAt FIRMWARE
```

Skip UART boot and only use the U-Boot web UI:

```powershell
.\download.ps1 -Model H87AM -StartAt FIRMWARE -SkipUartBoot
```

Shorten or extend the wait time for the device web UI:

```powershell
.\download.ps1 -Model H87AM -SkipUartBoot -WaitDeviceSeconds 30
```

Print the command without running download:

```powershell
.\download.ps1 -Model H87AM -StartAt FIRMWARE -SkipUartBoot -DryRun
```

## Download Steps

The normal download flow is:

1. UART boot with `mtk_uartboot.exe`
2. Reopen the same serial port at 115200 baud and repeatedly send `a` to force
   U-Boot Web UI mode
3. Wait for `http://<device-ip>/`
4. Upload and flash `bl2.img`
5. Upload and flash `gpt.bin`
6. Upload and flash `fip.bin`
7. Upload and flash the model firmware

Valid `-StartAt` values are:

```text
BL2
GPT
FIP
FIRMWARE
```

Use `-StartAt FIRMWARE -SkipUartBoot` only when the board is already in the
U-Boot web download page and reachable from the PC.

## macOS Operation

Open Terminal and enter this directory:

```bash
cd /path/to/MT7987A
```

Run the mac entry script:

```bash
./download-mac.sh
```

Run a specific model directly:

```bash
./download-mac.sh --model H87AM
```

Specify serial port and device IP:

```bash
./download-mac.sh --model H5000M --serial /dev/tty.usbserial-0001 --ip 192.168.1.1
```

Skip UART boot and start from firmware:

```bash
./download-mac.sh --model E87N --start-at FIRMWARE --skip-uart
```

Print the command without running download:

```bash
./download-mac.sh --model H87AM --start-at FIRMWARE --skip-uart --dry-run
```

## Directory Layout

Each model directory contains its own download files:

```text
MT7987A
  download.bat
  download.ps1
  download-mac.sh
  H87AM
    auto-download.bat
    auto-download.ps1
    bl2ram.bin
    bl2.img
    gpt.bin
    fip.bin
    HiGoROS-H5000AM-1-26-05-29-02.bin
    mac
      auto-download-mac.sh
      bl2ram.bin
      bl2.img
      gpt.bin
      fip.bin
```

`bl2ram.bin` is copied into both the Windows model directory and the `mac`
subdirectory.

## Troubleshooting

If PowerShell blocks the script, run:

```powershell
powershell.exe -ExecutionPolicy Bypass -NoProfile -File .\download.ps1 -Model H87AM
```

If the script reports:

```text
Device web UI is not reachable: http://192.168.1.1/
```

check that:

1. The board is powered on.
2. The PC network interface is connected to the board.
3. The PC can reach `192.168.1.1`.
4. The board is already in U-Boot web mode when using `-SkipUartBoot`.
5. The correct `-DeviceIp` was provided if the board IP is not `192.168.1.1`.

If the script reports a missing file, check that the selected model directory
contains the firmware listed in the Supported Models table.

## Verification

Run the entry test after changing scripts or replacing firmware packages:

```powershell
powershell.exe -ExecutionPolicy Bypass -NoProfile -File .\test-download-entry.ps1
```

Expected result:

```text
download entry tests passed
```
