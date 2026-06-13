"""Session debug logging for TimeDisk setup (debug mode c8c045)."""

from __future__ import annotations

import json
import time
from typing import Any

from homeassistant.core import HomeAssistant

SESSION_ID = "c8c045"
LOG_NAME = "timedisk_debug_c8c045.log"


def debug_log(
    hass: HomeAssistant,
    *,
    location: str,
    message: str,
    data: dict[str, Any] | None = None,
    hypothesis_id: str = "",
    run_id: str = "pre-fix",
) -> None:
    """Append one NDJSON debug line under the HA config directory."""
    # #region agent log
    payload = {
        "sessionId": SESSION_ID,
        "runId": run_id,
        "hypothesisId": hypothesis_id,
        "location": location,
        "message": message,
        "data": data or {},
        "timestamp": int(time.time() * 1000),
    }
    try:
        path = hass.config.path(LOG_NAME)
        with open(path, "a", encoding="utf-8") as log_file:
            log_file.write(json.dumps(payload) + "\n")
    except OSError:
        pass
    # #endregion
