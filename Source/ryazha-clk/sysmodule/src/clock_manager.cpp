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

#include "clock_manager.hpp"
#include <cstring>
#include <atomic>
#include "file_utils.hpp"
#include "board/board.hpp"
#include "process_management.hpp"
#include "errors.hpp"
#include "ipc_service.hpp"
#include "kip.hpp"
#include <i2c.h>
#include "board/display_refresh_rate.hpp"
#include <cstdio>
#include <crc32.h>
#include "config.hpp"
#include "integrations.hpp"
#include <nxExt/cpp/lockable_mutex.h>
#include "kip.hpp"
#include "governor.hpp"
#include "display/aula.hpp"

#define HOSPPC_HAS_BOOST (hosversionAtLeast(7,0,0))

namespace clockManager {


    bool gRunning = false;
    LockableMutex gContextMutex;
    RClkContext gContext = {};
    FreqTable gFreqTable[RClkModule_EnumMax];
    std::uint64_t gLastTempLogNs = 0;
    std::uint64_t gLastFreqLogNs = 0;
    std::uint64_t gLastPowerLogNs = 0;
    std::uint64_t gLastCsvWriteNs = 0;

    static std::atomic_bool gForceClockReapply{false};

    bool IsAssignableHz(RClkModule module, std::uint32_t hz)
    {
        switch (module) {
        case RClkModule_CPU:
            return hz >= 500000000;
        case RClkModule_MEM:
            return hz >= 665600000;
        default:
            return true;
        }
    }

    std::uint32_t GetMaxAllowedHz(RClkModule module, RClkProfile profile)
    {
        if (config::GetConfigValue(RClkConfigValue_UncappedClocks)) {
            return ~0; // Integer limit, uncapped clocks ON
        } else {
            if (module == RClkModule_GPU) {
                if (profile < RClkProfile_HandheldCharging) {
                    switch (board::GetSocType()) {
                    case RClkSocType_Erista:
                        return 460800000;
                    case RClkSocType_Mariko:
                        switch (config::GetConfigValue(KipConfigValue_marikoGpuUV)) {
                        case 0:
                            return 614400000;
                        case 1:
                            return 691200000;
                        case 2:
                            return 768000000;
                        default:
                            return 614400000;
                        }
                    default:
                        return 460800000;
                    }
                } else if (profile <= RClkProfile_HandheldChargingUSB) {
                    switch (board::GetSocType()) {
                    case RClkSocType_Erista:
                        return 768000000;
                    case RClkSocType_Mariko:
                        switch (config::GetConfigValue(KipConfigValue_marikoGpuUV)) {
                        case 0:
                            return 844800000;
                        case 1:
                            return 921600000;
                        case 2:
                            return 998400000;
                        default:
                            return 844800000;
                        }
                    default:
                        return 768000000;
                    }
                }
            } else if (module == RClkModule_CPU) {
                if (profile < RClkProfile_HandheldCharging && board::GetSocType() == RClkSocType_Erista) {
                    return 1581000000;
                } else {
                    return ~0;
                }
            }
        }
        return 0;
    }

    std::uint32_t GetNearestHz(RClkModule module, std::uint32_t inHz, std::uint32_t maxHz)
    {
        std::uint32_t *freqs = &gFreqTable[module].list[0];
        size_t count = gFreqTable[module].count - 1;

        size_t i = 0;
        while (i < count) {
            if (maxHz > 0 && freqs[i] >= maxHz) {
                break;
            }
            if (inHz <= ((std::uint64_t)freqs[i] + freqs[i + 1]) / 2) {
                break;
            }
            i++;
        }

        return freqs[i];
    }

    void ResetToStockClocks()
    {
        board::ResetToStockCpu();
        if (config::GetConfigValue(RClkConfigValue_LiveCpuUv)) {
            if (board::GetSocType() == RClkSocType_Erista)
                board::SetDfllTunings(config::GetConfigValue(KipConfigValue_eristaCpuUV), 0, 1581000000);
            else
                board::SetDfllTunings(config::GetConfigValue(KipConfigValue_marikoCpuUVLow), config::GetConfigValue(KipConfigValue_marikoCpuUVHigh), board::CalculateTbreak(config::GetConfigValue(KipConfigValue_tableConf)));
        }

        board::ResetToStockGpu();
    }

    bool ConfigIntervalTimeout(RClkConfigValue intervalMsConfigValue, std::uint64_t ns, std::uint64_t *lastLogNs)
    {
        std::uint64_t logInterval = config::GetConfigValue(intervalMsConfigValue) * 1000000ULL;
        bool shouldLog = logInterval && ((ns - *lastLogNs) > logInterval);

        if (shouldLog) {
            *lastLogNs = ns;
        }

        return shouldLog;
    }

    void RefreshFreqTableRow(RClkModule module)
    {
        std::scoped_lock lock{gContextMutex};

        std::uint32_t freqs[HOCCLK_FREQ_LIST_MAX];
        std::uint32_t count;

        fileUtils::LogLine("[mgr] %s freq list refresh", board::GetModuleName(module, true));
        board::GetFreqList(module, &freqs[0], HOCCLK_FREQ_LIST_MAX, &count);

        std::uint32_t *hz = &gFreqTable[module].list[0];
        gFreqTable[module].count = 0;
        for (std::uint32_t i = 0; i < count; i++) {
            if (!IsAssignableHz(module, freqs[i])) {
                continue;
            }

            *hz = freqs[i];
            fileUtils::LogLine("[mgr] %02u - %u - %u.%u MHz", gFreqTable[module].count, *hz, *hz / 1000000, *hz / 100000 - *hz / 1000000 * 10);

            gFreqTable[module].count++;
            hz++;
        }

        fileUtils::LogLine("[mgr] count = %u", gFreqTable[module].count);
    }

    void HandleSafetyFeatures()
    {
        if (config::GetConfigValue(RClkConfigValue_HandheldTDP) && (gContext.profile != RClkProfile_Docked)) {
            if (board::GetConsoleType() == RClkConsoleType_Hoag) {
                if (board::GetPowerMw(RClkPowerSensor_Avg) < -(int)config::GetConfigValue(RClkConfigValue_LiteTDPLimit)) {
                    ResetToStockClocks();
                    return;
                }
            } else {
                if (board::GetPowerMw(RClkPowerSensor_Avg) < -(int)config::GetConfigValue(RClkConfigValue_HandheldTDPLimit)) {
                    ResetToStockClocks();
                    return;
                }
            }
        }

        if (((tmp451TempSoc() / 1000) > (int)config::GetConfigValue(RClkConfigValue_ThermalThrottleThreshold)) && config::GetConfigValue(RClkConfigValue_ThermalThrottle)) {
            ResetToStockClocks();
            return;
        }
    }

    void HandleMiscFeatures()
    {
        static u32 tick = 0;
        if(++tick > 10) {
            tick = 0;
            
            if (config::GetConfigValue(RClkConfigValue_BatteryChargeCurrent)) {
                I2c_Bq24193_SetFastChargeCurrentLimit(config::GetConfigValue(RClkConfigValue_BatteryChargeCurrent));
            }
            I2c_BuckConverter_SetMvOut(&I2c_Display, config::GetConfigValue(RClkConfigValue_DisplayVoltage));

            AulaDisplay::SetDisplayColorMode((AulaColorMode)config::GetConfigValue(RClkConfigValue_AulaDisplayColorPreset));

        }
    }

    void DVFSBeforeSet(u32 targetHz)
    {
        s32 dvfsOffset = config::GetConfigValue(RClkConfigValue_DVFSOffset);
        u32 vmin = board::GetMinimumGpuVmin(targetHz / 1000000, board::GetGpuSpeedoBracket());

        if (vmin) {
            vmin += dvfsOffset;
        }

        board::PcvHijackGpuVolts(vmin);

        /* Update the voltage. */
        if (I2c_BuckConverter_GetMvOut(&I2c_Mariko_GPU) < vmin) {
            I2c_BuckConverter_SetMvOut(&I2c_Mariko_GPU, vmin);
        }

        gContext.voltages[RClkVoltage_GPU] = vmin * 1000;
    }

    void DVFSAfterSet(u32 targetHz)
    {
        s32 dvfsOffset = config::GetConfigValue(RClkConfigValue_DVFSOffset);
        dvfsOffset = std::max(dvfsOffset, -80);
        u32 vmin = board::GetMinimumGpuVmin(targetHz / 1000000, board::GetGpuSpeedoBracket());

        if (vmin) {
            vmin += dvfsOffset;
        }

        u32 maxHz = GetMaxAllowedHz(RClkModule_GPU, gContext.profile);
        u32 nearestHz = GetNearestHz(RClkModule_GPU, targetHz, maxHz);
        board::PcvHijackGpuVolts(vmin);

        if (targetHz) {
            board::SetHz(RClkModule_GPU, ~0);
            board::SetHz(RClkModule_GPU, nearestHz);
        } else {
            board::SetHz(RClkModule_GPU, ~0);
            board::ResetToStockGpu();
        }
    }

    void HandleCpuUv()
    {
        if (board::GetSocType() == RClkSocType_Erista)
            board::SetDfllTunings(config::GetConfigValue(KipConfigValue_eristaCpuUV), 0, 1581000000);
        else
            board::SetDfllTunings(config::GetConfigValue(KipConfigValue_marikoCpuUVLow), config::GetConfigValue(KipConfigValue_marikoCpuUVHigh), board::CalculateTbreak(config::GetConfigValue(KipConfigValue_tableConf)));
    }

    void DVFSReset()
    {
        if (board::GetSocType() == RClkSocType_Mariko && config::GetConfigValue(RClkConfigValue_DVFSMode) == DVFSMode_Hijack) {
            board::PcvHijackGpuVolts(0);

            u32 targetHz = gContext.overrideFreqs[RClkModule_GPU];
            if (!targetHz) {
                targetHz = config::GetAutoClockHz(gContext.applicationId, RClkModule_GPU, gContext.profile, false);
                if (!targetHz) {
                    targetHz = config::GetAutoClockHz(HOCCLK_GLOBAL_PROFILE_TID, RClkModule_GPU, gContext.profile, false);
                }
            }
            u32 maxHz = GetMaxAllowedHz(RClkModule_GPU, gContext.profile);
            u32 nearestHz = GetNearestHz(RClkModule_GPU, targetHz, maxHz);

            board::SetHz(RClkModule_GPU, ~0);
            if (targetHz) {
                board::SetHz(RClkModule_GPU, nearestHz);
            } else {
                board::ResetToStockGpu();
            }
        }
    }

    void HandleFreqReset(RClkModule module, bool isBoost)
    {
        switch (module) {
        case RClkModule_CPU:
            if (!(isBoost || (config::GetConfigValue(RClkConfigValue_OverwriteBoostMode) && isBoost)))
                board::ResetToStockCpu();
            if (config::GetConfigValue(RClkConfigValue_LiveCpuUv)) {
                if (board::GetSocType() == RClkSocType_Erista)
                    board::SetDfllTunings(config::GetConfigValue(KipConfigValue_eristaCpuUV), 0, 1581000000);
                else
                    board::SetDfllTunings(config::GetConfigValue(KipConfigValue_marikoCpuUVLow), config::GetConfigValue(KipConfigValue_marikoCpuUVHigh), board::CalculateTbreak(config::GetConfigValue(KipConfigValue_tableConf)));
            }
            break;
        case RClkModule_GPU:
            board::ResetToStockGpu();
            break;
        case RClkModule_MEM:
            board::ResetToStockMem();
            DVFSReset();
            break;
        case RClkModule_Display:
            if (config::GetConfigValue(RClkConfigValue_OverwriteRefreshRate)) {
                board::ResetToStockDisplay();
            }
            break;
        default:
            break;
        }
    }

    void SetClocks(bool isBoost)
    {
        std::uint32_t targetHz = 0;
        std::uint32_t maxHz = 0;
        std::uint32_t nearestHz = 0;

        if (isBoost && !config::GetConfigValue(RClkConfigValue_OverwriteBoostMode)) {
            u32 boostFreq = board::GetHz(RClkModule_CPU);
            if (boostFreq / 1000000 > 1785) {
                board::SetHz(RClkModule_CPU, boostFreq);
            }
            return; // Return if we aren't overwriting boost mode
        }

        bool returnRaw = false; // Return a value scaled to MHz instead of raw value
        for (unsigned int module = 0; module < RClkModule_EnumMax; module++) {
            u32 oldHz = board::GetHz((RClkModule)module); // Get Old hz (used primarily for DVFS Logic)

            if (module > RClkModule_MEM)
                returnRaw = true;
            else
                returnRaw = false;
            targetHz = gContext.overrideFreqs[module];
            if (!targetHz) {
                targetHz = config::GetAutoClockHz(gContext.applicationId, (RClkModule)module, gContext.profile, returnRaw);
                if (!targetHz)
                    targetHz = config::GetAutoClockHz(HOCCLK_GLOBAL_PROFILE_TID, (RClkModule)module, gContext.profile, returnRaw);
            }

            if (module == RClkModule_Governor) {
                governor::HandleGovernor(targetHz);
            }

            bool noCPU = governor::isCpuGovernorEnabled;
            bool noGPU = governor::isGpuGovernorEnabled;
            bool noDisp = governor::isVRREnabled;
            if (noDisp && module == RClkModule_Display)
                continue;

            if (module == RClkModule_Display && config::GetConfigValue(RClkConfigValue_OverwriteRefreshRate) && !noDisp) {
                if (targetHz) {
                    board::SetHz(RClkModule_Display, targetHz);
                    gContext.freqs[RClkModule_Display] = targetHz;
                    gContext.realFreqs[RClkModule_Display] = targetHz;
                } else {
                    HandleFreqReset(RClkModule_Display, isBoost);
                }
            }

            // Skip GPU and CPU if governors handle them
            if (module > RClkModule_MEM) {
                continue;
            }

            if (noCPU && module == RClkModule_CPU)
                continue;
            if (noGPU && module == RClkModule_GPU)
                continue;

            if (targetHz) {
                maxHz = GetMaxAllowedHz((RClkModule)module, gContext.profile);
                nearestHz = GetNearestHz((RClkModule)module, targetHz, maxHz);

                if (nearestHz != gContext.freqs[module]) {
                    fileUtils::LogLine(
                        "[mgr] %s clock set : %u.%u MHz (target = %u.%u MHz)",
                        board::GetModuleName((RClkModule)module, true),
                        nearestHz / 1000000, nearestHz / 100000 - nearestHz / 1000000 * 10,
                        targetHz / 1000000, targetHz / 100000 - targetHz / 1000000 * 10
                    );

                    if (module == RClkModule_MEM && board::GetSocType() == RClkSocType_Mariko && targetHz > oldHz && config::GetConfigValue(RClkConfigValue_DVFSMode) == DVFSMode_Hijack) {
                        DVFSBeforeSet(targetHz);
                    }

                    board::SetHz((RClkModule)module, nearestHz);
                    gContext.freqs[module] = nearestHz;

                    if (module == RClkModule_CPU && config::GetConfigValue(RClkConfigValue_LiveCpuUv)) {
                        HandleCpuUv();
                    }

                    if (module == RClkModule_MEM && board::GetSocType() == RClkSocType_Mariko && targetHz < oldHz && config::GetConfigValue(RClkConfigValue_DVFSMode) == DVFSMode_Hijack) {
                        DVFSAfterSet(targetHz);
                    }
                }
            } else {
                HandleFreqReset((RClkModule)module, isBoost);
            }
        }
    }

    bool RefreshContext()
    {
        bool hasChanged = false;

        std::uint32_t mode = 0;
        Result rc = apmExtGetCurrentPerformanceConfiguration(&mode);
        ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");

        std::uint64_t applicationId = processManagement::GetCurrentApplicationId();
        if (applicationId != gContext.applicationId) {
            fileUtils::LogLine("[mgr] TitleID change: %016lX", applicationId);
            gContext.applicationId = applicationId;
            hasChanged = true;
        }

        RClkProfile profile = board::GetProfile();
        if (profile != gContext.profile) {
            fileUtils::LogLine("[mgr] Profile change: %s", board::GetProfileName(profile, true));
            gContext.profile = profile;
            hasChanged = true;
        }

        // restore clocks to stock values on app or profile change
        if (hasChanged) {
            board::ResetToStock();
            if (board::GetSocType() == RClkSocType_Mariko && config::GetConfigValue(RClkConfigValue_DVFSMode) == DVFSMode_Hijack) {
                board::PcvHijackGpuVolts(0);
                board::SetHz(RClkModule_GPU, ~0);
                board::ResetToStockGpu();
            }
            WaitForNextTick();
        }

        std::uint32_t hz = 0;
        for (unsigned int module = 0; module < RClkModule_EnumMax; module++) {
            hz = board::GetHz((RClkModule)module);
            if (hz != 0 && hz != gContext.freqs[module]) {
                fileUtils::LogLine("[mgr] %s clock change: %u.%u MHz", board::GetModuleName((RClkModule)module, true), hz / 1000000, hz / 100000 - hz / 1000000 * 10);
                gContext.freqs[module] = hz;
                hasChanged = true;
            }

            hz = config::GetOverrideHz((RClkModule)module);
            if (hz != gContext.overrideFreqs[module]) {
                if (hz) {
                    fileUtils::LogLine("[mgr] %s override change: %u.%u MHz", board::GetModuleName((RClkModule)module, true), hz / 1000000, hz / 100000 - hz / 1000000 * 10);
                }
                gContext.overrideFreqs[module] = hz;
                hasChanged = true;
            }
        }

        std::uint64_t ns = armTicksToNs(armGetSystemTick());

        // temperatures do not and should not force a refresh, hasChanged untouched
        std::uint32_t millis = 0;
        bool shouldLogTemp = ConfigIntervalTimeout(RClkConfigValue_TempLogIntervalMs, ns, &gLastTempLogNs);
        for (unsigned int sensor = 0; sensor < RClkThermalSensor_EnumMax; sensor++) {
            millis = board::GetTemperatureMilli((RClkThermalSensor)sensor);
            if (shouldLogTemp) {
                fileUtils::LogLine("[mgr] %s temp: %u.%u °C", board::GetThermalSensorName((RClkThermalSensor)sensor, true), millis / 1000, (millis - millis / 1000 * 1000) / 100);
            }
            gContext.temps[sensor] = millis;
        }

        // power stats do not and should not force a refresh, hasChanged untouched
        std::int32_t mw = 0;
        bool shouldLogPower = ConfigIntervalTimeout(RClkConfigValue_PowerLogIntervalMs, ns, &gLastPowerLogNs);
        for (unsigned int sensor = 0; sensor < RClkPowerSensor_EnumMax; sensor++) {
            mw = board::GetPowerMw((RClkPowerSensor)sensor);
            if (shouldLogPower) {
                fileUtils::LogLine("[mgr] Power %s: %d mW", board::GetPowerSensorName((RClkPowerSensor)sensor, false), mw);
            }
            gContext.power[sensor] = mw;
        }

        // real freqs do not and should not force a refresh, hasChanged untouched
        std::uint32_t realHz = 0;
        bool shouldLogFreq = ConfigIntervalTimeout(RClkConfigValue_FreqLogIntervalMs, ns, &gLastFreqLogNs);
        for (unsigned int module = 0; module < RClkModule_EnumMax; module++) {
            realHz = board::GetRealHz((RClkModule)module);
            if (shouldLogFreq) {
                fileUtils::LogLine("[mgr] %s real freq: %u.%u MHz", board::GetModuleName((RClkModule)module, true), realHz / 1000000, realHz / 100000 - realHz / 1000000 * 10);
            }
            gContext.realFreqs[module] = realHz;
        }

        // ram load do not and should not force a refresh, hasChanged untouched
        for (unsigned int loadSource = 0; loadSource < RClkPartLoad_EnumMax; loadSource++) {
            gContext.partLoad[loadSource] = board::GetPartLoad((RClkPartLoad)loadSource);
        }

        for (unsigned int voltageSource = 0; voltageSource < RClkVoltage_EnumMax; voltageSource++) {
            gContext.voltages[voltageSource] = board::GetVoltage((RClkVoltage)voltageSource);
        }

        if (ConfigIntervalTimeout(RClkConfigValue_CsvWriteIntervalMs, ns, &gLastCsvWriteNs)) {
            fileUtils::WriteContextToCsv(&gContext);
        }

        // this->context->maxDisplayFreq = board::GetHighestDockedDisplayRate();
        u32 targetHz = gContext.overrideFreqs[RClkModule_Display];
        if (!targetHz) {
            targetHz = config::GetAutoClockHz(gContext.applicationId, RClkModule_Display, gContext.profile, true);
            if (!targetHz)
                targetHz = config::GetAutoClockHz(HOCCLK_GLOBAL_PROFILE_TID, RClkModule_Display, gContext.profile, true);
        }

        if (board::GetConsoleType() != RClkConsoleType_Hoag)
            board::SetDisplayRefreshDockedState(gContext.profile == RClkProfile_Docked);

        if (gContext.isSaltyNXInstalled)
            gContext.fps = integrations::GetSaltyNXFPS();
        else
            gContext.fps = 254; // N/A

        if (gContext.isSaltyNXInstalled)
            gContext.resolutionHeight = integrations::GetSaltyNXResolutionHeight();
        else
            gContext.resolutionHeight = 0; // N/A

        return hasChanged;
    }

    void Initialize()
    {
        gContext = {};
        gContext.applicationId = 0;
        gContext.profile = RClkProfile_Handheld;
        for (unsigned int module = 0; module < RClkModule_EnumMax; module++) {
            gContext.freqs[module] = 0;
            gContext.realFreqs[module] = 0;
            gContext.overrideFreqs[module] = 0;
            RefreshFreqTableRow((RClkModule)module);
        }

        gRunning = false;
        gLastTempLogNs = 0;
        gLastCsvWriteNs = 0;

        kip::GetKipData();

        board::FuseData *fuse = board::GetFuseData();

        gContext.speedos[RClkSpeedo_CPU] = fuse->cpuSpeedo;
        gContext.speedos[RClkSpeedo_GPU] = fuse->gpuSpeedo;
        gContext.speedos[RClkSpeedo_SOC] = fuse->socSpeedo;
        gContext.iddq[RClkSpeedo_CPU] = fuse->cpuIDDQ;
        gContext.iddq[RClkSpeedo_GPU] = fuse->gpuIDDQ;
        gContext.iddq[RClkSpeedo_SOC] = fuse->socIDDQ;
        gContext.waferX = fuse->waferX;
        gContext.waferY = fuse->waferY;

        gContext.dramID = board::GetDramID();
        gContext.isDram8GB = board::IsDram8GB();
        board::SetGpuSchedulingMode((GpuSchedulingMode)config::GetConfigValue(RClkConfigValue_GPUScheduling), (GpuSchedulingOverrideMethod)config::GetConfigValue(RClkConfigValue_GPUSchedulingMethod));
        gContext.gpuSchedulingMode = (GpuSchedulingMode)config::GetConfigValue(RClkConfigValue_GPUScheduling);

        gContext.isSysDockInstalled = integrations::GetSysDockState();
        gContext.isSaltyNXInstalled = integrations::GetSaltyNXState();
        if (gContext.isSaltyNXInstalled) {
            integrations::LoadSaltyNX();
        }

        gContext.isUsingRetroSuper = integrations::GetRETROSuperStatus();
        governor::startThreads();
    }

    void Exit()
    {
        governor::exitThreads();
    }

    RClkContext GetCurrentContext()
    {
        std::scoped_lock lock{gContextMutex};
        return gContext;
    }

    void SetRunning(bool running)
    {
        gRunning = running;
    }

    bool Running()
    {
        return gRunning;
    }

    void GetFreqList(RClkModule module, std::uint32_t *list, std::uint32_t maxCount, std::uint32_t *outCount)
    {
        ASSERT_ENUM_VALID(RClkModule, module);

        *outCount = std::min(maxCount, gFreqTable[module].count);
        memcpy(list, &gFreqTable[module].list[0], *outCount * sizeof(gFreqTable[0].list[0]));
    }

    void NotifyClockReapply()
    {
        gForceClockReapply.store(true, std::memory_order_release);
    }

    void Tick()
    {
        std::scoped_lock lock{gContextMutex};
        std::uint32_t mode = 0;
        Result rc = apmExtGetCurrentPerformanceConfiguration(&mode);
        ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");

        bool isBoost = apmExtIsBoostMode(mode);

        HandleSafetyFeatures();
        HandleMiscFeatures();

        if (RefreshContext() || config::Refresh() || gForceClockReapply.exchange(false, std::memory_order_acq_rel)) {
            SetClocks(isBoost);
        }
    }

    void WaitForNextTick()
    {
        if (board::GetHz(RClkModule_MEM) > 665000000)
            svcSleepThread(config::GetConfigValue(RClkConfigValue_PollingIntervalMs) * 1000000ULL);
        else
            svcSleepThread(5000 * 1000000ULL); // 5 seconds in sleep mode
    }
} // namespace clockManager
