#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT="$ROOT/bin/zorix-rust-core"
REPORT="$ROOT/docs/rust-build.json"
mkdir -p "$ROOT/bin" "$ROOT/docs"
if command -v rustc >/dev/null 2>&1; then
  rustc -O "$ROOT/src/ZorixRuntime.rs" -o "$OUT"
  printf '%s\n' '{"attempted":true,"compiled":true,"reason":"rustc available"}' > "$REPORT"
else
  rm -f "$OUT"
  printf '%s\n' '{"attempted":true,"compiled":false,"reason":"rustc unavailable; source and reproducible build target included"}' > "$REPORT"
fi
