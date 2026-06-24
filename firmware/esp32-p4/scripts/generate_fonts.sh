#!/usr/bin/env bash
# Generate custom LVGL fonts from TTF sources.
set -euo pipefail

FIRMWARE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FONTS_DIR="${FIRMWARE_ROOT}/components/ui_fonts/fonts"
OUT_DIR="${FIRMWARE_ROOT}/components/ui_fonts/src"
MONTSERRAT_URL="https://github.com/JulietaUla/Montserrat/raw/master/fonts/ttf/Montserrat-Regular.ttf"

die() { echo "generate_fonts: $*" >&2; exit 1; }

command -v npx >/dev/null 2>&1 || die "npx not found (install Node.js)"

mkdir -p "${FONTS_DIR}" "${OUT_DIR}"

MONTSERRAT_TTF="${FONTS_DIR}/Montserrat-Regular.ttf"
if [[ ! -f "${MONTSERRAT_TTF}" ]]; then
  echo "Downloading Montserrat-Regular.ttf..."
  curl -fsSL -o "${MONTSERRAT_TTF}" "${MONTSERRAT_URL}"
fi

echo "Generating lv_font_montserrat_64..."
npx --yes lv_font_conv \
  --font "${MONTSERRAT_TTF}" \
  --size 64 \
  --bpp 4 \
  --format lvgl \
  --no-prefilter \
  --symbols "+:-0123456789apm" \
  --force-fast-kern-format \
  --lv-font-name lv_font_montserrat_64 \
  -o "${OUT_DIR}/lv_font_montserrat_64.c"

echo "Done."
