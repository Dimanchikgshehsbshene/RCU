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


#include "app_profile_gui.h"

#include "../format.h"
#include "../../i18n.hpp"
#include "fatal_gui.h"
#include "labels.h"
#include "display_hz_trackbar.hpp"
AppProfileGui::AppProfileGui(std::uint64_t applicationId, RClkTitleProfileList* profileList)
{
    this->applicationId = applicationId;
    this->profileList = profileList;
}

AppProfileGui::~AppProfileGui()
{
    delete this->profileList;
}

void AppProfileGui::openFreqChoiceGui(tsl::elm::ListItem* listItem, RClkProfile profile, RClkModule module)
{
    std::uint32_t hzList[RCLK_FREQ_LIST_MAX];
    std::uint32_t hzCount;
    Result rc = rclkIpcGetFreqList(module, &hzList[0], RCLK_FREQ_LIST_MAX, &hzCount);
    if(R_FAILED(rc))
    {
        FatalGui::openWithResultCode("rclkIpcGetFreqList", rc);
        return;
    }
    std::map<uint32_t, std::string> labels = {};

    if (module == RClkModule_CPU) {
        bool isUsingUv = IsMariko() ? configList.values[KipConfigValue_marikoCpuUVHigh] : configList.values[KipConfigValue_eristaCpuUV];
        labels = IsMariko() ? (isUsingUv ? cpu_freq_label_m_uv : cpu_freq_label_m) : (isUsingUv ? cpu_freq_label_e_uv : cpu_freq_label_e);
    } else if (module == RClkModule_GPU) {
        labels = IsMariko() ? *(marikoUV[configList.values[KipConfigValue_marikoGpuUV]]) : *(eristaUV[configList.values[KipConfigValue_eristaGpuUV]]);
    }
    RamDisplayUnit memUnit = (RamDisplayUnit)configList.values[RClkConfigValue_RamDisplayUnit];
    tsl::changeTo<FreqChoiceGui>(this->profileList->mhzMap[profile][module] * 1000000, hzList, hzCount, module, [this, listItem, profile, module, memUnit](std::uint32_t hz) {
        this->profileList->mhzMap[profile][module] = hz / 1000000;
        std::uint32_t mhz = this->profileList->mhzMap[profile][module];
        listItem->setValue(module == RClkModule_MEM ? formatListFreqMem(mhz, memUnit) : formatListFreqMHz(mhz));
        Result rc = rclkIpcSetProfiles(this->applicationId, this->profileList);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
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

void AppProfileGui::addModuleListItem(RClkProfile profile, RClkModule module)
{
    tsl::elm::ListItem* listItem = new tsl::elm::ListItem(rclkFormatModule(module, true));
    RamDisplayUnit memUnit = (RamDisplayUnit)configList.values[RClkConfigValue_RamDisplayUnit];
    std::uint32_t mhz = this->profileList->mhzMap[profile][module];
    listItem->setValue(module == RClkModule_MEM ? formatListFreqMem(mhz, memUnit) : formatListFreqMHz(mhz));
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
            listItem->setValue(module == RClkModule_MEM ? formatListFreqMem(0, memUnit) : formatListFreqMHz(0));
            
            Result rc = rclkIpcSetProfiles(this->applicationId, this->profileList);
            if(R_FAILED(rc))
            {
                FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
                return false;
            }
            return true;
        }
        return false;
    });
    this->listElement->addItem(listItem);
}

void AppProfileGui::addModuleListItemToggle(RClkProfile profile, RClkModule module)
{
    const char* moduleName = rclkFormatModule(module, true);
    std::uint32_t currentValue = this->profileList->mhzMap[profile][module];
    
    tsl::elm::ToggleListItem* toggle = new tsl::elm::ToggleListItem(moduleName, currentValue != 0);
    
    toggle->setStateChangedListener([this, profile, module](bool state) {
        this->profileList->mhzMap[profile][module] = state ? 1 : 0;
        
        Result rc = rclkIpcSetProfiles(this->applicationId, this->profileList);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
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
    RClkProfile profile,
    RClkModule module,
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
        new tsl::elm::ListItem(rclkFormatModule(module, true));
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
                            rclkIpcSetProfiles(this->applicationId,
                                                 this->profileList);
                        if (R_FAILED(rc))
                        {
                            FatalGui::openWithResultCode(
                                "rclkIpcSetProfiles", rc);
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
                    rclkIpcSetProfiles(this->applicationId,
                                         this->profileList);
                if (R_FAILED(rc))
                {
                    FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
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
    RClkTitleProfileList* profileList;
    RClkProfile profile;
public:
    GovernorProfileSubMenuGui(uint64_t appId, RClkTitleProfileList* pList, RClkProfile prof)
        : applicationId(appId), profileList(pList), profile(prof) {}

    void listUI() override {
        BaseMenuGui::refresh(); // get latest context
        if(!this->context)
            return;
        Result rc = rclkIpcGetConfigValues(&configList);
        if (R_FAILED(rc)) [[unlikely]] {
            FatalGui::openWithResultCode("rclkIpcGetConfigValues", rc);
            return;
        }
        this->listElement->addItem(new tsl::elm::CategoryHeader(i18n::t("Governor")));

        static constexpr struct { const char* label; int shift; } kAll[] = {
            {"CPU", 0}, {"GPU", 8}, {"VRR", 16}
        };
        int count = configList.values[RClkConfigValue_OverwriteRefreshRate] || this->context->isUsingRetroSuper ? 3 : 2;

        for (int i = 0; i < count; i++) {
            const bool isVrrSlot = (kAll[i].shift == 16);
            u8 cur = (this->profileList->mhzMap[this->profile][RClkModule_Governor] >> kAll[i].shift) & 0xFF;
            // NamedStepTrackBar требует std::initializer_list<std::string>
            // (compile-time fixed list). Branch'имся: VRR-слот = 4 опции,
            // CPU/GPU = 3.
            tsl::elm::NamedStepTrackBar* bar;
            if (isVrrSlot) {
                bar = new tsl::elm::NamedStepTrackBar(
                    "",
                    { i18n::t("Do Not Override"), i18n::t("Disabled"),
                      i18n::t("Enabled"),         i18n::t("VRR-Auto") },
                    true, kAll[i].label);
            } else {
                bar = new tsl::elm::NamedStepTrackBar(
                    "",
                    { i18n::t("Do Not Override"), i18n::t("Disabled"),
                      i18n::t("Enabled") },
                    true, kAll[i].label);
            }
            bar->setProgress(cur);
            int shift = kAll[i].shift;
            bar->setValueChangedListener([this, shift](u8 value) {
                u32& packed = this->profileList->mhzMap[this->profile][RClkModule_Governor];
                packed = (packed & ~(0xFFu << shift)) | ((u32)value << shift);
                Result rc = rclkIpcSetProfiles(this->applicationId, this->profileList);
                if (R_FAILED(rc)) FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
            });
            this->listElement->addItem(bar);
        }
    }
};

void AppProfileGui::addDisplayHzTrackBar(RClkProfile profile,
                                         std::uint32_t minHz,
                                         std::uint32_t maxHz,
                                         std::uint32_t stepHz)
{
    std::string label = rclkFormatModule(RClkModule_Display, true);
    auto* bar = new ryazha_ui::DisplayHzTrackBar(minHz, maxHz, stepHz, label);
    u32 curHz = ryazha_ui::displayHzOrDefault(
        this->profileList->mhzMap[profile][RClkModule_Display]);
    bar->setProgress(ryazha_ui::displayHzToProgress(
        curHz, bar->minHz(), bar->maxHz(), bar->stepHz()));
    // Throttle apply -- иначе rapid slider drag шлёт rclkIpcSetProfiles
    // каждый кадр (~60/сек), сериализация RClkTitleProfileList тяжёлая,
    // UI зависает. Throttle 50ms = ~20 IPC/сек, плавно и без задержек.
    auto apply = ryazha_ui::throttleApply([this, profile](u32 hz) {
        this->profileList->mhzMap[profile][RClkModule_Display] = hz;
        Result rc = rclkIpcSetProfiles(this->applicationId, this->profileList);
        if (R_FAILED(rc)) FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
        ryazha_ui::syncLadderVrrMaxToPanelHz(hz);
    });
    bar->setValueChangedListener([bar, apply = std::move(apply)](u16 progress) mutable {
        apply(bar->minHz() + (u32)progress * bar->stepHz());
    });
    this->listElement->addItem(bar);
}

void AppProfileGui::addGovernorSection(RClkProfile profile) {
    auto* item = new tsl::elm::ListItem(i18n::t("Governor"));
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

void AppProfileGui::addProfileUI(RClkProfile profile)
{    
    BaseMenuGui::refresh();
    if(!this->context)
        return;
    Result rc = rclkIpcGetConfigValues(&configList);
    if (R_FAILED(rc)) [[unlikely]] {
        FatalGui::openWithResultCode("rclkIpcGetConfigValues", rc);
        return;
    }
    if((profile == RClkProfile_Docked && IsHoag()) || profile == RClkProfile_HandheldCharging)
        return;
    this->listElement->addItem(new tsl::elm::CategoryHeader(rclkFormatProfile(profile, true) + std::string(" ") + ult::DIVIDER_SYMBOL + " \ue0e3 Reset"));
    this->addModuleListItem(profile, RClkModule_CPU);
    this->addModuleListItem(profile, RClkModule_GPU);
    this->addModuleListItem(profile, RClkModule_MEM);
    #if IS_MINIMAL == 0
        // Inline Hz slider вместо табличного picker'а. Юзер сказал
        // "верни ползунок" -- DisplayHzTrackBar поддерживает Y=reset 60,
        // динамически адаптирует step под 101 шаг StepTrackBar лимит.
        if(configList.values[RClkConfigValue_OverwriteRefreshRate]) {
            if(profile != RClkProfile_Docked) {
                u32 minHz  = IsAula() ? 45u : 40u;
                u32 maxHz  = configList.values[RClkConfigValue_MaxDisplayClockH];
                u32 stepHz = this->context->isUsingRetroSuper ? 5u : 1u;
                this->addDisplayHzTrackBar(profile, minHz, maxHz, stepHz);
            } else {
                if(IsAula() && this->context->isSysDockInstalled) {
                    this->addDisplayHzTrackBar(profile, 40, 240, 1);
                } else if (IsAula() && !this->context->isSysDockInstalled) {
                    this->addDisplayHzTrackBar(profile, 50, 75, 1);
                } else {
                    this->addDisplayHzTrackBar(profile, 50, 120, 1);
                }
            }
        }
    #endif
    this->addGovernorSection(profile);
}

void AppProfileGui::listUI()
{
    this->addProfileUI(RClkProfile_Docked);
    this->addProfileUI(RClkProfile_Handheld);
    this->addProfileUI(RClkProfile_HandheldCharging);
    this->addProfileUI(RClkProfile_HandheldChargingOfficial);
    this->addProfileUI(RClkProfile_HandheldChargingUSB);
}

void AppProfileGui::changeTo(std::uint64_t applicationId)
{
    RClkTitleProfileList* profileList = new RClkTitleProfileList;
    Result rc = rclkIpcGetProfiles(applicationId, profileList);
    if(R_FAILED(rc))
    {
        delete profileList;
        FatalGui::openWithResultCode("rclkIpcGetProfiles", rc);
        return;
    }

    tsl::changeTo<AppProfileGui>(applicationId, profileList);
}

void AppProfileGui::update()
{
    BaseMenuGui::update();

    if((this->context && this->applicationId != this->context->applicationId) &&  this->applicationId != RCLK_GLOBAL_PROFILE_TID)
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