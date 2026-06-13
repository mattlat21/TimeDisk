"""TimeDisk Home Assistant integration."""

from __future__ import annotations

import logging

from homeassistant.config_entries import ConfigEntry
from homeassistant.const import Platform
from homeassistant.core import HomeAssistant

from .const import CONF_DEVICE_ID, DOMAIN
from .coordinator import TimeDiskCoordinator
from .debug_log import debug_log

_LOGGER = logging.getLogger(__name__)

PLATFORMS: list[Platform] = [
    Platform.SENSOR,
    Platform.BINARY_SENSOR,
    Platform.NUMBER,
    Platform.SELECT,
    Platform.TEXT,
    Platform.SWITCH,
    Platform.BUTTON,
]


async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Set up TimeDisk from a config entry."""
    device_id = entry.data.get(CONF_DEVICE_ID)
    debug_log(
        hass,
        location="timedisk/__init__.py:async_setup_entry:start",
        message="setup started",
        data={"device_id": device_id, "entry_id": entry.entry_id},
        hypothesis_id="H2",
    )
    try:
        coordinator = TimeDiskCoordinator(hass, entry.data[CONF_DEVICE_ID])
        debug_log(
            hass,
            location="timedisk/__init__.py:async_setup_entry:coordinator",
            message="coordinator created",
            data={"topic_status": coordinator.topic_status},
            hypothesis_id="H3",
        )
        await coordinator.async_setup()
        debug_log(
            hass,
            location="timedisk/__init__.py:async_setup_entry:subscribed",
            message="mqtt subscriptions ready",
            hypothesis_id="H3",
        )
        # Data arrives via retained status MQTT messages; no pull refresh needed.
        hass.data.setdefault(DOMAIN, {})[entry.entry_id] = coordinator
        await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
        debug_log(
            hass,
            location="timedisk/__init__.py:async_setup_entry:platforms",
            message="platform setup completed",
            data={"has_data": coordinator.data is not None},
            hypothesis_id="H1,H4",
        )
    except Exception as err:
        debug_log(
            hass,
            location="timedisk/__init__.py:async_setup_entry:error",
            message="setup failed",
            data={"error_type": type(err).__name__, "error": str(err)},
            hypothesis_id="H1,H3,H4,H5",
        )
        _LOGGER.exception("TimeDisk setup failed for %s", device_id)
        raise

    return True


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Unload TimeDisk."""
    unload_ok = await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
    if unload_ok:
        hass.data[DOMAIN].pop(entry.entry_id)
    return unload_ok
