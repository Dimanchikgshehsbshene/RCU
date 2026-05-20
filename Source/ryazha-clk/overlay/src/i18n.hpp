/*
 * Copyright (c) Ryazha-CLK Contributors
 *
 * Lightweight translation helper. Loads /config/ryazha-clk/lang/<lang>.json
 * once at startup and provides a small `t(key)` lookup that falls back to the
 * key itself if no translation is present (so untranslated UI still renders).
 *
 */

#pragma once

#include <string>

namespace i18n {

    // Loads the appropriate language file based on the system language.
    // Should be called once during overlay initialization.
    void Initialize();

    // Returns the translation for `key` if loaded; otherwise returns `key`.
    // Returned by value to avoid dangling-reference issues when a literal is
    // passed (the temporary std::string would die at end of statement).
    std::string t(const std::string& key);

} // namespace i18n
