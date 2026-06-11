#!/usr/bin/env bash
# Shared firmware version helpers (source from other scripts).

firmware_version_read() {
  local cmake_file="${1:-}"
  if [[ -z "${cmake_file}" ]]; then
    die "firmware_version_read: cmake file path required"
  fi
  [[ -f "${cmake_file}" ]] || die "version file not found: ${cmake_file}"

  FIRMWARE_VERSION_MAJ="$(sed -n 's/^set(FIRMWARE_VERSION_MAJ \([0-9][0-9]*\))/\1/p' "${cmake_file}" | head -n1)"
  FIRMWARE_VERSION_MIN="$(sed -n 's/^set(FIRMWARE_VERSION_MIN \([0-9][0-9]*\))/\1/p' "${cmake_file}" | head -n1)"
  FIRMWARE_VERSION_REV="$(sed -n 's/^set(FIRMWARE_VERSION_REV \([0-9][0-9]*\))/\1/p' "${cmake_file}" | head -n1)"

  [[ -n "${FIRMWARE_VERSION_MAJ}" && -n "${FIRMWARE_VERSION_MIN}" && -n "${FIRMWARE_VERSION_REV}" ]] \
    || die "could not parse maj.min.rev from ${cmake_file}"

  FIRMWARE_VERSION="${FIRMWARE_VERSION_MAJ}.${FIRMWARE_VERSION_MIN}.${FIRMWARE_VERSION_REV}"
  FIRMWARE_VERSION_DIR="v${FIRMWARE_VERSION}"
}
