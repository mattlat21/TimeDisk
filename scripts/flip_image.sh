#!/usr/bin/env bash
# Flip an SVG or PNG horizontally and/or vertically.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec python3 "${ROOT}/scripts/flip_image.py" "$@"
