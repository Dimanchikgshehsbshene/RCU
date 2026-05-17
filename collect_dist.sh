#!/bin/bash
set -e

# Цвета для вывода
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== ПОЛНАЯ СБОРКА RCU ИЗ ИСХОДНИКОВ ===${NC}"

# Проверка DEVKITPRO
if [ -z "$DEVKITPRO" ]; then
    echo -e "${RED}Ошибка: Переменная DEVKITPRO не установлена!${NC}"
    exit 1
fi

# 1. Подготовка структуры dist
DIST_ROOT="$(pwd)/dist"
rm -rf "$DIST_ROOT"
mkdir -p "$DIST_ROOT/atmosphere/kips"
mkdir -p "$DIST_ROOT/atmosphere/contents"
mkdir -p "$DIST_ROOT/switch/.overlays"
mkdir -p "$DIST_ROOT/switch/overlays"
mkdir -p "$DIST_ROOT/config/ryazha-clk"

# 2. Сборка лоадера (rcu.kip)
echo -e "${YELLOW}--- Сборка лоадера (rcu.kip) ---${NC}"
# Примечание: Лоадер требует наличия исходников Atmosphere в папке build/
if [ -d "build/stratosphere/loader" ]; then
    pushd build/stratosphere/loader > /dev/null
    make clean && make -j$(nproc)
    hactool -t kip1 out/nintendo_nx_arm64_armv8a/release/loader.kip --uncompress=rcu.kip
    cp rcu.kip "$DIST_ROOT/atmosphere/kips/rcu.kip"
    popd > /dev/null
    echo -e "${GREEN}Лоадер собран и скопирован.${NC}"
else
    echo -e "${RED}Ошибка: Исходники лоадера не найдены в build/stratosphere/loader!${NC}"
fi

# 3. Сборка ryazha-clk (системный модуль)
echo -e "${YELLOW}--- Сборка ryazha-clk sysmodule ---${NC}"
pushd Source/ryazha-clk/sysmodule > /dev/null
TITLE_ID=$(grep -oP '"title_id":\s*"0x\K(\w+)' perms.json)
make clean && make -j$(nproc)
mkdir -p "$DIST_ROOT/atmosphere/contents/$TITLE_ID/flags"
cp out/ryazha-clk.nsp "$DIST_ROOT/atmosphere/contents/$TITLE_ID/exefs.nsp"
[ -f toolbox.json ] && cp toolbox.json "$DIST_ROOT/atmosphere/contents/$TITLE_ID/toolbox.json"
touch "$DIST_ROOT/atmosphere/contents/$TITLE_ID/flags/boot2.flag"
popd > /dev/null
echo -e "${GREEN}Системный модуль собран (TID: $TITLE_ID).${NC}"

# 4. Сборка ryazha-clk (оверлей)
echo -e "${YELLOW}--- Сборка ryazha-clk overlay ---${NC}"
pushd Source/ryazha-clk/overlay > /dev/null
make clean && make -j$(nproc)
cp out/ryazha-clk.ovl "$DIST_ROOT/switch/.overlays/ryazha-clk.ovl"
cp out/ryazha-clk.ovl "$DIST_ROOT/switch/overlays/ryazha-clk.ovl"
popd > /dev/null
echo -e "${GREEN}Оверлей ryazha-clk собран.${NC}"

# 5. Сборка Ryazha-Status-Monitor
echo -e "${YELLOW}--- Сборка Ryazha-Status-Monitor ---${NC}"
# Если есть Makefile в корне RSM
if [ -f "Source/Ryazha-Status-Monitor/Makefile" ]; then
    pushd Source/Ryazha-Status-Monitor > /dev/null
    make clean && make -j$(nproc)
    cp Ryazha-Status-Monitor.ovl "$DIST_ROOT/switch/.overlays/Ryazha-Status-Monitor.ovl"
    popd > /dev/null
    echo -e "${GREEN}Ryazha-Status-Monitor собран.${NC}"
elif [ -f "Source/Ryazha-Status-Monitor/Ryazha-Status-Monitor.ovl" ]; then
    cp "Source/Ryazha-Status-Monitor/Ryazha-Status-Monitor.ovl" "$DIST_ROOT/switch/.overlays/Ryazha-Status-Monitor.ovl"
    echo -e "${GREEN}Ryazha-Status-Monitor (готовый бинарник) скопирован.${NC}"
fi

# 6. Сборка SaltyNX
echo -e "${YELLOW}--- Сборка SaltyNX ---${NC}"
if [ -d "Source/SaltyNX" ]; then
    pushd Source/SaltyNX > /dev/null
    make clean && make -j$(nproc)
    if [ -d "sdcard_out" ]; then
        cp -r sdcard_out/. "$DIST_ROOT/"
    fi
    popd > /dev/null
    echo -e "${GREEN}SaltyNX собран.${NC}"
fi

# 7. Копирование доп. ресурсов
echo -e "${YELLOW}--- Копирование ресурсов ---${NC}"
[ -f "Source/ryazha-clk/config.ini.template" ] && cp "Source/ryazha-clk/config.ini.template" "$DIST_ROOT/config/ryazha-clk/config.ini.template"
[ -f "README_RU.md" ] && cp "README_RU.md" "$DIST_ROOT/README.md"

echo -e "${BLUE}=== СБОРКА ЗАВЕРШЕНА. РЕЗУЛЬТАТ В ПАПКЕ /dist ===${NC}"
ls -R "$DIST_ROOT"
