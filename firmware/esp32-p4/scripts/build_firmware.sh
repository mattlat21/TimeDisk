#!/usr/bin/env bash
# Build TimeDisk firmware and collect binaries under binaries/vX.X.X/.
set -euo pipefail

FIRMWARE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${FIRMWARE_ROOT}/../.." && pwd)"
CMAKE_VERSION_FILE="${FIRMWARE_ROOT}/cmake/firmware_version.cmake"
BUILD_DIR="${FIRMWARE_ROOT}/build"
BINARIES_ROOT="${REPO_ROOT}/binaries"
PROJECT_NAME="TimeDisk"

die() { echo "build_firmware: $*" >&2; exit 1; }

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Run idf.py build and copy firmware artifacts into binaries/vX.X.X/.

Options:
  --upload        After a successful build, run scripts/upload_firmware.sh
  --skip-build    Only collect binaries from an existing build/ directory
  -h, --help      Show this help
EOF
}

ensure_idf() {
  if command -v idf.py >/dev/null 2>&1; then
    return 0
  fi

  local candidate
  for candidate in \
    "${IDF_PATH:-}/export.sh" \
    "${HOME}/esp/v5.4.1/esp-idf/export.sh" \
    "${HOME}/esp/v5.5.4/esp-idf/export.sh"; do
    if [[ -n "${candidate}" && -f "${candidate}" ]]; then
      # shellcheck source=/dev/null
      source "${candidate}"
      return 0
    fi
  done

  die "idf.py not found — source your ESP-IDF export.sh or set IDF_PATH"
}

copy_if_exists() {
  local src="$1"
  local dest_dir="$2"
  if [[ -f "${src}" ]]; then
    cp -f "${src}" "${dest_dir}/"
    echo "  $(basename "${src}")"
  fi
}

# shellcheck source=scripts/lib_firmware_version.sh
source "${FIRMWARE_ROOT}/scripts/lib_firmware_version.sh"

DO_UPLOAD=false
SKIP_BUILD=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --upload)
      DO_UPLOAD=true
      shift
      ;;
    --skip-build)
      SKIP_BUILD=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1 (try --help)"
      ;;
  esac
done

firmware_version_read "${CMAKE_VERSION_FILE}"
OUT_DIR="${BINARIES_ROOT}/${FIRMWARE_VERSION_DIR}"
APP_BIN="${BUILD_DIR}/${PROJECT_NAME}.bin"

ensure_idf

if [[ "${SKIP_BUILD}" == "false" ]]; then
  echo "build_firmware: building ${PROJECT_NAME} ${FIRMWARE_VERSION}"
  (cd "${FIRMWARE_ROOT}" && idf.py build)
fi

[[ -f "${APP_BIN}" ]] || die "app binary not found: ${APP_BIN} (build failed or use without --skip-build)"

mkdir -p "${OUT_DIR}"

echo "build_firmware: collecting artifacts -> ${OUT_DIR}/"
copy_if_exists "${APP_BIN}" "${OUT_DIR}"
copy_if_exists "${BUILD_DIR}/bootloader/bootloader.bin" "${OUT_DIR}"
copy_if_exists "${BUILD_DIR}/partition_table/partition-table.bin" "${OUT_DIR}"
copy_if_exists "${BUILD_DIR}/${PROJECT_NAME}.elf" "${OUT_DIR}"
copy_if_exists "${BUILD_DIR}/flasher_args.json" "${OUT_DIR}"
copy_if_exists "${BUILD_DIR}/ota_data_initial.bin" "${OUT_DIR}"

cp -f "${APP_BIN}" "${OUT_DIR}/latest.bin"
echo "  latest.bin"

{
  echo "project=${PROJECT_NAME}"
  echo "version=${FIRMWARE_VERSION}"
  echo "version_dir=${FIRMWARE_VERSION_DIR}"
  echo "built_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
} > "${OUT_DIR}/build_info.txt"
echo "  build_info.txt"

echo "build_firmware: done -> ${OUT_DIR}"

if [[ "${DO_UPLOAD}" == "true" ]]; then
  echo "build_firmware: uploading OTA binary"
  "${FIRMWARE_ROOT}/scripts/upload_firmware.sh" --version "${FIRMWARE_VERSION_DIR}" --file "${OUT_DIR}/${PROJECT_NAME}.bin"
fi
