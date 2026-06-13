"""TimeDisk sensor entities."""

from __future__ import annotations

from homeassistant.components.sensor import SensorEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, MODE_NAMES
from .coordinator import TimeDiskCoordinator


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    coordinator: TimeDiskCoordinator = hass.data[DOMAIN][entry.entry_id]
    async_add_entities(
        [
            TimeDiskModeSensor(coordinator, entry),
            TimeDiskModeRemainingSensor(coordinator, entry),
            TimeDiskBrightnessSensor(coordinator, entry),
            TimeDiskTimerRemainingSensor(coordinator, entry),
            TimeDiskScreenSensor(coordinator, entry),
            TimeDiskIpSensor(coordinator, entry),
            TimeDiskFwSensor(coordinator, entry),
        ]
    )


class TimeDiskBaseSensor(CoordinatorEntity, SensorEntity):
    """Shared TimeDisk sensor."""

    _attr_has_entity_name = True

    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator)
        device_id = entry.data["device_id"]
        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, device_id)},
            name=f"TimeDisk {device_id}",
            manufacturer="TimeDisk",
            model="ESP32-P4",
        )
        self._attr_unique_id = f"{device_id}_{self.entity_description.key if hasattr(self, 'entity_description') else self._key}"

    @property
    def available(self) -> bool:
        return self.coordinator.available


class TimeDiskModeSensor(TimeDiskBaseSensor):
    _key = "mode"

    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_mode"
        self._attr_name = "Mode"

    @property
    def native_value(self) -> str | None:
        mode = self.coordinator.runtime().get("current_mode")
        return MODE_NAMES.get(mode, mode)


class TimeDiskModeRemainingSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_mode_remaining"
        self._attr_name = "Mode remaining"
        self._attr_native_unit_of_measurement = "s"

    @property
    def native_value(self) -> int | None:
        return self.coordinator.runtime().get("mode_remaining_sec")


class TimeDiskBrightnessSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_brightness"
        self._attr_name = "Brightness"
        self._attr_native_unit_of_measurement = "%"

    @property
    def native_value(self) -> int | None:
        return self.coordinator.runtime().get("display_brightness")


class TimeDiskTimerRemainingSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_timer_remaining"
        self._attr_name = "Timer remaining"
        self._attr_native_unit_of_measurement = "s"

    @property
    def native_value(self) -> int | None:
        return self.coordinator.runtime().get("active_timer_remaining_sec")


class TimeDiskScreenSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_screen"
        self._attr_name = "Active screen"

    @property
    def native_value(self) -> str | None:
        return self.coordinator.context().get("active_screen")


class TimeDiskIpSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_ip"
        self._attr_name = "IP address"

    @property
    def native_value(self) -> str | None:
        return self.coordinator.context().get("ip") or None


class TimeDiskFwSensor(TimeDiskBaseSensor):
    def __init__(self, coordinator: TimeDiskCoordinator, entry: ConfigEntry) -> None:
        super().__init__(coordinator, entry)
        self._attr_unique_id = f"{entry.data['device_id']}_fw"
        self._attr_name = "Firmware"

    @property
    def native_value(self) -> str | None:
        return self.coordinator.data.get("fw_version") if self.coordinator.data else None
