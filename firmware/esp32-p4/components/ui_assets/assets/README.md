# UI assets (source folders)

Each asset lives in its own subfolder. The embed script reads **only** `embed.txt` in each folder — no asset-specific rules in the script.

## Folder layout

```
assets/
  wedge_shape_left/
    wedge_shape_left.af       # optional Affinity design source
    wedge_shape_left.svg      # export used for embedding
    embed.txt                 # conversion spec
  splash/
    splash.png
    embed.txt
  ...
```

**Embedded** assets (small A8 / icon overlays): `../src/<name>.c`  
**SPIFFS** assets (large RGB565): `../../spiffs_image/<name>.bin` (LZ4/RLE compressed)  
Generated declarations: `../include/ui_assets.h`  
Generated build list: `../CMakeLists.txt` (between `# BEGIN GENERATED ASSETS` / `# END GENERATED ASSETS`)

## After editing an asset

From the **firmware project** (`firmware/esp32-p4/`) or repo root:

```bash
./scripts/embed_ui_assets.sh
idf.py build
```

Requires `rsvg-convert` (`brew install librsvg`), `sips` (macOS), and a local venv at `firmware/esp32-p4/.venv-assets` for `LVGLImage.py` (vendored at `firmware/esp32-p4/scripts/LVGLImage.py`; falls back to `managed_components/lvgl__lvgl/scripts/LVGLImage.py` after `idf.py reconfigure`).

SPIFFS images are flashed with the firmware (`idf.py flash`). Partition or asset changes require a full reflash — not updated by app OTA alone.

## embed.txt format

Simple `key=value` lines. Lines starting with `#` are comments.

| Key | Required | Meaning |
|-----|----------|---------|
| `source` | yes | Export filename in this folder (`.svg` or `.png`) |
| `format` | yes | LVGL color format: `A8`, `RGB565`, or `RGB565A8` |
| `width` | no | Raster width in px (must pair with `height`) |
| `height` | no | Raster height in px (must pair with `width`) |
| `name` | no | Output basename; defaults to folder name |
| `description` | no | Comment in generated `ui_assets.h` |
| `storage` | no | `embedded` (default) or `spiffs` |
| `compress` | no | `auto` (default for SPIFFS), `NONE`, `RLE`, or `LZ4` |

**Rasterization (driven by spec + source type):**

- SVG + `width`/`height` → `rsvg-convert -w W -h H`
- SVG, no dimensions → native SVG size
- PNG + `width`/`height` → resize via `sips`
- PNG, no dimensions → embed as-is

**Storage split:**

- `storage=spiffs` — large RGB565 (splash, TOD backgrounds, menu buttons); compressed `.bin` on `storage` partition
- `storage=embedded` (default) — small wedges and icons linked into the app

## Adding a new asset

1. Create `assets/my_asset/`
2. Add design source (e.g. `my_asset.af`), export (`my_asset.svg` or `.png`), and `embed.txt`
3. Run `./scripts/embed_ui_assets.sh`
4. Reference from UI code:
   - Embedded: `lv_image_set_src(img, &my_asset)`
   - SPIFFS: `lv_image_set_src(img, ui_assets_spiffs_path("my_asset"))`

## Wireframes

Full-screen layouts live in `docs/wireframes/`. Individual wedge assets are maintained in their own folders here.
