"""Config flow for TimeDisk."""

from __future__ import annotations

import asyncio
import json
import logging
from typing import Any

import voluptuous as vol

from homeassistant.components import mqtt
from homeassistant.config_entries import ConfigFlow, ConfigFlowResult
from homeassistant.helpers import config_validation as cv

from .const import (
    CONF_DEVICE_ID,
    DOMAIN,
    TOPIC_ANNOUNCE_WILDCARD,
    TOPIC_AVAILABILITY_WILDCARD,
    TOPIC_STATUS_WILDCARD,
)

_LOGGER = logging.getLogger(__name__)


def _device_id_from_topic(topic: str, suffix: str) -> str | None:
    """Parse device_id from ``timedisk/<device_id>/<suffix>``."""
    parts = topic.split("/")
    if len(parts) == 3 and parts[0] == "timedisk" and parts[2] == suffix:
        return parts[1]
    return None

STEP_USER_SCHEMA = vol.Schema(
    {
        vol.Required(CONF_DEVICE_ID): cv.string,
    }
)


class TimeDiskConfigFlow(ConfigFlow, domain=DOMAIN):
    """Handle TimeDisk config flow."""

    VERSION = 1

    def __init__(self) -> None:
        self._discovered: dict[str, str] = {}

    async def async_step_user(
        self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        """Manual entry or discovery selection."""
        errors: dict[str, str] = {}

        if user_input is not None:
            device_id = user_input[CONF_DEVICE_ID].strip()
            await self.async_set_unique_id(device_id)
            self._abort_if_unique_id_configured()
            return self.async_create_entry(
                title=f"TimeDisk {device_id}",
                data={CONF_DEVICE_ID: device_id},
            )

        if not mqtt.is_connected(self.hass):
            errors["base"] = "mqtt_not_connected"
            return self.async_show_form(
                step_id="user",
                data_schema=STEP_USER_SCHEMA,
                errors=errors,
            )

        discovered = await self._async_discover_devices()
        if discovered:
            return await self.async_step_discover()

        return self.async_show_form(
            step_id="user",
            data_schema=STEP_USER_SCHEMA,
            errors=errors,
        )

    async def async_step_discover(
        self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        """Pick a discovered device."""
        discovered = await self._async_discover_devices()
        if user_input is not None:
            device_id = user_input[CONF_DEVICE_ID]
            await self.async_set_unique_id(device_id)
            self._abort_if_unique_id_configured()
            return self.async_create_entry(
                title=f"TimeDisk {device_id}",
                data={CONF_DEVICE_ID: device_id},
            )

        return self.async_show_form(
            step_id="discover",
            data_schema=vol.Schema(
                {
                    vol.Required(CONF_DEVICE_ID): vol.In(discovered),
                }
            ),
        )

    async def _async_discover_devices(self) -> dict[str, str]:
        """Discover devices from retained MQTT state and live announce."""
        discovered: dict[str, str] = {}

        def register(device_id: str, name: str | None = None) -> None:
            if not device_id:
                return
            label = name or "TimeDisk"
            discovered[device_id] = f"{label} ({device_id})"

        @mqtt.callback
        def announce_received(msg: mqtt.ReceiveMessage) -> None:
            try:
                payload = json.loads(msg.payload)
            except json.JSONDecodeError:
                return
            device_id = payload.get("device_id")
            if device_id:
                register(device_id, payload.get("name"))

        @mqtt.callback
        def status_received(msg: mqtt.ReceiveMessage) -> None:
            try:
                payload = json.loads(msg.payload)
            except json.JSONDecodeError:
                return
            device_id = payload.get("device_id")
            if device_id:
                register(device_id)

        @mqtt.callback
        def availability_received(msg: mqtt.ReceiveMessage) -> None:
            if msg.payload != b"online":
                return
            device_id = _device_id_from_topic(msg.topic, "availability")
            if device_id:
                register(device_id)

        unsub_announce = await mqtt.async_subscribe(
            self.hass,
            TOPIC_ANNOUNCE_WILDCARD,
            announce_received,
            0,
        )
        unsub_status = await mqtt.async_subscribe(
            self.hass,
            TOPIC_STATUS_WILDCARD,
            status_received,
            0,
        )
        unsub_availability = await mqtt.async_subscribe(
            self.hass,
            TOPIC_AVAILABILITY_WILDCARD,
            availability_received,
            0,
        )
        try:
            # Retained status/availability arrive on subscribe; keep listening
            # briefly for a live announce if the device connects during setup.
            await asyncio.sleep(2)
        finally:
            unsub_announce()
            unsub_status()
            unsub_availability()

        return discovered
