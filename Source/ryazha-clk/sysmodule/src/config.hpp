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

#include <rclk.h>
#include <switch.h>

#define CONFIG_VAL_SECTION "values"

namespace config {

    void Initialize();
    void Exit();

    bool Refresh();
    bool HasProfilesLoaded();

    std::uint8_t GetProfileCount(std::uint64_t tid);
    void GetProfiles(std::uint64_t tid, RClkTitleProfileList* out_profiles);
    bool SetProfiles(std::uint64_t tid, RClkTitleProfileList* profiles, bool immediate);
    std::uint32_t GetAutoClockHz(std::uint64_t tid, RClkModule module, RClkProfile profile, bool returnRaw);

    void SetEnabled(bool enabled);
    bool Enabled();
    void SetOverrideHz(RClkModule module, std::uint32_t hz);
    std::uint32_t GetOverrideHz(RClkModule module);

    std::uint64_t GetConfigValue(RClkConfigValue val);
    const char* GetConfigValueName(RClkConfigValue val, bool pretty);
    void GetConfigValues(RClkConfigValueList* out_configValues);
    bool SetConfigValues(RClkConfigValueList* configValues, bool immediate);
    bool ResetConfigValue(RClkConfigValue kval);
    bool SetConfigValue(RClkConfigValue kval, std::uint64_t value, bool immediate = true);

    extern uint64_t configValues[RClkConfigValue_EnumMax];

}
