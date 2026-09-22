#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

source_revision="${1:-$(git -C "$repo_root" rev-parse HEAD)}"
output="${2:--}"
template="$repo_root/fdroid/$WEGERT_PACKAGE_ID.yml.template"

[[ "$source_revision" =~ ^[0-9a-f]{40}$ ]]

python3 - "$template" "$output"     "$WEGERT_VERSION_NAME" "$WEGERT_VERSION_CODE" "$source_revision"     "$WEGERT_TARGET_SDK" "$WEGERT_BUILD_TOOLS" "$WEGERT_CMAKE_VERSION" "$WEGERT_NDK_VERSION" <<'PY'
from pathlib import Path
import sys

template, output, version_name, version_code, source_revision, target_sdk, build_tools, cmake, ndk = sys.argv[1:]
text = Path(template).read_text(encoding="utf-8")
values = {
    "@VERSION_NAME@": version_name,
    "@VERSION_CODE@": version_code,
    "@SOURCE_REVISION@": source_revision,
    "@TARGET_SDK@": target_sdk,
    "@BUILD_TOOLS@": build_tools,
    "@CMAKE@": cmake,
    "@NDK@": ndk,
}
for marker, value in values.items():
    if marker not in text:
        raise SystemExit(f"missing metadata template marker: {marker}")
    text = text.replace(marker, value)
if "@" in text:
    raise SystemExit("unexpanded @...@ marker remains in F-Droid metadata")
if output == "-":
    sys.stdout.write(text)
else:
    Path(output).write_text(text, encoding="utf-8")
PY
