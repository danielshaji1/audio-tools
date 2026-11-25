#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build-mac.sh
mkdir -p dist
bundle="Open Sound Meter Modded.app"
stage="$(mktemp -d "${TMPDIR:-/private/tmp}/osm-modded-package.XXXXXX")"
app="$stage/$bundle"
ditto --norsrc --noextattr build/AudioTools.app "$app"
macdeployqt "$app" -qmldir="$PWD/qml" -always-overwrite
test -d "$app/Contents/Resources/qml/QtQuick.2"
cp LICENSE "$app/Contents/Resources/COPYING"
xattr -cr "$app"
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
ditto --norsrc --noextattr "$app" "dist/$bundle"
ditto -ck --norsrc --keepParent "$app" "dist/Open-Sound-Meter-Modded-macOS-$(uname -m).zip"
