#!/usr/bin/env python3
"""
themeformat.py OPTIONS FILE...

Formats OmniShell theme JSON files into a stable, column-aligned style.

Options:
    -c          write to stdout, don't update file(s)
    -k NUM      id width, default 18
    -a NUM      wxartid width, default 19
"""

from __future__ import annotations

import getopt
import json
import os
import sys
from typing import Any, Dict, Tuple


def usage(out) -> None:
    out.write(__doc__.strip() + "\n")


def _as_str(v: Any) -> str:
    if v is None:
        return ""
    if isinstance(v, str):
        return v
    return str(v)


def _sorted_items(d: Dict[str, Any]) -> Tuple[Tuple[str, Any], ...]:
    return tuple(sorted(d.items(), key=lambda kv: kv[0]))


def format_theme(theme: Dict[str, Any], *, id_width: int, wxartid_width: int) -> str:
    # Expected: { "<app>": { "<id>": { "wxartid": "...", "asset": "..." }, ... }, ... }
    # We intentionally *do not* inject spaces inside JSON string literals (that would change keys/values).
    # Alignment is achieved by adding spaces *between* tokens.
    lines = ["{"]

    apps = _sorted_items(theme)
    for app_i, (app, icons_any) in enumerate(apps):
        if not isinstance(icons_any, dict):
            raise ValueError(f"Expected object for app '{app}', got {type(icons_any).__name__}")
        icons: Dict[str, Any] = icons_any  # type: ignore[assignment]

        lines.append(f'    {json.dumps(app)}: {{')

        icon_items = _sorted_items(icons)
        for icon_i, (icon_id, spec_any) in enumerate(icon_items):
            if not isinstance(spec_any, dict):
                raise ValueError(
                    f"Expected object for icon '{app}.{icon_id}', got {type(spec_any).__name__}"
                )
            spec: Dict[str, Any] = spec_any  # type: ignore[assignment]

            has_wxartid = "wxartid" in spec
            has_asset = "asset" in spec
            wxartid = _as_str(spec.get("wxartid")) if has_wxartid else ""
            asset = _as_str(spec.get("asset")) if has_asset else ""

            id_pad = " " * max(0, id_width - len(icon_id))
            # We don't pad inside the JSON string; we align by adding spaces between tokens.
            wx_pad = " " * max(0, wxartid_width - len(wxartid))

            # Use json.dumps for correct escaping.
            icon_id_json = json.dumps(icon_id)
            wx_json = json.dumps(wxartid)
            asset_json = json.dumps(asset)

            trailing = "," if icon_i != len(icon_items) - 1 else ""
            # Skip non-existent fields; keep the "asset" key aligned to a fixed column
            # (inside the `{ ... }`) whether or not `wxartid` exists.
            asset_col = len('"wxartid": ') + wxartid_width + len(", ")
            if has_wxartid and has_asset:
                # Put comma immediately after the wxartid value.
                # Then pad so `"asset":` starts at asset_col.
                after_comma_pos = len('"wxartid": ') + len(wx_json) + 1  # +1 for comma
                pad = max(1, asset_col - after_comma_pos)
                inner = f"\"wxartid\": {wx_json},{' ' * pad}\"asset\": {asset_json}"
            elif has_wxartid:
                inner = f"\"wxartid\": {wx_json}"
            elif has_asset:
                inner = f"{' ' * asset_col}\"asset\": {asset_json}"
            else:
                inner = ""

            lines.append(f"        {icon_id_json}{id_pad}: {{ {inner} }}{trailing}")

        lines.append("    }" + ("," if app_i != len(apps) - 1 else ""))

    lines.append("}")
    lines.append("")  # newline at EOF
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    try:
        optlist, args = getopt.getopt(argv, "ck:a:h")
    except getopt.GetoptError as e:
        sys.stderr.write(str(e) + "\n")
        usage(sys.stderr)
        return 2

    write_stdout = False
    id_width = 18
    wxartid_width = 19

    for opt, val in optlist:
        if opt == "-c":
            write_stdout = True
        elif opt == "-k":
            id_width = int(val)
        elif opt == "-a":
            wxartid_width = int(val)
        elif opt == "-h":
            usage(sys.stdout)
            return 0

    if not args:
        usage(sys.stderr)
        return 2

    exit_code = 0
    for path in args:
        try:
            with open(path, "r", encoding="utf-8") as f:
                theme = json.load(f)
            if not isinstance(theme, dict):
                raise ValueError(f"Top-level JSON must be an object, got {type(theme).__name__}")

            out_text = format_theme(theme, id_width=id_width, wxartid_width=wxartid_width)

            if write_stdout:
                # When multiple files are passed, separate outputs clearly but still valid JSON per chunk.
                if len(args) > 1:
                    sys.stdout.write(f"// {os.path.basename(path)}\n")
                sys.stdout.write(out_text)
            else:
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(out_text)
        except Exception as e:
            sys.stderr.write(f"{path}: {e}\n")
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

