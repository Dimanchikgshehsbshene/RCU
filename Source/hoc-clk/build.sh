#!/usr/bin/env bash
set -e

# Рекурсивный make иногда не видит DEVKITPRO из окружения (MSYS / агенты); задаём и прокидываем явно.
if [[ -z "${DEVKITPRO:-}" ]]; then
    if [[ -d /opt/devkitpro ]]; then
        export DEVKITPRO=/opt/devkitpro
    elif [[ -d /c/devkitPro ]]; then
        export DEVKITPRO=/c/devkitPro
    fi
fi

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DIST_DIR="$ROOT_DIR/dist"
CORES="$(nproc --all)"

if [[ -n "$1" ]]; then
    DIST_DIR="$1"
fi

echo "DIST_DIR: $DIST_DIR"
echo "CORES: $CORES"

echo "*** sysmodule ***"
TITLE_ID="$(grep -oP '"title_id":\s*"0x\K(\w+)' "$ROOT_DIR/sysmodule/perms.json")"

pushd "$ROOT_DIR/sysmodule"
make -j$CORES DEVKITPRO="$DEVKITPRO"
popd > /dev/null

mkdir -p "$DIST_DIR/atmosphere/contents/$TITLE_ID/flags"
cp -vf "$ROOT_DIR/sysmodule/out/hoc-clk.nsp" "$DIST_DIR/atmosphere/contents/$TITLE_ID/exefs.nsp"
>"$DIST_DIR/atmosphere/contents/$TITLE_ID/flags/boot2.flag"
cp -vf "$ROOT_DIR/sysmodule/toolbox.json" "$DIST_DIR/atmosphere/contents/$TITLE_ID/toolbox.json"

echo "*** overlay ***"
pushd "$ROOT_DIR/overlay"
make -j$CORES DEVKITPRO="$DEVKITPRO"
popd > /dev/null

mkdir -p "$DIST_DIR/switch/.overlays"
cp -vf "$ROOT_DIR/overlay/out/ryazha-clk.ovl" "$DIST_DIR/switch/.overlays/ryazha-clk.ovl"

echo "*** assets ***"
mkdir -p "$DIST_DIR/config/ryazha-clk"
mkdir -p "$DIST_DIR/config/ryazhahand"
cp -vf "$ROOT_DIR/config.ini.template" "$DIST_DIR/config/ryazha-clk/config.ini.template"
cp -vf "$ROOT_DIR/config.ini.template" "$DIST_DIR/config/ryazhahand/config.ini.template"
cp -vf "$ROOT_DIR/../../README.md" "$DIST_DIR/README.md"

echo "*** lang ***"
cp -r "$ROOT_DIR/overlay/lang/" "$DIST_DIR/config/ryazha-clk/lang/"
cp -r "$ROOT_DIR/overlay/lang/" "$DIST_DIR/config/ryazhahand/lang/"
