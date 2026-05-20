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

#include <rclk.h>
#include <switch.h>
#include <nxExt.h>
#include <cmath>
#include <battery.h>
#include <pwm.h>
#include "board.hpp"
#include "../soctherm.hpp"
#include "bq24193.hpp"
namespace board {

    s32 GetTemperatureMilli(RClkThermalSensor sensor) {
        s32 millis = 0;
        BatteryChargeInfo info;

        soctherm::TSensorTemps temps = {};
        soctherm::ReadSensors(temps);

        switch(sensor) {
            case RClkThermalSensor_SOC: {
                millis = tmp451TempSoc();
                break;
            }
            case RClkThermalSensor_PCB: {
                millis = tmp451TempPcb();
                break;
            }
            case RClkThermalSensor_Skin: {
                if (HOSSVC_HAS_TC) {
                    Result rc;
                    rc = tcGetSkinTemperatureMilliC(&millis);
                    ASSERT_RESULT_OK(rc, "tcGetSkinTemperatureMilliC");
                }
                break;
            }
            case RClkThermalSensor_Battery: {
                batteryInfoGetChargeInfo(&info);
                millis = batteryInfoGetTemperatureMiliCelsius(&info);
                break;
            }
            case RClkThermalSensor_PMIC: {
                millis = 50000;
                break;
            }
            case RClkThermalSensor_CPU: {
                millis = temps.cpu;
                break;
            }
            case RClkThermalSensor_GPU: {
                millis = temps.gpu;
                break;
            }
            case RClkThermalSensor_MEM: {
                millis = board::GetSocType() == RClkSocType_Mariko ? temps.pllx : temps.mem;
                break;
            }
            case RClkThermalSensor_PLLX: {
                millis = temps.pllx;
                break;
            }
            case RClkThermalSensor_BQ24193: {
                millis = bq24193::getBQTemp();
                break;
            }
            default: {
                ASSERT_ENUM_VALID(RClkThermalSensor, sensor);
            }
        }

        return std::max(0, millis);
    }

    s32 GetPowerMw(RClkPowerSensor sensor) {
        switch (sensor) {
            case RClkPowerSensor_Now:
                return max17050PowerNow();
            case RClkPowerSensor_Avg:
                return max17050PowerAvg();
            default:
                ASSERT_ENUM_VALID(RClkPowerSensor, sensor);
        }

        return 0;
    }

}
