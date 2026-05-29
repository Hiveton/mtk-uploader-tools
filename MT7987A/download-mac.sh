#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

MODEL=""
SERIAL=""
IP=""
START_AT=""
WAIT_DEVICE_SECONDS=""
SKIP_UART=0
DRY_RUN=0

models=(H87Pro H87AM H5MIFI H5000M H5000W E87N)

usage() {
  cat <<EOF
Usage: ./download-mac.sh [options]

Options:
  --model MODEL       H87Pro, H87AM, H5MIFI, H5000M, H5000W, or E87N
  --serial PORT       Serial port, for example /dev/tty.usbserial-0001
  --ip IP             Device IP, default is handled by board script
  --skip-uart         Skip mtk_uartboot and only run Web download
  --start-at STEP     BL2, GPT, FIP, or FIRMWARE
  --wait-device SEC   Override device web UI wait timeout
  --dry-run           Print target command without running it
  -h, --help          Show this help
EOF
}

die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

resolve_model() {
  local value="$1"
  local model

  if [[ -z "$value" ]]; then
    printf 'MT7987A download entry\n\n' >&2
    local i=1
    for model in "${models[@]}"; do
      printf '  %d. %s\n' "$i" "$model" >&2
      i=$((i + 1))
    done
    printf '\nSelect model: ' >&2
    read -r value
  fi

  if [[ "$value" =~ ^[1-6]$ ]]; then
    printf '%s\n' "${models[$((value - 1))]}"
    return
  fi

  for model in "${models[@]}"; do
    if [[ "${model,,}" == "${value,,}" ]]; then
      printf '%s\n' "$model"
      return
    fi
  done

  die "Unknown model: $value"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --model)
      [[ $# -ge 2 ]] || die "--model requires a value"
      MODEL="$2"
      shift 2
      ;;
    --serial)
      [[ $# -ge 2 ]] || die "--serial requires a value"
      SERIAL="$2"
      shift 2
      ;;
    --ip)
      [[ $# -ge 2 ]] || die "--ip requires a value"
      IP="$2"
      shift 2
      ;;
    --skip-uart)
      SKIP_UART=1
      shift
      ;;
    --start-at)
      [[ $# -ge 2 ]] || die "--start-at requires a value"
      START_AT="$2"
      shift 2
      ;;
    --wait-device)
      [[ $# -ge 2 ]] || die "--wait-device requires a value"
      WAIT_DEVICE_SECONDS="$2"
      shift 2
      ;;
    --dry-run)
      DRY_RUN=1
      shift
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

MODEL="$(resolve_model "$MODEL")"
TARGET_DIR="$SCRIPT_DIR/$MODEL/mac"
TARGET_SCRIPT="$TARGET_DIR/auto-download-mac.sh"

[[ -f "$TARGET_SCRIPT" ]] || die "Missing target script: $TARGET_SCRIPT"

cmd=("$TARGET_SCRIPT")
[[ -z "$SERIAL" ]] || cmd+=(--serial "$SERIAL")
[[ -z "$IP" ]] || cmd+=(--ip "$IP")
[[ -z "$START_AT" ]] || cmd+=(--start-at "$START_AT")
[[ "$SKIP_UART" -eq 0 ]] || cmd+=(--skip-uart)

printf 'Model: %s\n' "$MODEL"
printf 'Directory: %s\n' "$TARGET_DIR"
printf 'Script: %s\n' "$TARGET_SCRIPT"

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf 'Dry-run command:'
  if [[ -n "$WAIT_DEVICE_SECONDS" ]]; then
    printf ' WAIT_DEVICE_SECONDS=%q' "$WAIT_DEVICE_SECONDS"
  fi
  printf ' %q' "${cmd[@]}"
  printf '\n'
  exit 0
fi

cd "$TARGET_DIR"
if [[ -n "$WAIT_DEVICE_SECONDS" ]]; then
  export WAIT_DEVICE_SECONDS
fi
exec "${cmd[@]}"
