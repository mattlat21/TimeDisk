"""TimeDisk switch entities."""

from __future__ import annotations

from homeassistant.components.switch import SwitchEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN
from .coordinator import TimeDiskCoordinator


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator: TimeDiskCoordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(
        [
            TimeDiskMqttSwitch(coordinator, entry),
        ]
    )


class TimeDiskMqttSwitch(CoordinatorEntity, SwitchEntity):
    """Mirror mqtt_enabled from device status."""

    _attr_has_entity_name = True
    _attr_name = "MQTT enabled"

    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._attr_unique_id = f"{device_id}_mqtt_enabled"
        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, device_id)},
            name=f"TimeDisk {device_id}",
            manufacturer="TimeDisk",
            model="ESP32-P4",
        )

    @property
    def available(self) -> bool:
        return self.coordinator.available

    @property
    def is_on(self) -> bool | None:
        return bool(self.coordinator.config().get("mqtt_enabled"))

    async def async_turn_on(self, **kwargs) -> None:
        await self.coordinator.async_publish_config({"mqtt_enabled": True})

    async def async_turn_off(self, **kwargs) -> None:
        await self.coordinator.async_publish_config({"mqtt_enabled": False})
