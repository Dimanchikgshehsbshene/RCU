/*
 * Copyright (c) Ryazha-CLK Contributors
 *
 * See language_gui.h.
 *
 */

#include "language_gui.h"

#include "../../i18n.hpp"

#include <tesla.hpp>

namespace {

constexpr const char* CHECK_MARK = "✓"; // ✓
constexpr const char* R_ARROW    = "▶"; // ▶ (placeholder, unused here)

} // namespace

void LanguageGui::listUI() {
    this->listElement->addItem(new tsl::elm::CategoryHeader(i18n::t("Language")));

    const std::string current = i18n::CurrentLanguage();

    for (const auto& lang : i18n::AvailableLanguages()) {
        std::string code   = lang.code;
        std::string native = lang.native;

        auto* item = new tsl::elm::ListItem(native);
        if (code == current)
            item->setValue(CHECK_MARK);

        item->setClickListener([code](u64 keys) {
            if (keys & HidNpadButton_A) {
                i18n::ApplyLanguage(code);
                // Pop back so the parent menu re-renders with new strings.
                tsl::goBack();
                return true;
            }
            return false;
        });

        this->listElement->addItem(item);
    }
}

void LanguageGui::refresh() {
    // Nothing to refresh dynamically — list is static for the lifetime of the menu.
}
