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

#include "ipc_service.hpp"
#include <cstring>
#include <switch.h>
#include <nxExt.h>
#include "file_utils.hpp"
#include "errors.hpp"
#include "clock_manager.hpp"
#include "config.hpp"
#include "kip.hpp"
#include "auto_ryazha.hpp"
namespace ipcService {

    namespace {

        bool gRunning = false;
        Thread gThread;
        LockableMutex gThreadMutex;
        IpcServer gServer;

        Result GetApiVersion(u32* out_version) {
            *out_version = RCLK_IPC_API_VERSION;
            return 0;
        }

        Result GetVersionString(char* out_buf, size_t bufSize) {
            if (bufSize) {
                strncpy(out_buf, TARGET_VERSION, bufSize-1);
            }
            return 0;
        }

        Result GetCurrentContext(RClkContext* out_ctx) {
            *out_ctx = clockManager::GetCurrentContext();
            return 0;
        }

        Result ExitHandler() {
            clockManager::SetRunning(false);
            return 0;
        }

        Result GetProfileCount(std::uint64_t* tid, std::uint8_t* out_count) {
            if (!config::HasProfilesLoaded()) {
                return RCLK_ERROR(ConfigNotLoaded);
            }
            *out_count = config::GetProfileCount(*tid);
            return 0;
        }

        Result GetProfiles(std::uint64_t* tid, RClkTitleProfileList* out_profiles) {
            if (!config::HasProfilesLoaded()) {
                return RCLK_ERROR(ConfigNotLoaded);
            }
            config::GetProfiles(*tid, out_profiles);
            return 0;
        }

        Result SetProfiles(RClkIpc_SetProfiles_Args* args) {
            if (!config::HasProfilesLoaded()) {
                return RCLK_ERROR(ConfigNotLoaded);
            }
            RClkTitleProfileList profiles = args->profiles;
            if (!config::SetProfiles(args->tid, &profiles, true)) {
                return RCLK_ERROR(ConfigSaveFailed);
            }
            clockManager::NotifyClockReapply();
            return 0;
        }

        Result SetEnabled(std::uint8_t* enabled) {
            config::SetEnabled(*enabled);
            return 0;
        }

        Result SetOverride(RClkIpc_SetOverride_Args* args) {
            if (!RCLK_ENUM_VALID(RClkModule, args->module)) {
                return RCLK_ERROR(Generic);
            }
            config::SetOverrideHz(args->module, args->hz);
            clockManager::NotifyClockReapply();
            return 0;
        }

        Result GetConfigValuesHandler(RClkConfigValueList* out_configValues) {
            if (!config::HasProfilesLoaded()) {
                return RCLK_ERROR(ConfigNotLoaded);
            }
            config::GetConfigValues(out_configValues);
            return 0;
        }

        Result SetConfigValuesHandler(RClkConfigValueList* configValues) {
            if (!config::HasProfilesLoaded()) {
                return RCLK_ERROR(ConfigNotLoaded);
            }
            RClkConfigValueList copy = *configValues;
            if (!config::SetConfigValues(&copy, true)) {
                return RCLK_ERROR(ConfigSaveFailed);
            }
            clockManager::NotifyClockReapply();
            return 0;
        }

        Result GetFreqList(RClkIpc_GetFreqList_Args* args, std::uint32_t* out_list, std::size_t size, std::uint32_t* out_count) {
            if (!RCLK_ENUM_VALID(RClkModule, args->module)) {
                return RCLK_ERROR(Generic);
            }
            if (args->maxCount != size/sizeof(*out_list)) {
                return RCLK_ERROR(Generic);
            }
            clockManager::GetFreqList(args->module, out_list, args->maxCount, out_count);
            return 0;
        }

        Result ServiceHandlerFunc(void* arg, const IpcServerRequest* r, u8* out_data, size_t* out_dataSize) {
            (void)arg;
            switch (r->data.cmdId) {
                case RClkIpcCmd_GetApiVersion:
                    *out_dataSize = sizeof(u32);
                    return GetApiVersion((u32*)out_data);

                case RClkIpcCmd_GetVersionString:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        return GetVersionString(
                            (char*)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers)
                        );
                    }
                    break;

                case RClkIpcCmd_GetCurrentContext:
                    if (r->data.size >= sizeof(std::uint64_t) && r->hipc.meta.num_recv_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.recv_buffers);
                        if (bufSize >= sizeof(RClkContext)) {
                            return GetCurrentContext((RClkContext*)hipcGetBufferAddress(r->hipc.data.recv_buffers));
                        }
                    }
                    break;

                case RClkIpcCmd_Exit:
                    return ExitHandler();

                case RClkIpcCmd_GetProfileCount:
                    if (r->data.size >= sizeof(std::uint64_t)) {
                        *out_dataSize = sizeof(std::uint8_t);
                        return GetProfileCount((std::uint64_t*)r->data.ptr, (std::uint8_t*)out_data);
                    }
                    break;

                case RClkIpcCmd_GetProfiles:
                    if (r->data.size >= sizeof(std::uint64_t) && r->hipc.meta.num_recv_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.recv_buffers);
                        if (bufSize >= sizeof(RClkTitleProfileList)) {
                            return GetProfiles((std::uint64_t*)r->data.ptr, (RClkTitleProfileList*)hipcGetBufferAddress(r->hipc.data.recv_buffers));
                        }
                    }
                    break;

                case RClkIpcCmd_SetProfiles:
                    if (r->data.size >= sizeof(RClkIpc_SetProfiles_Args)) {
                        return SetProfiles((RClkIpc_SetProfiles_Args*)r->data.ptr);
                    }
                    break;

                case RClkIpcCmd_SetEnabled:
                    if (r->data.size >= sizeof(std::uint8_t)) {
                        return SetEnabled((std::uint8_t*)r->data.ptr);
                    }
                    break;

                case RClkIpcCmd_SetOverride:
                    if (r->data.size >= sizeof(RClkIpc_SetOverride_Args)) {
                        return SetOverride((RClkIpc_SetOverride_Args*)r->data.ptr);
                    }
                    break;

                case RClkIpcCmd_GetConfigValues:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.recv_buffers);
                        if (bufSize >= sizeof(RClkConfigValueList)) {
                            return GetConfigValuesHandler((RClkConfigValueList*)hipcGetBufferAddress(r->hipc.data.recv_buffers));
                        }
                    }
                    break;

                case RClkIpcCmd_SetConfigValues:
                    if (r->hipc.meta.num_send_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.send_buffers);
                        if (bufSize >= sizeof(RClkConfigValueList)) {
                            return SetConfigValuesHandler((RClkConfigValueList*)hipcGetBufferAddress(r->hipc.data.send_buffers));
                        }
                    }
                    break;

                case RClkIpcCmd_GetFreqList:
                    if (r->data.size >= sizeof(RClkIpc_GetFreqList_Args) && r->hipc.meta.num_recv_buffers >= 1) {
                        *out_dataSize = sizeof(std::uint32_t);
                        return GetFreqList(
                            (RClkIpc_GetFreqList_Args*)r->data.ptr,
                            (std::uint32_t*)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers),
                            (std::uint32_t*)out_data
                        );
                    }
                    break;

                case RClkIpcCmd_SetKipData:
                    if (r->data.size >= 0) {
                        kip::SetKipData();
                        return 0;
                    }
                    break;

                case RClkIpcCmd_GetLadderConfig:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.recv_buffers);
                        if (bufSize >= sizeof(RClkLadderConfig)) {
                            autoRyazha::GetConfig((RClkLadderConfig*)hipcGetBufferAddress(r->hipc.data.recv_buffers));
                            return 0;
                        }
                    }
                    break;

                case RClkIpcCmd_SetLadderConfig:
                    if (r->hipc.meta.num_send_buffers >= 1) {
                        size_t bufSize = hipcGetBufferSize(r->hipc.data.send_buffers);
                        if (bufSize >= sizeof(RClkLadderConfig)) {
                            autoRyazha::SetConfig((RClkLadderConfig*)hipcGetBufferAddress(r->hipc.data.send_buffers));
                            return 0;
                        }
                    }
                    break;
            }

            return RCLK_ERROR(Generic);
        }

        void ProcessThreadFunc(void* arg) {
            (void)arg;
            Result rc;
            while (true) {
                rc = ipcServerProcess(&gServer, &ServiceHandlerFunc, nullptr);
                if (R_FAILED(rc)) {
                    if (rc == KERNELRESULT(Cancelled)) {
                        return;
                    }
                    if (rc != KERNELRESULT(ConnectionClosed)) {
                        fileUtils::LogLine("[ipc] ipcServerProcess: [0x%x] %04d-%04d", rc, R_MODULE(rc), R_DESCRIPTION(rc));
                    }
                }
            }
        }

    }

    void Initialize() {
        std::int32_t priority;
        Result rc = svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
        ASSERT_RESULT_OK(rc, "svcGetThreadPriority");
        rc = ipcServerInit(&gServer, RCLK_IPC_SERVICE_NAME, 42);
        ASSERT_RESULT_OK(rc, "ipcServerInit");
        rc = threadCreate(&gThread, &ProcessThreadFunc, nullptr, NULL, 0x4000, priority, -2);
        ASSERT_RESULT_OK(rc, "threadCreate");
        gRunning = false;
    }

    void Exit() {
        SetRunning(false);
        Result rc = threadClose(&gThread);
        ASSERT_RESULT_OK(rc, "threadClose");
        rc = ipcServerExit(&gServer);
        ASSERT_RESULT_OK(rc, "ipcServerExit");
    }

    void SetRunning(bool running) {
        std::scoped_lock lock{gThreadMutex};
        if (gRunning == running) {
            return;
        }

        gRunning = running;

        if (running) {
            Result rc = threadStart(&gThread);
            ASSERT_RESULT_OK(rc, "threadStart");
        } else {
            svcCancelSynchronization(gThread.handle);
            threadWaitForExit(&gThread);
        }
    }

}
