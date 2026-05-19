/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
 
/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */


#pragma once

#include "../../ipc.h"
#include "../smoothing.h"
#include "base_gui.h"

// Bank жидких каналов для HUD. Сглаживаем только то, что реально "прыгает":
// нагрузки / температуры / мощность / FPS / частота дисплея. Frequencies ядер
// оставлены сырыми: они дискретные по таблице и прыжок между шагами — это
// информация, которую хочется видеть без сглаживания.
struct HudSmoothBank {
    // Frequencies (Hz). Делаем быстрый счётчик "шажочками по 1 МГц":
    //   alpha высокий, slew 300 MHz/s — скачок 100 МГц проезжает за ~0.33 сек
    //   линейно через все промежуточные значения. epsilon 0.1 МГц — текст
    //   перерисовывается на десятые доли.
    static constexpr float kFreqAlphaUp    = 0.55f;
    static constexpr float kFreqAlphaDown  = 0.45f;
    static constexpr float kFreqSlewHz     = 300'000'000.0f;
    static constexpr float kFreqEpsilonHz  = 100'000.0f;

    SmoothedChannel cpuHz   {{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};
    SmoothedChannel gpuHz   {{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};
    SmoothedChannel memHz   {{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};
    SmoothedChannel cpuRealHz{{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};
    SmoothedChannel gpuRealHz{{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};
    SmoothedChannel memRealHz{{kFreqAlphaUp, kFreqAlphaDown, kFreqSlewHz, kFreqSlewHz, kFreqEpsilonHz}};

    // Loads (%).
    SmoothedChannel cpuLoad{{0.30f, 0.20f, 1e9f, 1e9f, 1.0f}};
    SmoothedChannel gpuLoad{{0.30f, 0.20f, 1e9f, 1e9f, 1.0f}};
    SmoothedChannel memLoad{{0.30f, 0.20f, 1e9f, 1e9f, 1.0f}};
    SmoothedChannel batLoad{{0.10f, 0.10f, 1e9f, 1e9f, 0.1f}};
    SmoothedChannel fanLoad{{0.20f, 0.15f, 1e9f, 1e9f, 1.0f}};

    // Temps (миллиградусы С). Медленнее EMA, slew ограничен чтобы не дёргалось.
    SmoothedChannel socTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel pcbTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel skinTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel cpuTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel gpuTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel memTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};
    SmoothedChannel batTemp{{0.10f, 0.08f, 12000.0f, 6000.0f, 50.0f}};

    // Voltages (мкВ для CPU/GPU/SOC/EMC, мВ для батареи).
    SmoothedChannel cpuVolt{{0.15f, 0.12f, 1e9f, 1e9f, 1000.0f}};
    SmoothedChannel gpuVolt{{0.15f, 0.12f, 1e9f, 1e9f, 1000.0f}};
    SmoothedChannel socVolt{{0.15f, 0.12f, 1e9f, 1e9f, 1000.0f}};
    SmoothedChannel emcVolt{{0.15f, 0.12f, 1e9f, 1e9f, 1000.0f}};
    SmoothedChannel batVolt{{0.15f, 0.12f, 1e9f, 1e9f, 10.0f}};

    // Power (мВт).
    SmoothedChannel powerNow{{0.25f, 0.20f, 1e9f, 1e9f, 20.0f}};
    SmoothedChannel powerAvg{{0.10f, 0.08f, 1e9f, 1e9f, 20.0f}};

    // Display (Hz).
    SmoothedChannel displayHz{{0.72f, 0.62f, 1e9f, 1e9f, 0.25f}};

    // FPS живёт отдельно — hold+fade при пропадании сигнала.
    FpsStabilizer fps;

    // Гистерезис цветов температуры (общий для всех сенсоров).
    ThresholdHysteresis tempColor{70.0f, 68.0f, 85.0f, 82.0f};
};

class BaseMenuGui : public BaseGui
{
    protected:

    public:
        // u8 dockedHighestAllowedRefreshRate = 60;
        HocClkContext* context;
        std::uint64_t lastContextUpdate;    // совместимость: используется только для ladder.
        std::uint64_t lastTelemetryNs = 0;  // ~15 Гц чтение контек/ipc.
        std::uint64_t lastConfigNs    = 0;  // 1 Гц перечитывание config.
        std::uint64_t lastFrameTicks  = 0;  // для dt между кадрами (EMA/slew).
        HudSmoothBank smooth;
        HocClkConfigValueList configList;
        bool g_hardwareModelCached = false;
        bool g_isMariko = false;
        bool g_isAula = false;
        bool g_isHoag = false;
        SetSysProductModel HWmodel = SetSysProductModel_Invalid;
        
        bool IsAula() {
            if (!g_hardwareModelCached) {
                setsysGetProductModel(&HWmodel);
                g_hardwareModelCached = true;
            }
            g_isAula = (HWmodel == SetSysProductModel_Aula);
            return g_isAula;
        }
        bool IsHoag() {
            if (!g_hardwareModelCached) {
                setsysGetProductModel(&HWmodel);
                g_hardwareModelCached = true;
            }
            g_isHoag = (HWmodel == SetSysProductModel_Hoag);
            return g_isHoag;
        }
        bool IsMariko() {
            if (!g_hardwareModelCached) {
                setsysGetProductModel(&HWmodel);
                g_hardwareModelCached = true;
            }
            g_isMariko = (HWmodel == SetSysProductModel_Iowa || 
            HWmodel == SetSysProductModel_Hoag || 
            HWmodel == SetSysProductModel_Calcio || 
            HWmodel == SetSysProductModel_Aula);

            return g_isMariko;
        }

        bool IsErista() {
            return !IsMariko();
        }
        BaseMenuGui();
        ~BaseMenuGui();
        void preDraw(tsl::gfx::Renderer* renderer) override;
        tsl::elm::List* listElement;
        tsl::elm::Element* baseUI() override;
        void refresh() override;
        virtual void listUI() = 0;

    private:
        char displayStrings[48][32];  // Pre-formatted display strings
        tsl::Color tempColors[HocClkThermalSensor_EnumMax];  // Pre-computed temperature colors
};
