#include "../../i18n.hpp"
/*
 * Copyright (c) Souldbminer, Lightos_ and Ryazha-CLK Contributors
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
 */


#include "about_gui.h"
#include "../format.h"
#include <tesla.hpp>
#include <string>
#include "cat.h"
#include "ult_ext.h"

// tsl::elm::ListItem* custRevItem = NULL;
tsl::elm::ListItem* kipVersionItem = NULL;
tsl::elm::ListItem* SpeedoItem = NULL;
tsl::elm::ListItem* IddqItem = NULL;
tsl::elm::ListItem* DramModule = NULL;
tsl::elm::ListItem* sysdockStatusItem = NULL;
tsl::elm::ListItem* saltyNXStatusItem = NULL;
tsl::elm::ListItem* RETROStatusItem = NULL;
tsl::elm::ListItem* waferCordsItem = NULL;
tsl::elm::ListItem* ramVoltItem = NULL;
tsl::elm::ListItem* eristaPLLXItem = NULL;
tsl::elm::ListItem* dispVoltItem = NULL;
tsl::elm::ListItem* ramBWItemAll = NULL;
tsl::elm::ListItem* ramBWItemCpu = NULL;
tsl::elm::ListItem* ramBWItemGpu = NULL;
tsl::elm::ListItem* ramBWItemMax = NULL;
tsl::elm::ListItem* bqtempitem = NULL;
tsl::elm::ListItem* aotagTempItem = NULL;
tsl::elm::ListItem* cTypeItem = NULL;

ImageElement* CatImage = NULL;
HideableCategoryHeader* CatHeader = NULL;
HideableCustomDrawer* CatSpacer = NULL;
int lightosClickCount = 0;

AboutGui::AboutGui()
{
    memset(strings, 0, sizeof(strings));
}

AboutGui::~AboutGui()
{
}

void AboutGui::listUI()
{
    BaseMenuGui::refresh();

    if (!this->context)
        return;

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Voltages"))
    );

    ramVoltItem =
        new tsl::elm::ListItem(i18n::t("RAM Voltage:"));

    if(IsMariko()) {
        this->listElement->addItem(ramVoltItem);
    }
    dispVoltItem =
        new tsl::elm::ListItem(i18n::t("Display Voltage:"));
    this->listElement->addItem(dispVoltItem);

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Temperatures"))
    );
    eristaPLLXItem =
        new tsl::elm::ListItem(i18n::t("PLLX Temp:"));
    if(this->context->temps[RClkThermalSensor_AO] > 0) { // Only show if the value is valid (not -126, which means not patched)
        this->listElement->addItem(eristaPLLXItem);
    }

    aotagTempItem =
        new tsl::elm::ListItem(i18n::t("AOTAG Temp:"));
    this->listElement->addItem(aotagTempItem);

    bqtempitem =
        new tsl::elm::ListItem(i18n::t("BQ24193 Temp:"));
    this->listElement->addItem(bqtempitem);

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("RAM Bandwidth"))
    );

    ramBWItemMax =
        new tsl::elm::ListItem(i18n::t("RAM BW (Peak):"));
    this->listElement->addItem(ramBWItemMax);

    ramBWItemAll =
        new tsl::elm::ListItem(i18n::t("RAM BW (All):"));
    this->listElement->addItem(ramBWItemAll);

    ramBWItemCpu =
        new tsl::elm::ListItem(i18n::t("RAM BW (CPU):"));
    this->listElement->addItem(ramBWItemCpu);

    ramBWItemGpu =
        new tsl::elm::ListItem(i18n::t("RAM BW (GPU):"));
    this->listElement->addItem(ramBWItemGpu);


    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Hardware Info"))
    );

    cTypeItem =
        new tsl::elm::ListItem(i18n::t("Console Type:"));
    this->listElement->addItem(cTypeItem);

    SpeedoItem =
        new tsl::elm::ListItem(i18n::t("Speedo:"));
    this->listElement->addItem(SpeedoItem);

    IddqItem =
        new tsl::elm::ListItem(i18n::t("IDDQ:"));
    this->listElement->addItem(IddqItem);

    DramModule =
        new tsl::elm::ListItem(i18n::t("DRAM Module: "));
    this->listElement->addItem(DramModule);

    waferCordsItem =
        new tsl::elm::ListItem(i18n::t("Wafer Position:"));
    this->listElement->addItem(waferCordsItem);

    if(IsHoag()) {
        RETROStatusItem =
            new tsl::elm::ListItem(i18n::t("RR Display status:"));
        this->listElement->addItem(RETROStatusItem);
    }

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Software Info"))
    );

    // custRevItem = new tsl::elm::ListItem(i18n::t("CUST revision:"));
    // this->listElement->addItem(custRevItem);

    kipVersionItem = new tsl::elm::ListItem(i18n::t("KIP version:"));
    this->listElement->addItem(kipVersionItem);

    if(!IsHoag()) {
        sysdockStatusItem =
            new tsl::elm::ListItem(i18n::t("sys-dock status:"));
        this->listElement->addItem(sysdockStatusItem);
    }

    saltyNXStatusItem =
        new tsl::elm::ListItem(i18n::t("SaltyNX status:"));
    this->listElement->addItem(saltyNXStatusItem);

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Credits"))
    );

    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Developers"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Souldbminer"))
    );

    // Create special clickable item for Lightos
    auto lightosItem = new tsl::elm::ListItem(i18n::t("Lightos_"));
    lightosItem->setClickListener([this](u64 keys) -> bool {
        if (keys & HidNpadButton_A) {
            lightosClickCount++;
            if (lightosClickCount >= 10) {
                if (CatImage != NULL) CatImage->setVisible(true);
                if (CatHeader != NULL) CatHeader->setVisible(true);
                if (CatSpacer != NULL) CatSpacer->setVisible(true);
            }
            return true;
        }
        return false;
    });
    this->listElement->addItem(lightosItem);

    // ---- Contributors ----
    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Contributors"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Dom"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Blaise25"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("tetetete-ctrl"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("B3711"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("TDRR"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("MasaGratoR"))
    );

    // ---- Testers ----
    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Testers"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Samybigio2011"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("arcdelta"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Miki1305"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Happy"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Winnerboi77"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Blaise25"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("WE1ZARD"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Alvise"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("agjeococh"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Xenshen"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("Frost"))
    );

    // ---- Special Thanks ----
    this->listElement->addItem(
        new tsl::elm::CategoryHeader(i18n::t("Special Thanks"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("SciresM - Atmosphere CFW"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("KazushiMe - Switch OC Suite"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("hanai3Bi - Switch OC Suite & EOS"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("NaGaa95 - L4T-OC-Kernel"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("RetroNX - sys-clk"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("ppkantorski - Ultrahand"))
    );

    this->listElement->addItem(
        new tsl::elm::ListItem(i18n::t("CtCaer - Hekate, L4T and Proper Timings"))
    );

    // Create cat elements but hide them initially
    CatHeader = new HideableCategoryHeader("Cat");
    CatHeader->setVisible(false);
    this->listElement->addItem(CatHeader);

    CatImage = new ImageElement(CAT_DATA, CAT_WIDTH, CAT_HEIGHT);
    CatImage->setVisible(false);
    this->listElement->addItem(CatImage);

    CatSpacer = new HideableCustomDrawer(75);
    CatSpacer->setVisible(false);
    this->listElement->addItem(CatSpacer);
}

std::string AboutGui::formatRamModule() {
    switch (this->context->dramID) {
        case 0: return "HB-MGCH 4GB";
        case 4: return "HM-MGCH 6GB";
        case 7: return "HM-MGXX 8GB";

        case 1: return "NLE 4GB";
        case 2: return "WT:C 4GB";

        case 3:
        case 5 ... 6: return "NEE 4GB";

        case 8:
        case 12: return "AM-MGCJ 4GB";
        case 9:
        case 13: return "AM-MGCJ 8GB";

        case 10:
        case 14: return "NME 4GB";

        case 11:
        case 15: return "WT:E 4GB";

        case 17:
        case 19:
        case 24: return "AA-MGCL 4GB";

        case 18:
        case 23:
        case 28: return "AA-MGCL 8GB";

        case 20 ... 22: return "AB-MGCL 4GB";

        case 25 ... 27: return "WT:F 4GB";

        case 29 ... 31: return "x267 4GB";

        case 32 ... 34: return "WT:B 4GB";

        default: return "Unknown";
    }
}

void AboutGui::update()
{
    BaseMenuGui::update();
}

void AboutGui::refresh()
{
    BaseMenuGui::refresh();

    if (!this->context)
        return;
    // Format strings once per refresh
    sprintf(strings[0], "%u/%u/%u", this->context->speedos[RClkSpeedo_CPU], this->context->speedos[RClkSpeedo_GPU], this->context->speedos[RClkSpeedo_SOC]);
    // This is how hekate does it
    sprintf(strings[1], "%u/%u/%u", this->context->iddq[RClkSpeedo_CPU], this->context->iddq[RClkSpeedo_GPU], this->context->iddq[RClkSpeedo_SOC]);
    SpeedoItem->setValue(strings[0]);
    IddqItem->setValue(strings[1]);
    DramModule->setValue(formatRamModule());

    // custRevItem->setValue(std::to_string(this->context->custRev));

    kipVersionItem->setValue(std::to_string((this->context->kipVersion / 100) % 10) + "." + std::to_string((this->context->kipVersion / 10) % 10) + "." + std::to_string( this->context->kipVersion % 10) + " (Cust Rev " + std::to_string(this->context->custRev) + ")");
    if(!IsHoag())
        sysdockStatusItem->setValue(this->context->isSysDockInstalled ? "Installed" : "Not Installed");

    saltyNXStatusItem->setValue(this->context->isSaltyNXInstalled ? "Installed" : "Not Installed");

    if(IsHoag())
        RETROStatusItem->setValue(this->context->isUsingRetroSuper ? "Installed" : "Not Installed");

    sprintf(strings[2], "X: %d Y: %d", this->context->waferX, this->context->waferY);
    waferCordsItem->setValue(strings[2]);

    s32 millis = context->temps[RClkThermalSensor_PLLX];
    sprintf(strings[3], "%u.%u °C", millis / 1000U, (millis % 1000U) / 100U);
    eristaPLLXItem->setValue(strings[3]);

    millis = context->temps[RClkThermalSensor_AO];
    if(millis > 0) {
        sprintf(strings[11], "%u.%u °C", millis / 1000U, (millis % 1000U) / 100U);
    } else if (millis == -125) {
        sprintf(strings[11], "Invalid");
    } else if (millis == -126) {
        sprintf(strings[11], "Not Patched");
    }
    aotagTempItem->setValue(strings[11]);

    sprintf(strings[4], "%u.%u / %u mV", context->voltages[RClkVoltage_EMCVDD2] / 1000U, (context->voltages[RClkVoltage_EMCVDD2] % 1000U) / 100U, context->voltages[RClkVoltage_EMCVDDQ] / 1000);
    ramVoltItem->setValue(strings[4]);

    sprintf(strings[5], "%u.%u mV", context->voltages[RClkVoltage_Display] / 1000U, (context->voltages[RClkVoltage_Display] % 1000U) / 100U);
    dispVoltItem->setValue(strings[5]);

    sprintf(strings[6], "%u MB/s", context->partLoad[RClkPartLoad_RamBWAll]);
    ramBWItemAll->setValue(strings[6]);

    sprintf(strings[7], "%u MB/s", context->partLoad[RClkPartLoad_RamBWCpu]);
    ramBWItemCpu->setValue(strings[7]);

    sprintf(strings[8], "%u MB/s", context->partLoad[RClkPartLoad_RamBWGpu]);
    ramBWItemGpu->setValue(strings[8]);

    sprintf(strings[9], "%u MB/s", context->partLoad[RClkPartLoad_RamBWPeak]);
    ramBWItemMax->setValue(strings[9]);

    switch(context->temps[RClkThermalSensor_BQ24193]) {
        case BQ24193Temp_Normal:
            strcpy(strings[10], "Normal");
            break;
        case BQ24193Temp_Warm:
            strcpy(strings[10], "Warm");
            break;
        case BQ24193Temp_Hot:
            strcpy(strings[10], "Hot");
            break;
        case BQ24193Temp_Overheat:
            strcpy(strings[10], "Overheat");
            break;
        default:
            strcpy(strings[10], "Unknown");
    }

    bqtempitem->setValue(strings[10]);

    cTypeItem->setValue(hocClkFormatConsoleType(this->context->consoleType, true));

}
