/*
 * Copyright (c) Ryazha-CLK Contributors
 *
 * Language selection submenu. Shows the list of bundled languages with their
 * native names; selecting one persists the choice to /config/ryazha-clk/config.ini
 * and reloads the translation table for the rest of the session.
 *
 */

#pragma once

#include "base_menu_gui.h"

class LanguageGui : public BaseMenuGui {
public:
    LanguageGui()  = default;
    ~LanguageGui() = default;

    void listUI() override;
    void refresh() override;
};
