#!/usr/bin/env bash
# Upload the OTA firmware binary to a remote server via SFTP.
set -euo pipefail

FIRMWARE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "${FIRMWARE_ROOT}/../.." && pwd)"
CMAKE_VERSION_FILE="${FIRMWARE_ROOT}/cmake/firmware_version.cmake"
ENV_FILE="${REPO_ROOT}/.env"
PROJECT_NAME="TimeDisk"

die() { echo "upload_firmware: $*" >&2; exit 1; }

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Upload ${PROJECT_NAME}.bin (or --file) to the remote path configured in .env.

Options:
  --version DIR   Version folder under binaries/ (default: read from cmake/firmware_version.cmake)
  --file PATH     Local firmware .bin to upload (default: binaries/vX.X.X/${PROJECT_NAME}.bin)
  -h, --help      Show this help
EOF
}

load_env() {
  [[ -f "${ENV_FILE}" ]] || die "missing ${ENV_FILE} — copy .env.example and fill in SFTP settings"
  # shellcheck source=/dev/null
  set -a
  source "${ENV_FILE}"
  set +a

  : "${SFTP_HOST:?SFTP_HOST is required in .env}"
  : "${SFTP_USER:?SFTP_USER is required in .env}"
  : "${SFTP_REMOTE_DIR:?SFTP_REMOTE_DIR is required in .env}"
  : "${SFTP_REMOTE_FILENAME:=latest.bin}"
  SFTP_PORT="${SFTP_PORT:-22}"

  if [[ -z "${SFTP_KEY_PATH:-}" && -z "${SFTP_PASSWORD:-}" ]]; then
    die "set SFTP_KEY_PATH in .env (required for automated uploads on Nearly Free Speech)"
  fi
}

auth_failure_hint() {
  cat >&2 <<'EOF'
upload_firmware: authentication failed.

Nearly Free Speech does not allow automated SFTP with a stored password.
Use an SSH key added to your NFSN member keychain (Profile tab), then set
SFTP_KEY_PATH in .env and remove SFTP_PASSWORD.

Verify interactively first:
  sftp YOUR_USER@ssh.nyc1.nearlyfreespeech.net

NFSN web files upload to /home/public/... (not /files/... at the SSH root).
EOF
}

# shellcheck source=scripts/lib_firmware_version.sh
source "${FIRMWARE_ROOT}/scripts/lib_firmware_version.sh"

VERSION_DIR=""
LOCAL_FILE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      [[ $# -ge 2 ]] || die "--version requires a value"
      VERSION_DIR="$2"
      shift 2
      ;;
    --file)
      [[ $# -ge 2 ]] || die "--file requires a path"
      LOCAL_FILE="$2"
      shift 2
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

load_env

if [[ -z "${VERSION_DIR}" ]]; then
  firmware_version_read "${CMAKE_VERSION_FILE}"
  VERSION_DIR="${FIRMWARE_VERSION_DIR}"
fi

if [[ -z "${LOCAL_FILE}" ]]; then
  LOCAL_FILE="${REPO_ROOT}/binaries/${VERSION_DIR}/${PROJECT_NAME}.bin"
fi

[[ -f "${LOCAL_FILE}" ]] || die "firmware binary not found: ${LOCAL_FILE} (run scripts/build_firmware.sh first)"

BATCH_FILE="$(mktemp)"
trap 'rm -f "${BATCH_FILE}"' EXIT

REMOTE_DIR="${SFTP_REMOTE_DIR%/}"
printf 'cd %s\nput "%s" %s\n' "${REMOTE_DIR}" "${LOCAL_FILE}" "${SFTP_REMOTE_FILENAME}" > "${BATCH_FILE}"

SFTP_TARGET="${SFTP_USER}@${SFTP_HOST}"
SFTP_ARGS=(-P "${SFTP_PORT}" -b "${BATCH_FILE}" -o "BatchMode=no" -o "StrictHostKeyChecking=accept-new")

if [[ -n "${SFTP_KEY_PATH:-}" ]]; then
  [[ -f "${SFTP_KEY_PATH}" ]] || die "SFTP_KEY_PATH not found: ${SFTP_KEY_PATH}"
  echo "upload_firmware: uploading via SSH key"
  SFTP_ARGS=(-P "${SFTP_PORT}" -b "${BATCH_FILE}" -o "BatchMode=yes" \
    -o "StrictHostKeyChecking=accept-new" -i "${SFTP_KEY_PATH}")
  if ! sftp "${SFTP_ARGS[@]}" "${SFTP_TARGET}"; then
    auth_failure_hint
    exit 1
  fi
elif [[ -n "${SFTP_PASSWORD:-}" ]]; then
  command -v sshpass >/dev/null 2>&1 \
    || die "sshpass not found — install with: brew install hudochenkov/sshpass/sshpass"
  echo "upload_firmware: uploading via password auth (may fail on Nearly Free Speech automated uploads)"
  SFTP_ARGS+=(-o "PreferredAuthentications=password,keyboard-interactive" -o "PubkeyAuthentication=no")
  if ! SSHPASS="${SFTP_PASSWORD}" sshpass -e sftp "${SFTP_ARGS[@]}" "${SFTP_TARGET}"; then
    auth_failure_hint
    exit 1
  fi
else
  die "set SFTP_KEY_PATH in .env"
fi

echo "upload_firmware: uploaded ${LOCAL_FILE} -> ${REMOTE_DIR}/${SFTP_REMOTE_FILENAME}"
