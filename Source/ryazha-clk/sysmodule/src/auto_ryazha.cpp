/*
 * Copyright (c) Souldbminer, Lightos_ and Ryazha-CLK Contributors
 *
 * Sysmodule-side реализация Ryazha-Авто. Полностью самостоятельный поток,
 * который каждые ~200 мс:
 *   - берёт текущий context через clockManager::GetCurrentContext();
 *   - читает актуальные RClkConfigValueList через config::GetConfigValues();
 *   - прогоняет логику ladder (FPS up/down, TDP/thermal throttle, RAM pin);
 *   - применяет новые частоты CPU/GPU/RAM/Display через config::SetOverrideHz.
 *
 * Конфиг ladder хранится в /config/ryazha-clk/ryazha-auto.ini и защищён
 * мьютексом, чтобы IPC-поток overlay'а мог писать его одновременно с
 * tick-ом.
 *
 */

#include "auto_ryazha.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <sys/stat.h>
#include <vector>

#include <nxExt/cpp/lockable_mutex.h>

#include "mgr/clock_manager.hpp"
#include "file/config.hpp"
#include "file/errors.hpp"
#include "file/file_utils.hpp"
#include "board/board.hpp"
#include "display/display_refresh_rate.hpp"

namespace autoRyazha {

namespace {

constexpr u64  kTickIntervalNs     = 200'000'000ULL;     // 5 Hz
constexpr u64  kTableRefreshNs     = 10ULL * 1'000'000'000ULL;
constexpr u8   kFpsUnavailable     = 254;
constexpr u32  kHoldUpCycle        = 2;
constexpr u32  kHoldUpPro          = 2;
constexpr u32  kHoldDownCycle      = 6;
constexpr u32  kHoldDownPro        = 8;
constexpr u32  kThermCooldown      = 5;
/** Короткая пауза перед опросом телеметрии (Вт / FPS и т.д.), чтобы выбор шага лестницы шёл по чуть более «успевшим» данным. */
constexpr u64  kTelemetrySettleNs  = 500'000ULL;       // 0.5 ms
constexpr const char* kConfigPath  = "/config/ryazha-clk/ryazha-auto.ini";

inline int clampInt(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

// Как HandleSafetyFeatures: fuel gauge даёт отрицательные mW при разряде; лимит сравниваем с «-lim».
bool PowerOverTdpMw(s32 pMw, u64 limitMw) {
    if (limitMw == 0) return false;
    const s32 lim = (s32)std::min<u64>(limitMw, 0x7FFFFFFFULL);
    if (lim <= 0) return false;
    return pMw < -lim;
}

// Hardware panel max refresh.
u8 PanelMaxHz() {
    static u8 cached = 0;
    if (cached != 0) return cached;
    SetSysProductModel m = SetSysProductModel_Invalid;
    Result rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        (void)setsysGetProductModel(&m);
        setsysExit();
    }
    cached = (m == SetSysProductModel_Aula) ? 65 : 75;
    return cached;
}

struct RuntimeState {
    RClkLadderConfig cfg{};
    LockableMutex      cfgMutex;

    // Freq tables (из clockManager).
    std::vector<u32> cpuTable, gpuTable, ramTable;
    bool  tablesValid = false;
    u64   lastTableNs = 0;

    // Steps (индексы в соответствующих таблицах).
    int   cpuStep = -1, gpuStep = -1;

    // Cycle cursors.
    u32   upCycleCnt   = 0;
    bool  downCpuFirst = true;
    bool  tdpCpuFirst  = true;

    // Hold / cooldown.
    u32   holdUpTicks     = 0;
    u32   holdDownTicks   = 0;
    u32   fpsLowStreak    = 0;
    u32   thermCooldown   = 0;

    // RAM pin.
    bool  ramPinned       = false;

    // VRR runtime.
    u8    lastVrrHz       = 0;
    bool  vrrOverrideActive = false;
    u32   vrrDownHold     = 0;
    std::deque<u8> fpsWindow;
    std::deque<u8> ladderFpsWindow;
    bool  vrrConfigApplied = false;

    std::atomic<bool> gRunning{false};
    Thread            gThread{};
};

RuntimeState g;

void DefaultConfig(RClkLadderConfig& c) {
    std::memset(&c, 0, sizeof(c));
    c.enabled              = 0;
    c.targetFps            = 60;
    c.tolerance            = 2;
    c.algo                 = RClkLadderAlgo_Cycle;
    c.tempCapCpuMillideg   = 82000;
    c.tempCapGpuMillideg   = 78000;
    c.tdpCapMw             = 0;
    c.vrrMode              = RClkLadderVrr_Off;
    c.vrrMinHz             = 45;
    c.vrrMaxHz             = 65;
    c.predictorEnable      = 1;
    c.predictorWindowTicks = 4;
    c.predictorAggressive  = 6;
    c.predictorSpikeFps    = 8;
    c.oledAutoGamma        = (board::GetConsoleType() == RClkConsoleType_Aula) ? 1 : 0;
}

// === Persistence =========================================================
void SaveConfigLocked() {
    ::mkdir("/config", 0777);
    ::mkdir("/config/ryazha-clk", 0777);

    FILE* f = std::fopen(kConfigPath, "w");
    if (!f) return;
    auto w = [f](const char* k, u64 v) {
        std::fprintf(f, "%s=%llu\n", k, (unsigned long long)v);
    };
    w("enabled",              g.cfg.enabled);
    w("targetFps",            g.cfg.targetFps);
    w("tolerance",            g.cfg.tolerance);
    w("algo",                 g.cfg.algo);
    w("tempCapCpuMillideg",   g.cfg.tempCapCpuMillideg);
    w("tempCapGpuMillideg",   g.cfg.tempCapGpuMillideg);
    w("tdpCapMw",             g.cfg.tdpCapMw);
    w("userMinCpuHz",         g.cfg.userMinCpuHz);
    w("userMaxCpuHz",         g.cfg.userMaxCpuHz);
    w("userMinGpuHz",         g.cfg.userMinGpuHz);
    w("userMaxGpuHz",         g.cfg.userMaxGpuHz);
    w("vrrMode",              g.cfg.vrrMode);
    w("vrrMinHz",             g.cfg.vrrMinHz);
    w("vrrMaxHz",             g.cfg.vrrMaxHz);
    w("predictorEnable",      g.cfg.predictorEnable);
    w("predictorWindowTicks", g.cfg.predictorWindowTicks);
    w("predictorAggressive",  g.cfg.predictorAggressive);
    w("predictorSpikeFps",    g.cfg.predictorSpikeFps);
    w("oledAutoGamma",        g.cfg.oledAutoGamma);
    std::fclose(f);
}

void LoadConfigLocked() {
    DefaultConfig(g.cfg);
    FILE* f = std::fopen(kConfigPath, "r");
    if (!f) return;

    char line[256];
    while (std::fgets(line, sizeof(line), f)) {
        char* eq = std::strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        const char* key = line;
        const u64 v = std::strtoull(eq + 1, nullptr, 10);

        if      (!std::strcmp(key, "enabled"))              g.cfg.enabled              = (u8)v;
        else if (!std::strcmp(key, "targetFps"))            g.cfg.targetFps            = (u8)v;
        else if (!std::strcmp(key, "tolerance"))            g.cfg.tolerance            = (u8)v;
        else if (!std::strcmp(key, "algo"))                 g.cfg.algo                 = (u8)(v % RClkLadderAlgo_EnumMax);
        else if (!std::strcmp(key, "tempCapCpuMillideg"))   g.cfg.tempCapCpuMillideg   = (u32)v;
        else if (!std::strcmp(key, "tempCapGpuMillideg"))   g.cfg.tempCapGpuMillideg   = (u32)v;
        else if (!std::strcmp(key, "tdpCapMw"))             g.cfg.tdpCapMw             = (u32)v;
        else if (!std::strcmp(key, "userMinCpuHz"))         g.cfg.userMinCpuHz         = (u32)v;
        else if (!std::strcmp(key, "userMaxCpuHz"))         g.cfg.userMaxCpuHz         = (u32)v;
        else if (!std::strcmp(key, "userMinGpuHz"))         g.cfg.userMinGpuHz         = (u32)v;
        else if (!std::strcmp(key, "userMaxGpuHz"))         g.cfg.userMaxGpuHz         = (u32)v;
        else if (!std::strcmp(key, "vrrMode"))              g.cfg.vrrMode              = (u8)(v % RClkLadderVrr_EnumMax);
        else if (!std::strcmp(key, "vrrMinHz"))             g.cfg.vrrMinHz             = (u8)v;
        else if (!std::strcmp(key, "vrrMaxHz"))             g.cfg.vrrMaxHz             = (u8)v;
        else if (!std::strcmp(key, "predictorEnable"))      g.cfg.predictorEnable      = (u8)v;
        else if (!std::strcmp(key, "predictorWindowTicks")) g.cfg.predictorWindowTicks = (u8)v;
        else if (!std::strcmp(key, "predictorAggressive"))  g.cfg.predictorAggressive  = (u8)v;
        else if (!std::strcmp(key, "predictorSpikeFps"))    g.cfg.predictorSpikeFps    = (u8)v;
        else if (!std::strcmp(key, "oledAutoGamma"))        g.cfg.oledAutoGamma        = (u8)v;
    }
    std::fclose(f);

    if (g.cfg.targetFps < 30 || g.cfg.targetFps > 120) g.cfg.targetFps = 60;
    if (g.cfg.predictorWindowTicks == 0)               g.cfg.predictorWindowTicks = 4;
    if (g.cfg.predictorAggressive > 10)                g.cfg.predictorAggressive  = 6;
}

// === Freq tables =========================================================
void RefreshTables(u64 now_ns) {
    if (g.tablesValid && (now_ns - g.lastTableNs) < kTableRefreshNs) return;

    auto pull = [](RClkModule m, std::vector<u32>& out) {
        u32 raw[RCLK_FREQ_LIST_MAX];
        u32 cnt = 0;
        out.clear();
        clockManager::GetFreqList(m, raw, RCLK_FREQ_LIST_MAX, &cnt);
        if (cnt == 0) return;
        out.assign(raw, raw + cnt);
        std::sort(out.begin(), out.end());
    };
    pull(RClkModule_CPU, g.cpuTable);
    pull(RClkModule_GPU, g.gpuTable);
    pull(RClkModule_MEM, g.ramTable);
    g.tablesValid = !g.cpuTable.empty() && !g.gpuTable.empty();
    g.lastTableNs = now_ns;
}

int FindStep(const std::vector<u32>& t, u32 hz) {
    if (t.empty()) return -1;
    int best = 0;
    for (int i = 0; i < (int)t.size(); ++i) {
        if (t[i] <= hz) best = i;
        else            break;
    }
    return best;
}

bool StepCpu(int delta, int lo, int hi) {
    if (g.cpuTable.empty() || g.cpuStep < 0) return false;
    int target = clampInt(g.cpuStep + delta, lo, hi);
    if (target == g.cpuStep) return false;
    g.cpuStep = target;
    config::SetOverrideHz(RClkModule_CPU, g.cpuTable[g.cpuStep]);
    return true;
}
bool StepGpu(int delta, int lo, int hi) {
    if (g.gpuTable.empty() || g.gpuStep < 0) return false;
    int target = clampInt(g.gpuStep + delta, lo, hi);
    if (target == g.gpuStep) return false;
    g.gpuStep = target;
    config::SetOverrideHz(RClkModule_GPU, g.gpuTable[g.gpuStep]);
    return true;
}

void EnsureRamMax() {
    if (g.ramPinned || g.ramTable.empty()) return;
    config::SetOverrideHz(RClkModule_MEM, g.ramTable.back());
    g.ramPinned = true;
}

void ComputeBounds(const RClkLadderConfig& cfg,
                   int& cpuLo, int& cpuHi, int& gpuLo, int& gpuHi) {
    cpuLo = 0; cpuHi = (int)g.cpuTable.size() - 1;
    gpuLo = 0; gpuHi = (int)g.gpuTable.size() - 1;

    u64 maxCpuHz = 0;
    if (board::GetSocType() == RClkSocType_Mariko) {
        // mariko_cpu_max_clock may appear as MHz, kHz, or Hz depending on source.
        const u64 raw = config::GetConfigValue(KipConfigValue_marikoCpuMaxClock);
        if (raw >= 100000000ULL)      maxCpuHz = raw;             // already Hz
        else if (raw >= 10000ULL)     maxCpuHz = raw * 1000ULL;   // kHz -> Hz
        else if (raw > 0)             maxCpuHz = raw * 1000000ULL;// MHz -> Hz
        else                          maxCpuHz = 0;
    } else {
        // Erista UI ceiling is stored in MHz and needs a single scale-up.
        maxCpuHz = config::GetConfigValue(RClkConfigValue_EristaMaxCpuClock) * 1000000ULL;
    }
    if (maxCpuHz > 0 && maxCpuHz <= 1428000000ULL) {
        // Treat stale 1428 cap as invalid for auto ladder headroom.
        maxCpuHz = 0;
    }
    if (maxCpuHz > 0 && !g.cpuTable.empty()) {
        const int idx = FindStep(g.cpuTable, (u32)std::min<u64>(maxCpuHz, 0xFFFFFFFFULL));
        if (idx >= 0) cpuHi = std::min(cpuHi, idx);
    }
    if (cfg.userMinCpuHz > 0 && !g.cpuTable.empty()) cpuLo = std::max(cpuLo, FindStep(g.cpuTable, cfg.userMinCpuHz));
    if (cfg.userMaxCpuHz > 0 && !g.cpuTable.empty()) cpuHi = std::min(cpuHi, FindStep(g.cpuTable, cfg.userMaxCpuHz));
    if (cfg.userMinGpuHz > 0 && !g.gpuTable.empty()) gpuLo = std::max(gpuLo, FindStep(g.gpuTable, cfg.userMinGpuHz));
    if (cfg.userMaxGpuHz > 0 && !g.gpuTable.empty()) gpuHi = std::min(gpuHi, FindStep(g.gpuTable, cfg.userMaxGpuHz));

    if (cpuLo > cpuHi) cpuLo = cpuHi = clampInt(g.cpuStep >= 0 ? g.cpuStep : cpuHi, 0, (int)g.cpuTable.size() - 1);
    if (gpuLo > gpuHi) gpuLo = gpuHi = clampInt(g.gpuStep >= 0 ? g.gpuStep : gpuHi, 0, (int)g.gpuTable.size() - 1);
}

// === Cycle / Pro steps ===================================================
void DoCycleUp(int cpuLo, int cpuHi, int gpuLo, int gpuHi) {
    const u32 pos = g.upCycleCnt++;
    if (pos == 0)             StepGpu(+1, gpuLo, gpuHi);
    else if ((pos % 2) == 1)  StepCpu(+1, cpuLo, cpuHi);
    else                      StepGpu(+2, gpuLo, gpuHi);
}

bool DoCycleDown(int cpuLo, int cpuHi, int gpuLo, int gpuHi) {
    bool did;
    if (g.downCpuFirst) { did = StepCpu(-1, cpuLo, cpuHi); g.downCpuFirst = false; }
    else                { did = StepGpu(-1, gpuLo, gpuHi); g.downCpuFirst = true;  }
    return did;
}

void DoProUp(u8 cpuLoad, u8 gpuLoad, int err,
             int cpuLo, int cpuHi, int gpuLo, int gpuHi) {
    int dPri = 1, dSec = 0;
    if      (err >= 10) { dPri = 3; dSec = 1; }
    else if (err >= 5)  { dPri = 2; dSec = 1; }

    // "Pro" should be load-aware, but 90% hard-threshold made CPU dominate
    // too often in real games. Use relative pressure with a bias to GPU when
    // loads are close so we don't get stuck below useful GPU steps.
    const int loadDelta = (int)cpuLoad - (int)gpuLoad;
    const bool cpuPrimary = loadDelta >= 15;

    if (cpuPrimary) {
        StepCpu(+dPri, cpuLo, cpuHi);
        // Keep GPU advancing under heavy FPS deficit; otherwise CPU can race
        // ahead while GPU remains under-scaled.
        if (dSec > 0 || err >= 8) StepGpu(+std::max(1, dSec), gpuLo, gpuHi);
    } else {
        StepGpu(+dPri, gpuLo, gpuHi);
        if (dSec > 0) StepCpu(+dSec, cpuLo, cpuHi);
    }
}

bool DoProDown(u8 cpuLoad, u8 gpuLoad,
               int cpuLo, int cpuHi, int gpuLo, int gpuHi) {
    // Primary choice by relative load, with fallback to the other rail when
    // the preferred one is already at the lower bound.
    if (cpuLoad <= gpuLoad) {
        if (StepCpu(-1, cpuLo, cpuHi)) return true;
        return StepGpu(-1, gpuLo, gpuHi);
    }
    if (StepGpu(-1, gpuLo, gpuHi)) return true;
    return StepCpu(-1, cpuLo, cpuHi);
}

// === VRR =================================================================
void ClearVrrOverride() {
    if (!g.vrrOverrideActive) return;
    config::SetOverrideHz(RClkModule_Display, 0);
    g.vrrOverrideActive = false;
    g.lastVrrHz = 0;
}

static inline u8 NormalizeVrrMode(u8 mode) {
    // Legacy modes removed from UI: keep old INI values working by treating
    // them as Auto behavior on the sysmodule side.
    if (mode == RClkLadderVrr_On || mode == RClkLadderVrr_SuperPro) {
        return RClkLadderVrr_Auto;
    }
    return mode;
}

void ApplyVrr(const RClkLadderConfig& cfg, bool fpsKnown, u8 fps) {
    const u8 vrrMode = NormalizeVrrMode(cfg.vrrMode);
    if (vrrMode == RClkLadderVrr_Off) {
        ClearVrrOverride();
        g.vrrDownHold = 0;
        return;
    }

    u8 lo = cfg.vrrMinHz ? cfg.vrrMinHz : 40;
    u8 hi = cfg.vrrMaxHz ? cfg.vrrMaxHz : 60;
    // OLED panel behaves poorly below 43 Hz in practice; hard-clamp VRR floor.
    if (board::GetConsoleType() == RClkConsoleType_Aula) {
        lo = std::max<u8>(lo, 43);
    }
    // Hard cap by hardware panel max (Aula=65, LCD=75) so we never ask the
    // panel for a refresh it can't physically do.
    hi = std::min<u8>(hi, PanelMaxHz());
    if (lo > hi) std::swap(lo, hi);
    u8 hz;
    if (lo == hi) {
        hz = lo; g.vrrDownHold = 0;
    } else if (!fpsKnown) {
        hz = hi; g.vrrDownHold = 0;
    } else {
        u8 effFps = fps;
        if (cfg.predictorEnable) {
            g.fpsWindow.push_back(fps);
            const u8 maxWin = std::max<u8>(1, cfg.predictorWindowTicks);
            while (g.fpsWindow.size() > maxWin) g.fpsWindow.pop_front();
            u32 sum = 0; for (u8 v : g.fpsWindow) sum += v;
            const u32 avg = sum / g.fpsWindow.size();
            const int diff = std::abs((int)fps - (int)avg);
            if (diff > (int)cfg.predictorSpikeFps) {
                effFps = fps;
                g.fpsWindow.clear();
                g.fpsWindow.push_back(fps);
            } else {
                const float alpha = std::clamp<float>(cfg.predictorAggressive / 10.0f, 0.0f, 1.0f);
                effFps = (u8)std::lround(alpha * fps + (1.0f - alpha) * avg);
            }
        }

        const int margin = 2;
        u8 want = (u8)clampInt((int)effFps + margin, lo, hi);

        if (vrrMode == RClkLadderVrr_Smart && g.lastVrrHz != 0) {
            if (std::abs((int)want - (int)g.lastVrrHz) <= 1) want = g.lastVrrHz;
        }
        if (g.lastVrrHz != 0 && want < g.lastVrrHz) {
            const u32 hold = (vrrMode == RClkLadderVrr_Smart) ? 4 : 2;
            if (++g.vrrDownHold < hold) want = g.lastVrrHz;
            else                         g.vrrDownHold = 0;
        } else {
            g.vrrDownHold = 0;
        }
        hz = want;
    }

    if (hz != g.lastVrrHz) {
        config::SetOverrideHz(RClkModule_Display, hz);
        g.lastVrrHz = hz;
        g.vrrOverrideActive = true;
    }
    if (cfg.oledAutoGamma && board::GetConsoleType() == RClkConsoleType_Aula) {
        display::CorrectOledGamma(hz);
    }
}

u8 GetStableLadderFps(const RClkLadderConfig& cfg, u8 fps) {
    if (!cfg.predictorEnable) {
        return fps;
    }

    g.ladderFpsWindow.push_back(fps);
    const u8 maxWin = std::max<u8>(1, cfg.predictorWindowTicks);
    while (g.ladderFpsWindow.size() > maxWin) {
        g.ladderFpsWindow.pop_front();
    }

    u32 sum = 0;
    for (u8 v : g.ladderFpsWindow) {
        sum += v;
    }

    const u32 avg = sum / g.ladderFpsWindow.size();
    const int diff = std::abs((int)fps - (int)avg);
    if (diff > (int)cfg.predictorSpikeFps) {
        g.ladderFpsWindow.clear();
        g.ladderFpsWindow.push_back(fps);
        return fps;
    }

    const float alpha = std::clamp<float>(cfg.predictorAggressive / 10.0f, 0.0f, 1.0f);
    return (u8)std::lround(alpha * fps + (1.0f - alpha) * avg);
}

// === Tick ================================================================
void Tick() {
    const u64 now_ns = armTicksToNs(armGetSystemTick());

    // Копируем текущий cfg под замком, дальше работаем локально.
    RClkLadderConfig cfg;
    {
        std::scoped_lock lock{g.cfgMutex};
        cfg = g.cfg;
    }

    // VRR RCLK preconditions — один раз за сессию (не на каждом тике!).
    const u8 vrrMode = NormalizeVrrMode(cfg.vrrMode);
    if (vrrMode != RClkLadderVrr_Off && !g.vrrConfigApplied) {
        u8 needMax = std::max<u8>(cfg.vrrMaxHz, 60);
        bool changed = false;
        if (config::GetConfigValue(RClkConfigValue_OverwriteRefreshRate) == 0) {
            config::SetConfigValue(RClkConfigValue_OverwriteRefreshRate, 1, /*immediate=*/true);
            changed = true;
        }
        if (config::GetConfigValue(RClkConfigValue_MaxDisplayClockH) < needMax) {
            config::SetConfigValue(RClkConfigValue_MaxDisplayClockH, needMax, /*immediate=*/true);
            changed = true;
        }
        if (changed) g.lastVrrHz = 0;
        g.vrrConfigApplied = true;
    } else if (vrrMode == RClkLadderVrr_Off) {
        g.vrrConfigApplied = false;
    }

    if (!cfg.enabled) {
        RClkContext ctxEarly = clockManager::GetCurrentContext();
        const u8 fpsEarly = ctxEarly.fps;
        const bool fpsKnownEarly = (fpsEarly != kFpsUnavailable);
        ApplyVrr(cfg, fpsKnownEarly, fpsEarly);
        return;
    }

    svcSleepThread(kTelemetrySettleNs);
    RClkContext ctx = clockManager::GetCurrentContext();
    const u8 fps      = ctx.fps;
    const bool fpsKnown = (fps != kFpsUnavailable);
    const u8 cpuLoad  = (u8)std::min<u32>(100, ctx.partLoad[RClkPartLoad_CPUMax] / 10);
    const u8 gpuLoad  = (u8)std::min<u32>(100, ctx.partLoad[RClkPartLoad_GPU]    / 10);
    const u32 tCpu    = ctx.temps[RClkThermalSensor_CPU];
    const u32 tGpu    = ctx.temps[RClkThermalSensor_GPU];
    const s32 pNow    = ctx.power[RClkPowerSensor_Now];

    RefreshTables(now_ns);
    if (!g.tablesValid) return;

    if (g.cpuStep < 0) g.cpuStep = FindStep(g.cpuTable, ctx.freqs[RClkModule_CPU]);
    if (g.gpuStep < 0) g.gpuStep = FindStep(g.gpuTable, ctx.freqs[RClkModule_GPU]);

    int cpuLo, cpuHi, gpuLo, gpuHi;
    ComputeBounds(cfg, cpuLo, cpuHi, gpuLo, gpuHi);

    if (fpsKnown) EnsureRamMax();

    bool handled = false;

    // Thermal throttle (cooldown pattern).
    const bool thermCpu = (cfg.tempCapCpuMillideg > 0) && (tCpu >= cfg.tempCapCpuMillideg);
    const bool thermGpu = (cfg.tempCapGpuMillideg > 0) && (tGpu >= cfg.tempCapGpuMillideg);
    if (thermCpu || thermGpu) {
        if (g.thermCooldown == 0) {
            g.thermCooldown = kThermCooldown;
            if (thermCpu) StepCpu(-1, cpuLo, cpuHi);
            if (thermGpu) StepGpu(-1, gpuLo, gpuHi);
        } else {
            --g.thermCooldown;
        }
        g.holdUpTicks = 0; g.holdDownTicks = 0; g.fpsLowStreak = 0;
        handled = true;
    } else {
        g.thermCooldown = 0;
    }

    // TDP: по одному шагу за тик (CPU/GPU по очереди); на следующем тике снова читаем мощность из context.
    if (!handled) {
        u64 effTdp = cfg.tdpCapMw;
        if (effTdp == 0) {
            if (board::GetConsoleType() == RClkConsoleType_Hoag)
                effTdp = config::GetConfigValue(RClkConfigValue_LiteTDPLimit);
            else
                effTdp = config::GetConfigValue(RClkConfigValue_HandheldTDPLimit);
        }
        if (PowerOverTdpMw(pNow, effTdp)) {
            g.holdUpTicks = 0; g.holdDownTicks = 0; g.fpsLowStreak = 0;
            handled = true;
            if (g.tdpCpuFirst) {
                if (!StepCpu(-1, cpuLo, cpuHi)) (void)StepGpu(-1, gpuLo, gpuHi);
                g.tdpCpuFirst = false;
            } else {
                if (!StepGpu(-1, gpuLo, gpuHi)) (void)StepCpu(-1, cpuLo, cpuHi);
                g.tdpCpuFirst = true;
            }
        }
    }

    // FPS-driven ladder.
    if (!handled && fpsKnown) {
        const u8  effTarget = cfg.targetFps;
        const u8  stableFps = GetStableLadderFps(cfg, fps);
        const u8  evalFps   = (effTarget > 60) ? stableFps : fps;
        const int err       = (int)effTarget - (int)evalFps;
        const bool fpsLow = err > (int)cfg.tolerance;
        // Use the same tolerance window for downshift eligibility.
        // This makes 61..74 targets behave like the 60 preset in practice:
        // stable FPS inside [target-tolerance, target] can start downshift.
        const bool fpsOk  = (int)evalFps >= ((int)effTarget - (int)cfg.tolerance);

        if (fpsLow) {
            // For 61..74 targets tiny telemetry dips can briefly toggle fpsLow
            // and permanently block downshift. Require a short low streak before
            // clearing holdDownTicks, while keeping upshift responsive.
            if (++g.fpsLowStreak >= 2) {
                g.holdDownTicks = 0;
            }
            const u32 holdUp = (cfg.algo == RClkLadderAlgo_Cycle) ? kHoldUpCycle : kHoldUpPro;
            if (++g.holdUpTicks >= holdUp) {
                g.holdUpTicks = 0;
                if (cfg.algo == RClkLadderAlgo_Cycle)
                    DoCycleUp(cpuLo, cpuHi, gpuLo, gpuHi);
                else
                    DoProUp(cpuLoad, gpuLoad, err, cpuLo, cpuHi, gpuLo, gpuHi);
            }
        } else if (fpsOk) {
            g.fpsLowStreak = 0;
            g.holdUpTicks = 0;
            const u32 holdDn = (cfg.algo == RClkLadderAlgo_Cycle) ? kHoldDownCycle : kHoldDownPro;
            if (++g.holdDownTicks >= holdDn) {
                g.holdDownTicks = 0;
                if (cfg.algo == RClkLadderAlgo_Cycle)
                    DoCycleDown(cpuLo, cpuHi, gpuLo, gpuHi);
                else
                    DoProDown(cpuLoad, gpuLoad, cpuLo, cpuHi, gpuLo, gpuHi);
            }
        }
    }

    ApplyVrr(cfg, fpsKnown, fps);
}

void ThreadFunc(void*) {
    while (g.gRunning.load(std::memory_order_acquire)) {
        Tick();
        svcSleepThread(kTickIntervalNs);
    }
}

} // namespace

// === Public API ==========================================================
void Initialize() {
    std::scoped_lock lock{g.cfgMutex};
    LoadConfigLocked();

    s32 priority = 0x2C;
    (void)svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    Result rc = threadCreate(&g.gThread, &ThreadFunc, nullptr, nullptr, 0x4000, priority, -2);
    if (R_FAILED(rc)) {
        fileUtils::LogLine("[auto_ryazha] threadCreate FAILED: 0x%x", rc);
    } else {
        fileUtils::LogLine("[auto_ryazha] Initialize OK: cfg.enabled=%u vrrMode=%u algo=%u",
                           (unsigned)g.cfg.enabled, (unsigned)g.cfg.vrrMode,
                           (unsigned)g.cfg.algo);
    }
}

void Start() {
    if (g.gRunning.exchange(true)) {
        fileUtils::LogLine("[auto_ryazha] Start: already running");
        return;
    }
    Result rc = threadStart(&g.gThread);
    if (R_FAILED(rc)) {
        fileUtils::LogLine("[auto_ryazha] threadStart FAILED: 0x%x", rc);
        g.gRunning.store(false);
    } else {
        fileUtils::LogLine("[auto_ryazha] Thread started, tick=%llu ns",
                           (unsigned long long)kTickIntervalNs);
    }
}

void Exit() {
    if (!g.gRunning.exchange(false)) return;
    threadWaitForExit(&g.gThread);
    threadClose(&g.gThread);
}

void GetConfig(RClkLadderConfig* out) {
    if (!out) return;
    std::scoped_lock lock{g.cfgMutex};
    *out = g.cfg;
}

void SetConfig(const RClkLadderConfig* in) {
    if (!in) return;
    std::scoped_lock lock{g.cfgMutex};
    const bool wasEnabled = g.cfg.enabled != 0;
    const u8   oldVrr     = g.cfg.vrrMode;
    g.cfg = *in;
    SaveConfigLocked();

    // Сброс runtime при включении или смене алгоритма — чтобы не было хвостов.
    if (!wasEnabled && g.cfg.enabled) {
        g.cpuStep = -1; g.gpuStep = -1;
        g.upCycleCnt = 0;
        g.downCpuFirst = true; g.tdpCpuFirst = true;
        g.holdUpTicks = g.holdDownTicks = 0;
        g.fpsLowStreak = 0;
        g.thermCooldown = 0;
        g.ramPinned = false;
    }
    if (wasEnabled && !g.cfg.enabled) {
        // Снимаем наши overrides, иначе останутся прилипшими.
        config::SetOverrideHz(RClkModule_CPU, 0);
        config::SetOverrideHz(RClkModule_GPU, 0);
        config::SetOverrideHz(RClkModule_MEM, 0);
        g.ramPinned = false;
    }
    if (oldVrr != g.cfg.vrrMode) {
        if (g.cfg.vrrMode == RClkLadderVrr_Off) {
            config::SetOverrideHz(RClkModule_Display, 0);
            g.vrrOverrideActive = false;
            g.lastVrrHz = 0;
        }
        g.vrrConfigApplied = false;  // разрешаем один раз применить конфиг RCLK для VRR
        g.fpsWindow.clear();
    }
    g.ladderFpsWindow.clear();
}

} // namespace autoRyazha
