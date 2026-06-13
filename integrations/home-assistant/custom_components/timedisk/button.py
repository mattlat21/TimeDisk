"""TimeDisk button entities."""

from __future__ import annotations

from homeassistant.components.button import ButtonEntity
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
    buttons = [
        ("start_sleep_cycle", "Start sleep cycle", {"action": "start_sleep_cycle"}),
        ("start_rest_cycle", "Start rest cycle", {"action": "start_rest_cycle"}),
        ("end_cycle", "End cycle", {"action": "end_cycle"}),
        ("cancel_timer", "Cancel timer", {"action": "cancel_timer"}),
    ]
    async_add_entities(
        TimeDiskButton(coordinator, entry, key, name, payload)
        for key, name, payload in buttons
    )


class TimeDiskButton(CoordinatorEntity, ButtonEntity):
    """Send a command action to the device."""

    _attr_has_entity_name = True

    def __init__(
        self,
        coordinator: TimeDiskCoordinator,
        entry: ConfigEntry,
        key: str,
        name: str,
        payload: dict,
    ) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._payload = payload
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

    async def async_press(self) -> None:
        await self.coordinator.async_send_command(self._payload)
