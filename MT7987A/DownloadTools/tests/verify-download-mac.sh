#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOWNLOAD_COMMAND_SOURCE="$ROOT/DownloadTools/src/DownloadCommand.cpp"
MAIN_WINDOW_SOURCE="$ROOT/DownloadTools/src/MainWindow.cpp"
DOWNLOAD_PS1="$ROOT/download.ps1"

if grep -F 'command.program = "/bin/bash"' "$DOWNLOAD_COMMAND_SOURCE" >/dev/null; then
  printf 'DownloadCommand.cpp must not force macOS downloads through /bin/bash\n' >&2
  exit 1
fi

grep -F 'command.program = QDir(m_root).filePath("download-mac.sh");' "$DOWNLOAD_COMMAND_SOURCE" >/dev/null

if grep -F 'm_serialPort->addItem(label, port.portName());' "$MAIN_WINDOW_SOURCE" >/dev/null; then
  printf 'MainWindow.cpp must not pass short macOS serial port names to download scripts\n' >&2
  exit 1
fi

grep -F 'port.systemLocation()' "$MAIN_WINDOW_SOURCE" >/dev/null
grep -F 'options.skipUartBoot = m_skipUartBoot->isChecked() || m_startStep != "BL2";' "$MAIN_WINDOW_SOURCE" >/dev/null
grep -F 'm_skipUartBoot->setEnabled(!webOnlyStart);' "$MAIN_WINDOW_SOURCE" >/dev/null
grep -F 'setFont(QFont("Segoe UI", 14));' "$MAIN_WINDOW_SOURCE" >/dev/null
grep -F 'resize(1280, 820);' "$MAIN_WINDOW_SOURCE" >/dev/null
grep -F 'setMinimumSize(1120, 720);' "$MAIN_WINDOW_SOURCE" >/dev/null
if grep -E 'font-size: ([0-9]|1[01])pt' "$MAIN_WINDOW_SOURCE" >/dev/null; then
  printf 'MainWindow.cpp must not keep sub-12pt UI fonts after the global font enlargement\n' >&2
  exit 1
fi
grep -F '$StartAt -ne "BL2"' "$DOWNLOAD_PS1" >/dev/null
grep -F '$SkipUartBoot = $true' "$DOWNLOAD_PS1" >/dev/null

while IFS= read -r script; do
  grep -F 'BROM_BAUDRATE="${BROM_BAUDRATE:-460800}"' "$script" >/dev/null
  grep -F 'BL2_BAUDRATE="${BL2_BAUDRATE:-460800}"' "$script" >/dev/null
  grep -F 'UART baud: BROM=$BROM_BAUDRATE BL2=$BL2_BAUDRATE' "$script" >/dev/null
  grep -F 'trap cleanup_child TERM INT EXIT' "$script" >/dev/null
  grep -F 'CURRENT_CHILD_PID=$!' "$script" >/dev/null
  grep -F 'if [[ "$START_AT" != "BL2" ]]; then' "$script" >/dev/null
  grep -F 'SKIP_UART_BOOT=1' "$script" >/dev/null
  if awk '
    /require_file "\$UART_TOOL"/ { uart_require=NR }
    /if \[\[ "\$SKIP_UART_BOOT" -eq 0 \]\]/ { uart_block=NR }
    END { exit !(uart_require > uart_block) }
  ' "$script"; then
    :
  else
    printf 'UART files must only be required inside the UART boot branch: %s\n' "$script" >&2
    exit 1
  fi
  if awk '
    /require_file "\$WEB_BL2"/ && !in_case { early=1 }
    /case "\$step_name" in/ { in_case=1 }
    END { exit early }
  ' "$script"; then
    :
  else
    printf 'Web step files must be required only when their step runs: %s\n' "$script" >&2
    exit 1
  fi
done < <(find "$ROOT" -path '*/mac/auto-download-mac.sh' -type f | sort)

while IFS= read -r script; do
  grep -F '$StartAt -ne "BL2"' "$script" >/dev/null
  grep -F '$SkipUartBoot = $true' "$script" >/dev/null
  if grep -F '$webBl2Path = Resolve-ExistingFile -Primary $WebBl2' "$script" >/dev/null; then
    printf 'Windows step files must be resolved only when their step runs: %s\n' "$script" >&2
    exit 1
  fi
  grep -F '$stepFilePath = Resolve-ExistingFile -Primary $step.File' "$script" >/dev/null
done < <(find "$ROOT" -path '*/auto-download.ps1' -type f | sort)

output="$(
  "$ROOT/download-mac.sh" \
    --model h87am \
    --serial cu.usbserial-110 \
    --ip 192.168.1.1 \
    --start-at FIRMWARE \
    --wait-device 120 \
    --dry-run
)"

printf '%s\n' "$output" | grep -F "Model: H87AM" >/dev/null
printf '%s\n' "$output" | grep -F "Directory: $ROOT/H87AM/mac" >/dev/null
printf '%s\n' "$output" | grep -F "WAIT_DEVICE_SECONDS=120" >/dev/null
printf '%s\n' "$output" | grep -F -- "--start-at FIRMWARE" >/dev/null
printf '%s\n' "$output" | grep -F -- "--skip-uart" >/dev/null

bl2_output="$(
  "$ROOT/download-mac.sh" \
    --model H87AM \
    --serial /dev/cu.usbserial-110 \
    --ip 192.168.1.1 \
    --start-at BL2 \
    --wait-device 120 \
    --dry-run
)"
if printf '%s\n' "$bl2_output" | grep -F -- "--skip-uart" >/dev/null; then
  printf 'BL2 start must keep UART boot enabled by default\n' >&2
  exit 1
fi

printf 'download-mac.sh macOS command verification passed\n'
