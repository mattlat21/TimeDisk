#!/usr/bin/env python3
"""Flip an SVG or PNG horizontally and/or vertically."""

from __future__ import annotations

import argparse
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)


def die(message: str, code: int = 1) -> None:
    print(f"flip_image: {message}", file=sys.stderr)
    raise SystemExit(code)


def parse_bool(value: str) -> bool:
    normalized = value.strip().lower()
    if normalized in {"1", "true", "t", "yes", "y", "on"}:
        return True
    if normalized in {"0", "false", "f", "no", "n", "off"}:
        return False
    die(f"expected a boolean value, got {value!r}")


def prompt_bool(label: str, default: bool | None = None) -> bool:
    if default is None:
        hint = "[y/n]"
    else:
        hint = "[Y/n]" if default else "[y/N]"
    while True:
        raw = input(f"{label} {hint}: ").strip().lower()
        if not raw and default is not None:
            return default
        if raw in {"y", "yes", "true", "1"}:
            return True
        if raw in {"n", "no", "false", "0"}:
            return False
        print("Please enter y or n.")


def parse_length(value: str | None) -> float | None:
    if value is None:
        return None
    match = re.match(r"^([0-9]+(?:\.[0-9]+)?)", value.strip())
    if not match:
        return None
    return float(match.group(1))


def svg_dimensions(root: ET.Element) -> tuple[float, float, float, float]:
    viewbox = root.get("viewBox") or root.get("viewbox")
    if viewbox:
        parts = [float(part) for part in viewbox.replace(",", " ").split()]
        if len(parts) == 4:
            return parts[0], parts[1], parts[2], parts[3]

    width = parse_length(root.get("width"))
    height = parse_length(root.get("height"))
    if width is not None and height is not None:
        return 0.0, 0.0, width, height

    die("could not determine SVG size (need viewBox or width/height)")


def svg_transform(flip_h: bool, flip_v: bool, min_x: float, min_y: float, width: float, height: float) -> str:
    transforms: list[str] = []
    if flip_h:
        transforms.append(f"translate({min_x + width}, {min_y})")
        transforms.append("scale(-1, 1)")
        min_x = min_x + width
        width = -width
    if flip_v:
        effective_height = abs(height)
        base_y = min_y if height >= 0 else min_y + height
        transforms.append(f"translate({min_x}, {base_y + effective_height})")
        transforms.append("scale(1, -1)")
    return " ".join(transforms)


def flip_svg(input_path: Path, output_path: Path, flip_h: bool, flip_v: bool) -> None:
    tree = ET.parse(input_path)
    root = tree.getroot()
    if root.tag != f"{{{SVG_NS}}}svg" and root.tag != "svg":
        die(f"not an SVG root element: {input_path}")

    min_x, min_y, width, height = svg_dimensions(root)
    transform = svg_transform(flip_h, flip_v, min_x, min_y, width, abs(height))

    wrapper = ET.Element(f"{{{SVG_NS}}}g", {"transform": transform})
    children = list(root)
    for child in children:
        root.remove(child)
        wrapper.append(child)
    root.append(wrapper)

    tree.write(output_path, encoding="utf-8", xml_declaration=True)


def flip_png_sips(input_path: Path, output_path: Path, flip_h: bool, flip_v: bool) -> None:
    current = input_path
    temp_files: list[Path] = []

    try:
        if flip_h:
            target = output_path if not flip_v else Path(tempfile.mkstemp(suffix=".png")[1])
            if flip_v:
                temp_files.append(target)
            subprocess.run(
                ["sips", "-f", "horizontal", str(current), "--out", str(target)],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
            )
            current = target

        if flip_v:
            subprocess.run(
                ["sips", "-f", "vertical", str(current), "--out", str(output_path)],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
            )
        elif not flip_h:
            shutil.copy2(input_path, output_path)
    finally:
        for temp in temp_files:
            temp.unlink(missing_ok=True)


def flip_png_pillow(input_path: Path, output_path: Path, flip_h: bool, flip_v: bool) -> None:
    try:
        from PIL import Image
    except ImportError:
        die("PNG flip requires macOS sips or Pillow (`pip install pillow`)")

    image = Image.open(input_path)
    if flip_h:
        image = image.transpose(Image.FLIP_LEFT_RIGHT)
    if flip_v:
        image = image.transpose(Image.FLIP_TOP_BOTTOM)
    image.save(output_path)


def flip_png(input_path: Path, output_path: Path, flip_h: bool, flip_v: bool) -> None:
    if shutil.which("sips") and platform.system() == "Darwin":
        flip_png_sips(input_path, output_path, flip_h, flip_v)
        return
    flip_png_pillow(input_path, output_path, flip_h, flip_v)


def default_output_path(input_path: Path, flip_h: bool, flip_v: bool) -> Path:
    if flip_h and flip_v:
        suffix = "-flipped-hv"
    elif flip_h:
        suffix = "-flipped-h"
    elif flip_v:
        suffix = "-flipped-v"
    else:
        suffix = "-flipped"
    return input_path.with_name(f"{input_path.stem}{suffix}{input_path.suffix}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Flip an SVG or PNG horizontally and/or vertically.")
    parser.add_argument("-i", "--input", dest="input_path", help="Input SVG or PNG file")
    parser.add_argument("-o", "--output", dest="output_path", help="Output file path")
    parser.add_argument("--flip-horizontal", "--horizontal", dest="flip_h", action="store_true", default=None,
                        help="Flip horizontally")
    parser.add_argument("--no-flip-horizontal", dest="flip_h", action="store_false",
                        help="Do not flip horizontally")
    parser.add_argument("--flip-vertical", "--vertical", dest="flip_v", action="store_true", default=None,
                        help="Flip vertically")
    parser.add_argument("--no-flip-vertical", dest="flip_v", action="store_false",
                        help="Do not flip vertically")
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    input_raw = args.input_path
    if not input_raw:
        input_raw = input("Input file: ").strip()
    if not input_raw:
        die("input file is required")

    input_path = Path(input_raw).expanduser().resolve()
    if not input_path.is_file():
        die(f"input file not found: {input_path}")

    ext = input_path.suffix.lower()
    if ext not in {".svg", ".png"}:
        die(f"unsupported file type {ext!r} (expected .svg or .png)")

    flip_h = args.flip_h
    if flip_h is None:
        flip_h = prompt_bool("Flip horizontally?", default=False)

    flip_v = args.flip_v
    if flip_v is None:
        flip_v = prompt_bool("Flip vertically?", default=False)

    if not flip_h and not flip_v:
        die("at least one flip direction must be enabled")

    if args.output_path:
        output_path = Path(args.output_path).expanduser().resolve()
    else:
        default_out = default_output_path(input_path, flip_h, flip_v)
        output_raw = input(f"Output file [{default_out}]: ").strip()
        output_path = Path(output_raw).expanduser().resolve() if output_raw else default_out

    output_path.parent.mkdir(parents=True, exist_ok=True)

    if ext == ".svg":
        flip_svg(input_path, output_path, flip_h, flip_v)
    else:
        flip_png(input_path, output_path, flip_h, flip_v)

    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()
