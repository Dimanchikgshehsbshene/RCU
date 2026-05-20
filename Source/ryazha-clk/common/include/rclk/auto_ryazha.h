/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * Shared types for Ryazha-Авто auto-OC / VRR subsystem.
 * Lives in sysmodule (actual decision engine) and overlay (UI shell).
 *
 * Любое поле, добавленное сюда, участвует в IPC blob-е GetLadderConfig /
 * SetLadderConfig. Размер всей структуры должен совпадать на обеих сторонах.
 *
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    RClkLadderAlgo_Cycle = 0,    // GPU+1 → CPU+1 → (GPU+2, CPU+1)×N; вниз CPU-1 ↔ GPU-1
    RClkLadderAlgo_Pro   = 1,    // Умный — load-aware шаг + плавное снижение
    RClkLadderAlgo_EnumMax,
} RClkLadderAlgo;

typedef enum
{
    RClkLadderVrr_Off       = 0,
    RClkLadderVrr_On        = 1, // legacy: Display = vrrMaxHz
    RClkLadderVrr_Auto      = 2, // Display следует за FPS в окне [vrrMin..vrrMax]
    RClkLadderVrr_Smart     = 3, // Auto + гистерезис ±1 Hz
    RClkLadderVrr_SuperPro  = 4, // Display = panelMax всегда, taretFps безлимит
    RClkLadderVrr_EnumMax,
} RClkLadderVrrMode;

typedef struct
{
    uint8_t  enabled;                 // основной тумблер Ryazha-Авто
    uint8_t  targetFps;
    uint8_t  tolerance;
    uint8_t  algo;                    // RClkLadderAlgo

    uint32_t tempCapCpuMillideg;      // 0 = не использовать
    uint32_t tempCapGpuMillideg;
    uint32_t tdpCapMw;                // 0 = брать HandheldTDPLimit

    uint32_t userMinCpuHz;
    uint32_t userMaxCpuHz;
    uint32_t userMinGpuHz;
    uint32_t userMaxGpuHz;

    uint8_t  vrrMode;                 // RClkLadderVrrMode
    uint8_t  vrrMinHz;
    uint8_t  vrrMaxHz;
    uint8_t  predictorEnable;
    uint8_t  predictorWindowTicks;
    uint8_t  predictorAggressive;
    uint8_t  predictorSpikeFps;
    uint8_t  oledAutoGamma;

    uint8_t  _pad[48];                // запас на будущие поля без ломки IPC ABI
} RClkLadderConfig;

// Размер фиксирован для IPC-совместимости. Меняем только за счёт _pad.
#ifdef __cplusplus
static_assert(sizeof(RClkLadderConfig) == 88,
              "RClkLadderConfig must stay binary-stable for IPC");
#endif

#ifdef __cplusplus
}
#endif
