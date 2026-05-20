# Timezone catalog

Embedded read-only list for the timezone startup wizard. Firmware: `components/app_time` (`timezone_catalog.c`).

**Related:** [data_model.md](data_model.md#timezone) · [screen_flow.md](screen_flow.md)

---

## Structure

Each **country** has a display label and one or more **locations**:

| Field | Stored in NVS | Purpose |
| ----- | ------------- | ------- |
| Country label | no | First dropdown |
| Location label | no | Second dropdown (filtered by country) |
| `timezone_id` | yes | IANA identifier, e.g. `America/New_York` |
| POSIX `TZ` string | no | Mapped at runtime for `setenv` / `tzset` and DST |

The UI never shows POSIX strings; they are looked up from `timezone_id` when applying or previewing local time.

---

## Curated regions (initial ship set)

| Country | Example locations | `timezone_id` examples |
| ------- | ----------------- | ---------------------- |
| United Kingdom | London | `Europe/London` |
| Ireland | Dublin | `Europe/Dublin` |
| United States | Eastern, Central, Mountain, Pacific | `America/New_York`, `America/Chicago`, … |
| Canada | Eastern, Pacific | `America/Toronto`, `America/Vancouver` |
| Australia | Sydney, Melbourne, Perth | `Australia/Sydney`, `Australia/Perth` |
| New Zealand | Auckland | `Pacific/Auckland` |
| Germany | Berlin | `Europe/Berlin` |
| France | Paris | `Europe/Paris` |
| Netherlands | Amsterdam | `Europe/Amsterdam` |
| Spain | Madrid | `Europe/Madrid` |
| Italy | Rome | `Europe/Rome` |
| India | Mumbai | `Asia/Kolkata` |
| Japan | Tokyo | `Asia/Tokyo` |
| Singapore | Singapore | `Asia/Singapore` |
| United Arab Emirates | Dubai | `Asia/Dubai` |
| South Africa | Johannesburg | `Africa/Johannesburg` |
| Brazil | São Paulo | `America/Sao_Paulo` |
| Mexico | Mexico City | `America/Mexico_City` |

Add rows in `timezone_catalog.c` only; no NVS schema change required.

---

## POSIX mapping notes

- ESP-IDF newlib uses the `TZ` environment variable. Each catalog entry includes a POSIX `TZ` string (e.g. `EST5EDT,M3.2.0,M11.1.0` for US Eastern).
- `app_time_apply_timezone_id()` sets `TZ`, calls `tzset()`, and clears the libc time cache so `localtime_r` reflects the selection.
- Preview on the wizard uses the same path without writing NVS until **Next**.
