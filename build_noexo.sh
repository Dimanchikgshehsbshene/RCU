#!/bin/sh

CORES="$(nproc --all)"

SRC="Source/Atmosphere/stratosphere/loader/"
DEST="build/stratosphere/loader/"
mkdir -p "dist/atmosphere/kips/"
mkdir -p "$DEST"

echo
echo "*** Patching loader ***"
cp -vr "$SRC"/. "$DEST"/
echo

echo "CORES: $CORES"
echo

echo "*** Compiling loader ***"
cd build/stratosphere/loader || exit 1
make -j$CORES
hactool -t kip1 out/nintendo_nx_arm64_armv8a/release/loader.kip --uncompress=rcu.kip
cd ../../../ # exit
cp -v build/stratosphere/loader/rcu.kip dist/atmosphere/kips/rcu.kip

cd Source/ryazha-clk-clk/
./build.sh
cp -r dist/ ../../

cd ../../

echo "*** Compiling ryazha-clk-monitor ***"
cd Source/Ryazha-CLK-Monitor/
make -j$CORES
cp -v Ryazha-CLK-Monitor.ovl ../../dist/switch/.overlays/Ryazha-CLK-Monitor.ovl
