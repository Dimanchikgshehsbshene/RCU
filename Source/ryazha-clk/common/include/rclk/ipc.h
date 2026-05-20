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

#include <stdint.h>
#include "board.h"
#include "clock_manager.h"

#define HOCCLK_IPC_API_VERSION 2
#define HOCCLK_IPC_SERVICE_NAME "hoc:clk"

enum RClkIpcCmd
{
    RClkIpcCmd_GetApiVersion = 0,
    RClkIpcCmd_GetVersionString = 1,
    RClkIpcCmd_GetCurrentContext = 2,
    RClkIpcCmd_Exit = 3,
    RClkIpcCmd_GetProfileCount = 4,
    RClkIpcCmd_GetProfiles = 5,
    RClkIpcCmd_SetProfiles = 6,
    RClkIpcCmd_SetEnabled = 7,
    RClkIpcCmd_SetOverride = 8,
    RClkIpcCmd_GetConfigValues = 9,
    RClkIpcCmd_SetConfigValues = 10,
    RClkIpcCmd_GetFreqList = 11,
    RClkIpcCmd_SetKipData = 12,
    RClkIpcCmd_GetKipData = 13,
    RClkIpcCmd_GetLadderConfig = 14,
    RClkIpcCmd_SetLadderConfig = 15,
};


typedef struct
{
    uint64_t tid;
    RClkTitleProfileList profiles;
} RClkIpc_SetProfiles_Args;

typedef struct
{
    RClkModule module;
    uint32_t hz;
} RClkIpc_SetOverride_Args;

typedef struct
{
    RClkModule module;
    uint32_t maxCount;
} RClkIpc_GetFreqList_Args;