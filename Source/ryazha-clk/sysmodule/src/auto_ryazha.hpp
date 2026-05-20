/*
 * Copyright (c) Souldbminer, Lightos_ and Ryazha CLK Contributors
 *
 * Sysmodule-side Ryazha-Авто / VRR подсистема. Работает в фоновом потоке —
 * не зависит от открытого оверлея и продолжает регулировать CPU/GPU/RAM
 * частоты и герцовку дисплея поверх игры.
 *
 */

#pragma once

#include <rclk.h>
#include <switch.h>

namespace autoRyazha {

    // === Lifecycle =========================================================
    void Initialize();                       // грузит INI, готовит поток
    void Exit();                             // закрывает поток корректно
    void Start();                            // запустить tick-поток (вызывать после clockManager::SetRunning(true))

    // === IPC-визитные карточки =============================================
    void GetConfig(RClkLadderConfig* out);         // snapshot cfg для overlay
    void SetConfig(const RClkLadderConfig* cfg);   // overlay прислал новый cfg — применим и сохраним

}
