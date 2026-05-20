/*
 *
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
#include "../../ipc.h"
#include "base_menu_gui.h"
#include <initializer_list>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>
#include "freq_choice_gui.h"
#include "value_choice_gui.h"
class MiscGui : public BaseMenuGui
{
public:
    MiscGui();
    ~MiscGui();
    void listUI() override;
    void refresh() override;
    
protected:
    RClkConfigValueList* configList;
    std::map<RClkConfigValue, tsl::elm::ListItem*> configButtons;
    std::map<RClkConfigValue, ValueRange> configRanges;
    std::map<RClkConfigValue, std::vector<NamedValue>> configNamedValues;
    std::map<RClkConfigValue, tsl::elm::ToggleListItem*> configToggles;
    std::map<RClkConfigValue, std::tuple<tsl::elm::TrackBar*, tsl::elm::ListItem*, std::vector<uint64_t>>> configTrackbars;
    std::set<RClkConfigValue> configButtonSKeys;
    std::map<RClkConfigValue, std::string> configButtonSSubtext;
    std::set<RClkConfigValue> emcClockConfigs;
    
    void addConfigToggle(RClkConfigValue configVal, const char* altName, bool kip = false);
    void addConfigTrackbar(RClkConfigValue configVal, const char* altName, const ValueRange& range, bool kip = true);
    void addMappedConfigTrackbar(RClkConfigValue configVal, const char* altName,
                                  std::vector<u32> vals,
                                  std::initializer_list<std::string> names, bool kip = true);
    void addConfigButton(RClkConfigValue configVal,
        const char* altName,
        const ValueRange& range,
        const std::string& categoryName,
        const ValueThresholds* thresholds,
        const std::map<uint32_t, std::string>& labels = {},
        const std::vector<NamedValue>& namedValues = {},
        bool showDefaultValue = true,
        bool kip = false);

    void addConfigButtonS(RClkConfigValue configVal,
        const char* altName,
        const ValueRange& range,
        const std::string& categoryName,
        const ValueThresholds* thresholds,
        const std::map<uint32_t, std::string>& labels = {},
        const std::vector<NamedValue>& namedValues = {},
        bool showDefaultValue = true,
        const char* subText = nullptr,
        bool kip = false);
    void addFreqButton(RClkConfigValue configVal,
                            const char* altName,
                            RClkModule module,
                            const std::map<uint32_t, std::string>& labels = {});
    void updateConfigToggles();
    
    tsl::elm::ToggleListItem* enabledToggle;
    u8 frameCounter = 60;
    bool shouldSaveKip = false;
};