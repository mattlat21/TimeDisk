"""MQTT coordinator for TimeDisk status and commands."""

from __future__ import annotations

import json
import logging
from typing import Any

from homeassistant.components import mqtt
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator

from .const import TOPIC_PREFIX_FMT

_LOGGER = logging.getLogger(__name__)


class TimeDiskCoordinator(DataUpdateCoordinator[dict[str, Any]]):
    """Fetch and publish TimeDisk MQTT topics."""

    def __init__(self, hass: HomeAssistant, device_id: str) -> None:
        self.device_id = device_id
        prefix = TOPIC_PREFIX_FMT.format(device_id=device_id)
        self.topic_status = f"{prefix}/status"
        self.topic_availability = f"{prefix}/availability"
        self.topic_config_set = f"{prefix}/config/set"
        self.topic_command = f"{prefix}/command"
        super().__init__(hass, _LOGGER, name=f"TimeDisk {device_id}")
        self._available = False

    async def async_setup(self) -> None:
        """Subscribe to device topics."""
        await mqtt.async_subscribe(
            self.hass,
            self.topic_status,
            self._handle_status,
            1,
        )
        await mqtt.async_subscribe(
            self.hass,
            self.topic_availability,
            self._handle_availability,
            1,
        )

    @callback
    def _handle_availability(self, msg: mqtt.ReceiveMessage) -> None:
        self._available = msg.payload == b"online"
        self.async_update_listeners()

    @callback
    def _handle_status(self, msg: mqtt.ReceiveMessage) -> None:
        try:
            data = json.loads(msg.payload)
        except json.JSONDecodeError:
            _LOGGER.warning("Invalid status JSON from %s", msg.topic)
            return
        self.async_set_updated_data(data)
        self._available = True

    @property
    def available(self) -> bool:
        return self._available

    def runtime(self) -> dict[str, Any]:
        return self.data.get("runtime", {}) if self.data else {}

    def config(self) -> dict[str, Any]:
        return self.data.get("config", {}) if self.data else {}

    def context(self) -> dict[str, Any]:
        return self.data.get("context", {}) if self.data else {}

    async def async_publish_config(self, patch: dict[str, Any]) -> None:
        await mqtt.async_publish(
            self.hass,
            self.topic_config_set,
            json.dumps(patch),
            qos=1,
        )

    async def async_send_command(self, payload: dict[str, Any]) -> None:
        await mqtt.async_publish(
            self.hass,
            self.topic_command,
            json.dumps(payload),
            qos=1,
        )
