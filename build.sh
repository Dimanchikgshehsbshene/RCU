#!/bin/sh

# Подставляем DEVKITPRO, если shell его не видит (типично для не-login MSYS).
case ${DEVKITPRO-} in
    "") [ -d /c/devkitPro ] && export DEVKITPRO=/c/devkitPro ;;
esac

SRC="Source/Atmosphere/stratosphere/loader/"
DEST="build/stratosphere/loader/"
mkdir -p "dist/atmosphere/kips/"
mkdir -p "$DEST"

cp -r "$SRC"/. "$DEST"/

cd build/stratosphere/loader || exit 1
make -j8 DEVKITPRO="${DEVKITPRO}"
hactool -t kip1 out/nintendo_nx_arm64_armv8a/release/loader.kip --uncompress=rcu.kip
cd ../../../ # exit
cp build/stratosphere/loader/rcu.kip dist/atmosphere/kips/rcu.kip

cd Source/hoc-clk/
./build.sh
cp -r dist/ ../../

cd ../../

cd Source/Horizon-OC-Monitor/
make -j8 DEVKITPRO="${DEVKITPRO}"
cp Horizon-OC-Monitor.ovl ../../dist/switch/.overlays/Horizon-OC-Monitor.ovl