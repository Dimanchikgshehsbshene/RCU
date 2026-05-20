/*
 * Copyright (c) Souldbminer, Lightos_ and Ryazha CLK Contributors
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
#include <switch.h>
#include <rclk.h>
#include <nxExt.h>
#include "../errors.hpp"

namespace board {

    void SetHz(RClkModule module, u32 hz);

    u32 GetHz(RClkModule module);
    u32 GetRealHz(RClkModule module);
    void GetFreqList(RClkModule module, u32 *outList, u32 maxCount, u32 *outCount);
    u32 GetHighestDockedDisplayRate();

    void ResetToStock();
    void ResetToStockDisplay();

    template <typename Getter>
    void ResetToStockModule(Getter getHzFunc, RClkModule module) {
        Result rc = 0;

        if (hosversionAtLeast(9, 0, 0)) {
            u32 confId = 0;
            rc = apmExtGetCurrentPerformanceConfiguration(&confId);
            ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");

            RClkApmConfiguration* apmConfiguration = nullptr;
            for (size_t i = 0; rclk_g_apm_configurations[i].id; ++i) {

                if (rclk_g_apm_configurations[i].id == confId) {
                    apmConfiguration = &rclk_g_apm_configurations[i];
                    break;
                }
            }

            if (!apmConfiguration) {
                ERROR_THROW("Unknown apm configuration: %x", confId);
            }

            SetHz(module, getHzFunc(*apmConfiguration));
        } else {
            u32 mode = 0;
            rc = apmExtGetPerformanceMode(&mode);
            ASSERT_RESULT_OK(rc, "apmExtGetPerformanceMode");

            rc = apmExtSysRequestPerformanceMode(mode);
            ASSERT_RESULT_OK(rc, "apmExtSysRequestPerformanceMode");
        }
    }

    inline void ResetToStockCpu() {
        ResetToStockModule([](const RClkApmConfiguration& cfg) {return cfg.cpu_hz; }, RClkModule_CPU);
    }

    inline void ResetToStockGpu() {
        ResetToStockModule([](const RClkApmConfiguration& cfg){ return cfg.gpu_hz; }, RClkModule_GPU);
    }

    inline void ResetToStockMem() {
        ResetToStockModule([](const RClkApmConfiguration& cfg){ return cfg.mem_hz; }, RClkModule_MEM);
    }

}
