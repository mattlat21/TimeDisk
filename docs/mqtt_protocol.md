# TimeDisk MQTT protocol

TimeDisk uses a native JSON protocol over MQTT. Topic prefix:

```text
timedisk/<device_id>/
```

`device_id` defaults to `timedisk-` + 12 uppercase hex digits from the STA MAC (e.g. `timedisk-A1B2C3D4E5F6`). Shown read-only on **Settings → MQTT**.

Home Assistant integration: [`integrations/home-assistant/README.md`](../integrations/home-assistant/README.md).

## Topics

| Topic | Direction | Retain | Payload |
|-------|-----------|--------|---------|
| `…/availability` | device → broker | yes | `online` / `offline` (LWT) |
| `…/status` | device → broker | yes | Full JSON snapshot |
| `…/config/set` | HA → device | no | Partial JSON config patch |
| `…/command` | HA → device | no | Action JSON |
| `…/announce` | device → broker | no | Birth JSON for HA discovery |

`status` is published on connect, every 30 s, and on runtime/config changes (debounced ~500 ms).

## `status` JSON

```json
{
  "device_id": "timedisk-AABBCCDDEEFF",
  "fw_version": "0.1.2",
  "runtime": {
    "current_mode": "wake",
    "mode_remaining_sec": 0,
    "cycle_active": false,
    "time_valid": true,
    "display_brightness": 100,
    "timer_running": false,
    "active_timer_remaining_sec": 0,
    "active_timer_total_sec": 0
  },
  "config": {
    "wifi_networks": [{"ssid": "MyWiFi", "password_set": true}],
    "wifi_network_count": 1,
    "ntp_server": "pool.ntp.org",
    "timezone_set": true,
    "timezone_id": "Australia/Melbourne",
    "timeout_splash_sec": 3,
    "wind_down_sec": 0,
    "sleep_sec": 0,
    "rest_sec": 0,
    "mqtt_enabled": true,
    "mqtt_host": "192.168.1.10",
    "mqtt_port": 1883,
    "mqtt_username": "",
    "mqtt_password_set": false
  },
  "context": {
    "active_screen": "tod_bright",
    "wifi_status": "ready",
    "ip": "192.168.1.42",
    "uptime_sec": 3600,
    "mqtt_connected": true
  }
}
```

**Security:** `status` never includes Wi-Fi passwords or `aa_pin`. Password fields are represented as `password_set` / `mqtt_password_set` booleans.

### `current_mode` values

`wake`, `wind_down`, `sleep`, `rest`

## `config/set` JSON

Partial object; only included keys are updated and saved to NVS.

```json
{
  "timeout_tod_dim_sec": 300,
  "sleep_sec": 3600,
  "mqtt_host": "10.0.0.5",
  "mqtt_port": 1883,
  "mqtt_enabled": true,
  "mqtt_username": "user",
  "mqtt_password": "secret",
  "aa_pin": "1234"
}
```

Supported keys match writable NVS fields in [`data_model.md`](data_model.md) (timeouts, theme colours, timer, schedule, MQTT, NTP, timezone, AA). Wi-Fi list updates are not supported via MQTT in v1.

## `command` JSON

```json
{ "action": "start_sleep_cycle" }
{ "action": "start_rest_cycle" }
{ "action": "end_cycle" }
{ "action": "start_timer", "duration_sec": 300, "style_id": 0 }
{ "action": "cancel_timer" }
```

| Action | Behaviour |
|--------|-----------|
| `start_sleep_cycle` | `mode_engine_start_cycle()` using current NVS schedule → TOD bright |
| `start_rest_cycle` | Sets `sleep_sec=0`, saves schedule, starts cycle → TOD bright |
| `end_cycle` | `mode_engine_switch_to_wake()` |
| `start_timer` | Starts countdown timer (`duration_sec` optional, `style_id` optional) |
| `cancel_timer` | Stops timer without adult-auth gate |

Commands are executed on the LVGL thread via `lv_async_call()`.

## `announce` JSON

Published once per MQTT connect:

```json
{
  "device_id": "timedisk-AABBCCDDEEFF",
  "name": "TimeDisk",
  "fw_version": "0.1.2"
}
```

HA integration listens on `timedisk/+/announce` during config flow.
