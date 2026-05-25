/*
 *
 * Copyright (c) Souldbminer and Ryazha CLK Contributors
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

#include "../format.h"
#include "../../i18n.hpp"
#include "fatal_gui.h"
#include "global_override_gui.h"
#include "value_choice_gui.h"
#include "labels.h"
#include "display_hz_trackbar.hpp"

GlobalOverrideGui::GlobalOverrideGui()
{
    for (std::uint16_t m = 0; m < RClkModule_EnumMax; m++) {
        this->listItems[m] = nullptr;
        this->listHz[m] = 0;
    }
}

void GlobalOverrideGui::openFreqChoiceGui(RClkModule module)
{
    std::uint32_t hzList[RCLK_FREQ_LIST_MAX];
    std::uint32_t hzCount;
    Result rc =
    rclkIpcGetFreqList(module, &hzList[0], RCLK_FREQ_LIST_MAX, &hzCount);
    if (R_FAILED(rc)) {
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
    tsl::changeTo<FreqChoiceGui>(
    this->context->overrideFreqs[module], hzList, hzCount, module,
    [this, module](std::uint32_t hz) {
        Result rc = rclkIpcSetOverride(module, hz);
        if (R_FAILED(rc)) {
            FatalGui::openWithResultCode("rclkIpcSetOverride", rc);
            return false;
        }

        this->lastContextUpdate = armGetSystemTick();
        this->context->overrideFreqs[module] = hz;

        return true;
    },
    true, labels
    );
}

void GlobalOverrideGui::openValueChoiceGui(
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

void GlobalOverrideGui::addModuleListItemValue(
    RClkModule module,
    const std::string& categoryName,
    std::uint32_t min,
    std::uint32_t max,
    std::uint32_t step,
    const std::string& suffix,
    std::uint32_t divisor,
    int decimalPlaces, 
    ValueThresholds thresholds,
    const std::vector<NamedValue>& namedValues,
    bool showDefaultValue
)
{
    bool hasNamedValues = !namedValues.empty();

    if (!hasNamedValues) {
        this->customFormatModules[module] = std::make_tuple(suffix, divisor, decimalPlaces);
    }

    tsl::elm::ListItem* listItem =
        new tsl::elm::ListItem(rclkFormatModule(module, true));
    
    listItem->setValue(FREQ_DEFAULT_TEXT);
    
    listItem->setClickListener(
        [this,
         listItem,
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
         hasNamedValues,
         showDefaultValue](u64 keys)
        {
            if ((keys & HidNpadButton_A) == HidNpadButton_A)
            {
                if (!this->context) {
                    return false;
                }
                
                std::uint32_t currentValue =
                    this->context->overrideFreqs[module] * divisor;
                
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
                    
                    [this, listItem, module, divisor, suffix, decimalPlaces, thresholds, namedValues, hasNamedValues, showDefaultValue](std::uint32_t value) -> bool
                    {
                        if (!this->context) {
                            return false;
                        }
                        
                        this->context->overrideFreqs[module] = value / divisor;
                        this->listHz[module] = value / divisor;
                        
                        if (value == 0) {
                            listItem->setValue(FREQ_DEFAULT_TEXT);
                        } else if (hasNamedValues) {
                            for (const auto& namedValue : namedValues) {
                                if (namedValue.value == value / divisor) {
                                    listItem->setValue(namedValue.name);
                                    break;
                                }
                            }
                        } else {
                            char buf[32];
                            if (decimalPlaces > 0) {
                                double displayValue = (double)value / divisor;
                                snprintf(buf, sizeof(buf), "%.*f%s", 
                                        decimalPlaces, displayValue, suffix.c_str());
                            } else {
                                snprintf(buf, sizeof(buf), "%u%s", 
                                        value / divisor, suffix.c_str());
                            }
                            listItem->setValue(buf);
                        }
                        
                        Result rc =
                            rclkIpcSetOverride(module, this->context->overrideFreqs[module]);
                        
                        if (R_FAILED(rc))
                        {
                            FatalGui::openWithResultCode(
                                "rclkIpcSetOverride", rc);
                            return false;
                        }
                        
                        this->lastContextUpdate = armGetSystemTick();
                        return true;
                    },
                    
                    thresholds,
                    false,
                    std::map<std::uint32_t, std::string>(),
                    namedValues,
                    showDefaultValue
                );
                
                return true;
            }
            else if ((keys & HidNpadButton_Y) == HidNpadButton_Y)
            {
                if (!this->context) {
                    return false;
                }
                
                this->context->overrideFreqs[module] = 0;
                this->listHz[module] = 0;
                listItem->setValue(FREQ_DEFAULT_TEXT);
                
                Result rc = rclkIpcSetOverride(module, 0);
                
                if (R_FAILED(rc))
                {
                    FatalGui::openWithResultCode("rclkIpcSetOverride", rc);
                    return false;
                }
                
                this->lastContextUpdate = armGetSystemTick();
                return true;
            }
            
            return false;
        });
    
    this->listElement->addItem(listItem);
    this->listItems[module] = listItem;
}

void GlobalOverrideGui::addModuleListItem(RClkModule module)
{
    tsl::elm::ListItem *listItem =
    new tsl::elm::ListItem(rclkFormatModule(module, true));
    RamDisplayUnit memUnit = (RamDisplayUnit)configList.values[RClkConfigValue_RamDisplayUnit];
    listItem->setValue(module == RClkModule_MEM ? formatListFreqMem(0, memUnit) : formatListFreqMHz(0));
    listItem->setClickListener([this, module](u64 keys) {
        if ((keys & HidNpadButton_A) == HidNpadButton_A) {
            this->openFreqChoiceGui(module);
            return true;
        } else if ((keys & HidNpadButton_Y) == HidNpadButton_Y) {
            Result rc = rclkIpcSetOverride(module, 0);
            if (R_FAILED(rc)) {
                FatalGui::openWithResultCode("rclkIpcSetOverride", rc);
                return false;
            }

            this->lastContextUpdate = armGetSystemTick();
            this->context->overrideFreqs[module] = 0;
            this->listHz[module] = 0;

            this->listItems[module]->setValue(module == RClkModule_MEM ? formatListFreqMem(0, (RamDisplayUnit)configList.values[RClkConfigValue_RamDisplayUnit]) : formatListFreqHz(0));

            return true;
        }
        return false;
    });

    this->listElement->addItem(listItem);
    this->listItems[module] = listItem;
}

void GlobalOverrideGui::addModuleToggleItem(RClkModule module)
{
    const char *moduleName = rclkFormatModule(module, true);
    bool isOn = this->listHz[module];

    tsl::elm::ToggleListItem *toggle =
    new tsl::elm::ToggleListItem(moduleName, isOn);

    toggle->setStateChangedListener([this, module, toggle](bool state) {
        Result rc = rclkIpcSetOverride(module, state ? 1 : 0);
        if (R_FAILED(rc)) {
            FatalGui::openWithResultCode("rclkIpcSetProfiles", rc);
        }
        this->lastContextUpdate = armGetSystemTick();
        this->context->overrideFreqs[module] = 0;
        this->listHz[module] = 0;
    });
    this->listElement->addItem(toggle);
    this->listItems[module] = toggle;
}

class GovernorOverrideSubMenuGui : public BaseMenuGui {
    u32 packed;
public:
    GovernorOverrideSubMenuGui(u32 initialPacked) : packed(initialPacked) {}

    void listUI() override {
        BaseMenuGui::refresh(); // get latest context
        if(!this->context)
            return;
        Result rc = rclkIpcGetConfigValues(&configList); // idk why this is needed, probably some refreshing issue
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
            u8 cur = (this->packed >> kAll[i].shift) & 0xFF;
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
                this->packed = (this->packed & ~(0xFFu << shift)) | ((u32)value << shift);
                Result rc = rclkIpcSetOverride(RClkModule_Governor, this->packed);
                if (R_FAILED(rc)) FatalGui::openWithResultCode("rclkIpcSetOverride", rc);
                this->lastContextUpdate = armGetSystemTick();
            });
            this->listElement->addItem(bar);
        }
    }
};

void GlobalOverrideGui::addGovernorSection() {
    auto* item = new tsl::elm::ListItem(i18n::t("Governor"));
    item->setValue("\u2192"); // right arrow
    item->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            u32 packed = this->context ? this->context->overrideFreqs[RClkModule_Governor] : 0;
            tsl::changeTo<GovernorOverrideSubMenuGui>(packed);
            return true;
        }
        return false;
    });
    this->listElement->addItem(item);
}

void GlobalOverrideGui::listUI()
{
    BaseMenuGui::refresh(); // get latest context
    if(!this->context)
        return;

    Result rc = rclkIpcGetConfigValues(&configList); // idk why this is needed, probably some refreshing issue
    if (R_FAILED(rc)) [[unlikely]] {
        FatalGui::openWithResultCode("rclkIpcGetConfigValues", rc);
        return;
    }

    this->listElement->addItem(new tsl::elm::CategoryHeader(
    i18n::t("Temporary Overrides") + " " + ult::DIVIDER_SYMBOL + " \ue0e3 " + i18n::t("Reset")));
    this->addModuleListItem(RClkModule_CPU);
    this->addModuleListItem(RClkModule_GPU);
    this->addModuleListItem(RClkModule_MEM);
    #if IS_MINIMAL == 0
        if(configList.values[RClkConfigValue_OverwriteRefreshRate]) {
            // Inline Hz trackbar. listItems[Display] остаётся nullptr
            // (bar -- не ListItem), но refresh() будет двигать
            // displayHzBar->setProgress() через side-table lastDisplayHz.
            u32 minHz  = IsAula() ? 45u : 40u;
            u32 maxHz  = configList.values[RClkConfigValue_MaxDisplayClockH];
            u32 stepHz = this->context->isUsingRetroSuper ? 5u : 1u;
            auto* bar = new ryazha_ui::DisplayHzTrackBar(minHz, maxHz, stepHz, "Display");
            u32 curHz = ryazha_ui::displayHzOrDefault(this->context->overrideFreqs[RClkModule_Display]);
            bar->setProgress(ryazha_ui::displayHzToProgress(curHz, bar->minHz(), bar->maxHz(), bar->stepHz()));
            auto apply = ryazha_ui::throttleApply([this](u32 hz) {
                rclkIpcSetOverride(RClkModule_Display, hz);
                ryazha_ui::syncLadderVrrMaxToPanelHz(hz);
                this->lastDisplayHz = hz;
            });
            bar->setValueChangedListener([bar, apply = std::move(apply)](u16 progress) mutable {
                apply(bar->minHz() + (u32)progress * bar->stepHz());
            });
            this->listElement->addItem(bar);
            this->displayHzBar = bar;
            this->lastDisplayHz = curHz;
        }
    #endif

    this->addGovernorSection();
}

void GlobalOverrideGui::refresh()
{
    BaseMenuGui::refresh();

    if (!this->context)
        return;

    for (std::uint16_t m = 0; m < RClkModule_EnumMax; m++) {
        if (m == RClkModule_Governor) {
            this->listHz[m] = this->context->overrideFreqs[m];
            continue;
        }

        if (this->listItems[m] != nullptr &&
            this->listHz[m] != this->context->overrideFreqs[m]) {
            
            auto it = this->customFormatModules.find((RClkModule)m);
            if (it != this->customFormatModules.end()) {
                std::string suffix = std::get<0>(it->second);
                std::uint32_t divisor = std::get<1>(it->second);
                int decimalPlaces = std::get<2>(it->second);
                
                if (this->context->overrideFreqs[m] == 0) {
                    this->listItems[m]->setValue(FREQ_DEFAULT_TEXT);
                } else {
                    char buf[32];
                    if (decimalPlaces > 0) {
                        double displayValue = (double)this->context->overrideFreqs[m] / divisor;
                        snprintf(buf, sizeof(buf), "%.*f%s", 
                                decimalPlaces, displayValue, suffix.c_str());
                    } else {
                        snprintf(buf, sizeof(buf), "%u%s", 
                                this->context->overrideFreqs[m] / divisor, suffix.c_str());
                    }
                    this->listItems[m]->setValue(buf);
                }
            } else {
                this->listItems[m]->setValue(
                    m == RClkModule_MEM
                        ? formatListFreqHzMem(this->context->overrideFreqs[m], (RamDisplayUnit)configList.values[RClkConfigValue_RamDisplayUnit])
                        : formatListFreqHz(this->context->overrideFreqs[m]));
            }
            
            this->listHz[m] = this->context->overrideFreqs[m];
        }
    }

    // DisplayHzTrackBar (если есть) не лежит в listItems[] -- синкаем через
    // side-table. Если sysmodule сам обновил overrideFreqs[Display] (например
    // через VRR auto-adjust), bar тоже двинется.
    if (this->displayHzBar) {
        u32 cur = this->context->overrideFreqs[RClkModule_Display];
        if (cur != this->lastDisplayHz) {
            u32 disp = ryazha_ui::displayHzOrDefault(cur);
            this->displayHzBar->setProgress(ryazha_ui::displayHzToProgress(
                disp, this->displayHzBar->minHz(),
                this->displayHzBar->maxHz(), this->displayHzBar->stepHz()));
            this->lastDisplayHz = cur;
        }
    }
}