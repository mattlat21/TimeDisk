**TimeDisk** — circular touch UI for kids' sleep/rest scheduling on the Waveshare ESP32-P4 display.

## Repository layout

| Path | Contents |
|------|----------|
| [`firmware/esp32-p4/`](firmware/esp32-p4/) | ESP-IDF firmware (LVGL UI, NVS settings, Wi‑Fi, OTA) |
| [`docs/`](docs/) | Screen flows, data model, wireframes |
| [`binaries/`](binaries/) | Release firmware artifacts (gitignored) |
| [`scripts/`](scripts/) | Convenience wrappers → `firmware/esp32-p4/scripts/` |

## Firmware

See [`firmware/esp32-p4/README.md`](firmware/esp32-p4/README.md). Quick start:

```bash
cd firmware/esp32-p4
idf.py build flash
```

Or from the repo root:

```bash
./scripts/build_firmware.sh
```

## Documentation

- [Screen flow](docs/screen_flow.md) — full UI navigation diagram
- [Screen flow (simplified)](docs/screen_flow_simple.md) — screens only
- [Data model](docs/data_model.md) — settings, timeouts, schedules, theme
- [Mode flow](docs/mode_flow.md) — Wake / Wind Down / Sleep / Rest cycling
- [Adult authentication](docs/adult_authentication.md) — PIN, maths, and `aa_methods` bitmask
- [Data flow](docs/data_flow.md) — boot, NVS, and runtime data paths
- [Wireframes](docs/wireframes/README.md) — 720×720 SVG screens for Figma

Code is free to use and modify as you like, but not for commercial use — **CC BY-NC** (Attribution-NonCommercial).
