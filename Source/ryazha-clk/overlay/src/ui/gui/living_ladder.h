/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * Оверлейный прокси для Ryazha-Авто.
 * ВСЯ логика (VRR, TDP throttle, ladder, RAM-pin) теперь живёт в sysmodule
 * (см. sysmodule/src/auto_ryazha.cpp). Оверлей — это тонкий UI:
 *   1) забирает cfg через rclkIpcGetLadderConfig;
 *   2) отдаёт обновлённый cfg через rclkIpcSetLadderConfig на каждый клик;
 *   3) НЕ имеет собственного tick'а.
 *
 */

#pragma once

#include <cstdint>
#include <switch/types.h>
#include <rclk/auto_ryazha.h>

using LadderConfig = RClkLadderConfig;

enum LadderAlgo : u8 {
    LadderAlgo_Cycle   = RClkLadderAlgo_Cycle,
    LadderAlgo_Pro     = RClkLadderAlgo_Pro,
    LadderAlgo_EnumMax = RClkLadderAlgo_EnumMax,
};

enum LadderVrrMode : u8 {
    LadderVrr_Off      = RClkLadderVrr_Off,
    LadderVrr_On       = RClkLadderVrr_On,
    LadderVrr_Auto     = RClkLadderVrr_Auto,
    LadderVrr_Smart    = RClkLadderVrr_Smart,
    LadderVrr_SuperPro = RClkLadderVrr_SuperPro,
    LadderVrr_EnumMax  = RClkLadderVrr_EnumMax,
};

class LivingLadderProxy {
public:
    // Singleton, ленивая загрузка из sysmodule при первом вызове.
    LadderConfig&       config();
    const LadderConfig& config() const;

    // Отправить cfg в sysmodule (вызывается после каждого изменения в UI).
    void push();
    // Перечитать cfg из sysmodule (на всякий случай при входе в меню).
    void pull();

private:
    mutable LadderConfig cfg_{};
    mutable bool         loaded_ = false;
    void ensureLoaded() const;
};

LivingLadderProxy& livingLadder();
