# TimeDisk — ESP32-P4 firmware

ESP-IDF + LVGL application for the **Waveshare ESP32-P4 WiFi6 Touch LCD** (3.4" or 4"; set panel size in `sdkconfig` under **Board Support Package → Display**).

## Build and flash

From this directory:

```bash
idf.py build flash
```

From the repository root:

```bash
./scripts/build_firmware.sh
```

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/build_firmware.sh` | Build and collect binaries under `../../binaries/vX.X.X/` |
| `scripts/upload_firmware.sh` | OTA upload via SFTP (reads `../../.env`) |
| `scripts/embed_ui_assets.sh` | Regenerate embedded UI assets from `components/ui_assets/assets/` |

## Layout

```
firmware/esp32-p4/
  main/           Application entry
  components/     IDF components (UI, app logic, BSP extras)
  cmake/          Firmware version for builds and OTA
  scripts/        Build, asset, and upload helpers
  partitions.csv
  sdkconfig.defaults
```
