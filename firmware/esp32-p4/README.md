# TimeDisk — ESP32-P4 firmware

ESP-IDF + LVGL application for the **Waveshare ESP32-P4 WiFi6 Touch LCD** (3.4" or 4"; set panel size in `sdkconfig` under **Board Support Package → Display**).

## Build and flash

After a fresh clone (or when `managed_components/` is missing), fetch dependencies and apply vendored patches:

```bash
idf.py reconfigure
./scripts/apply_managed_patches.sh
```

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
| `scripts/apply_managed_patches.sh` | Apply vendored patches to fetched `managed_components/` |
| `scripts/upload_firmware.sh` | OTA upload via SFTP (reads `../../.env`) |
| `scripts/embed_ui_assets.sh` | Regenerate embedded UI assets from `components/ui_assets/assets/` |

## Home Assistant

MQTT is optional. Enable it under **Settings → MQTT** on the device, then use the custom HA integration:

- Protocol: [`docs/mqtt_protocol.md`](../../docs/mqtt_protocol.md)
- Integration: [`integrations/home-assistant/README.md`](../../integrations/home-assistant/README.md)

## Layout

```
firmware/esp32-p4/
  main/           Application entry
  components/     IDF components (UI, app logic, app_mqtt, BSP extras, esp_hosted_patches)
  cmake/          Firmware version for builds and OTA
  scripts/        Build, asset, and upload helpers
  partitions.csv
  sdkconfig.defaults
```
