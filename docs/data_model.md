# TimeDisk data model

Canonical schema for persisted settings and runtime state. Firmware: `firmware/esp32-p4/components/app_config` (`app_nvs.c`, namespace `timedisk_cfg`).

**Related docs:** [mode_flow.md](mode_flow.md) · [adult_authentication.md](adult_authentication.md) · [data_flow.md](data_flow.md) · [screen_flow.md](screen_flow.md)

---

## Overview

| Layer | Storage | Lifetime | Purpose |
| ----- | ------- | -------- | ------- |
| **Config** | NVS (`timedisk_cfg` namespace) | Survives reboot | WiFi, NTP, timeouts, theme, schedules, AA, timer defaults |
| **Runtime** | RAM | Current session | Active mode, countdowns, UI session state |
| **UI assets** | SPIFFS (`storage` partition) | Survives reboot | Large RGB565 images (splash, TOD backgrounds, menu buttons) as LZ4-compressed LVGL `.bin` files |
| **UI assets (embedded)** | App flash | Shipped with firmware | Small A8 wedge silhouettes and icon overlays |

[`partitions.csv`](../firmware/esp32-p4/partitions.csv) provides ~24 KB NVS, **15 MB SPIFFS** (`storage`), and 8 MB × 2 OTA app slots on 32 MB flash. SPIFFS is mounted at `/spiffs` at boot; LVGL loads images via `S:/<name>.bin`. Wedge/icon assets remain embedded in the app binary.

**Asset updates:** changing SPIFFS images requires `idf.py build flash` (or `idf.py spiffs-flash`). App OTA alone does not update the `storage` partition. Partition table changes require a full USB reflash.

All fields below are intended for **NVS** unless noted.

---

## Network

| Field | Type | Max length | Default | Notes |
| ----- | ---- | ---------- | ------- | ----- |
| `wifi_network_count` | uint8 | — | `0` | Number of saved networks (max 5) |
| `wifi_networks[]` | array | 5 entries | *(empty)* | Ordered list; firmware tries each in order on boot |
| `wifi_networks[].ssid` | string | 32 | *(unset)* | Network name |
| `wifi_networks[].password` | string | 64 | *(unset)* | WPA passphrase; may be empty string |
| `wifi_networks[].password_set` | bool | — | `false` | `false` until user completes password wizard or Settings saves password for that network |
| `ntp_server` | string | 128 | `pool.ntp.org` | Hostname or IP for SNTP |

NVS schema v2 stores networks as a single `wifi_list` blob. Devices on schema v1 are migrated automatically (legacy `wifi_ssid` / `wifi_pass` → first list entry).

### Setup access point (fallback)

If no saved network connects during boot, the device starts an **open** soft-AP:

| Setting | Value |
| ------- | ----- |
| SSID | `TimeDisk-Setup` |
| IP | `192.168.4.1` |
| Web UI | `http://192.168.4.1/` (no authentication) |

The web UI lists saved networks (add, edit, delete) and a **Connect** button to retry STA + SNTP. The HTTP server stays running on the LAN after Wi‑Fi connects (`http://<sta-ip>/` from any device on the same network). During setup AP fallback it is also reachable at `http://192.168.4.1/` while joined to **TimeDisk-Setup**.

---

| Field | Unset (null / missing in NVS) | Empty string `""` |
| ----- | ------------------------------ | ------------------- |
| `wifi_network_count == 0` | Show [startup_wizard_ssid](screen_flow.md) on boot | — |
| `wifi_networks[0].password_set == false` | Show [startup_wizard_password](screen_flow.md) on boot (first network only) | Valid — open network or no passphrase |

**Blank SSID:** invalid; entry is dropped on load. **Settings → Networking** supports add, edit, and delete for up to five networks.

### Startup wizard (boot only)

After splash, before Loading — see [screen_flow.md](screen_flow.md) Boot subgraph:

0. **`startup_wizard_theme`** — if `theme_set` is false; user picks primary and secondary from preset swatches; **Next** saves `ui_primary_color`, `ui_secondary_color`, sets `theme_set = true`.
1. **`startup_wizard_ssid`** — if no networks are saved; **Next** saves non-blank SSID as the first list entry.
2. **`startup_wizard_password`** — if the first network’s `password_set` is false; **Next** saves to NVS (password may be left blank → store `""`).

Theme wizard runs before WiFi SSID. SSID wizard runs before password wizard. Settings can change these later from **Settings → Networking** (submenu: Wi‑Fi Name, Wi‑Fi Password, NTP) without using the startup screens.

---

## Timezone

Wall-clock display uses UTC from SNTP plus a configured local timezone. Catalog: [timezone_catalog.md](timezone_catalog.md).

| Field | Type | Max length | Default (unset) | Notes |
| ----- | ---- | ---------- | --------------- | ----- |
| `timezone_set` | bool | — | `false` | `false` = null / never configured; show [startup_wizard_timezone](screen_flow.md) |
| `timezone_id` | string | 48 | *(empty)* | IANA ID from the location dropdown, e.g. `Europe/London` |

### Unset vs configured

| State | NVS | Boot behaviour |
| ----- | --- | -------------- |
| **Unset (null)** | `timezone_set == false` | After Loading and `time_valid`, show timezone startup wizard |
| **Configured** | `timezone_set == true` and non-empty `timezone_id` | Apply timezone on boot; skip wizard |

On **Next**, firmware writes `timezone_id`, sets `timezone_set = true`, applies TZ to libc, and saves NVS. An empty `timezone_id` alone does **not** count as configured.

The **country** dropdown is UI-only; only `timezone_id` is persisted.

### Startup wizard (boot only)

After [Loading](screen_flow.md) when `time_valid == true` — see [screen_flow.md](screen_flow.md) Boot subgraph:

3. **`startup_wizard_timezone`** — if `timezone_set` is false; user picks **Country** then **Location**; preview clock updates in real time; **Next** saves `timezone_id` and continues to Time of Day.

Skipped when timezone was previously set. Erasing NVS clears `timezone_set` and shows the wizard again after the next successful time sync.

---

## Timeouts

All values are **`uint32`**, unit **seconds**, stored in NVS.

| Field | Default | Description |
| ----- | ------- | ----------- |
| `timeout_splash_sec` | `3` | How long the splash screen is shown after power up |
| `timeout_tod_dim_sec` | `600` | Idle time on Time of Day (bright) with no user input before the display dims (5 s fade) |
| `timeout_tod_menu_sec` | `120` | Idle time on Time of Day (bright) before the Menu wedge is hidden |
| `timeout_aa_sec` | `60` | Idle time on any Adult Authentication screen before returning to the previous screen |
| `timeout_main_menu_sec` | `60` | Idle time on Main Menu with no input before returning to Time of Day |
| `timeout_timer_dim_sec` | `900` | Idle time on Timer (bright) with no user input before dimming |

---

## Theme

| Field | Type | Format | Notes |
| ----- | ---- | ------ | ----- |
| `theme_set` | bool | — | `false` = never completed theme startup wizard; show [startup_wizard_theme](screen_flow.md) |
| `ui_primary_color` | `uint32` | RGB888 `0xRRGGBB` | Main UI accent (maps to screen ring); default `#7A24BC` (`0x7A24BC`) |
| `ui_secondary_color` | `uint32` | RGB888 `0xRRGGBB` | Supporting accent; default `#6BCA24` (`0x6BCA24`) |

### Unset vs configured

| State | NVS | Boot behaviour |
| ----- | --- | -------------- |
| **Unset** | `theme_set == false` | After splash, show theme startup wizard (before WiFi) |
| **Configured** | `theme_set == true` | Skip theme wizard; apply colours via `ui_theme_init()` |

On **Next**, firmware writes both colour fields, sets `theme_set = true`, and saves NVS. Completing the wizard with factory-default hex values still counts as configured.

The UI layer reads these when building LVGL styles (`ui_theme.c`). Both colours are persisted in NVS and can be changed from **Settings → Colours** (`ui_screen_settings.c`); Save calls `app_nvs_save_theme()` and `ui_theme_init()`.

Wi‑Fi and NTP strings are edited from **Settings → Networking** in `ui_screen_settings.c` (per-field Save → `app_config_save_network()`).

---

## Standalone timer

Separate from [time-of-day mode cycling](mode_flow.md). Started from Main Menu → Start Timer.

| Field | Type | Unit | Notes |
| ----- | ---- | ---- | ----- |
| `timer_duration_sec` | `uint32` | seconds | Last user-chosen duration; stored in NVS and pre-filled next time; default `300` (5 min). Active countdown uses `active_timer_remaining_sec` in RAM. |
| `timer_style_id` | `uint8` | — | Index into the timer style catalog below |

During development the timer screen shows a **countdown**; visual design will be refined later. After `timeout_timer_dim_sec` with no input on Timer (bright), the display dims to Timer (dim); tap restores bright (see [screen_flow.md](screen_flow.md)).

### Timer style catalog

| ID | Name | Description |
| -- | ---- | ----------- |
| 0 | `default` | Placeholder circular countdown (initial implementation) |
| 1 | `ring` | Thick progress ring *(TBD)* |
| 2 | `minimal` | Large numeric only *(TBD)* |

Additional styles may use SPIFFS assets in a later implementation. `timer_style_id` must be `<` number of defined styles.

---

## Schedule (time-of-day modes)

Durations for automatic mode cycling. All fields are **`uint32`**, unit **seconds**.

| Field | Description |
| ----- | ----------- |
| `wind_down_sec` | Length of **Wind Down** mode |
| `sleep_sec` | Length of **Sleep** mode |
| `rest_sec` | Length of **Rest** mode |

**Zero means skip:** if a mode’s duration is `0`, that mode is not entered during the cycle. See [mode_flow.md](mode_flow.md).

NVS and the mode engine store **net** segment durations (the time each mode actually runs). Main Menu wizards collect **gross** durations in the UI and convert on save — see below.

### Sleep wizard (Main Menu → Sleep)

User sets sleep (required, > 0), rest (optional, ≥ 0), and wind down. On completion, net values are written to NVS and the device returns to **Wake** mode, then the automatic cycle begins.

| Wizard step | Gross value | End-time subtitle |
| ----------- | ----------- | ----------------- |
| Wake up Time | `gross_sleep` | now + `gross_sleep` |
| Rest End Time | `gross_rest` | now + `gross_sleep` + `gross_rest` |
| Wind Down | `gross_wd` | now + `gross_wd` |

**Validation:** `gross_sleep > 0`; `gross_rest ≥ 0`; `gross_wd < gross_sleep` (strict; `0` skips Wind Down).

**Save transform (net NVS):**

- `wind_down_sec = gross_wd`
- `sleep_sec = gross_sleep - gross_wd` (wind down eats into sleep only, not rest)
- `rest_sec = gross_rest`

**Reload (first wizard step):** `gross_sleep = sleep_sec + wind_down_sec`, `gross_rest = rest_sec`, `gross_wd = wind_down_sec`.

### Rest wizard (Main Menu → Rest)

User sets rest (required, > 0) and wind down. On completion:

- `wind_down_sec = gross_wd`
- `rest_sec = gross_rest - gross_wd` (wind down eats into rest)
- **`sleep_sec` is forced to `0`** (Sleep is always skipped for Rest schedules)

| Wizard step | Gross value | End-time subtitle |
| ----------- | ----------- | ----------------- |
| Rest End Time | `gross_rest` | now + `gross_rest` |
| Wind Down | `gross_wd` | now + `gross_wd` |

**Validation:** `gross_rest > 0`; `gross_wd < gross_rest` (strict; `0` skips Wind Down).

**Reload (first wizard step):** `gross_rest = rest_sec + wind_down_sec`, `gross_wd = wind_down_sec`.

Wind-down subtraction keeps the rest-end clock time consistent: e.g. rest gross 5 min and wind down 1 min → Wind Down 1 min then Rest 4 min (ends at the same wall time as the 5 min gross rest step showed).

### Settings → Schedule

Edits **net** `wind_down_sec`, `sleep_sec`, and `rest_sec` directly (no gross/end-time wizard semantics).

### Screen flow label mapping

Wizard screen names in [screen_flow.md](screen_flow.md). Logical field names:

| Screen flow label (today) | Wizard gross field | NVS field(s) |
| ------------------------- | ------------------ | ------------ |
| Sleep: Set Wake up Time | `gross_sleep` | `sleep_sec` (net) |
| Sleep: Set Rest End Time | `gross_rest` | `rest_sec` |
| Sleep: Set Wind Down Time | `gross_wd` | `wind_down_sec`; subtracted from `sleep_sec` on save |
| Rest: Set Rest End Time | `gross_rest` | `rest_sec` (net) |
| Rest: Set Wind Down Time | `gross_wd` | `wind_down_sec`; subtracted from `rest_sec` on save |

Wizard **order** in the screen-flow diagram may be reconciled later; NVS always uses net `wind_down_sec`, `sleep_sec`, `rest_sec`.

---

## Adult authentication

| Field | Type | Values | Notes |
| ----- | ---- | ------ | ----- |
| `aa_methods` | `uint8` | Bitmask | Each bit enables one method; see below |
| `aa_pin` | string | 4 | 4-digit PIN, plain decimal digits; default `"0000"` |

### `aa_methods` bit flags

| Bit | Mask | Name | When set |
| --- | ---- | ---- | -------- |
| 0 | `0x01` | `AA_METHOD_PIN` | PIN screen required |
| 1 | `0x02` | `AA_METHOD_MATHS` | Maths screen required |

Bits 2–7 reserved (`0xFC`). Test with `(aa_methods & mask) != 0`.

| `aa_methods` | Enabled | Behaviour |
| ------------ | ------- | --------- |
| `0x00` | — | **Auto-pass** (no adult security) |
| `0x01` | PIN | Correct PIN required |
| `0x02` | Maths | Correct answer required |
| `0x03` | PIN + Maths | PIN first, then maths; both must pass |

Full behaviour: [adult_authentication.md](adult_authentication.md).

---

## Master settings table

| Name | Type | Unit | NVS | Default | Used by |
| ---- | ---- | ---- | --- | ------- | ------- |
| `wifi_network_count` | uint8 | — | yes | `0` | Boot, WiFi |
| `wifi_networks[]` | array | — | yes | empty | Boot, WiFi (try in order) |
| `ntp_server` | string | — | yes | `pool.ntp.org` | Boot, SNTP |
| `timezone_set` | bool | — | yes | `false` | Boot, local time |
| `timezone_id` | string | — | yes | empty | Boot, local time |
| `timeout_splash_sec` | uint32 | s | yes | `3` | Boot UI |
| `timeout_tod_dim_sec` | uint32 | s | yes | `600` | Time of Day dim |
| `timeout_tod_menu_sec` | uint32 | s | yes | `120` | Time of Day menu wedge |
| `timeout_aa_sec` | uint32 | s | yes | `60` | Adult Authentication |
| `timeout_main_menu_sec` | uint32 | s | yes | `60` | Main Menu |
| `timeout_timer_dim_sec` | uint32 | s | yes | `900` | Standalone timer |
| `theme_set` | bool | — | yes | `false` | Boot theme wizard |
| `ui_primary_color` | uint32 | RGB888 | yes | `0x7A24BC` | All UI |
| `ui_secondary_color` | uint32 | RGB888 | yes | `0x6BCA24` | All UI |
| `timer_duration_sec` | uint32 | s | yes | `300` | Standalone timer |
| `timer_style_id` | uint8 | — | yes | `0` | Standalone timer |
| `wind_down_sec` | uint32 | s | yes | `0` | Mode engine |
| `sleep_sec` | uint32 | s | yes | `0` | Mode engine |
| `rest_sec` | uint32 | s | yes | `0` | Mode engine |
| `aa_methods` | uint8 | bitmask | yes | `0x00` | Adult Authentication |
| `aa_pin` | string | — | yes | `"0000"` | Adult Authentication |

---

## Runtime state (RAM)

Not persisted unless noted. Reset on reboot; mode schedule reloads from NVS.

| Name | Type | Description |
| ---- | ---- | ----------- |
| `current_mode` | enum | `Wake`, `WindDown`, `Sleep`, `Rest` |
| `mode_remaining_sec` | uint32 | Countdown for the active mode segment |
| `cycle_active` | bool | `true` after Sleep/Rest wizard completes until cycle returns to Wake and idle |
| `time_valid` | bool | System time obtained from NTP |
| `display_brightness` | uint8 | Current backlight level (bright vs dim) |
| `active_timer_remaining_sec` | uint32 | Standalone timer countdown; `0` when no timer running |
| `timer_running` | bool | Standalone timer active |
| `aa_session` | struct | See [adult_authentication.md](adult_authentication.md) |

---

## Out of scope (this document)

- C struct definitions and NVS key strings
- WiFi / SNTP implementation
- Timer style asset format
