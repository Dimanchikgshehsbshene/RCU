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

#include <map>
#include "base_menu_gui.h"
#include "living_ladder.h"

class LivingLadderGui : public BaseMenuGui
{
public:
    LivingLadderGui() = default;
    ~LivingLadderGui() = default;

    void listUI() override;
    void refresh() override;

private:
    // Раздел «Ryazha-Авто».
    tsl::elm::ToggleListItem* enabledToggle  = nullptr;
    tsl::elm::ListItem*       targetFpsItem  = nullptr;
    tsl::elm::ListItem*       algoItem       = nullptr;

    // Лимиты.
    tsl::elm::ListItem*       tempCpuItem    = nullptr;
    tsl::elm::ListItem*       tempGpuItem    = nullptr;
    tsl::elm::NamedStepTrackBar* tdpCapItem  = nullptr;

    // Границы.
    tsl::elm::ListItem*       userCpuMinItem = nullptr;
    tsl::elm::ListItem*       userCpuMaxItem = nullptr;
    tsl::elm::ListItem*       userGpuMinItem = nullptr;
    tsl::elm::ListItem*       userGpuMaxItem = nullptr;

    // VRR.
    tsl::elm::NamedStepTrackBar* vrrModeItem = nullptr;
    tsl::elm::ListItem*       vrrMinItem     = nullptr;
    tsl::elm::ListItem*       vrrMaxItem     = nullptr;
    tsl::elm::ToggleListItem* predictorToggle = nullptr;
    tsl::elm::ListItem*       predictorWindowItem = nullptr;
    tsl::elm::NamedStepTrackBar* predictorAggrItem = nullptr;
    tsl::elm::ListItem*       predictorSpikeItem  = nullptr;

    // OLED.
    tsl::elm::ToggleListItem* oledGammaToggle = nullptr;

    void refreshControlLabels();

    static const char* algoName(LadderAlgo a);
    static const char* vrrModeName(LadderVrrMode m);
};
