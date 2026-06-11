# Firmware release version — single source of truth for the whole project.
# Bump these for each release; PROJECT_VER and app_version_*() follow automatically.

set(FIRMWARE_VERSION_MAJ 0)
set(FIRMWARE_VERSION_MIN 1)
set(FIRMWARE_VERSION_REV 1)

set(FIRMWARE_VERSION "${FIRMWARE_VERSION_MAJ}.${FIRMWARE_VERSION_MIN}.${FIRMWARE_VERSION_REV}")

# ESP-IDF embeds PROJECT_VER in the app image descriptor (esp_app_desc).
set(PROJECT_VER "${FIRMWARE_VERSION}")
