"""TimeDisk text entities."""

from __future__ import annotations

from homeassistant.components.text import TextEntity, TextMode
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN
from .coordinator import TimeDiskCoordinator

TEXTS = [
    ("mqtt_host", "MQTT host", 64),
    ("mqtt_username", "MQTT username", 64),
    ("mqtt_password", "MQTT password", 64),
    ("ntp_server", "NTP server", 128),
    ("timezone_id", "Timezone", 48),
]


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator: TimeDiskCoordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(
        TimeDiskText(coordinator, entry, key, name, max_len)
        for key, name, max_len in TEXTS
    )


class TimeDiskText(CoordinatorEntity, TextEntity):
    """Writable text config field."""

    _attr_has_entity_name = True
    _attr_mode = TextMode.TEXT

    def __init__(
        self,
        coordinator: TimeDiskCoordinator,
        entry: ConfigEntry,
        key: str,
        name: str,
        max_len: int,
    ) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._config_key = key
        self._attr_unique_id = f"{device_id}_{key}"
        self._attr_name = name
        self._attr_native_max = max_len
        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, device_id)},
            name=f"TimeDisk {device_id}",
            manufacturer="TimeDisk",
            model="ESP32-P4",
        )
        if key == "mqtt_password":
            self._attr_mode = TextMode.PASSWORD

    @property
    def available(self) -> bool:
        return self.coordinator.available

    @property
    def native_value(self) -> str | None:
        if self._config_key == "mqtt_password":
            return "" if self.coordinator.config().get("mqtt_password_set") else ""
        return self.coordinator.config().get(self._config_key)

    async def async_set_value(self, value: str) -> None:
        await self.coordinator.async_publish_config({self._config_key: value})
