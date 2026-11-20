#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 || ! -d "$1" ]]; then
  echo "Usage: bash import-voices.sh /path/to/recordings" >&2
  exit 1
fi

base="$(cd "$(dirname "$0")" && pwd)"
target="$base/ArenaController/VoiceAssets.h"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT

for name in Fight Stop Round Blue Red Final BlueWins RedWins; do
  source_file=
  for extension in wav mp3; do
    if [[ -f "$1/$name.$extension" ]]; then
      source_file="$1/$name.$extension"
      break
    fi
  done
  if [[ -z "$source_file" ]]; then
    echo "Missing recording: $name.wav or $name.mp3" >&2
    exit 1
  fi
  ffmpeg -nostdin -loglevel error -i "$source_file" -vn \
    -ac 1 -ar 16000 -c:a pcm_u8 -f u8 "$scratch/voice$name.raw"
done

python3 - "$scratch" "$target" <<'PY'
import sys
import tempfile
from pathlib import Path

source, target = map(Path, sys.argv[1:])
lines = ["#pragma once", "#include <stdint.h>", "",
         "// Imported recordings. Mono unsigned PCM at 16000 Hz.", ""]
for path in sorted(source.glob("*.raw")):
    data = path.read_bytes()
    if not data or min(data) == max(data):
        raise SystemExit(f"Empty or constant recording: {path.name}")
    lines.append(f"static const uint8_t {path.stem}[] = {{")
    for start in range(0, len(data), 20):
        lines.append("  " + ", ".join(map(str, data[start:start + 20])) + ",")
    lines.extend(["};", ""])

with tempfile.NamedTemporaryFile(mode="w", dir=target.parent, delete=False) as temporary:
    temporary.write("\n".join(lines))
Path(temporary.name).replace(target)
PY
