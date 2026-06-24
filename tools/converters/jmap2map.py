#!/usr/bin/env python3
"""
jmap2map.py - Convert a .jmap collision map to a C header file.

As a module (via build_asset.py):
    Dispatched automatically for .jmap files.

Standalone usage:
    python3 jmap2map.py <input.jmap> <output.h>

See tile_map.json for mapping values.
"""

import json
import os
import re
import argparse
import subprocess
import shutil
import sys
from pathlib import Path


def _load_tile_map():
    """Load TILE_MAP from tile_map.json, searching relative to this script."""
    script_dir = Path(__file__).resolve().parent
    # Check next to this script, then one level up (for converters/ subdir layout)
    candidates = [
        script_dir / "tile_map.json",
        script_dir / "../tile_map.json",
    ]
    for json_path in candidates:
        if json_path.exists():
            with json_path.open("r") as f:
                data = json.load(f)

            raw_map = data.get("TILE_MAP", {})
            clean_map = {}

            # Flatten the dictionary: filter out group strings and merge legacy aliases
            for key, val in raw_map.items():
                if key.startswith("_group_"):
                    continue
                if key == "_legacy_aliases":
                    for leg_k, leg_v in val.items():
                        clean_map[leg_k] = leg_v
                    continue

                clean_map[key] = val

            return clean_map

    raise FileNotFoundError(
        f"Could not find tile_map.json. Searched: {[str(p) for p in candidates]}"
    )


def parse_jmap(path_obj, tile_map):
    """
    Returns (rows, cam_zones).
    rows      = list of list of ints (tile map).
    cam_zones = list of dicts, each with keys:
        id, x1, z1, x2, z2, cam_wx, cam_wz, height, smoothing,
        corners: list of 4 (tx, tz) tuples [TL, TR, BR, BL].
        If corner data is absent from the file, corners default to the bounding rect.
    """
    rows = []
    cam_zones = []
    in_cam_zones = False

    with path_obj.open("r") as f:
        for raw in f:
            t = raw.strip()

            # Section headers
            if t == "[CAMERA_ZONES]":
                in_cam_zones = True
                continue
            if t.startswith("[") and t.endswith("]"):
                in_cam_zones = False
                continue

            if in_cam_zones:
                t = t.split("#")[0].strip()
                if not t:
                    continue
                parts = [p.strip() for p in t.split(",")]
                if len(parts) >= 5:
                    try:
                        zid  = int(parts[0])
                        x1   = int(parts[1])
                        z1   = int(parts[2])
                        x2   = int(parts[3])
                        z2   = int(parts[4])
                        camwx = None if len(parts) <= 5 or parts[5] == "null" else float(parts[5])
                        camwz = None if len(parts) <= 6 or parts[6] == "null" else float(parts[6])
                        height    = float(parts[7]) if len(parts) > 7 else 3.0
                        smoothing = float(parts[8]) if len(parts) > 8 else 0.1
                        # Optional corner data appended by the map editor:
                        # indices 9-16 are tlx, tlz, trx, trz, brx, brz, blx, blz
                        if len(parts) >= 17:
                            try:
                                corners = [
                                    (int(parts[9]),  int(parts[10])),  # TL
                                    (int(parts[11]), int(parts[12])),  # TR
                                    (int(parts[13]), int(parts[14])),  # BR
                                    (int(parts[15]), int(parts[16])),  # BL
                                ]
                            except ValueError:
                                corners = None
                        else:
                            corners = None
                        # Fall back to plain bounding rect if corners are absent
                        if corners is None:
                            corners = [
                                (x1,     z1),
                                (x2 + 1, z1),
                                (x2 + 1, z2 + 1),
                                (x1,     z2 + 1),
                            ]
                        # Parse cutouts: ", cutouts: x1 z1 x2 z2 | ..."
                        cutouts = []
                        cutout_marker = t.find("cutouts:")
                        if cutout_marker != -1:
                            cutout_data = t[cutout_marker + 8:].strip()
                            for entry in cutout_data.split("|"):
                                nums = entry.strip().split()
                                if len(nums) >= 4:
                                    try:
                                        cutouts.append((
                                            int(nums[0]), int(nums[1]),
                                            int(nums[2]), int(nums[3]),
                                        ))
                                    except ValueError:
                                        pass
                        cam_zones.append({
                            "id": zid, "x1": x1, "z1": z1, "x2": x2, "z2": z2,
                            "cam_wx": camwx, "cam_wz": camwz,
                            "height": height, "smoothing": smoothing,
                            "corners": corners,
                            "cutouts": cutouts,
                        })
                    except ValueError:
                        pass
                continue

            line = t.split("#")[0].strip()
            if not line:
                continue
            tokens = [tok.strip() for tok in line.split(",") if tok.strip()]
            if not tokens:
                continue
            row = []
            for tok in tokens:
                if tok not in tile_map:
                    raise ValueError(f"Unknown tile token '{tok}' in {path_obj.name}")
                row.append(tile_map[tok])
            rows.append(row)

    return rows, cam_zones


def validate(rows, path_obj):
    if not rows:
        raise ValueError(f"No map data found in {path_obj.name}")
    width = len(rows[0])
    for i, row in enumerate(rows):
        if len(row) != width:
            raise ValueError(
                f"Row {i} has {len(row)} tiles, expected {width} (in {path_obj.name})"
            )
    return len(rows), width


def to_header(rows, height, width, stem, cam_zones=None):
    guard = re.sub(r"[^A-Z0-9]", "_", stem.upper()) + "_H"
    define_prefix = re.sub(r"[^A-Z0-9]", "_", stem.upper())
    cam_zones = cam_zones or []

    lines = []
    lines.append(f"// Auto-generated from {stem}.jmap - do not edit by hand")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append(f"#define {define_prefix}_MAP_WIDTH  {width}")
    lines.append(f"#define {define_prefix}_MAP_HEIGHT {height}")
    lines.append("")

    # CRITICAL FIX: Changed uint8_t to uint16_t to support tile IDs > 255
    lines.append(
        f"static const uint16_t {stem}_map[{define_prefix}_MAP_HEIGHT][{define_prefix}_MAP_WIDTH] = {{"
    )
    for r, row in enumerate(rows):
        comma = "," if r < height - 1 else ""
        values = ", ".join(str(v) for v in row)
        lines.append(f"    {{ {values} }}{comma}")
    lines.append("};")
    lines.append("")

    # Camera zones
    if cam_zones:
        lines.append("// Camera zone structs")
        lines.append("// cam_zone_corner_t: a single grid-line tile coordinate pair.")
        lines.append("typedef struct {")
        lines.append("    uint16_t tx, tz;")
        lines.append("} cam_zone_corner_t;")
        lines.append("")
        lines.append("// cam_zone_t: trigger rect + camera parameters + 4 corners + cutout rects.")
        lines.append("// x1/z1/x2/z2 are the axis-aligned bounding rect (tile coords, inclusive).")
        lines.append("// corners[4] are [TL, TR, BR, BL] in grid-line tile coords.")
        lines.append("// cutouts[]: rectangular holes carved out of the zone (tile coords, inclusive).")
        lines.append("typedef struct {")
        lines.append("    uint16_t x1, z1, x2, z2;")
        lines.append("} cam_zone_cutout_t;")
        lines.append("")
        # Find the maximum cutout count across all zones, for fixed-size array.
        max_cutouts = max((len(z["cutouts"]) for z in cam_zones), default=0)
        lines.append(f"#define {define_prefix}_CAM_ZONE_MAX_CUTOUTS {max(max_cutouts, 1)}")
        lines.append("")
        lines.append("typedef struct {")
        lines.append("    uint8_t           id;")
        lines.append("    uint16_t          x1, z1, x2, z2;         /* bounding rect, inclusive */")
        lines.append("    float             cam_x, cam_z;            /* world origin (NaN = unset) */")
        lines.append("    float             height;")
        lines.append("    float             smoothing;")
        lines.append("    cam_zone_corner_t corners[4];              /* TL, TR, BR, BL */")
        lines.append(f"    cam_zone_cutout_t cutouts[{define_prefix}_CAM_ZONE_MAX_CUTOUTS]; /* carved holes */")
        lines.append("    uint8_t           cutout_count;")
        lines.append("} cam_zone_t;")
        lines.append("")
        lines.append(f"#define {define_prefix}_CAM_ZONE_COUNT {len(cam_zones)}")
        lines.append(f"static const cam_zone_t {stem}_cam_zones[{define_prefix}_CAM_ZONE_COUNT] = {{")
        for i, z in enumerate(cam_zones):
            zid       = z["id"]
            x1, z1    = z["x1"], z["z1"]
            x2, z2    = z["x2"], z["z2"]
            camwx     = z["cam_wx"]
            camwz     = z["cam_wz"]
            height    = z["height"]
            smoothing = z["smoothing"]
            corners   = z["corners"]   # list of 4 (tx, tz) tuples
            cutouts   = z["cutouts"]   # list of (cx1, cz1, cx2, cz2) tuples
            comma = "," if i < len(cam_zones) - 1 else ""
            cx = f"{camwx:.4f}f" if camwx is not None else "__builtin_nanf(\"\")"  # NAN sentinel
            cz = f"{camwz:.4f}f" if camwz is not None else "__builtin_nanf(\"\")"  # NAN sentinel
            corner_init = ", ".join(f"{{ {c[0]}, {c[1]} }}" for c in corners)
            # Pad cutouts array to max_cutouts with zeroes.
            cutout_entries = [f"{{ {c[0]}, {c[1]}, {c[2]}, {c[3]} }}" for c in cutouts]
            while len(cutout_entries) < max(max_cutouts, 1):
                cutout_entries.append("{ 0, 0, 0, 0 }")
            cutout_init = ", ".join(cutout_entries)
            lines.append(
                f"    {{ {zid}, {x1}, {z1}, {x2}, {z2}, "
                f"{cx}, {cz}, {height:.4f}f, {smoothing:.4f}f, "
                f"{{ {corner_init} }}, {{ {cutout_init} }}, {len(cutouts)} }}{comma}  // cam_{zid}"
            )
        lines.append("};")
        lines.append("")
    else:
        lines.append(f"#define {define_prefix}_CAM_ZONE_COUNT 0")
        lines.append("")

    lines.append("#endif")
    return "\n".join(lines) + "\n"


# Entry point for build_asset.py dispatch


def _format_cpp_h_files(paths: list[str]) -> None:
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        return
    inputs = [p for p in paths if os.path.exists(p)]
    if not inputs:
        return
    try:
        subprocess.run([clang_format, "--style=file", "-i", *inputs], check=True)
    except subprocess.CalledProcessError:
        print("Warning: clang-format failed on generated files.", file=sys.stderr)


def convert(input_file: str, output_file: str, config: dict) -> None:
    """build_asset.py-compatible entry point."""
    tile_map = _load_tile_map()

    jmap_path = Path(input_file)
    out_path = Path(output_file)

    # If the caller passed a directory, derive the .h filename automatically
    if out_path.is_dir() or (not out_path.suffix):
        out_path = out_path / (jmap_path.stem + ".h")

    stem = jmap_path.stem

    rows, cam_zones = parse_jmap(jmap_path, tile_map)
    height, width = validate(rows, jmap_path)
    header = to_header(rows, height, width, stem, cam_zones)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(header)
    _format_cpp_h_files([str(out_path)])

    zone_msg = f" + {len(cam_zones)} cam zone(s)" if cam_zones else ""
    print(f"Written: {out_path.name} / ({width}x{height}){zone_msg}")


# Standalone CLI


def main():
    parser = argparse.ArgumentParser(
        description="Convert a .jmap collision map to a C header file."
    )
    parser.add_argument("input", help="Input .jmap file")
    parser.add_argument("output", help="Output .h file (or directory)")
    args = parser.parse_args()

    convert(args.input, args.output, {})


if __name__ == "__main__":
    main()
