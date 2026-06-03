# UI assets (source files)

Files in this folder are **not** loaded by the firmware at runtime. They are design sources that get compiled into embedded C bitmaps under `../src/`.

## After editing an asset

From the **repository root**, run:

```bash
./scripts/embed_ui_assets.sh
idf.py build
```

Then flash as usual (`idf.py flash`).

The script needs `rsvg-convert` (e.g. `brew install librsvg`) and creates a local Python venv at `.venv-assets` for `LVGLImage.py` dependencies.

## Naming rules

| Filename pattern | LVGL format | Typical size |
|------------------|-------------|--------------|
| `wedge_shape_left.svg`, `wedge_shape_right.svg` | A8 (alpha mask, tinted in code) | 209×106 |
| `wedge_shape_wide.svg` | A8 | 443×106 |
| `icon_wedge_*.svg` | RGB565A8 (white icon on transparent) | 209×106 |
| `icon_wedge_menu_wide.svg` | RGB565A8 | 443×106 |
| `splash.png` or `splash.svg` | RGB565 | 720×720 |

The output `.c` file name matches the source basename (e.g. `icon_wedge_settings.svg` → `../src/icon_wedge_settings.c`).

## Adding a new asset

1. Add the `.svg` or `.png` here using one of the naming patterns above (or extend `scripts/embed_ui_assets.sh`).
2. Run `./scripts/embed_ui_assets.sh`.
3. Add `extern const lv_image_dsc_t your_name;` to `../include/ui_assets.h`.
4. Add `src/your_name.c` to `../CMakeLists.txt` `SRCS`.
5. Reference `&your_name` from UI code.

## Wireframes

Full-screen layouts live in `docs/wireframes/`. Corner wedges are derived from `docs/wireframes/startup_wizard_ssid.svg`.
