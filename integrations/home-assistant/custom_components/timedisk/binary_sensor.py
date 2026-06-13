"""TimeDisk binary sensor entities."""

from __future__ import annotations

from homeassistant.components.binary_sensor import BinarySensorEntity
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
            TimeDiskBinarySensor(coordinator, entry, "cycle_active", "Cycle active"),
            TimeDiskBinarySensor(coordinator, entry, "timer_running", "Timer running"),
            TimeDiskBinarySensor(coordinator, entry, "time_valid", "Time valid"),
            TimeDiskBinarySensor(coordinator, entry, "mqtt_connected", "MQTT connected", True),
        ]
    )


class TimeDiskBinarySensor(CoordinatorEntity, BinarySensorEntity):
    """Runtime boolean from status JSON."""

    _attr_has_entity_name = True

    def __init__(
        self,
        coordinator: TimeDiskCoordinator,
        entry: ConfigEntry,
        key: str,
        name: str,
        context: bool = False,
    ) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._key = key
        self._context = context
        self._attr_unique_id = f"{device_id}_{key}"
        self._attr_name = name
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
        if self._context:
            return bool(self.coordinator.context().get(self._key))
        return bool(self.coordinator.runtime().get(self._key))
