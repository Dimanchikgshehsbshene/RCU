/*
 * Ползунок частоты дисплея: Y — сброс к 60 Гц; синхронизация потолка VRR Ryazha-Авто.
 * Важно: у StepTrackBar должно быть минимум 2 шага, иначе деление на (m_numSteps-1) ломает ввод.
 */
#pragma once

#include <algorithm>
#include <string>

#include <hocclk/auto_ryazha.h>
#include <tesla.hpp>
#include <tsl_utils.hpp>

#include "living_ladder.h"

namespace ryazha_ui {

inline constexpr u32 kDefaultPanelDisplayHz = 60;

/** 0 в конфиге / профиле / оверрайде трактуем как «по умолчанию панель» — 60 Гц. */
inline u32 displayHzOrDefault(u32 hz) {
    return hz == 0 ? kDefaultPanelDisplayHz : hz;
}

inline u32 quantizeDisplayHz(u32 hz, u32 minHz, u32 maxHz, u32 stepHz) {
    hz = std::clamp(hz, minHz, maxHz);
    if (stepHz <= 1) return hz;
    u32 off = (hz - minHz) % stepHz;
    if (off != 0) hz -= off;
    return std::max(minHz, hz);
}

inline u16 displayHzToProgress(u32 hz, u32 minHz, u32 maxHz, u32 stepHz) {
    hz = quantizeDisplayHz(hz, minHz, maxHz, stepHz);
    return (u16)((hz - minHz) / stepHz);
}

inline bool ladderVrrCoreActive(u8 mode) {
    if (mode == HocClkLadderVrr_On || mode == HocClkLadderVrr_SuperPro)
        mode = HocClkLadderVrr_Auto;
    return mode != HocClkLadderVrr_Off;
}

inline void syncLadderVrrMaxToPanelHz(u32 hz) {
    livingLadder().pull();
    auto& c = livingLadder().config();
    if (!ladderVrrCoreActive(c.vrrMode)) return;
    u8 cap = (u8)std::min<u32>(hz, 255u);
    c.vrrMaxHz = std::max(c.vrrMinHz, cap);
    livingLadder().push();
}

inline void syncLadderVrrMaxToHocCap(u32 maxCapHz) {
    livingLadder().pull();
    auto& c = livingLadder().config();
    if (!ladderVrrCoreActive(c.vrrMode)) return;
    u8 cap = (u8)std::min<u32>(maxCapHz, 255u);
    if (c.vrrMaxHz > cap) c.vrrMaxHz = std::max(c.vrrMinHz, cap);
    livingLadder().push();
}

class DisplayHzTrackBar : public tsl::elm::NamedStepTrackBar {
    u32 m_minHz = 40;
    u32 m_maxHz = 60;
    u32 m_stepHz = 1;

public:
    DisplayHzTrackBar(u32 minHz, u32 maxHz, u32 stepHz, const std::string& label)
        : tsl::elm::NamedStepTrackBar("", {""}, true, label) {
        m_stepDescriptions.clear();

        u32 st = stepHz ? stepHz : 1u;
        u32 lo = minHz;
        u32 hi = maxHz;

        if (hi == 0) {
            hi = std::max(lo + st, kDefaultPanelDisplayHz);
        }
        if (lo == 0 && hi > 0) {
            lo = 40;
        }
        if (hi < lo) std::swap(lo, hi);
        if (hi <= lo) hi = lo + st;

        /* libtesla StepTrackBar двигает m_value шагами 100/(m_numSteps-1); при n>101 это целочисленно 0 — ввод ломается. */
        u32 n = (hi - lo) / st + 1;
        if (n < 2) {
            hi = lo + st;
            n = 2;
        }
        while (n > 101u) {
            ++st;
            n = (hi - lo) / st + 1;
            if (n < 2) {
                hi = lo + st;
                n = 2;
            }
        }

        m_minHz = lo;
        m_maxHz = lo + (n - 1) * st;
        m_stepHz = st;

        m_stepDescriptions.reserve(n);
        for (u32 i = 0; i < n; i++) {
            m_stepDescriptions.push_back(std::to_string(lo + i * st) + " Гц");
        }
        m_numSteps = (u8)m_stepDescriptions.size();
        if (m_numSteps < 2) {
            m_stepDescriptions.clear();
            m_stepDescriptions.push_back(std::to_string(lo) + " Гц");
            m_stepDescriptions.push_back(std::to_string(lo + st) + " Гц");
            m_maxHz = lo + st;
            m_numSteps = 2;
        }
        m_selection = m_stepDescriptions[0];
    }

    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                     HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick) override {
        if (this->hasFocus() && (keysDown & KEY_Y)) {
            u16 p = displayHzToProgress(kDefaultPanelDisplayHz, m_minHz, m_maxHz, m_stepHz);
            NamedStepTrackBar::setProgress(p);
            this->m_valueChangedListener(p);
            return true;
        }
        return tsl::elm::NamedStepTrackBar::handleInput(keysDown, keysHeld, touchPos, leftJoyStick, rightJoyStick);
    }

    u32 minHz() const { return m_minHz; }
    u32 maxHz() const { return m_maxHz; }
    u32 stepHz() const { return m_stepHz; }
};

} // namespace ryazha_ui
