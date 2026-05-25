# ryazha-clk

Sysmodule + Tesla overlay для управления частотами Switch (CPU/GPU/RAM/Display).
Title ID `00FF0000636C6BFF`. Часть проекта RCU.

См. полное описание в [главном README](../../README.md).

## Layout

- `sysmodule/` — фоновый сервис, exefs.nsp
- `overlay/` — Tesla overlay, ryazha-clk.ovl
- `common/` — общий header'ы + IPC client (рaza с overlay'ем и sysmodule'ем)
- `build.sh` — сборочный скрипт (вызывается из корневого build pipeline)
