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
#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint64_t u64;
typedef int64_t s64;
typedef enum
{
    HocClkConsoleType_Icosa = 0, // V1
    HocClkConsoleType_Iowa,      // V2
    HocClkConsoleType_Hoag,      // Lite
    HocClkConsoleType_Aula,      // OLED
    HocClkConsoleType_EnumMax,
} HocClkConsoleType;
typedef enum {
    HocClkVoltage_SOC = 0, // VDD_SOC rail. 
    HocClkVoltage_EMCVDD2, // DRAM VDD2 rail
    HocClkVoltage_CPU,     // CPU rail
    HocClkVoltage_GPU,     // GPU rail
    HocClkVoltage_EMCVDDQ, // DRAM VDDQ rail
    HocClkVoltage_Display, // Display rail
    HocClkVoltage_Battery, // Battery voltage
    HocClkVoltage_EnumMax,
} HocClkVoltage;
typedef enum
{
    HocClkSocType_Erista = 0,
    HocClkSocType_Mariko,
    HocClkSocType_EnumMax,
} HocClkSocType;
typedef enum
{
    HocClkModule_CPU = 0, 
    HocClkModule_GPU,
    HocClkModule_MEM,
    HocClkModule_Governor,
    HocClkModule_EnumMax,
} HocClkModule;
typedef enum
{
    HocClkThermalSensor_SOC = 0, // SoC temperature in millicelcius
    HocClkThermalSensor_PCB,     // PCB temperature in millicelcius
    HocClkThermalSensor_Skin,    // "Skin" temperature in millicelcius
    HocClkThermalSensor_Battery, // Battery temperature in millicelcius
    HocClkThermalSensor_PMIC, // Always return 50.0C, as thats the only reasonable value the PMIC sensor can generate
    HocClkThermalSensor_CPU, // CPU temperature in millicelcius
    HocClkThermalSensor_GPU, // GPU temperature in millicelcius
    HocClkThermalSensor_MEM, // MEM temperature in millicelcius. Returns the PLLX sensor value on Mariko
    HocClkThermalSensor_PLLX, // PLLX temperature in millicelcius
    HocClkThermalSensor_AO, // AOTAG
    HocClkThermalSensor_BQ24193, // BQ24193 temperature. Refer to BQ24193Temp for returned values
    HocClkThermalSensor_EnumMax
} HocClkThermalSensor;
typedef enum
{
    HocClkPowerSensor_Now = 0,
    HocClkPowerSensor_Avg,
    HocClkPowerSensor_EnumMax
} HocClkPowerSensor;
typedef enum
{
    HocClkPartLoad_CPU = 0,
    HocClkPartLoad_GPU,
    HocClkPartLoad_RamBWAll,
    HocClkPartLoad_RamBWCpu,
    HocClkPartLoad_RamBWGpu,
    HocClkPartLoad_RamBWPeak, // Maximum possible RAM bandwidth
    HocClkPartLoad_EnumMax
} HocClkPartLoad;
typedef enum
{
    HocClkGovernor_None = 0,
    HocClkGovernor_Performance,
    HocClkGovernor_Powersave,
    HocClkGovernor_EnumMax,
} HocClkGovernor;
enum {
    DVFSMode_Disabled = 0,
    DVFSMode_Hijack, // PCV hijack dvfs
    DVFSMode_EnumMax,
};
typedef struct
{
    u16 cpuSpeedo;
    u16 gpuSpeedo;
    u16 socSpeedo;
    u16 cpuIDDQ;
    u16 gpuIDDQ;
    u16 socIDDQ;
    s16 waferX;
    s16 waferY;
} FuseData;
typedef enum {
    RamDisplayUnit_MHz = 0,
    RamDisplayUnit_MTs,
    RamDisplayUnit_MHzMTs,
    RamDisplayUnit_EnumMax,
} RamDisplayUnit;
typedef enum {
    BQ24193Temp_Normal = 0,
    BQ24193Temp_Warm,
    BQ24193Temp_Hot,
    BQ24193Temp_Overheat,
    BQ24193Temp_EnumMax
} BQ24193Temp;
typedef enum AulaColorMode {
    AulaDisplayColorMode_DoNotOverride = 0xFF,
    AulaDisplayColorMode_Saturated = 0x0,
    AulaDisplayColorMode_Washed = 0x45,
    AulaDisplayColorMode_Basic = 0x03, // Default
    AulaDisplayColorMode_Vivid = 0x43,
    AulaDisplayColorMode_Night1 = 0x15,
    AulaDisplayColorMode_Night2 = 0x35,
    AulaDisplayColorMode_Night3 = 0x75,
} AulaColorMode;
#ifdef __cplusplus
}
#endif
