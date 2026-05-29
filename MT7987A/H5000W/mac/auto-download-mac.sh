#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SERIAL_PORT="${SERIAL_PORT:-}"
DEVICE_IP="${DEVICE_IP:-192.168.1.1}"
UART_TOOL="${UART_TOOL:-$SCRIPT_DIR/mtk_uartboot}"
RAM_BL2="${RAM_BL2:-$SCRIPT_DIR/bl2ram.bin}"
UART_FIP="${UART_FIP:-$SCRIPT_DIR/fip-h5s.bin}"
WEB_BL2="${WEB_BL2:-$SCRIPT_DIR/bl2.img}"
WEB_GPT="${WEB_GPT:-$SCRIPT_DIR/gpt.bin}"
WEB_FIP="${WEB_FIP:-$SCRIPT_DIR/fip.bin}"
FIRMWARE="${FIRMWARE:-$SCRIPT_DIR/HiGoROS-H5000M-1-26-04-29-09.bin}"
BROM_BAUDRATE="${BROM_BAUDRATE:-921600}"
BL2_BAUDRATE="${BL2_BAUDRATE:-1500000}"
WAIT_DEVICE_SECONDS="${WAIT_DEVICE_SECONDS:-120}"
HTTP_TIMEOUT_SECONDS="${HTTP_TIMEOUT_SECONDS:-60}"
UPDATE_TIMEOUT_SECONDS="${UPDATE_TIMEOUT_SECONDS:-1800}"
AFTER_UPLOAD_DELAY_SECONDS="${AFTER_UPLOAD_DELAY_SECONDS:-8}"
SKIP_UART_BOOT=0
START_AT="${START_AT:-BL2}"

log() {
  printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"
}

die() {
  log "ERROR: $*"
  exit 1
}

usage() {
  cat <<EOF
Usage: ./auto-download-mac.sh [options]

Options:
  --serial PORT        Serial port, for example /dev/tty.usbserial-0001
  --ip IP             Device IP, default: 192.168.1.1
  --skip-uart         Skip mtk_uartboot and only run Web download
  --start-at STEP     BL2, GPT, FIP, or FIRMWARE. Default: BL2
  -h, --help          Show this help

Environment variables can also override paths:
  UART_TOOL RAM_BL2 UART_FIP WEB_BL2 WEB_GPT WEB_FIP FIRMWARE
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --serial)
      [[ $# -ge 2 ]] || die "--serial requires a value"
      SERIAL_PORT="$2"
      shift 2
      ;;
    --ip)
      [[ $# -ge 2 ]] || die "--ip requires a value"
      DEVICE_IP="$2"
      shift 2
      ;;
    --skip-uart)
      SKIP_UART_BOOT=1
      shift
      ;;
    --start-at)
      [[ $# -ge 2 ]] || die "--start-at requires a value"
      START_AT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1"
      ;;
  esac
done

require_file() {
  [[ -f "$1" ]] || die "File not found: $1"
}

resolve_uart_fip() {
  if [[ -f "$UART_FIP" ]]; then
    printf '%s\n' "$UART_FIP"
    return
  fi

  if [[ -f "$SCRIPT_DIR/fip.bin" ]]; then
    printf '%s\n' "$SCRIPT_DIR/fip.bin"
    return
  fi

  die "File not found: $UART_FIP or $SCRIPT_DIR/fip.bin"
}

detect_serial_port() {
  local port
  for port in /dev/tty.usbserial* /dev/tty.usbmodem* /dev/cu.usbserial* /dev/cu.usbmodem*; do
    if [[ -e "$port" ]]; then
      printf '%s\n' "$port"
      return
    fi
  done

  die "No USB serial port found. Pass one with --serial /dev/tty.usbserial-xxxx"
}

wait_device_web() {
  local base_url="http://$DEVICE_IP/"
  local deadline=$((SECONDS + WAIT_DEVICE_SECONDS))

  while (( SECONDS < deadline )); do
    if curl --noproxy '*' -fsS --max-time 3 "$base_url" >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
  done

  die "Device web UI is not reachable: $base_url"
}

upload_image() {
  local name="$1"
  local page="$2"
  local field="$3"
  local file="$4"
  local page_url="http://$DEVICE_IP/$page"
  local upload_url="http://$DEVICE_IP/upload"
  local size
  local output
  local http_code

  require_file "$file"
  size="$(wc -c < "$file" | tr -d ' ')"

  log "START $name"
  log "Page: $page_url"
  log "File: $file ($size bytes)"
  log "Upload: $upload_url, field: $field"

  curl --noproxy '*' -fsS --max-time "$HTTP_TIMEOUT_SECONDS" "$page_url" >/dev/null

  set +e
  output="$(curl --noproxy '*' -sS --max-time "$HTTP_TIMEOUT_SECONDS" \
    -w $'\nHTTP_CODE=%{http_code}\n' \
    -F "$field=@$file" \
    "$upload_url" 2>&1)"
  local curl_exit=$?
  set -e

  printf '%s\n' "$output" | while IFS= read -r line; do
    if [[ "$line" == HTTP_CODE=* ]]; then
      log "Upload HTTP: ${line#HTTP_CODE=}"
    elif [[ -n "$line" ]]; then
      log "Reply: $line"
    fi
  done

  http_code="$(printf '%s\n' "$output" | awk -F= '/^HTTP_CODE=/{print $2}' | tail -n 1)"
  [[ "$curl_exit" -eq 0 ]] || die "$name upload failed, curl exit code: $curl_exit"
  [[ "$http_code" == "200" ]] || die "$name upload failed, HTTP: ${http_code:-unknown}"
  ! printf '%s\n' "$output" | grep -Eq '(^|[[:space:]])fail([[:space:]]|$)' || die "$name upload failed: device replied fail"

  log "UPLOAD DONE $name"
}

update_image() {
  local name="$1"
  local update_url="http://$DEVICE_IP/flashing.html"
  local result_url="http://$DEVICE_IP/result"
  local result

  log "UPDATE START $name"
  log "Update page: $update_url"
  curl --noproxy '*' -fsS --max-time "$HTTP_TIMEOUT_SECONDS" "$update_url" >/dev/null

  log "Waiting result: $result_url"
  result="$(curl --noproxy '*' -fsS --max-time "$UPDATE_TIMEOUT_SECONDS" "$result_url" | tr -d '\r\n\t ')"
  log "Update result: $result"

  [[ "$result" != "failed" ]] || die "$name update failed"
  [[ -n "$result" ]] || die "$name update returned empty result"

  log "UPDATE DONE $name"
}

run_step() {
  upload_image "$1" "$2" "$3" "$4"
  update_image "$1"
  sleep "$AFTER_UPLOAD_DELAY_SECONDS"
  wait_device_web
}

case "$START_AT" in
  BL2|GPT|FIP|FIRMWARE) ;;
  *) die "--start-at must be BL2, GPT, FIP, or FIRMWARE" ;;
esac

require_file "$UART_TOOL"
require_file "$RAM_BL2"
require_file "$WEB_BL2"
require_file "$WEB_GPT"
require_file "$WEB_FIP"
require_file "$FIRMWARE"
UART_FIP="$(resolve_uart_fip)"

log "MT7987A auto download started"

if [[ "$SKIP_UART_BOOT" -eq 0 ]]; then
  if [[ -z "$SERIAL_PORT" ]]; then
    SERIAL_PORT="$(detect_serial_port)"
  fi

  chmod +x "$UART_TOOL" 2>/dev/null || true

  log "UART boot via $SERIAL_PORT"
  "$UART_TOOL" \
    -s "$SERIAL_PORT" \
    -p "$RAM_BL2" \
    -a \
    -f "$UART_FIP" \
    --brom-load-baudrate "$BROM_BAUDRATE" \
    --bl2-load-baudrate "$BL2_BAUDRATE"
  log "UART boot finished"
fi

log "Waiting for U-Boot web UI: http://$DEVICE_IP/"
wait_device_web
log "U-Boot web UI is reachable"

started=0

run_named_step() {
  local step_name="$1"

  if [[ "$started" -eq 0 && "$step_name" != "$START_AT" ]]; then
    return
  fi
  started=1

  case "$step_name" in
    BL2)
      run_step "BL2" "bl2.html" "bl2" "$WEB_BL2"
      ;;
    GPT)
      run_step "GPT" "gpt.html" "gpt" "$WEB_GPT"
      ;;
    FIP)
      run_step "FIP" "uboot.html" "fip" "$WEB_FIP"
      ;;
    FIRMWARE)
      run_step "FIRMWARE" "" "firmware" "$FIRMWARE"
      ;;
  esac
}

run_named_step "BL2"
run_named_step "GPT"
run_named_step "FIP"
run_named_step "FIRMWARE"

log "All download steps finished"
