# Сборка RCU (под текущую структуру репозитория)

## Рекомендуемый способ (как в CI)
1. Установите devkitPro (devkitA64/devkitARM), `make`, `python3`, `hactool`.
2. Клонируйте репозиторий и инициализируйте сабмодули:
   - `git clone --recursive <repo-url>`
3. Запустите GitHub Actions workflow `.github/workflows/build.yml` **или** повторите его шаги локально.

## Локальная сборка через скрипт
- Основной скрипт: `./collect_dist.sh`
- Результат: `dist/`

Скрипт собирает:
- `rcu.kip` в `dist/atmosphere/kips/`
- `ryazha-clk` sysmodule в `dist/atmosphere/contents/<TITLE_ID>/`
- overlay в:
  - `dist/switch/.overlays/ryazha-clk.ovl`
  - `dist/switch/overlays/ryazha-clk.ovl` (для интеграций, где hidden-пути фильтруются)

## Важно
- Для сборки лоадера нужна подготовленная папка `build/stratosphere/loader`.
- Версия Atmosphere берётся из `ams_ver.txt` в CI-конвейере.
