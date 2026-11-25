#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build-mac.sh
mkdir -p dist
stage="$(mktemp -d "$PWD/build/package.XXXXXX")"
app="$stage/AudioTools.app"
ditto build/AudioTools.app "$app"
macdeployqt "$app" -qmldir="$PWD/qml" -always-overwrite
test -d "$app/Contents/Resources/qml/QtQuick.2"
cp LICENSE "$app/Contents/Resources/COPYING"
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
ditto "$app" "dist/Audio Tools.app"
ditto -ck --keepParent "dist/Audio Tools.app" "dist/Audio-Tools-macOS-$(uname -m).zip"
