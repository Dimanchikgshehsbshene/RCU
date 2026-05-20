#include "../../i18n.hpp"
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


#include "main_gui.h"

#include "fatal_gui.h"
#include "app_profile_gui.h"
#include "global_override_gui.h"
#include "misc_gui.h"
#include "living_ladder_gui.h"

void MainGui::listUI()
{
    // this->enabledToggle = new tsl::elm::ToggleListItem(i18n::t("Enable"), false);
    // enabledToggle->setStateChangedListener([this](bool state) {
    //     Result rc = rclkIpcSetEnabled(state);
    //     if(R_FAILED(rc))
    //     {
    //         FatalGui::openWithResultCode("rclkIpcSetEnabled", rc);
    //     }

    //     this->lastContextUpdate = armGetSystemTick();
    //     this->context->enabled = state;
    // });
    // this->listElement->addItem(this->enabledToggle);

    tsl::elm::ListItem* appProfileItem = new tsl::elm::ListItem(i18n::t("Профиль приложения"));
    appProfileItem->setClickListener([this](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            AppProfileGui::changeTo(this->context->applicationId);
            return true;
        }

        return false;
    });
    this->listElement->addItem(appProfileItem);


    tsl::elm::ListItem* globalProfileItem = new tsl::elm::ListItem(i18n::t("Глобальный профиль"));
    globalProfileItem->setClickListener([this](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            AppProfileGui::changeTo(RCLK_GLOBAL_PROFILE_TID);
            return true;
        }

        return false;
    });
    this->listElement->addItem(globalProfileItem);

    tsl::elm::ListItem* globalOverrideItem = new tsl::elm::ListItem(i18n::t("Временный профиль"));
    globalOverrideItem->setClickListener([this](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            tsl::changeTo<GlobalOverrideGui>();
            return true;
        }

        return false;
    });
    this->listElement->addItem(globalOverrideItem);

    // Пункт Ryazha-Авто специально скрыт из списка — открывается только кнопкой X (см. handleInput).

    //this->listElement->addItem(new tsl::elm::CategoryHeader(i18n::t("Misc")));

    tsl::elm::ListItem* miscItem = new tsl::elm::ListItem(i18n::t("Настройки"));
    miscItem->setClickListener([this](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            tsl::changeTo<MiscGui>();
            return true;
        }

        return false;
    });
    this->listElement->addItem(miscItem);

}

void MainGui::refresh()
{
    BaseMenuGui::refresh();
    //if(this->context)
    //{
    //    this->enabledToggle->setState(this->context->enabled);
    //}
}

bool MainGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick)
{
    (void)keysHeld; (void)touchPos; (void)leftJoyStick; (void)rightJoyStick;

    if ((keysDown & HidNpadButton_X) == HidNpadButton_X)
    {
        tsl::changeTo<LivingLadderGui>();
        return true;
    }
    return false;
}
