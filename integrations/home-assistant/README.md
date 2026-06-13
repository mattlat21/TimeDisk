# TimeDisk Home Assistant integration

Custom integration that talks to TimeDisk firmware over the native MQTT JSON protocol ([`docs/mqtt_protocol.md`](../../docs/mqtt_protocol.md)).

## Prerequisites

1. [Home Assistant MQTT integration](https://www.home-assistant.io/integrations/mqtt/) configured and connected to the same broker as the device.
2. TimeDisk firmware with MQTT enabled (**Settings → MQTT** on device, or `mqtt_enabled` via MQTT).
3. Broker host/port/user/password saved on the device and reachable from Home Assistant.

## Install

1. Copy `integrations/home-assistant/custom_components/timedisk` into your Home Assistant `config/custom_components/` directory.
2. Restart Home Assistant.
3. **Settings → Devices & Services → Add Integration → TimeDisk**.
4. Either pick a discovered device (from retained `timedisk/+/status` / `timedisk/+/availability`, or a live `timedisk/+/announce`) or enter the device ID shown on the device MQTT settings screen (e.g. `timedisk-AABBCCDDEEFF`).

## Entities

The integration exposes sensors (mode, countdowns, IP, firmware), binary sensors (cycle/timer/time/MQTT), numbers (timeouts and schedule durations), text fields (broker, NTP, timezone), a timer style select, an MQTT enable switch, and buttons to start/end cycles and cancel the timer.

## Test checklist

- [ ] Device publishes `timedisk/<id>/availability` = `online` after connect
- [ ] HA shows mode and countdown updating from `status`
- [ ] Change a timeout number in HA → device NVS updates → `status` reflects change
- [ ] **Start sleep cycle** button → TOD shows correct mode background
- [ ] **End cycle** button → returns to wake
- [ ] **Start timer** via MQTT command `{"action":"start_timer","duration_sec":120}` → timer screen
- [ ] **Cancel timer** button stops countdown
