#!/usr/bin/env bash
exec "$(cd "$(dirname "$0")/.." && pwd)/firmware/esp32-p4/scripts/upload_firmware.sh" "$@"
