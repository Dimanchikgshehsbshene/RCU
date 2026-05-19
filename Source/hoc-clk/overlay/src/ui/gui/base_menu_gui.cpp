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


#include "base_menu_gui.h"
#include "fatal_gui.h"
#include "../format.h"
#include "living_ladder.h"

#include <algorithm>
#include <cmath>

// Cache hardware model to avoid repeated syscalls

BaseMenuGui::BaseMenuGui() : tempColors{ tsl::Color(0), tsl::Color(0), tsl::Color(0), tsl::Color(0), tsl::Color(0), tsl::Color(0), tsl::Color(0), }
{
    tsl::initializeThemeVars();
    this->context = nullptr;
    this->lastContextUpdate = 0;
    this->listElement = nullptr;


    // Pre-cache hardware model during initialization
    IsAula();
    IsMariko();
    IsHoag();

    // Initialize display strings
    memset(displayStrings, 0, sizeof(displayStrings));
}

BaseMenuGui::~BaseMenuGui() {
    delete this->context; // delete handles nullptr automatically
}

// Fast preDraw - just renders pre-computed strings
void BaseMenuGui::preDraw(tsl::gfx::Renderer* renderer) {
    BaseGui::preDraw(renderer);
    if(!this->context) [[unlikely]] return;

    // All constants pre-calculated and cached
    const char* labels[] = {
        "ID", "Профиль", "ЦП", "ГП", "ОЗУ", "SoC", "Плата", "Кожа", "Сейчас", "Ср.", "BAT", "PMIC", "Вент.", IsAula() ? "OLED" : "LCD", "FPS", "Разр."
    };

    static constexpr u32 dataPositions[6] = {75, 212, 348, 212, 354, 333};

    static u32 labelWidths[10];
    static bool positionsInitialized = false;

    if (!positionsInitialized) {
        for (int i = 0; i < 10; i++) {
            labelWidths[i] = renderer->getTextDimensions(labels[i], false, SMALL_TEXT_SIZE).first;
        }
        positionsInitialized = true;
    }
    static u32 positions[10] = {24-1, 310-labelWidths[1], 24-1, 192-labelWidths[3], 332-labelWidths[4], 24-1, 192 - labelWidths[6], 332-labelWidths[7], 192 - labelWidths[8], 332-labelWidths[9]};

    static u32 maxProfileValueWidth = renderer->getTextDimensions("Официальная зарядка", false, SMALL_TEXT_SIZE).first; // longest word

    u32 y = 91;

    // === TOP SECTION ===
    renderer->drawRoundedRect(14, 70-1, 420, 30+2, 12.0f, renderer->aWithOpacity(tsl::tableBGColor));

    // App ID - use pre-formatted string
    renderer->drawString(labels[0], false, positions[0], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(displayStrings[0], false, positions[0] + labelWidths[0] + 9, y, SMALL_TEXT_SIZE, tsl::infoTextColor);

    // Profile - use pre-formatted string
    renderer->drawString(labels[1], false, 463 - maxProfileValueWidth - labelWidths[1] - 9, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(displayStrings[1], false, 463 - maxProfileValueWidth, y, SMALL_TEXT_SIZE, tsl::infoTextColor);

    y += 38; // Direct assignment instead of += 38

    // === MAIN DATA SECTION ===
    renderer->drawRoundedRect(14, 106, 420, 156, 10.0f, renderer->aWithOpacity(tsl::tableBGColor));

    // === FREQUENCY SECTION ===
    // Labels first (better cache locality)
    renderer->drawString(labels[2], false, positions[2], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[3], false, positions[3], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[4], false, positions[4], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    // Current frequencies - use pre-formatted strings
    renderer->drawString(displayStrings[2], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // CPU
    renderer->drawString(displayStrings[3], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // GPU
    renderer->drawString(displayStrings[4], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // MEM

    y += 20; // Direct assignment (129 + 20)


    renderer->drawString(displayStrings[5], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // CPU real
    renderer->drawString(displayStrings[6], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // GPU real
    renderer->drawString(displayStrings[7], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // MEM real

    renderer->drawString(displayStrings[28], false, positions[2], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_CPU]);  // CPU Real Temp
    renderer->drawString(displayStrings[29], false, positions[3], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_GPU]);  // GPU Real Temp
    renderer->drawString(displayStrings[30], false, positions[4], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_MEM]);  // RAM Real Temp

    // === REAL FREQUENCIES ===

    y += 20; // Direct assignment (149 + 20)

    // === VOLTAGES ===
    renderer->drawString(displayStrings[8], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // CPU voltage
    renderer->drawString(displayStrings[9], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // GPU voltage

    renderer->drawStringWithColoredSections(displayStrings[10], false, {""}, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor, tsl::separatorColor);

    renderer->drawString(displayStrings[19], false, positions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // CPU Usage
    renderer->drawString(displayStrings[17], false, positions[3], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // GPU Usage
    renderer->drawString(displayStrings[18], false, positions[4], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // RAM Usage

    y += 22; // Direct assignment (169 + 22)

    // === TEMPERATURE SECTION ===
    // Labels
    renderer->drawString(labels[5], false, positions[5], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[6], false, positions[6], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[7], false, positions[7], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);

    // Temperatures with color - use pre-computed colors
    renderer->drawString(displayStrings[11], false, dataPositions[0] - 1, y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_SOC]);  // SOC
    renderer->drawString(displayStrings[12], false, dataPositions[1], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_PCB]);  // PCB
    renderer->drawString(displayStrings[13], false, dataPositions[2], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_Skin]);  // Skin

    y += 20; // Direct assignment (191 + 20)

    renderer->drawString(displayStrings[14], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor); // SOC voltage

    // Power labels and values
    renderer->drawString(labels[8], false, positions[8]-1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[9], false, positions[9], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);

    renderer->drawString(displayStrings[15], false, dataPositions[3], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // Power now
    renderer->drawString(displayStrings[16], false, dataPositions[4], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // Power avg

    y+=20;

    renderer->drawString(labels[10], false, positions[2], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);

    renderer->drawString(displayStrings[20], false, dataPositions[0], y, SMALL_TEXT_SIZE, tempColors[HocClkThermalSensor_Battery]);  // Battery

    renderer->drawString(labels[12], false, positions[3], y, SMALL_TEXT_SIZE, tsl::sectionTextColor); // fan label

    renderer->drawString(displayStrings[24], false, dataPositions[1] + 5, y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // fan speed

    renderer->drawString(labels[13], false, positions[4] + 4, y, SMALL_TEXT_SIZE, tsl::sectionTextColor); // disp label

    renderer->drawString(displayStrings[25], false, dataPositions[2] + 6, y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // disp freq

    y+=20;

    renderer->drawString(displayStrings[21], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // Bat voltage
    renderer->drawString(displayStrings[23], false, positions[2] - 2, y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // Bat Age

    if(this->context->isSaltyNXInstalled) {

        renderer->drawString(labels[15], false, positions[3] + 7, y, SMALL_TEXT_SIZE, tsl::sectionTextColor); // RES label
        renderer->drawString(displayStrings[27], false, dataPositions[1] + 5, y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // RES

        renderer->drawString(labels[14], false, positions[4] + 9, y, SMALL_TEXT_SIZE, tsl::sectionTextColor); // FPS label
        renderer->drawString(displayStrings[26], false, dataPositions[2] + 6, y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // FPS

    }

    y+=20;
}

// === Multi-rate refresh ==================================================
// - Frame tick (каждый кадр): шаг EMA/slew по каналам, пересборка строк с
//   гистерезисом — UI «жидкий», но CPU-дешёвый.
// - Telemetry (~15 Гц): чтение HocClkContext через IPC и тик Living Ladder.
// - Config (~1 Гц): перечитывание настроек HOC.
// Такое разделение убирает «дёрганье» цифр и снижает IPC-нагрузку.
void BaseMenuGui::refresh()
{
    const u64 ticks  = armGetSystemTick();
    const u64 now_ns = armTicksToNs(ticks);

    // --- 1) per-frame: шагнуть EMA/slew ------------------------------------
    if (this->context) {
        const u64 dt_ticks = (this->lastFrameTicks != 0) ? (ticks - this->lastFrameTicks) : 0;
        this->lastFrameTicks = ticks;
        float dt = dt_ticks ? (armTicksToNs(dt_ticks) / 1e9f) : 0.0f;
        if (dt > 0.25f) dt = 0.25f;   // anti-pause: не выпускать джампы после сворачивания
        if (dt > 0.0f) {
            auto& s = this->smooth;
            s.cpuHz.step(dt);    s.gpuHz.step(dt);    s.memHz.step(dt);
            s.cpuRealHz.step(dt);s.gpuRealHz.step(dt);s.memRealHz.step(dt);
            s.cpuLoad.step(dt);  s.gpuLoad.step(dt); s.memLoad.step(dt);
            s.batLoad.step(dt);  s.fanLoad.step(dt);
            s.socTemp.step(dt);  s.pcbTemp.step(dt); s.skinTemp.step(dt);
            s.cpuTemp.step(dt);  s.gpuTemp.step(dt); s.memTemp.step(dt); s.batTemp.step(dt);
            s.cpuVolt.step(dt);  s.gpuVolt.step(dt); s.socVolt.step(dt);
            s.emcVolt.step(dt);  s.batVolt.step(dt);
            s.powerNow.step(dt); s.powerAvg.step(dt);
            s.displayHz.step(dt);
            s.fps.step(dt, now_ns);
        }
    } else {
        this->lastFrameTicks = ticks;
    }

    // --- 2) telemetry @ 5 Hz (+ config @ 1 Hz) -----------------------------
    bool haveFreshTelemetry = false;
    const bool telemetryDue = (this->lastTelemetryNs == 0) ||
                              (now_ns - this->lastTelemetryNs >= 66'666'667ULL);
    if (telemetryDue) {
        this->lastTelemetryNs  = now_ns;
        this->lastContextUpdate = ticks;   // совместимость с прежними вызовами.

        if (!this->context) [[unlikely]] this->context = new HocClkContext;

        Result rc = hocclkIpcGetCurrentContext(this->context);
        if (R_FAILED(rc)) [[unlikely]] {
            FatalGui::openWithResultCode("hocclkIpcGetCurrentContext", rc);
            return;
        }
        haveFreshTelemetry = true;

        if (this->lastConfigNs == 0 || now_ns - this->lastConfigNs >= 1'000'000'000ULL) {
            this->lastConfigNs = now_ns;
            rc = hocclkIpcGetConfigValues(&configList);
            if (R_FAILED(rc)) [[unlikely]] {
                FatalGui::openWithResultCode("hocclkIpcGetConfigValues", rc);
                return;
            }
        }

        // Feed каналы сырыми данными из IPC-контекста.
        auto& s = this->smooth;
        auto feedTemp = [](SmoothedChannel& c, u32 milli) { c.feedRaw((float)milli); };
        s.cpuHz.feedRaw((float)context->freqs[HocClkModule_CPU]);
        s.gpuHz.feedRaw((float)context->freqs[HocClkModule_GPU]);
        s.memHz.feedRaw((float)context->freqs[HocClkModule_MEM]);
        s.cpuRealHz.feedRaw((float)context->realFreqs[HocClkModule_CPU]);
        s.gpuRealHz.feedRaw((float)context->realFreqs[HocClkModule_GPU]);
        s.memRealHz.feedRaw((float)context->realFreqs[HocClkModule_MEM]);
        s.cpuLoad.feedRaw(context->partLoad[HocClkPartLoad_CPUMax] / 10.0f);
        s.gpuLoad.feedRaw(context->partLoad[HocClkPartLoad_GPU]    / 10.0f);
        s.memLoad.feedRaw(context->partLoad[HocClkPartLoad_EMC]    / 10.0f);
        s.batLoad.feedRaw(context->partLoad[HocClkPartLoad_BAT]    / 1000.0f);
        s.fanLoad.feedRaw((float)context->partLoad[HocClkPartLoad_FAN]);
        feedTemp(s.socTemp,  context->temps[HocClkThermalSensor_SOC]);
        feedTemp(s.pcbTemp,  context->temps[HocClkThermalSensor_PCB]);
        feedTemp(s.skinTemp, context->temps[HocClkThermalSensor_Skin]);
        feedTemp(s.cpuTemp,  context->temps[HocClkThermalSensor_CPU]);
        feedTemp(s.gpuTemp,  context->temps[HocClkThermalSensor_GPU]);
        feedTemp(s.memTemp,  context->temps[HocClkThermalSensor_MEM]);
        feedTemp(s.batTemp,  context->temps[HocClkThermalSensor_Battery]);
        s.cpuVolt.feedRaw((float)context->voltages[HocClkVoltage_CPU]);
        s.gpuVolt.feedRaw((float)context->voltages[HocClkVoltage_GPU]);
        s.socVolt.feedRaw((float)context->voltages[HocClkVoltage_SOC]);
        const u32 emcV = (configList.values[HocClkConfigValue_RAMVoltDisplayMode] == RamDisplayMode_VDDQ)
                          ? context->voltages[HocClkVoltage_EMCVDDQ]
                          : context->voltages[HocClkVoltage_EMCVDD2];
        s.emcVolt.feedRaw((float)emcV);
        s.batVolt.feedRaw((float)context->voltages[HocClkVoltage_Battery]);
        s.powerNow.feedRaw((float)context->power[0]);
        s.powerAvg.feedRaw((float)context->power[1]);
        s.displayHz.feedRaw((float)context->realFreqs[HocClkModule_Display]);
        s.fps.feed(context->fps, now_ns);

        // Ladder tick УБРАН — живёт в sysmodule (auto_ryazha.cpp) и работает
        // поверх игры с закрытым оверлеем. Мы теперь только рисуем HUD.
    }

    if (!this->context) return;
    if (!haveFreshTelemetry) {
        // Strings всё равно пересобираем, но только по dirty-каналам внизу.
    }

    // === FORMAT DISPLAY STRINGS =============================================
    // App ID (hex conversion) — статично, меняется редко; но дёшево.
    sprintf(displayStrings[0], "%016lX", context->applicationId);

    // Profile
    strcpy(displayStrings[1], hocclkFormatProfile(context->profile, true));
    if (std::strcmp(displayStrings[1], "Docked") == 0) {
        std::strcpy(displayStrings[1], "Док");
    } else if (std::strcmp(displayStrings[1], "Handheld") == 0) {
        std::strcpy(displayStrings[1], "Портатив");
    } else if (std::strcmp(displayStrings[1], "Charging") == 0) {
        std::strcpy(displayStrings[1], "Зарядка");
    } else if (std::strcmp(displayStrings[1], "USB Charger") == 0) {
        std::strcpy(displayStrings[1], "USB зарядка");
    } else if (std::strcmp(displayStrings[1], "PD Charger") == 0) {
        std::strcpy(displayStrings[1], "PD зарядка");
    }

    // Current frequencies — источником значений служат жидкие каналы,
    // поэтому переходы между шагами DVFS проезжают через промежуточные
    // МГц "счётчиком", а не мгновенно.
    auto& smF = this->smooth;

    auto formatFreqMHz = [](char* dst, float hzVal) {
        const u32 hz = (u32)std::max(0.0f, hzVal);
        sprintf(dst, "%u.%u МГц", hz / 1000000U, (hz / 100000U) % 10U);
    };

    formatFreqMHz(displayStrings[2], smF.cpuHz.value());
    formatFreqMHz(displayStrings[3], smF.gpuHz.value());

    // MEM target.
    {
        const std::uint32_t unit = configList.values[HocClkConfigValue_RamDisplayUnit];
        const u32 hzT = (u32)std::max(0.0f, smF.memHz.value());
        const u32 hzR = (u32)std::max(0.0f, smF.memRealHz.value());
        u32 mhz   = hzT / 1000000U;
        u32 mts   = mhz * 2;
        u32 tenth = (hzT / 100000U) % 10U;
        if (unit == RamDisplayUnit_MTs) {
            sprintf(displayStrings[4], "%u МТ/с", mts);
        } else if (unit == RamDisplayUnit_MHz) {
            sprintf(displayStrings[4], "%u.%u МГц", mhz, tenth);
        } else {
            // MHzMTs: таргет рисуется как real.
            mhz   = hzR / 1000000U;
            tenth = (hzR / 100000U) % 10U;
            sprintf(displayStrings[4], "%u.%u МГц", mhz, tenth);
        }
    }

    // Real frequencies через сглаженные каналы.
    formatFreqMHz(displayStrings[5], smF.cpuRealHz.value());
    formatFreqMHz(displayStrings[6], smF.gpuRealHz.value());

    {
        const std::uint32_t unit = configList.values[HocClkConfigValue_RamDisplayUnit];
        const u32 hzR = (u32)std::max(0.0f, smF.memRealHz.value());
        const u32 mhz = hzR / 1000000U;
        const u32 mts = mhz * 2;
        const u32 tenth = (hzR / 100000U) % 10U;
        if (unit == RamDisplayUnit_MTs || unit == RamDisplayUnit_MHzMTs)
            sprintf(displayStrings[7], "%u МТ/с", mts);
        else
            sprintf(displayStrings[7], "%u.%u МГц", mhz, tenth);
    }

    // Жидкие каналы -> форматирование. displayStrings пересобираем всегда,
    // это быстро; дёрганье UI на резких шумах уже гасится EMA+slew внутри каналов.
    auto& sm = this->smooth;

    // Voltages (из uV → mV). Квантуем к 0.1 mV; гистерезис уже заложен в epsilon.
    {
        const float mv_cpu = sm.cpuVolt.value() / 1000.0f;
        const float mv_gpu = sm.gpuVolt.value() / 1000.0f;
        sprintf(displayStrings[8], "%.1f мВ", mv_cpu);
        sprintf(displayStrings[9], "%.1f мВ", mv_gpu);
    }

    switch(configList.values[HocClkConfigValue_RAMVoltDisplayMode]) {
        case RamDisplayMode_VDD2:
        case RamDisplayMode_VDDQ: {
            const u32 uv = (u32)std::max(0.0f, sm.emcVolt.value());
            sprintf(displayStrings[10], "%u.%u мВ", uv / 1000U, (uv % 1000U) / 100U);
            break;
        }
        default:
            strcpy(displayStrings[10], "Н/Д");
            break;
    }

    // Temperatures — цвет через гистерезисные пороги (warning/danger).
    auto setTempString = [&](int idx, HocClkThermalSensor sensor, float milliDeg, bool withUnit) {
        const u32 mi = (u32)std::max(0.0f, milliDeg);
        if (withUnit) sprintf(displayStrings[idx], "%u.%u °C", mi / 1000U, (mi % 1000U) / 100U);
        else          sprintf(displayStrings[idx], "%u.%u",    mi / 1000U, (mi % 1000U) / 100U);
        const float deg = milliDeg * 0.001f;
        const int state = sm.tempColor.update(deg);
        // 0 = ok, 1 = warn, 2 = danger. GradientColor сам рассчитывает плавный цвет,
        // но мы используем его как «базу» и не переключаемся при болтанке ±0.1°C.
        tsl::Color c = tsl::GradientColor(deg);
        if (state == 2)      c = tsl::Color(15, 0, 0, 15);      // danger
        else if (state == 1) c = tsl::Color(15, 10, 0, 15);     // warn
        tempColors[sensor] = c;
    };

    setTempString(11, HocClkThermalSensor_SOC,     sm.socTemp.value(),  true);
    setTempString(12, HocClkThermalSensor_PCB,     sm.pcbTemp.value(),  true);
    setTempString(13, HocClkThermalSensor_Skin,    sm.skinTemp.value(), true);
    setTempString(20, HocClkThermalSensor_Battery, sm.batTemp.value(),  true);
    setTempString(28, HocClkThermalSensor_CPU,     sm.cpuTemp.value(),  false);
    setTempString(29, HocClkThermalSensor_GPU,     sm.gpuTemp.value(),  false);
    setTempString(30, HocClkThermalSensor_MEM,     sm.memTemp.value(),  false);

    // SOC voltage (cached).
    sprintf(displayStrings[14], "%u мВ", (u32)std::max(0.0f, sm.socVolt.value() / 1000.0f));

    // Power (mW). Округляем к целому после EMA.
    sprintf(displayStrings[15], "%d мВт", (int)std::lround(sm.powerNow.value()));
    sprintf(displayStrings[16], "%d мВт", (int)std::lround(sm.powerAvg.value()));

    // Loads (%). Целые после EMA (+1 единица epsilon в конфиге канала).
    sprintf(displayStrings[17], "%u%%", (unsigned)std::max(0, (int)std::lround(sm.gpuLoad.value())));
    sprintf(displayStrings[18], "%u%%", (unsigned)std::max(0, (int)std::lround(sm.memLoad.value())));
    sprintf(displayStrings[19], "%u%%", (unsigned)std::max(0, (int)std::lround(sm.cpuLoad.value())));

    // Battery voltage (mV).
    sprintf(displayStrings[21], "%d мВ", (int)std::lround(sm.batVolt.value()));

    sprintf(displayStrings[23], "%u%%", (unsigned)std::max(0, (int)std::lround(sm.batLoad.value())));
    sprintf(displayStrings[24], "%u%%", (unsigned)std::max(0, (int)std::lround(sm.fanLoad.value())));

    sprintf(displayStrings[25], "%u Гц", (unsigned)std::max(0, (int)std::lround(sm.displayHz.value())));

    if (this->context->isSaltyNXInstalled) {
        if (!sm.fps.hasAny()) {
            strcpy(displayStrings[26], "Н/Д");
        } else {
            const int fps = (int)std::lround(sm.fps.value());
            const int clamped = std::max(0, std::min(999, fps));
            sprintf(displayStrings[26], "%d", clamped);
        }

        if (context->resolutionHeight == 0) {
            strcpy(displayStrings[27], "Н/Д");
        } else {
            sprintf(displayStrings[27], "%up", context->resolutionHeight);
        }
    }
}

tsl::elm::Element* BaseMenuGui::baseUI()
{
    auto* list = new tsl::elm::List();
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer*, s32, s32, s32, s32) {}), 35); // add a bit of space
    this->listElement = list;
    this->listUI();

    return list;
}