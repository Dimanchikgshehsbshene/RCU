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

// Жидкие счётчики (ТЗ 5.x):
//   - EMA с отдельными alpha для rise/fall (чтобы спад не обрывался).
//   - Slew limiter (ограничение скорости изменения в ед./сек).
//   - Hysteresis по epsilon на текст (строка не «мигает»).
//   - FpsStabilized: hold + fade для случая fps==N/A, чтобы цифра не падала в 0.

#pragma once

#include <cmath>
#include <cstdint>

struct SmoothConfig {
    float alphaUp        = 0.30f;   // EMA-коэффициент при росте
    float alphaDown      = 0.15f;   // EMA-коэффициент при падении
    float slewUpPerSec   = 1e9f;    // макс. прирост displayValue в единицах/сек
    float slewDownPerSec = 1e9f;    // макс. падение displayValue в единицах/сек
    float epsilon        = 0.0f;    // порог «дёргается текст» (ниже — UI не перерисовывается)
};

class SmoothedChannel {
public:
    SmoothedChannel() = default;
    explicit SmoothedChannel(const SmoothConfig& cfg) : cfg_(cfg) {}

    void setConfig(const SmoothConfig& cfg) { cfg_ = cfg; }

    void reset() {
        init_      = false;
        ema_       = 0.0f;
        display_   = 0.0f;
        lastShown_ = -1e30f;
    }

    // Скармливаем сырой замер (на частоте телеметрии 5–10 Гц).
    void feedRaw(float v) { raw_ = v; hasRaw_ = true; }

    // Продвигаем EMA + slew на dt сек (вызывается каждый кадр).
    void step(float dtSec) {
        if (!hasRaw_) return;
        if (!init_) {
            ema_ = display_ = raw_;
            init_ = true;
            return;
        }
        const float alpha = (raw_ > ema_) ? cfg_.alphaUp : cfg_.alphaDown;
        ema_ += alpha * (raw_ - ema_);

        const float delta      = ema_ - display_;
        const float slewRate   = (delta >= 0.0f) ? cfg_.slewUpPerSec : cfg_.slewDownPerSec;
        const float maxDelta   = slewRate * dtSec;
        const float absDelta   = std::fabs(delta);
        if (absDelta <= maxDelta) {
            display_ = ema_;
        } else {
            display_ += std::copysign(maxDelta, delta);
        }
    }

    float value() const { return display_; }
    float raw()   const { return raw_; }
    bool  isInit() const { return init_; }

    // true → UI должен перерисовать текст.
    bool takeDirty() {
        if (!init_) return false;
        if (std::fabs(display_ - lastShown_) > cfg_.epsilon) {
            lastShown_ = display_;
            return true;
        }
        return false;
    }

private:
    SmoothConfig cfg_{};
    float raw_      = 0.0f;
    float ema_      = 0.0f;
    float display_  = 0.0f;
    float lastShown_= -1e30f;
    bool  init_     = false;
    bool  hasRaw_   = false;
};

// FPS специальный случай: 254 в HOC == «нет данных».
// Держим последнее известное значение holdMs, потом плавно уводим к 0 за fadeMs,
// и только после этого отпускаем канал в N/A.
class FpsStabilizer {
public:
    FpsStabilizer() {
        // FPS «подвижное» значение: чуть быстрее EMA, epsilon 0.3 fps.
        cfg_.alphaUp        = 0.40f;
        cfg_.alphaDown      = 0.25f;
        cfg_.slewUpPerSec   = 240.0f;  // не даём подскочить на полэкрана
        cfg_.slewDownPerSec = 120.0f;
        cfg_.epsilon        = 0.3f;
        sm_.setConfig(cfg_);
    }

    void setHoldFadeMs(uint32_t hold, uint32_t fade) {
        holdNs_ = (uint64_t)hold * 1'000'000ULL;
        fadeNs_ = (uint64_t)fade * 1'000'000ULL;
    }

    void feed(uint8_t rawFps, uint64_t nowNs) {
        if (rawFps == 254u) {
            if (hasGood_ && staleAt_ == 0) staleAt_ = nowNs;
        } else {
            hasGood_  = true;
            lastGood_ = rawFps;
            staleAt_  = 0;
            sm_.feedRaw((float)rawFps);
        }
    }

    void step(float dtSec, uint64_t nowNs) {
        if (staleAt_ != 0) {
            const uint64_t heldNs = nowNs - staleAt_;
            if (heldNs < holdNs_) {
                sm_.feedRaw((float)lastGood_);
            } else if (heldNs < holdNs_ + fadeNs_) {
                const float t = float(heldNs - holdNs_) / float(fadeNs_);
                sm_.feedRaw((float)lastGood_ * (1.0f - t));
            } else {
                sm_.reset();
                hasGood_ = false;
                staleAt_ = 0;
                return;
            }
        }
        if (hasGood_) sm_.step(dtSec);
    }

    bool  available() const { return hasGood_ && staleAt_ == 0; }
    bool  hasAny()    const { return hasGood_; }
    float value()     const { return sm_.value(); }
    bool  takeDirty()       { return sm_.takeDirty(); }

private:
    SmoothConfig cfg_{};
    SmoothedChannel sm_;
    uint8_t  lastGood_ = 0;
    bool     hasGood_  = false;
    uint64_t staleAt_  = 0;
    uint64_t holdNs_   = 2'000'000'000ULL;   // 2 сек держим «последнее хорошее»
    uint64_t fadeNs_   = 500'000'000ULL;     // 0.5 сек плавно к 0
};

// Гистерезис по порогу (для цветов: warning/danger).
// Состояние переключается только когда значение перешло порог + hysteresis.
class ThresholdHysteresis {
public:
    ThresholdHysteresis(float warnUp, float warnDn,
                        float dangerUp, float dangerDn)
        : wUp_(warnUp), wDn_(warnDn), dUp_(dangerUp), dDn_(dangerDn) {}

    // 0 = ok, 1 = warning, 2 = danger.
    int update(float v) {
        if (state_ == 0) {
            if (v >= dUp_) state_ = 2;
            else if (v >= wUp_) state_ = 1;
        } else if (state_ == 1) {
            if (v >= dUp_) state_ = 2;
            else if (v < wDn_) state_ = 0;
        } else { // 2
            if (v < dDn_) state_ = (v >= wDn_) ? 1 : 0;
        }
        return state_;
    }
    int state() const { return state_; }

private:
    float wUp_, wDn_, dUp_, dDn_;
    int state_ = 0;
};
