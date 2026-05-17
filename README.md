# RCU (Ryazha Clock Utility)

RCU is an open-source overclocking tool for Nintendo Switch consoles running Atmosphere custom firmware. It enables advanced CPU, GPU, and RAM tuning with integrated configuration tools.

## Disclaimer
> **THIS TOOL CAN BE DANGEROUS IF MISUSED. PROCEED WITH CAUTION.**
> Due to the design of Horizon OS, overclocking RAM can cause NAND or SD corruption.
> Ensure you have a full NAND, PROINFO, EMUMMC, and SD backup before proceeding.

## About
RCU provides a comprehensive suite for performance tuning on the Nintendo Switch. It is designed to work seamlessly with Atmosphere and offers granular control over system clocks and voltages.

## Default Clocks
* **CPU:** Up to 1963MHz (Mariko) / 1785MHz (Erista)
* **GPU:** Up to 1075MHz (Mariko) / 921MHz (Erista)
* **RAM:** Up to 1866/2133MHz (Mariko) / 1600MHz (Erista)
* Overvolting and undervolting support
* Built-in configurator
* Compatible with most homebrew applications

## Installation
1. Ensure you have the latest versions of Atmosphere and Ultrahand Overlay.
2. Download and extract the RCU package to the root of your SD card.
3. If using Hekate, edit `hekate_ipl.ini` to include:
   ```
   kip1=atmosphere/kips/rcu.kip
   ```
   No changes are needed if using fusee.

## Configuration
1. Open the RCU Overlay.
2. Navigate to the settings menu.
3. Adjust your overclocking settings as desired.
4. Select "Save KIP Settings" to apply your configuration.

## Building from Source
Refer to COMPILING.md for detailed build instructions.

## Clock Table

### MEM Clocks (MHz)
* 3200: Max on Mariko, JEDEC.
* 2933: JEDEC.
* 2666: JEDEC.
* 2400: Max on Erista, JEDEC.
* 2133: Mariko JEDEC standard max (4266 Modules).
* 1996: JEDEC standard.
* 1866: Mariko JEDEC standard max (3733 Modules).
* 1600: Official docked, boost mode, Erista safe max, JEDEC.
* 1331: Official handheld, JEDEC.
* 1065, 800, 665.

### CPU Clocks (MHz)
* 2703: Mariko absolute max (dangerous).
* 2601: Unsafe.
* 2499, 2397, 2295, 2193, 2091.
* 1963: Mariko no UV max clock.
* 1887.
* 1785: Erista no UV max clock, boost mode.
* 1683, 1581, 1428, 1326, 1224, 1122.
* 1020: Official docked and handheld.
* 918, 816, 714, 612.

### GPU Clocks (MHz)
* 1536: Absolute max clock on Mariko (very dangerous).
* 1459, 1382, 1305.
* 1267: NVIDIA T214 (Mariko) rating.
* 1228: Mariko HiOPT safe clock.
* 1152: Mariko SLT max clock.
* 1075: Mariko no UV max clock; absolute max on Erista (very dangerous).
* 998: NVIDIA T210 (Erista) rating.
* 960: Erista SLT/HiOPT safe max clock.
* 921: Erista no UV max clock.
* 844, 768, 691, 614, 537, 460, 384, 307, 230, 153, 76.

## Credits
* **Souldbminer** - ryazha-clk and loader development
* **Lightos** - Loader patches development, ryazha-clk development, guides
* **SciresM** - Atmosphere CFW
* **CTCaer** - L4T, Hekate, proper RAM timings
* **KazushiMe** - Switch OC Suite
* **Hanai3bi (Meha)** - Switch OC Suite, EOS, sys-clk-eos
* **NaGaa95** - L4T-OC kernel, Status Monitor fork
* **B3711 (halop)** - EOS
* **sys-clk team** - sys-clk
* **Dominatorul** - Soctherm driver, guides, general help
* **ppkantorski** - Ultrahand sys-clk and Status Monitor fork
* **MasaGratoR and ZachyCatGames** - General help
* **MasaGratoR** - Status Monitor and Display Refresh Rate driver
* **Nvidia** - Tegra X1 Technical Reference Manual, soctherm driver, L4T
