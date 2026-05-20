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


#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <switch.h>
#include <string.h>
#include <stdatomic.h>
#include <rclk/client/ipc.h>

static Service g_rclkSrv;
static atomic_size_t g_refCnt;

bool rclkIpcRunning()
{
    Handle handle;
    bool running = R_FAILED(smRegisterService(&handle, smEncodeName(HOCCLK_IPC_SERVICE_NAME), false, 1));

    if (!running)
    {
        smUnregisterService(smEncodeName(HOCCLK_IPC_SERVICE_NAME));
    }

  return running;
}

Result rclkIpcInitialize(void)
{
    Result rc = 0;

    g_refCnt++;

    if (serviceIsActive(&g_rclkSrv))
        return 0;

    rc = smGetService(&g_rclkSrv, HOCCLK_IPC_SERVICE_NAME);

    if (R_FAILED(rc)) rclkIpcExit();

    return rc;
}

void rclkIpcExit(void)
{
    if (--g_refCnt == 0)
    {
        serviceClose(&g_rclkSrv);
    }
}

Result rclkIpcGetAPIVersion(u32* out_ver)
{
    return serviceDispatchOut(&g_rclkSrv, RClkIpcCmd_GetApiVersion, *out_ver);
}

Result rclkIpcGetVersionString(char* out, size_t len)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_GetVersionString,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{out, len}},
    );
}

Result rclkIpcGetCurrentContext(RClkContext* out_context)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_GetCurrentContext,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{out_context, sizeof(RClkContext)}},
    );
}

Result rclkIpcGetProfileCount(u64 tid, u8* out_count)
{
    return serviceDispatchInOut(&g_rclkSrv, RClkIpcCmd_GetProfileCount, tid, *out_count);
}

Result rclkIpcSetEnabled(bool enabled)
{
    u8 enabledRaw = (u8)enabled;
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_SetEnabled, enabledRaw);
}

Result rclkIpcSetOverride(RClkModule module, u32 hz)
{
    RClkIpc_SetOverride_Args args = {
        .module = module,
        .hz = hz
    };
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_SetOverride, args);
}

Result rclkIpcGetProfiles(u64 tid, RClkTitleProfileList* out_profiles)
{
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_GetProfiles, tid,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{out_profiles, sizeof(RClkTitleProfileList)}},
    );
}

Result rclkIpcSetProfiles(u64 tid, RClkTitleProfileList* profiles)
{
    RClkIpc_SetProfiles_Args args;
    args.tid = tid;
    memcpy(&args.profiles, profiles, sizeof(RClkTitleProfileList));
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_SetProfiles, args);
}

Result rclkIpcGetConfigValues(RClkConfigValueList* out_configValues)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_GetConfigValues,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{out_configValues, sizeof(RClkConfigValueList)}},
    );
}

Result rclkIpcSetConfigValues(RClkConfigValueList* configValues)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_SetConfigValues,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = {{configValues, sizeof(RClkConfigValueList)}},
    );
}

Result rclkIpcGetFreqList(RClkModule module, u32* list, u32 maxCount, u32* outCount)
{
    RClkIpc_GetFreqList_Args args = {
        .module = module,
        .maxCount = maxCount
    };
    return serviceDispatchInOut(&g_rclkSrv, RClkIpcCmd_GetFreqList, args, *outCount,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{list, maxCount * sizeof(u32)}},
    );
}

Result hocClkIpcSetKipData()
{
    u32 temp = 0;
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_SetKipData, temp);
}

Result hocClkIpcGetKipData()
{
    u32 temp = 0;
    return serviceDispatchIn(&g_rclkSrv, RClkIpcCmd_GetKipData, temp);
}

Result rclkIpcGetLadderConfig(RClkLadderConfig* out_cfg)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_GetLadderConfig,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = {{out_cfg, sizeof(RClkLadderConfig)}},
    );
}

Result rclkIpcSetLadderConfig(const RClkLadderConfig* cfg)
{
    return serviceDispatch(&g_rclkSrv, RClkIpcCmd_SetLadderConfig,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = {{(void*)cfg, sizeof(RClkLadderConfig)}},
    );
}