"""TimeDisk number entities for schedule and timeout fields."""

from __future__ import annotations

from homeassistant.components.number import NumberEntity, NumberMode
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN
from .coordinator import TimeDiskCoordinator

NUMBERS = [
    ("timeout_splash_sec", "Splash timeout", 0, 3600, 1),
    ("timeout_tod_dim_sec", "TOD dim timeout", 0, 86400, 60),
    ("timeout_tod_menu_sec", "TOD menu timeout", 0, 3600, 1),
    ("timeout_aa_sec", "Adult auth timeout", 0, 3600, 1),
    ("timeout_main_menu_sec", "Menu timeout", 0, 3600, 1),
    ("timeout_timer_dim_sec", "Timer dim timeout", 0, 86400, 60),
    ("timer_duration_sec", "Timer duration", 0, 86400, 60),
    ("wind_down_sec", "Wind down duration", 0, 86400, 60),
    ("sleep_sec", "Sleep duration", 0, 86400, 60),
    ("rest_sec", "Rest duration", 0, 86400, 60),
    ("mqtt_port", "MQTT port", 1, 65535, 1),
]


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator: TimeDiskCoordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(
        TimeDiskNumber(coordinator, entry, key, name, min_v, max_v, step)
        for key, name, min_v, max_v, step in NUMBERS
    )


class TimeDiskNumber(CoordinatorEntity, NumberEntity):
    """Writable config number via config/set."""

    _attr_has_entity_name = True
    _attr_mode = NumberMode.BOX

    def __init__(
        self,
        coordinator: TimeDiskCoordinator,
        entry: ConfigEntry,
        key: str,
        name: str,
        min_value: float,
        max_value: float,
        step: float,
    ) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._config_key = key
        self._attr_unique_id = f"{device_id}_{key}"
        self._attr_name = name
        self._attr_native_min_value = min_value
        self._attr_native_max_value = max_value
        self._attr_native_step = step
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
    def native_value(self) -> float | None:
        value = self.coordinator.config().get(self._config_key)
        return float(value) if value is not None else None

    async def async_set_native_value(self, value: float) -> None:
        await self.coordinator.async_publish_config({self._config_key: int(value)})
