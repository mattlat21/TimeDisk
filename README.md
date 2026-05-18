**TimeDisk** — ESP-IDF + LVGL app for the **Waveshare ESP32-P4 WiFi6 Touch LCD** (3.4" or 4"; set panel size in `sdkconfig` under **Board Support Package → Display**).

Circular touch UI: enter a 4-digit PIN, then solve a simple arithmetic challenge on the keypad.

Build and flash:

```bash
idf.py build flash
```

## Documentation

- [Screen flow](docs/screen_flow.md) — full UI navigation diagram
- [Screen flow (simplified)](docs/screen_flow_simple.md) — screens only
- [Data model](docs/data_model.md) — settings, timeouts, schedules, theme
- [Mode flow](docs/mode_flow.md) — Wake / Wind Down / Sleep / Rest cycling
- [Adult authentication](docs/adult_authentication.md) — PIN, maths, and `aa_methods` bitmask
- [Data flow](docs/data_flow.md) — boot, NVS, and runtime data paths
- [Wireframes](docs/wireframes/README.md) — 720×720 SVG screens for Figma (splash, menu, AA, timer, …)

Code is free to use and modify as you like, but not for commercial use — **CC BY-NC** (Attribution-NonCommercial).
