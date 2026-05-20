# TimeDisk wireframes (720×720)

Low-fi **SVG** wireframes for the circular Waveshare 4″ display (`720×720`). Import into Figma, refine, then implement in LVGL.

**Related:** [screen_flow.md](../screen_flow.md) · [adult_authentication.md](../adult_authentication.md) · [data_model.md](../data_model.md) · [main/ui_screens.cpp](../../main/ui_screens.cpp)

---

## Canvas

| Property | Value |
| -------- | ----- |
| Size | 720 × 720 px |
| Shape | Circle (clip + 6 px accent ring) |
| Background | `#000000` |

---

## Import into Figma

1. **File → Import** (or drag files onto the canvas).
2. Select the imported group → **Ungroup** as needed.
3. Rename the frame to the **screen ID** (column below).
4. Build a **Prototype** flow using [screen_flow.md](../screen_flow.md).
5. Turn repeated groups (keypad, side buttons) into **Components** after the first screen.

---

## Colour reference

| Hex | Usage |
| --- | ----- |
| `#7A24BC` | Outer ring, primary accent (`ui_primary_color`) |
| `#5A189A` | Purple panels (PIN bar, equation boxes) |
| `#1D4ED8` | Keypad buttons |
| `#E87A2E` | Back / Cancel side button |
| `#3CB85C` | OK / Next / Yes side button |
| `#6BCA24` | Secondary accent (`ui_secondary_color`) — menu highlights |
| `#FFFFFF` | Text |

---

## Screen index

| File | Screen ID | Notes |
| ---- | --------- | ----- |
| [splash.svg](splash.svg) | `splash` | Boot branding |
| *(TBD)* | `startup_wizard_theme` | Title "What's your favourite colours?"; Primary/Secondary swatch rows centred ~y=292/372; preview labels; green Next wedge |
| [startup_wizard_ssid.svg](startup_wizard_ssid.svg) | `startup_wizard_ssid` | WiFi SSID entry + keypad |
| *(TBD)* | `startup_wizard_timezone` | Country + location dropdowns, live HH:MM preview, Next wedge |
| [aa_pin.svg](aa_pin.svg) | `aa_start` / `aa_end` | 4-digit PIN |
| [aa_maths.svg](aa_maths.svg) | `aa_start` / `aa_end` | Maths challenge |
| [main_menu.svg](main_menu.svg) | `menu` | Four quadrant wedges, orange Back circle in center, black cross separators |
| [timer_duration.svg](timer_duration.svg) | `timer_duration` | Set duration |
| [timer_style.svg](timer_style.svg) | `timer_style` | Pick timer style |
| [timer_bright.svg](timer_bright.svg) | `timer_bright` | Active countdown |
| [timer_dim.svg](timer_dim.svg) | `timer_dim` | Dimmed countdown |
| [timer_triggered.svg](timer_triggered.svg) | `timer_triggered` | Timer finished |
| [timer_confirm.svg](timer_confirm.svg) | `confirm` | End timer confirmation |

---

## Layout constants (matches `ui_screens.cpp` at 720 px)

| Element | Position / size |
| ------- | ---------------- |
| PIN / input bar | 340 × 56, x=190, y=72 |
| Keypad button | 72 × 72 circle, gap 14, grid centered (start x=238) |
| Keypad start Y | 268 (PIN), 300 (maths) |
| Side Back | 64 × 110, x=36, y=330 |
| Side OK | 64 × 110, x=620, y=330 |
| Maths boxes | 64 × 64, y=88, outlined answer box |

---

## Not yet wireframed

`startup_wizard_password`, `startup_wizard_timezone` (layout: title ~y=72, country dropdown ~y=200, location ~y=300, preview clock ~y=420, green Next wedge), `loading`, Time of Day modes. Theme wizard implemented in firmware (`ui_screen_startup_theme_wizard.c`).

**Settings** (`ui_screen_settings.c`): hub with six vertical menu buttons; **Cancel wedge** (bottom-left) → menu. Each sub-panel uses **Cancel** / **Save** wedges (same positions as timer duration). **Networking**: submenu (Wi‑Fi Name, Wi‑Fi Password, NTP) then per-field edit screens matching the startup Wi‑Fi wizard (keyboard, field bar, **Cancel** / **Save** wedges on the edit view; list view keeps panel **Cancel** / **Save** → hub). Schedule sub-panel: three compact duration editors (wind down, sleep, rest). Sleep/Rest menu wizards reuse the shared duration editor (`ui_duration_editor.c`) with **Cancel** / **Next** wedges and optional “ends at” subtitle.
