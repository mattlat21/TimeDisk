#!/usr/bin/env bash
exec "$(cd "$(dirname "$0")/.." && pwd)/firmware/esp32-p4/scripts/embed_ui_assets.sh" "$@"
