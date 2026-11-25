#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
cd build
qmake ../OpenSoundMeter.pro
# Qt 5 lists the retired AGL stub even though its active OpenGL backend uses CGL.
sed -i '' 's/ -framework AGL / /g' Makefile
make -j"$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"
cp ../Info.plist AudioTools.app/Contents/Info.plist
