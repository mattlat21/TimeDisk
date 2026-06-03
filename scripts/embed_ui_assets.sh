#!/usr/bin/env bash
# Embed all images under components/ui_assets/assets into src/*.c for firmware.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS_DIR="${ROOT}/components/ui_assets/assets"
OUT_DIR="${ROOT}/components/ui_assets/src"
LVGL_IMAGE="${ROOT}/managed_components/lvgl__lvgl/scripts/LVGLImage.py"
VENV="${ROOT}/.venv-assets"

WEDGE_W=209
WEDGE_H=106
WEDGE_WIDE_W=443

die() { echo "embed_ui_assets: $*" >&2; exit 1; }

command -v rsvg-convert >/dev/null 2>&1 || die "rsvg-convert not found (brew install librsvg)"

[[ -f "${LVGL_IMAGE}" ]] || die "LVGLImage.py not found at ${LVGL_IMAGE}"

if [[ ! -d "${VENV}" ]]; then
  python3 -m venv "${VENV}"
fi
# shellcheck source=/dev/null
source "${VENV}/bin/activate"
pip install -q pypng lz4

mkdir -p "${OUT_DIR}"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

embed_png() {
  local name="$1"
  local cf="$2"
  local png="$3"
  echo "  ${name} (${cf})"
  python "${LVGL_IMAGE}" --ofmt C --cf "${cf}" -o "${OUT_DIR}" --name "${name}" "${png}"
}

for src in "${ASSETS_DIR}"/*; do
  [[ -e "${src}" ]] || continue
  base="$(basename "${src}")"
  stem="${base%.*}"
  ext="${base##*.}"

  case "${ext}" in
    svg|SVG)
      png="${TMP}/${stem}.png"
      case "${stem}" in
        wedge_shape_wide)
          rsvg-convert -w "${WEDGE_WIDE_W}" -h "${WEDGE_H}" "${src}" -o "${png}"
          embed_png "${stem}" A8 "${png}"
          ;;
        wedge_shape_*)
          rsvg-convert -w "${WEDGE_W}" -h "${WEDGE_H}" "${src}" -o "${png}"
          embed_png "${stem}" A8 "${png}"
          ;;
        icon_wedge_menu_wide)
          rsvg-convert -w "${WEDGE_WIDE_W}" -h "${WEDGE_H}" "${src}" -o "${png}"
          embed_png "${stem}" RGB565A8 "${png}"
          ;;
        icon_wedge_*)
          rsvg-convert -w "${WEDGE_W}" -h "${WEDGE_H}" "${src}" -o "${png}"
          embed_png "${stem}" RGB565A8 "${png}"
          ;;
        splash)
          rsvg-convert "${src}" -o "${png}"
          embed_png "${stem}" RGB565 "${png}"
          ;;
        *)
          die "unknown SVG '${base}' — use wedge_shape_*, icon_wedge_*, or splash.svg"
          ;;
      esac
      ;;
    png|PNG)
      case "${stem}" in
        splash)
          embed_png "${stem}" RGB565 "${src}"
          ;;
        wedge_shape_wide)
          embed_png "${stem}" A8 "${src}"
          ;;
        wedge_shape_*)
          embed_png "${stem}" A8 "${src}"
          ;;
        icon_wedge_menu_wide)
          embed_png "${stem}" RGB565A8 "${src}"
          ;;
        icon_wedge_*)
          embed_png "${stem}" RGB565A8 "${src}"
          ;;
        *)
          die "unknown PNG '${base}' — use wedge_shape_*, icon_wedge_*, or splash.png"
          ;;
      esac
      ;;
    *)
      echo "embed_ui_assets: skipping ${base} (not .svg or .png)"
      ;;
  esac
done

echo "Done. Embedded C files written to ${OUT_DIR}"
echo "Rebuild firmware: idf.py build"
