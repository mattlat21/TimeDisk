"""TimeDisk select entities."""

from __future__ import annotations

from homeassistant.components.select import SelectEntity
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
            TimeDiskTimerStyleSelect(coordinator, entry),
        ]
    )


class TimeDiskTimerStyleSelect(CoordinatorEntity, SelectEntity):
    """Timer style selector."""

    _attr_has_entity_name = True
    _attr_name = "Timer style"
    _attr_options = ["ring", "water"]

    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._attr_unique_id = f"{device_id}_timer_style_id"
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
    def current_option(self) -> str | None:
        style_id = self.coordinator.config().get("timer_style_id", 0)
        if 0 <= style_id < len(self._attr_options):
            return self._attr_options[style_id]
        return self._attr_options[0]

    async def async_select_option(self, option: str) -> None:
        style_id = self._attr_options.index(option)
        await self.coordinator.async_publish_config({"timer_style_id": style_id})
