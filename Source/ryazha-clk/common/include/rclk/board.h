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
#include <stddef.h>
#include <stdbool.h>
#include <switch/types.h>
typedef enum
{
    RClkSocType_Erista = 0, // T210, found in Icosa and Copper
    RClkSocType_Mariko,     // T214/T210B01, found in Hoag, Iowa, Calcio and Aula
//  RClkSocType_Drake,      // T239, found in Switch 2. Maybe someday...
    RClkSocType_EnumMax
} RClkSocType;

typedef enum
{
    RClkConsoleType_Icosa = 0, // V1
    RClkConsoleType_Copper,    // Unreleased Erista
    RClkConsoleType_Hoag,      // Lite
    RClkConsoleType_Iowa,      // V2
    RClkConsoleType_Calcio,    // Unreleased Mariko
    RClkConsoleType_Aula,      // OLED
    RClkConsoleType_EnumMax,
} RClkConsoleType;

typedef enum {
    RClkVoltage_SOC = 0,
    RClkVoltage_EMCVDD2,
    RClkVoltage_CPU,
    RClkVoltage_GPU,
    RClkVoltage_EMCVDDQ, // Returns VDD2 on Erista
    RClkVoltage_Display,
    RClkVoltage_Battery,
    RClkVoltage_EnumMax,
} RClkVoltage;

typedef enum
{
    RClkProfile_Handheld = 0,
    RClkProfile_HandheldCharging, // Not a real profile, just a marker
    RClkProfile_HandheldChargingUSB,
    RClkProfile_HandheldChargingOfficial,
    RClkProfile_Docked, // Not shown on Lites
    RClkProfile_EnumMax
} RClkProfile;

typedef enum
{
    RClkModule_CPU = 0,
    RClkModule_GPU,
    RClkModule_MEM,
    RClkModule_Governor,
    RClkModule_Display,
    RClkModule_EnumMax,
} RClkModule;

typedef enum
{
    RClkThermalSensor_SOC = 0,
    RClkThermalSensor_PCB,
    RClkThermalSensor_Skin,
    RClkThermalSensor_Battery,
    RClkThermalSensor_PMIC, // Always return 50.0C, as thats the only reasonable value the PMIC sensor can generate
    RClkThermalSensor_CPU,
    RClkThermalSensor_GPU,
    RClkThermalSensor_MEM, // Returns the PLLX sensor value on Mariko
    RClkThermalSensor_PLLX, 
    RClkThermalSensor_BQ24193,
    RClkThermalSensor_EnumMax
} RClkThermalSensor;

typedef enum
{
    RClkPowerSensor_Now = 0,
    RClkPowerSensor_Avg,
    RClkPowerSensor_EnumMax
} RClkPowerSensor;

typedef enum
{
    RClkPartLoad_EMC = 0,
    RClkPartLoad_EMCCpu,
    RClkPartLoad_GPU,
    RClkPartLoad_CPUMax,
    RClkPartLoad_BAT, // Battery raw charge percentage
    RClkPartLoad_FAN,
    RClkPartLoad_RamBWAll,
    RClkPartLoad_RamBWCpu,
    RClkPartLoad_RamBWGpu,
    RClkPartLoad_RamBWPeak, 
    RClkPartLoad_EnumMax
} RClkPartLoad;

typedef enum {
    RClkSpeedo_CPU = 0,
    RClkSpeedo_GPU,
    RClkSpeedo_SOC,
    RClkSpeedo_EnumMax,
} RClkSpeedo;

typedef enum {
    GPUUVLevel_HiOPT = 0,
    GPUUVLevel_HiOPT15,
    GPUUVLevel_HighUV,
    GPUUVLevel_EnumMax,
} GPUUndervoltLevel;

enum {
    DVFSMode_Disabled = 0,
    DVFSMode_Hijack,
    // DVFSMode_OfficialService,
    // DVFSMode_Hack,
    DVFSMode_EnumMax,
};

typedef enum {
    GpuSchedulingMode_DoNotOverride = 0,
    GpuSchedulingMode_Enabled,
    GpuSchedulingMode_Disabled,
    GpuSchedulingMode_EnumMax,
} GpuSchedulingMode;

typedef enum {
    GpuSchedulingOverrideMethod_Ini = 0,
    GpuSchedulingOverrideMethod_NvService,
    GpuSchedulingOverrideMethod_EnumMax,
} GpuSchedulingOverrideMethod;
typedef enum {
    ComponentGovernor_DoNotOverride = 0,
    ComponentGovernor_Disabled      = 1,
    ComponentGovernor_Enabled       = 2,
    ComponentGovernor_EnumMax,
} ComponentGovernorState;
typedef enum {
    RamDisplayMode_VDD2 = 0,
    RamDisplayMode_VDDQ,
    RamDisplayMode_EnumMax,
} RamDisplayMode;

typedef enum {
    MemoryFrequencyMeasurementMode_PLL = 0,
    MemoryFrequencyMeasurementMode_Actmon,
    MemoryFrequencyMeasurementMode_EnumMax,
} MemoryFrequencyMeasurementMode;

typedef enum {
    RamDisplayUnit_MHz = 0,
    RamDisplayUnit_MTs,
    RamDisplayUnit_MHzMTs,
    RamDisplayUnit_EnumMax,
} RamDisplayUnit;

typedef enum AulaColorMode {
    AulaDisplayColorMode_Saturated = 0x0,
    AulaDisplayColorMode_Washed = 0x45,
    AulaDisplayColorMode_Basic = 0x03, // Default
    AulaDisplayColorMode_PowerReset = 0x20, // Reset to on power on
    AulaDisplayColorMode_Natural = 0x23,
    AulaDisplayColorMode_Vivid = 0x65,
    AulaDisplayColorMode_Night0 = 0x43,
    AulaDisplayColorMode_Night1 = 0x15,
    AulaDisplayColorMode_Night2 = 0x35,
    AulaDisplayColorMode_Night3 = 0x75,
} AulaColorMode;
// typedef enum {
// 	PANEL_JDI_XXX062M     = 0x10,
// 	PANEL_JDI_LAM062M109A = 0x0910, // SI.
// 	PANEL_JDI_LPM062M326A = 0x2610, // LTPS.
// 	PANEL_INL_P062CCA_AZ1 = 0x0F20,
// 	PANEL_AUO_A062TAN01   = 0x0F30,
// 	PANEL_INL_2J055IA_27A = 0x1020,
// 	PANEL_AUO_A055TAN01   = 0x1030,
// 	PANEL_SHP_LQ055T1SW10 = 0x1040,
// 	PANEL_SAM_AMS699VC01  = 0x2050,
    
// 	PANEL_RR_SUPER5_OLED_V1  = 0x10E0,
// 	PANEL_RR_SUPER5_OLED_HD_V1  = 0x10E1,
// 	PANEL_RR_SUPER7_IPS_V1  = 0x0FE0,
// 	PANEL_RR_SUPER7_IPS_HD_V1  = 0x0FE1
// 	// Found on 6/2" clones. Unknown markings. Clone of AUO A062TAN01.
// 	// Quality seems JDI like. Has bad low backlight scaling. ID: [83] 94 [0F]. Sometimes reports [30] 94 [0F]. Both IDs have correct CRC16.
// 	PANEL_OEM_CLONE_6_2   = 0x0F83,
// 	// Found on 5.5" clones with AUO A055TAN02 (59.05A30.001) fake markings.
// 	PANEL_OEM_CLONE_5_5   = 0x00B3,
// 	// Found on 5.5" clones with AUO A055TAN02 (59.05A30.001) fake markings.
// 	PANEL_OEM_CLONE       = 0x0000
// 	//0x0F40 [40] 94 [0F], 5.5" clone
// } RClkDisplayPanel;

#define HOCCLK_ENUM_VALID(n, v) ((v) < n##_EnumMax)

// Packed u32
// Bits 0-7 - CPU
// Bits 8-15 - GPU
// Bits 16-23 - VRR
// Bits 24-32 - unused

inline u32 GovernorStatePack(u8 cpu, u8 gpu, u8 vrr) {
    return (u32)cpu | ((u32)gpu << 8) | ((u32)vrr << 16);
}
inline u8 GovernorStateCpu(u32 p) {
    return (u8)(p         & 0xFF);
}
inline u8 GovernorStateGpu(u32 p) {
    return (u8)((p >>  8) & 0xFF);
}
inline u8 GovernorStateVrr(u32 p) {
    return (u8)((p >> 16) & 0xFF);
}

static inline const char* rclkFormatModule(RClkModule module, bool pretty)
{
    switch(module)
    {
        case RClkModule_CPU:
            return pretty ? "CPU" : "cpu";
        case RClkModule_GPU:
            return pretty ? "GPU" : "gpu";
        case RClkModule_MEM:
            return pretty ? "Memory" : "mem";
        case RClkModule_Display:
            return pretty ? "Display" : "display";
        case RClkModule_Governor:
            return pretty ? "Governor" : "governor";
        default:
            return "null";
    }
}

static inline const char* rclkFormatThermalSensor(RClkThermalSensor thermSensor, bool pretty)
{
    switch(thermSensor) {
        case RClkThermalSensor_SOC:
            return pretty ? "SOC" : "soc";
        case RClkThermalSensor_PCB:
            return pretty ? "PCB" : "pcb";
        case RClkThermalSensor_Skin:
            return pretty ? "Skin" : "skin";
        case RClkThermalSensor_Battery:
            return pretty ? "BAT" : "battery";
        case RClkThermalSensor_PMIC:
            return pretty ? "PMIC" : "pmic";
        case RClkThermalSensor_CPU:
            return pretty ? "CPU" : "cpu";
        case RClkThermalSensor_GPU:
            return pretty ? "GPU" : "gpu";
        case RClkThermalSensor_MEM:
            return pretty ? "MEM" : "mem";
        case RClkThermalSensor_PLLX:
            return pretty ? "PLLX" : "pllx";
        default:
            return "unknown";
    }
}

static inline const char* rclkFormatPowerSensor(RClkPowerSensor powSensor, bool pretty)
{
    switch(powSensor)
    {
        case RClkPowerSensor_Now:
            return pretty ? "Now" : "now";
        case RClkPowerSensor_Avg:
            return pretty ? "Avg" : "avg";
        default:
            return "unknown";
    }
}

static inline const char* rclkFormatProfile(RClkProfile profile, bool pretty)
{
    switch(profile)
    {
        case RClkProfile_Docked:
            return pretty ? "Docked" : "docked";
        case RClkProfile_Handheld:
            return pretty ? "Handheld" : "handheld";
        case RClkProfile_HandheldCharging:
            return pretty ? "Charging" : "handheld_charging";
        case RClkProfile_HandheldChargingUSB:
            return pretty ? "USB Charger" : "handheld_charging_usb";
        case RClkProfile_HandheldChargingOfficial:
            return pretty ? "PD Charger" : "handheld_charging_official";
        default:
            return "unknown";
    }
}


static inline const char* hocClkFormatVoltage(RClkVoltage voltage, bool pretty)
{
    switch(voltage)
    {
        case RClkVoltage_CPU:
            return pretty ? "CPU" : "cpu";
        case RClkVoltage_GPU:
            return pretty ? "GPU" : "gpu";
        case RClkVoltage_EMCVDD2:
            return pretty ? "VDD2" : "vdd2";
        case RClkVoltage_EMCVDDQ:
            return pretty ? "VDDQ" : "vddq";
        case RClkVoltage_SOC:
            return pretty ? "SOC" : "soc";
        case RClkVoltage_Display:
            return pretty ? "Display" : "display";
        default:
            return "unknown";
    }
}

// static inline const char* hocClkFormatPanel(RClkDisplayPanel panel, bool pretty)
// {
//     switch(panel)
//     {
//         case PANEL_JDI_XXX062M:
//             return pretty ? "JDI XXX062M" : "jdi_xxx062m";
//         case PANEL_JDI_LAM062M109A:
//             return pretty ? "JDI LAM062M109A" : "jdi_lam062m109a";
//         case PANEL_JDI_LPM062M326A:
//             return pretty ? "JDI LPM062M326A" : "jdi_lpm062m326a";
//         case PANEL_INL_P062CCA_AZ1:
//             return pretty ? "INL P062CCA-AZ1" : "inl_p062cca_az1";
//         case PANEL_AUO_A062TAN01:
//             return pretty ? "AUO A062TAN01" : "auo_a062tan01";
//         case PANEL_INL_2J055IA_27A:
//             return pretty ? "INL 2J055IA-27A" : "inl_2j055ia_27a";
//         case PANEL_AUO_A055TAN01:
//             return pretty ? "AUO A055TAN01" : "auo_a055tan01";
//         case PANEL_SHP_LQ055T1SW10:
//             return pretty ? "SHP LQ055T1SW10" : "shp_lq055t1sw10";
//         case PANEL_SAM_AMS699VC01:
//             return pretty ? "SAM AMS699VC01" : "sam_ams699vc01";
//         case PANEL_RR_SUPER5_OLED_V1:
//             return pretty ? "SUPER5 OLED" : "rr_super5_oled_v1";
//         case PANEL_RR_SUPER5_OLED_HD_V1:
//             return pretty ? "SUPER5 OLED HD" : "rr_super5_oled_hd_v1";
//         case PANEL_RR_SUPER7_IPS_V1:
//             return pretty ? "SUPER7 IPS" : "rr_super7_ips_v1";
//         case PANEL_RR_SUPER7_IPS_HD_V1:
//             return pretty ? "RR Super7 IPS HD V1" : "rr_super7_ips_hd_v1";
//         case PANEL_OEM_CLONE_6_2:
//             return pretty ? "OEM Clone 6.2" : "oem_clone_6_2";
//         case PANEL_OEM_CLONE_5_5:
//             return pretty ? "OEM Clone 5.5" : "oem_clone_5_5";
//         case PANEL_OEM_CLONE:
//             return pretty ? "OEM Clone" : "oem_clone";
//         default:
//             return "unknown";
//     }
// }