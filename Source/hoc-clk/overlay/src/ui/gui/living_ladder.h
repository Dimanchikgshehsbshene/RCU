/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * Оверлейный прокси для Ryazha-Авто.
 * ВСЯ логика (VRR, TDP throttle, ladder, RAM-pin) теперь живёт в sysmodule
 * (см. sysmodule/src/auto_ryazha.cpp). Оверлей — это тонкий UI:
 *   1) забирает cfg через hocclkIpcGetLadderConfig;
 *   2) отдаёт обновлённый cfg через hocclkIpcSetLadderConfig на каждый клик;
 *   3) НЕ имеет собственного tick'а.
 *
 */

#pragma once

#include <cstdint>
#include <switch/types.h>
#include <hocclk/auto_ryazha.h>

using LadderConfig = HocClkLadderConfig;

enum LadderAlgo : u8 {
    LadderAlgo_Cycle   = HocClkLadderAlgo_Cycle,
    LadderAlgo_Pro     = HocClkLadderAlgo_Pro,
    LadderAlgo_EnumMax = HocClkLadderAlgo_EnumMax,
};

enum LadderVrrMode : u8 {
    LadderVrr_Off      = HocClkLadderVrr_Off,
    LadderVrr_On       = HocClkLadderVrr_On,
    LadderVrr_Auto     = HocClkLadderVrr_Auto,
    LadderVrr_Smart    = HocClkLadderVrr_Smart,
    LadderVrr_SuperPro = HocClkLadderVrr_SuperPro,
    LadderVrr_EnumMax  = HocClkLadderVrr_EnumMax,
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
