"""Constants for the TimeDisk MQTT integration."""

DOMAIN = "timedisk"

CONF_DEVICE_ID = "device_id"

TOPIC_ANNOUNCE_WILDCARD = "timedisk/+/announce"
TOPIC_STATUS_WILDCARD = "timedisk/+/status"
TOPIC_AVAILABILITY_WILDCARD = "timedisk/+/availability"
TOPIC_PREFIX_FMT = "timedisk/{device_id}"

ATTR_RUNTIME = "runtime"
ATTR_CONFIG = "config"
ATTR_CONTEXT = "context"

MODE_NAMES = {
    "wake": "Wake",
    "wind_down": "Wind Down",
    "sleep": "Sleep",
    "rest": "Rest",
}
