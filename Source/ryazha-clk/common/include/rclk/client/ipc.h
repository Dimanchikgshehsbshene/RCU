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

#include "types.h"
#include "../config.h"
#include "../board.h"
#include "../ipc.h"
#include "../auto_ryazha.h"

bool rclkIpcRunning();
Result rclkIpcInitialize(void);
void rclkIpcExit(void);

Result rclkIpcGetAPIVersion(u32* out_ver);
Result rclkIpcGetVersionString(char* out, size_t len);
Result rclkIpcGetCurrentContext(RClkContext* out_context);
Result rclkIpcGetProfileCount(u64 tid, u8* out_count);
Result rclkIpcSetEnabled(bool enabled);
Result rclkIpcExitCmd();
Result rclkIpcSetOverride(RClkModule module, u32 hz);
Result rclkIpcGetProfiles(u64 tid, RClkTitleProfileList* out_profiles);
Result rclkIpcSetProfiles(u64 tid, RClkTitleProfileList* profiles);
Result rclkIpcGetConfigValues(RClkConfigValueList* out_configValues);
Result rclkIpcSetConfigValues(RClkConfigValueList* configValues);
Result rclkIpcGetFreqList(RClkModule module, u32* list, u32 maxCount, u32* outCount);
Result hocClkIpcSetKipData();
Result hocClkIpcGetKipData();

Result rclkIpcGetLadderConfig(RClkLadderConfig* out_cfg);
Result rclkIpcSetLadderConfig(const RClkLadderConfig* cfg);

static inline Result rclkIpcRemoveOverride(RClkModule module)
{
    return rclkIpcSetOverride(module, 0);
}
