#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build-mac.sh
mkdir -p dist
stage="$(mktemp -d "${TMPDIR:-/private/tmp}/audio-tools-package.XXXXXX")"
app="$stage/AudioTools.app"
ditto --norsrc --noextattr build/AudioTools.app "$app"
macdeployqt "$app" -qmldir="$PWD/qml" -always-overwrite
test -d "$app/Contents/Resources/qml/QtQuick.2"
cp LICENSE "$app/Contents/Resources/COPYING"
xattr -cr "$app"
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
ditto --norsrc --noextattr "$app" "dist/Audio Tools.app"
ditto -ck --norsrc --keepParent "$app" "dist/Audio-Tools-macOS-$(uname -m).zip"
