#!/bin/bash
set -e

# Цвета для вывода
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Сборка RCU и подготовка релизных файлов ===${NC}"

# 1. Создаем структуру папок в dist
DIST_ROOT="$(pwd)/dist"
mkdir -p "$DIST_ROOT/atmosphere/kips"
mkdir -p "$DIST_ROOT/atmosphere/contents"
mkdir -p "$DIST_ROOT/switch/.overlays"
mkdir -p "$DIST_ROOT/config/ryazha-clk"

# 2. Собираем лоадер (rcu.kip)
echo -e "${GREEN}--- Сборка лоадера ---${NC}"
# В реальной среде здесь был бы вызов make в папке лоадера
# Для примера используем логику из build.sh
if [ -f "build/stratosphere/loader/rcu.kip" ]; then
    cp "build/stratosphere/loader/rcu.kip" "$DIST_ROOT/atmosphere/kips/rcu.kip"
    echo "rcu.kip скопирован"
else
    echo "Внимание: rcu.kip не найден, пропуск"
fi

# 3. Собираем ryazha-clk (sysmodule и overlay)
echo -e "${GREEN}--- Сборка ryazha-clk ---${NC}"
RYAZHA_DIR="Source/ryazha-clk"
if [ -d "$RYAZHA_DIR" ]; then
    # Получаем TITLE_ID из perms.json
    TITLE_ID=$(grep -oP '"title_id":\s*"0x\K(\w+)' "$RYAZHA_DIR/sysmodule/perms.json")
    echo "TITLE_ID: $TITLE_ID"
    
    mkdir -p "$DIST_ROOT/atmosphere/contents/$TITLE_ID/flags"
    
    # Копируем системный модуль
    if [ -f "$RYAZHA_DIR/sysmodule/out/ryazha-clk.nsp" ]; then
        cp "$RYAZHA_DIR/sysmodule/out/ryazha-clk.nsp" "$DIST_ROOT/atmosphere/contents/$TITLE_ID/exefs.nsp"
        touch "$DIST_ROOT/atmosphere/contents/$TITLE_ID/flags/boot2.flag"
        [ -f "$RYAZHA_DIR/sysmodule/toolbox.json" ] && cp "$RYAZHA_DIR/sysmodule/toolbox.json" "$DIST_ROOT/atmosphere/contents/$TITLE_ID/toolbox.json"
        echo "Системный модуль скопирован"
    fi
    
    # Копируем оверлей
    if [ -f "$RYAZHA_DIR/overlay/out/ryazha-clk.ovl" ]; then
        cp "$RYAZHA_DIR/overlay/out/ryazha-clk.ovl" "$DIST_ROOT/switch/.overlays/ryazha-clk.ovl"
        echo "Оверлей скопирован"
    fi
    
    # Копируем конфиг-шаблон
    [ -f "$RYAZHA_DIR/config.ini.template" ] && cp "$RYAZHA_DIR/config.ini.template" "$DIST_ROOT/config/ryazha-clk/config.ini.template"
fi

# 4. Копируем Ryazha-Status-Monitor
echo -e "${GREEN}--- Сборка Ryazha-Status-Monitor ---${NC}"
RSM_DIR="Source/Ryazha-Status-Monitor"
if [ -d "$RSM_DIR" ]; then
    if [ -f "$RSM_DIR/Ryazha-Status-Monitor.ovl" ]; then
        cp "$RSM_DIR/Ryazha-Status-Monitor.ovl" "$DIST_ROOT/switch/.overlays/Ryazha-Status-Monitor.ovl"
        echo "Ryazha-Status-Monitor скопирован"
    fi
fi

# 5. Копируем SaltyNX
echo -e "${GREEN}--- Сборка SaltyNX ---${NC}"
SALTY_DIR="Source/SaltyNX"
if [ -d "$SALTY_DIR/sdcard_out" ]; then
    cp -r "$SALTY_DIR/sdcard_out/." "$DIST_ROOT/"
    echo "SaltyNX файлы скопированы"
fi

# 6. Финализация
echo -e "${BLUE}=== Сборка завершена. Все файлы в папке /dist ===${NC}"
ls -R "$DIST_ROOT"
