/*
 * Copyright (c) Ryazha-CLK Contributors
 *
 * Lightweight translation helper for the Ryazha-CLK overlay.
 *
 * Behaviour:
 *   - Default language is Russian ("ru") for fresh installs.
 *   - User preference is persisted in /config/ryazha-clk/config.ini under
 *     [config] language=<code>.
 *   - If no preference is set, falls back to the Switch's system language.
 *   - Loads /config/ryazha-clk/lang/<lang>.json once per language; subsequent
 *     calls to t() are an O(1) hash lookup.
 *
 */

#pragma once

#include <string>
#include <vector>

namespace i18n {

    // Loads the appropriate language file. Idempotent; cheap if already loaded.
    void Initialize();

    // Switch the active language at runtime (e.g. from a settings menu).
    // Persists the choice to config.ini and reloads the translation table.
    // Named ApplyLanguage so it does not collide with libnx's `SetLanguage` enum.
    void ApplyLanguage(const std::string& code);

    // Returns the active language code (e.g. "ru", "en"). Empty before Initialize().
    std::string CurrentLanguage();

    // Returns the translation for `key` if loaded; otherwise returns `key`.
    // Returned by value to avoid dangling references when literals are passed.
    std::string t(const std::string& key);

    struct LangEntry {
        const char* code;   // file/ini code  (e.g. "ru")
        const char* native; // language name in itself (e.g. "Русский")
    };

    // All shipped languages — used to populate the language settings menu.
    const std::vector<LangEntry>& AvailableLanguages();

} // namespace i18n
