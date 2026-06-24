#!/usr/bin/env bash
# Copy vendored patches into managed_components/ after idf.py reconfigure.
set -euo pipefail

FIRMWARE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PATCH_SRC="${FIRMWARE_ROOT}/components/esp_hosted_patches/port_esp_hosted_host_init.c"
DEST="${FIRMWARE_ROOT}/managed_components/espressif__esp_hosted/host/port/esp/freertos/src/port_esp_hosted_host_init.c"

die() { echo "apply_managed_patches: $*" >&2; exit 1; }

[[ -f "${PATCH_SRC}" ]] || die "patch file missing: ${PATCH_SRC}"

if [[ ! -f "${DEST}" ]]; then
  die "esp_hosted not fetched — run: idf.py reconfigure"
fi

cp -f "${PATCH_SRC}" "${DEST}"
echo "apply_managed_patches: applied esp_hosted host-init defer patch"
