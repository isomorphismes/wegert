#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=${1:-"$repo_root/build/direct/assets/wegert.frag"}

mkdir -p "$(dirname -- "$output")"
python3 - "$repo_root/wegert.frag.in" "$repo_root/wegert_color.glsl" "$output" <<'PY'
from pathlib import Path
import sys

template_path, color_path, output_path = map(Path, sys.argv[1:])
marker = "/*__WEGERT_COLOR_CORE__*/"
template = template_path.read_text(encoding="utf-8")
if template.count(marker) != 1:
    raise SystemExit("Wegert fragment template must contain exactly one coloring-core marker")
color = color_path.read_text(encoding="utf-8")
output_path.write_text(template.replace(marker, color), encoding="utf-8")
PY
