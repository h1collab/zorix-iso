#!/usr/bin/env bash
set -euo pipefail
OWNER="${OWNER:-h1collab}"
REPO="${REPO:-zorix-iso}"
TAG="${TAG:-v0.7.0}"
ASSETS_DIR="${1:-.}"

command -v gh >/dev/null || { echo "gh CLI is required" >&2; exit 2; }
gh auth status >/dev/null

required=(
  ZorixOS-0.7.iso
  ZorixOS-0.7.iso.sha256
  zorix-os-0.7-source-and-tools.tar.gz
  zorix-os-0.7-source-and-tools.tar.gz.sha256
)
for f in "${required[@]}"; do
  test -f "$ASSETS_DIR/$f" || { echo "Missing: $ASSETS_DIR/$f" >&2; exit 3; }
done

sha256sum -c "$ASSETS_DIR/ZorixOS-0.7.iso.sha256"

if gh release view "$TAG" --repo "$OWNER/$REPO" >/dev/null 2>&1; then
  gh release upload "$TAG" --repo "$OWNER/$REPO" --clobber "${required[@]/#/$ASSETS_DIR/}"
else
  gh release create "$TAG" --repo "$OWNER/$REPO" --title "Zorix OS 0.7 Liquid Glass / Rust Experimental" "${required[@]/#/$ASSETS_DIR/}"
fi
