#!/usr/bin/env bash
# Embed UI assets from per-folder specs (assets/<name>/embed.txt).
# Embedded assets → src/*.c; SPIFFS assets → spiffs_image/*.bin (LZ4/RLE compressed).
set -euo pipefail

FIRMWARE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS_DIR="${FIRMWARE_ROOT}/components/ui_assets/assets"
OUT_DIR="${FIRMWARE_ROOT}/components/ui_assets/src"
SPIFFS_DIR="${FIRMWARE_ROOT}/spiffs_image"
HEADER="${FIRMWARE_ROOT}/components/ui_assets/include/ui_assets.h"
CMAKE="${FIRMWARE_ROOT}/components/ui_assets/CMakeLists.txt"
VENV="${FIRMWARE_ROOT}/.venv-assets"
SPIFFS_MAX_BYTES=$((14 * 1024 * 1024))

die() { echo "embed_ui_assets: $*" >&2; exit 1; }

resolve_lvgl_image() {
  local candidate
  for candidate in \
    "${FIRMWARE_ROOT}/scripts/LVGLImage.py" \
    "${FIRMWARE_ROOT}/managed_components/lvgl__lvgl/scripts/LVGLImage.py"; do
    if [[ -f "${candidate}" ]]; then
      printf '%s' "${candidate}"
      return 0
    fi
  done
  return 1
}

command -v rsvg-convert >/dev/null 2>&1 || die "rsvg-convert not found (brew install librsvg)"
command -v sips >/dev/null 2>&1 || die "sips not found (macOS required for PNG resize)"

LVGL_IMAGE="$(resolve_lvgl_image)" || die "LVGLImage.py not found — expected scripts/LVGLImage.py (vendored) or managed_components/lvgl__lvgl/scripts/LVGLImage.py (run idf.py reconfigure)"

if [[ ! -d "${VENV}" ]]; then
  python3 -m venv "${VENV}"
fi
# shellcheck source=/dev/null
source "${VENV}/bin/activate"
pip install -q pypng lz4

mkdir -p "${OUT_DIR}" "${SPIFFS_DIR}"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

embed_png_c() {
  local name="$1"
  local cf="$2"
  local png="$3"
  echo "  ${name} (${cf}, embedded)"
  python "${LVGL_IMAGE}" --ofmt C --cf "${cf}" -o "${OUT_DIR}" --name "${name}" "${png}"
}

bin_size() {
  local f="$1"
  if stat -f%z "${f}" >/dev/null 2>&1; then
    stat -f%z "${f}"
  else
    stat -c%s "${f}"
  fi
}

pick_compress() {
  local name="$1"
  local cf="$2"
  local png="$3"
  local compress_pref="$4"
  local best_method="NONE"
  local best_size=0
  local method size out

  if [[ "${compress_pref}" != "auto" ]]; then
    printf '%s' "${compress_pref}"
    return 0
  fi

  for method in NONE RLE LZ4; do
    out="${TMP}/${name}_${method}.bin"
    python "${LVGL_IMAGE}" --ofmt BIN --cf "${cf}" --compress "${method}" \
      -o "${TMP}" --name "${name}_${method}" "${png}" >/dev/null
    size="$(bin_size "${out}")"
    if [[ "${best_size}" -eq 0 || "${size}" -lt "${best_size}" ]]; then
      best_size="${size}"
      best_method="${method}"
    fi
  done
  printf '%s' "${best_method}"
}

embed_png_spiffs() {
  local name="$1"
  local cf="$2"
  local png="$3"
  local compress_pref="$4"
  local method out size

  method="$(pick_compress "${name}" "${cf}" "${png}" "${compress_pref}")"
  out="${SPIFFS_DIR}/${name}.bin"
  python "${LVGL_IMAGE}" --ofmt BIN --cf "${cf}" --compress "${method}" \
    -o "${SPIFFS_DIR}" --name "${name}" "${png}" >/dev/null
  size="$(bin_size "${out}")"
  echo "  ${name} (${cf}, spiffs, ${method}, ${size} bytes)"
  SPIFFS_TOTAL=$((SPIFFS_TOTAL + size))
}

parse_embed_txt() {
  local spec="$1"
  NAME=""
  SOURCE=""
  FORMAT=""
  WIDTH=""
  HEIGHT=""
  DESCRIPTION=""
  STORAGE="embedded"
  COMPRESS="auto"

  while IFS= read -r line || [[ -n "${line}" ]]; do
    line="${line%%#*}"
    line="${line#"${line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"
    [[ -z "${line}" ]] && continue
    [[ "${line}" != *"="* ]] && die "invalid line in ${spec}: ${line}"

    local key="${line%%=*}"
    local value="${line#*=}"
    key="${key%"${key##*[![:space:]]}"}"
    value="${value#"${value%%[![:space:]]*}"}"

    case "${key}" in
      name) NAME="${value}" ;;
      source) SOURCE="${value}" ;;
      format) FORMAT="${value}" ;;
      width) WIDTH="${value}" ;;
      height) HEIGHT="${value}" ;;
      description) DESCRIPTION="${value}" ;;
      storage) STORAGE="${value}" ;;
      compress) COMPRESS="${value}" ;;
      *) die "unknown key '${key}' in ${spec}" ;;
    esac
  done < "${spec}"
}

rasterize_asset() {
  local asset_dir="$1"
  local src_path="${asset_dir}/${SOURCE}"
  local ext="${SOURCE##*.}"
  local png="${TMP}/${NAME}.png"

  ext="$(printf '%s' "${ext}" | tr '[:upper:]' '[:lower:]')"

  case "${ext}" in
    svg)
      if [[ -n "${WIDTH}" && -n "${HEIGHT}" ]]; then
        rsvg-convert -w "${WIDTH}" -h "${HEIGHT}" "${src_path}" -o "${png}"
      else
        rsvg-convert "${src_path}" -o "${png}"
      fi
      ;;
    png)
      if [[ -n "${WIDTH}" && -n "${HEIGHT}" ]]; then
        sips -z "${HEIGHT}" "${WIDTH}" "${src_path}" --out "${png}" >/dev/null
      else
        cp "${src_path}" "${png}"
      fi
      ;;
    *)
      die "unsupported source extension '.${ext}' for ${SOURCE}"
      ;;
  esac

  printf '%s' "${png}"
}

asset_group() {
  local n="$1"
  case "${n}" in
    splash) printf 'splash' ;;
    tod_*) printf 'tod' ;;
    btn_start_*) printf 'btn_start' ;;
    wedge_shape_*) printf 'wedge_shape' ;;
    icon_wedge_*) printf 'icon_wedge' ;;
    *) printf '' ;;
  esac
}

generate_header() {
  local names_file="$1"
  local desc_file="$2"
  local prev_group=""
  local i=0

  {
    echo "/**"
    echo " * @file ui_assets.h"
    echo " * @brief UI image assets (generated by scripts/embed_ui_assets.sh)."
    echo " */"
    echo
    echo "#pragma once"
    echo
    echo "#include \"lvgl.h\""
    echo "#include \"esp_err.h\""
    echo
    echo "/** Mount SPIFFS (storage partition) and prepare LVGL S: drive paths. */"
    echo "esp_err_t ui_assets_init(void);"
    echo
    echo "/** LVGL file path for a SPIFFS asset, e.g. \"S:/tod_wake.bin\". */"
    echo "const char *ui_assets_spiffs_path(const char *name);"
    echo

    while IFS= read -r name; do
      i=$((i + 1))
      desc="$(sed -n "${i}p" "${desc_file}")"
      group="$(asset_group "${name}")"

      if [[ -n "${group}" && "${group}" != "${prev_group}" ]]; then
        case "${group}" in
          tod) echo "/** Full-screen Time of Day backgrounds (720×720 RGB565, SPIFFS). */" ;;
          btn_start) echo "/** Menu button images (RGB565, SPIFFS). */" ;;
          wedge_shape) echo "/** A8 wedge silhouettes — tint at runtime via image_recolor. */" ;;
          icon_wedge) echo "/** White icon overlays, composed on top of a wedge button. */" ;;
        esac
        prev_group="${group}"
      fi

      if [[ -n "${desc}" ]]; then
        echo "/** ${desc} */"
      fi
      echo "extern const lv_image_dsc_t ${name};"
      echo
    done < "${names_file}"
  } > "${HEADER}"
}

generate_cmake_srcs() {
  local names_file="$1"
  local tmp_cmake="${TMP}/CMakeLists.txt"
  local block_file="${TMP}/generated_srcs.txt"

  awk '
    /# BEGIN GENERATED ASSETS/ { print; in_block=1; next }
    /# END GENERATED ASSETS/ { in_block=0 }
    in_block { next }
    { print }
  ' "${CMAKE}" > "${tmp_cmake}"

  : > "${block_file}"
  while IFS= read -r name; do
    printf '        "src/%s.c"\n' "${name}" >> "${block_file}"
  done < "${names_file}"

  awk -v block_file="${block_file}" '
    /# BEGIN GENERATED ASSETS/ {
      print
      while ((getline line < block_file) > 0) print line
      close(block_file)
      skip=1
      next
    }
    /# END GENERATED ASSETS/ { skip=0; print; next }
    skip { next }
    { print }
  ' "${tmp_cmake}" > "${CMAKE}"
}

NAMES_FILE="${TMP}/asset_names.txt"
DESCS_FILE="${TMP}/asset_descs.txt"
: > "${NAMES_FILE}"
: > "${DESCS_FILE}"
SPIFFS_TOTAL=0

asset_dirs=()
while IFS= read -r dir; do
  asset_dirs+=("${dir}")
done < <(find "${ASSETS_DIR}" -mindepth 1 -maxdepth 1 -type d | sort)

[[ ${#asset_dirs[@]} -gt 0 ]] || die "no asset folders found under ${ASSETS_DIR}"

for asset_dir in "${asset_dirs[@]}"; do
  folder="$(basename "${asset_dir}")"
  spec="${asset_dir}/embed.txt"

  [[ -f "${spec}" ]] || die "missing embed.txt in ${folder}/"

  parse_embed_txt "${spec}"
  [[ -n "${NAME}" ]] || NAME="${folder}"
  [[ -n "${SOURCE}" ]] || die "missing source= in ${folder}/embed.txt"
  [[ -n "${FORMAT}" ]] || die "missing format= in ${folder}/embed.txt"
  [[ -f "${asset_dir}/${SOURCE}" ]] || die "missing source file ${folder}/${SOURCE}"

  case "${FORMAT}" in
    A8|RGB565|RGB565A8) ;;
    *) die "unknown format '${FORMAT}' in ${folder}/embed.txt" ;;
  esac

  case "${STORAGE}" in
    embedded|spiffs) ;;
    *) die "unknown storage '${STORAGE}' in ${folder}/embed.txt (use embedded or spiffs)" ;;
  esac

  case "${COMPRESS}" in
    auto|NONE|RLE|LZ4) ;;
    *) die "unknown compress '${COMPRESS}' in ${folder}/embed.txt" ;;
  esac

  if [[ ( -n "${WIDTH}" && -z "${HEIGHT}" ) || ( -z "${WIDTH}" && -n "${HEIGHT}" ) ]]; then
    die "width and height must both be set or both omitted in ${folder}/embed.txt"
  fi

  echo "${folder}/"
  png="$(rasterize_asset "${asset_dir}")"

  if [[ "${STORAGE}" == "spiffs" ]]; then
    embed_png_spiffs "${NAME}" "${FORMAT}" "${png}" "${COMPRESS}"
    rm -f "${OUT_DIR}/${NAME}.c"
  else
    embed_png_c "${NAME}" "${FORMAT}" "${png}"
    printf '%s\n' "${NAME}" >> "${NAMES_FILE}"
    printf '%s\n' "${DESCRIPTION}" >> "${DESCS_FILE}"
  fi
done

generate_header "${NAMES_FILE}" "${DESCS_FILE}"
generate_cmake_srcs "${NAMES_FILE}"

echo "SPIFFS payload: ${SPIFFS_TOTAL} bytes ($(find "${SPIFFS_DIR}" -name '*.bin' | wc -l | tr -d ' ') files)"
if [[ "${SPIFFS_TOTAL}" -gt "${SPIFFS_MAX_BYTES}" ]]; then
  die "SPIFFS payload ${SPIFFS_TOTAL} bytes exceeds limit ${SPIFFS_MAX_BYTES}"
fi

echo "Done. Embedded C files: ${OUT_DIR}"
echo "SPIFFS bins: ${SPIFFS_DIR}"
echo "Generated ${HEADER} and updated ${CMAKE}"
echo "Rebuild firmware: idf.py build"
