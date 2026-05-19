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


#include "app_profile_gui.h"

#include "../format.h"
#include "fatal_gui.h"
#include "labels.h"
#include "living_ladder.h"
#include "display_hz_trackbar.hpp"

static const char* moduleNameRu(HocClkModule module) {
    switch (module) {
        case HocClkModule_CPU: return "ЦП";
        case HocClkModule_GPU: return "ГП";
        case HocClkModule_MEM: return "ОЗУ";
        default: return hocclkFormatModule(module, true);
    }
}

/** Если для USB/зарядки частота дисплея не задана — показываем и применяем значение «Портатив». */
static u32 effectiveTitleProfileDisplayHz(const HocClkTitleProfileList* pl, HocClkProfile profile) {
    u32 v = pl->mhzMap[profile][HocClkModule_Display];
    if (v != 0)
        return v;
    switch (profile) {
        case HocClkProfile_HandheldChargingUSB:
        case HocClkProfile_HandheldChargingOfficial:
        case HocClkProfile_HandheldCharging:
            return pl->mhzMap[HocClkProfile_Handheld][HocClkModule_Display];
        default:
            return 0;
    }
}

static std::string profileNameRu(HocClkProfile profile) {
    std::string p = hocclkFormatProfile(profile, true);
    if (p == "Docked") return "Док";
    if (p == "Handheld") return "Портатив";
    if (p == "Charging") return "Зарядка";
    if (p == "USB Charger") return "USB зарядка";
    if (p == "PD Charger") return "PD зарядка";
    return p;
}
AppProfileGui::AppProfileGui(std::uint64_t applicationId, HocClkTitleProfileList* profileList)
{
    this->applicationId = applicationId;
    this->profileList = profileList;
}

AppProfileGui::~AppProfileGui()
{
    delete this->profileList;
}

void AppProfileGui::openFreqChoiceGui(tsl::elm::ListItem* listItem, HocClkProfile profile, HocClkModule module)
{
    std::uint32_t hzList[HOCCLK_FREQ_LIST_MAX];
    std::uint32_t hzCount;
    Result rc = hocclkIpcGetFreqList(module, &hzList[0], HOCCLK_FREQ_LIST_MAX, &hzCount);
    if(R_FAILED(rc))
    {
        FatalGui::openWithResultCode("hocclkIpcGetFreqList", rc);
        return;
    }
    std::map<uint32_t, std::string> labels = {};

    if (module == HocClkModule_CPU) {
        bool isUsingUv = IsMariko() ? configList.values[KipConfigValue_marikoCpuUVHigh] : configList.values[KipConfigValue_eristaCpuUV];
        labels = IsMariko() ? (isUsingUv ? cpu_freq_label_m_uv : cpu_freq_label_m) : (isUsingUv ? cpu_freq_label_e_uv : cpu_freq_label_e);
    } else if (module == HocClkModule_GPU) {
        labels = IsMariko() ? *(marikoUV[configList.values[KipConfigValue_marikoGpuUV]]) : *(eristaUV[configList.values[KipConfigValue_eristaGpuUV]]);
    }
    RamDisplayUnit memUnit = (RamDisplayUnit)configList.values[HocClkConfigValue_RamDisplayUnit];
    tsl::changeTo<FreqChoiceGui>(this->profileList->mhzMap[profile][module] * 1000000, hzList, hzCount, module, [this, listItem, profile, module, memUnit](std::uint32_t hz) {
        this->profileList->mhzMap[profile][module] = hz / 1000000;
        std::uint32_t mhz = this->profileList->mhzMap[profile][module];
        listItem->setValue(module == HocClkModule_MEM ? formatListFreqMem(mhz, memUnit) : formatListFreqMHz(mhz));
        Result rc = hocclkIpcSetProfiles(this->applicationId, this->profileList);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
            return false;
        }

        return true;
    }, true, labels
    );
}

void AppProfileGui::openValueChoiceGui(
    tsl::elm::ListItem* listItem,
    std::uint32_t currentValue,
    const ValueRange& range,
    const std::string& categoryName,
    ValueChoiceListener listener,
    const ValueThresholds& thresholds,
    bool enableThresholds,
    const std::map<std::uint32_t, std::string>& labels,
    const std::vector<NamedValue>& namedValues,
    bool showDefaultValue
)
{
    tsl::changeTo<ValueChoiceGui>(
        currentValue,
        range,
        categoryName,
        listener,
        thresholds,
        enableThresholds,
        labels,
        namedValues,
        showDefaultValue,
        true
    );
}

void AppProfileGui::addModuleListItem(HocClkProfile profile, HocClkModule module)
{
    tsl::elm::ListItem* listItem = new tsl::elm::ListItem(moduleNameRu(module));
    RamDisplayUnit memUnit = (RamDisplayUnit)configList.values[HocClkConfigValue_RamDisplayUnit];
    std::uint32_t mhz = this->profileList->mhzMap[profile][module];
    listItem->setValue(module == HocClkModule_MEM ? formatListFreqMem(mhz, memUnit) : formatListFreqMHz(mhz));
    listItem->setClickListener([this, listItem, profile, module, memUnit](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A)
        {
            this->openFreqChoiceGui(listItem, profile, module);
            return true;
        }
        else if((keys & HidNpadButton_Y) == HidNpadButton_Y)
        {
            // Reset to "Default" (0 MHz)
            this->profileList->mhzMap[profile][module] = 0;
            listItem->setValue(module == HocClkModule_MEM ? formatListFreqMem(0, memUnit) : formatListFreqMHz(0));
            
            Result rc = hocclkIpcSetProfiles(this->applicationId, this->profileList);
            if(R_FAILED(rc))
            {
                FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
                return false;
            }
            return true;
        }
        return false;
    });
    this->listElement->addItem(listItem);
}

void AppProfileGui::addModuleListItemToggle(HocClkProfile profile, HocClkModule module)
{
    const char* moduleName = moduleNameRu(module);
    std::uint32_t currentValue = this->profileList->mhzMap[profile][module];
    
    tsl::elm::ToggleListItem* toggle = new tsl::elm::ToggleListItem(moduleName, currentValue != 0);
    
    toggle->setStateChangedListener([this, profile, module](bool state) {
        this->profileList->mhzMap[profile][module] = state ? 1 : 0;
        
        Result rc = hocclkIpcSetProfiles(this->applicationId, this->profileList);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
        }
    });
    
    this->listElement->addItem(toggle);
}

std::string AppProfileGui::formatValueDisplay(
    std::uint32_t value,
    const std::vector<NamedValue>& namedValues,
    const std::string& suffix,
    std::uint32_t divisor,
    int decimalPlaces
)
{
    if (value == 0) {
        return FREQ_DEFAULT_TEXT;
    }
    
    if (!namedValues.empty()) {
        for (const auto& namedValue : namedValues) {
            if (namedValue.value == value) {
                return namedValue.name;
            }
        }
    }
    
    char buf[32];
    if (decimalPlaces > 0) {
        double displayValue = (double)value / divisor;
        snprintf(buf, sizeof(buf), "%.*f%s", decimalPlaces, displayValue, suffix.c_str());
    } else {
        snprintf(buf, sizeof(buf), "%u%s", value / divisor, suffix.c_str());
    }
    return std::string(buf);
}

void AppProfileGui::addModuleListItemValue(
    HocClkProfile profile,
    HocClkModule module,
    const std::string& categoryName,
    std::uint32_t min,
    std::uint32_t max,
    std::uint32_t step,
    const std::string& suffix,
    std::uint32_t divisor,
    int decimalPlaces,
    ValueThresholds thresholds,
    std::vector<NamedValue> namedValues,
    bool showDefaultValue
)
{
    tsl::elm::ListItem* listItem =
        new tsl::elm::ListItem(moduleNameRu(module));
    std::uint32_t storedValue = this->profileList->mhzMap[profile][module];
    
    listItem->setValue(this->formatValueDisplay(storedValue, namedValues, suffix, divisor, decimalPlaces));
    
    listItem->setClickListener(
        [this,
         listItem,
         profile,
         module,
         categoryName,
         min,
         max,
         step,
         suffix,
         divisor,
         decimalPlaces,
         thresholds,
         namedValues,
         showDefaultValue](u64 keys)
        {
            if ((keys & HidNpadButton_A) == HidNpadButton_A)
            {
                std::uint32_t currentValue =
                    this->profileList->mhzMap[profile][module] * divisor;
                ValueRange range(
                    min,
                    max,
                    step,
                    suffix,
                    divisor,
                    decimalPlaces
                );
                this->openValueChoiceGui(
                    listItem,
                    currentValue,
                    range,
                    categoryName,
                    [this, listItem, profile, module, divisor, suffix, decimalPlaces, thresholds, namedValues](std::uint32_t value) -> bool
                    {
                        this->profileList->mhzMap[profile][module] = value / divisor;
                        listItem->setValue(this->formatValueDisplay(value / divisor, namedValues, suffix, divisor, decimalPlaces));
                        
                        Result rc =
                            hocclkIpcSetProfiles(this->applicationId,
                                                 this->profileList);
                        if (R_FAILED(rc))
                        {
                            FatalGui::openWithResultCode(
                                "hocclkIpcSetProfiles", rc);
                            return false;
                        }
                        return true;
                    },
                    thresholds,
                    false,
                    {},
                    namedValues,
                    showDefaultValue
                );
                return true;
            }
            else if ((keys & HidNpadButton_Y) == HidNpadButton_Y)
            {
                this->profileList->mhzMap[profile][module] = 0;
                listItem->setValue(FREQ_DEFAULT_TEXT);
                Result rc =
                    hocclkIpcSetProfiles(this->applicationId,
                                         this->profileList);
                if (R_FAILED(rc))
                {
                    FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
                    return false;
                }
                return true;
            }
            return false;
        });
    this->listElement->addItem(listItem);
}

class GovernorProfileSubMenuGui : public BaseMenuGui {
    uint64_t applicationId;
    HocClkTitleProfileList* profileList;
    HocClkProfile profile;
public:
    GovernorProfileSubMenuGui(uint64_t appId, HocClkTitleProfileList* pList, HocClkProfile prof)
        : applicationId(appId), profileList(pList), profile(prof) {}

    void listUI() override {
        BaseMenuGui::refresh(); // get latest context
        if(!this->context)
            return;
        Result rc = hocclkIpcGetConfigValues(&configList);
        if (R_FAILED(rc)) [[unlikely]] {
            FatalGui::openWithResultCode("hocclkIpcGetConfigValues", rc);
            return;
        }
        this->listElement->addItem(new tsl::elm::CategoryHeader("Говернер"));

        static constexpr struct { const char* label; int shift; } kAll[] = {
            {"ЦП", 0}, {"ГП", 8}, {"VRR", 16}
        };
        int count = configList.values[HocClkConfigValue_OverwriteRefreshRate] || this->context->isUsingRetroSuper ? 3 : 2;

        for (int i = 0; i < count; i++) {
            const bool isVrr = (kAll[i].shift == 16);
            u8 cur = (this->profileList->mhzMap[this->profile][HocClkModule_Governor] >> kAll[i].shift) & 0xFF;

            tsl::elm::NamedStepTrackBar* bar;
            if (isVrr) {
                bar = new tsl::elm::NamedStepTrackBar(
                    "", {"Не определять", "Отключено", "Включено", "Ryazha-Авто"},
                    true, kAll[i].label
                );
                const auto vm = livingLadder().config().vrrMode;
                if (vm == LadderVrr_Auto || vm == LadderVrr_Smart || vm == LadderVrr_SuperPro) {
                    cur = 3;
                }
            } else {
                bar = new tsl::elm::NamedStepTrackBar(
                    "", {"Не определять", "Отключено", "Включено"},
                    true, kAll[i].label
                );
            }
            bar->setProgress(cur);

            const int shift = kAll[i].shift;
            const bool vrrCaptured = isVrr;
            bar->setValueChangedListener([this, shift, vrrCaptured](u8 value) {
                u8 sysValue = value;
                if (vrrCaptured) {
                    auto& c = livingLadder().config();
                    const bool coreHad = (c.vrrMode == LadderVrr_Auto ||
                                          c.vrrMode == LadderVrr_Smart ||
                                          c.vrrMode == LadderVrr_SuperPro);
                    if (value == 3) {
                        sysValue = 1;   // sysmodule про "3" не знает — даём ему Disabled.
                        if (!coreHad) {
                            c.vrrMode = LadderVrr_Auto;
                            livingLadder().push();
                        }
                    } else if (coreHad) {
                        c.vrrMode = LadderVrr_Off;
                        livingLadder().push();
                    }
                }
                u32& packed = this->profileList->mhzMap[this->profile][HocClkModule_Governor];
                packed = (packed & ~(0xFFu << shift)) | ((u32)sysValue << shift);
                Result rc = hocclkIpcSetProfiles(this->applicationId, this->profileList);
                if (R_FAILED(rc)) FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
            });
            this->listElement->addItem(bar);
        }
    }
};

void AppProfileGui::addGovernorSection(HocClkProfile profile) {
    auto* item = new tsl::elm::ListItem("Говернер");
    item->setValue("\u2192"); // Right arrow
    item->setClickListener([this, profile](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<GovernorProfileSubMenuGui>(
                this->applicationId, this->profileList, profile
            );
            return true;
        }
        return false;
    });
    this->listElement->addItem(item);
}

void AppProfileGui::addProfileUI(HocClkProfile profile)
{    
    BaseMenuGui::refresh();
    if(!this->context)
        return;
    Result rc = hocclkIpcGetConfigValues(&configList);
    if (R_FAILED(rc)) [[unlikely]] {
        FatalGui::openWithResultCode("hocclkIpcGetConfigValues", rc);
        return;
    }
    if (profile == HocClkProfile_Docked && IsHoag()) {
        /* Lite: док-режима нет — показываем заголовок и пояснение вместо полного молчаливого пропуска. */
        this->listElement->addItem(new tsl::elm::CategoryHeader(profileNameRu(profile) + std::string(" ") + ult::DIVIDER_SYMBOL));
        auto* liteInfo = new tsl::elm::ListItem("Профиль дока на Switch Lite не используется");
        liteInfo->setValue("\u2014");
        this->listElement->addItem(liteInfo);
        return;
    }
    if (profile == HocClkProfile_HandheldCharging)
        return;
    this->listElement->addItem(new tsl::elm::CategoryHeader(profileNameRu(profile) + std::string(" ") + ult::DIVIDER_SYMBOL + " \ue0e3 Сброс"));
    this->addModuleListItem(profile, HocClkModule_CPU);
    this->addModuleListItem(profile, HocClkModule_GPU);
    this->addModuleListItem(profile, HocClkModule_MEM);
    #if IS_MINIMAL == 0
        if(configList.values[HocClkConfigValue_OverwriteRefreshRate]) {
            auto addDisplayTrackbar = [this, profile](u32 minHz, u32 maxHz, u32 stepHz) {
                u32 hi = ryazha_ui::displayHzOrDefault(maxHz);
                auto* bar = new ryazha_ui::DisplayHzTrackBar(minHz, hi, stepHz, "Дисплей");
                u32 cur = ryazha_ui::displayHzOrDefault(effectiveTitleProfileDisplayHz(this->profileList, profile));
                cur = std::clamp(cur, bar->minHz(), bar->maxHz());
                bar->setProgress(ryazha_ui::displayHzToProgress(cur, bar->minHz(), bar->maxHz(), bar->stepHz()));
                bar->setValueChangedListener([this, profile, bar](u16 value) {
                    const u32 hz = std::min(bar->maxHz(), bar->minHz() + (u32)value * bar->stepHz());
                    this->profileList->mhzMap[profile][HocClkModule_Display] = hz;
                    if (profile == HocClkProfile_Handheld) {
                        this->profileList->mhzMap[HocClkProfile_HandheldChargingUSB][HocClkModule_Display] = hz;
                        this->profileList->mhzMap[HocClkProfile_HandheldChargingOfficial][HocClkModule_Display] = hz;
                    }
                    Result rc = hocclkIpcSetProfiles(this->applicationId, this->profileList);
                    if (R_FAILED(rc)) {
                        FatalGui::openWithResultCode("hocclkIpcSetProfiles", rc);
                    }
                    ryazha_ui::syncLadderVrrMaxToPanelHz(hz);
                });
                this->listElement->addItem(bar);
            };

            if(profile != HocClkProfile_Docked) {
                u32 cap = (u32)configList.values[HocClkConfigValue_MaxDisplayClockH];
                addDisplayTrackbar(IsAula() ? 45u : 40u, cap, this->context->isUsingRetroSuper ? 5u : 1u);
            } else if(IsAula() && this->context->isSysDockInstalled) {
                addDisplayTrackbar(40u, 240u, 1u);
            } else if (IsAula() && !this->context->isSysDockInstalled) {
                addDisplayTrackbar(50u, 75u, 1u);
            } else {
                addDisplayTrackbar(50u, 120u, 1u);
            }
        }
    #endif
    this->addGovernorSection(profile);
}

void AppProfileGui::listUI()
{
    this->addProfileUI(HocClkProfile_Docked);
    this->addProfileUI(HocClkProfile_Handheld);
    this->addProfileUI(HocClkProfile_HandheldCharging);
    this->addProfileUI(HocClkProfile_HandheldChargingOfficial);
    this->addProfileUI(HocClkProfile_HandheldChargingUSB);
}

void AppProfileGui::changeTo(std::uint64_t applicationId)
{
    HocClkTitleProfileList* profileList = new HocClkTitleProfileList;
    Result rc = hocclkIpcGetProfiles(applicationId, profileList);
    if(R_FAILED(rc))
    {
        delete profileList;
        FatalGui::openWithResultCode("hocclkIpcGetProfiles", rc);
        return;
    }

    tsl::changeTo<AppProfileGui>(applicationId, profileList);
}

void AppProfileGui::update()
{
    BaseMenuGui::update();

    if((this->context && this->applicationId != this->context->applicationId) &&  this->applicationId != HOCCLK_GLOBAL_PROFILE_TID)
    {
        tsl::changeTo<FatalGui>(
            "Application changed\n\n"
            "\n"
            "The running application changed\n\n"
            "while editing was going on.",
            ""
        );
    }
}