/*
 * Copyright (c) Ryazha-CLK Contributors
 *
 * See i18n.hpp for an overview.
 *
 */

#include "i18n.hpp"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>
#include <switch.h>

namespace i18n {

namespace {

constexpr const char* kLangDir    = "sdmc:/config/ryazha-clk/lang/";
constexpr const char* kConfigPath = "sdmc:/config/ryazha-clk/config.ini";

std::unordered_map<std::string, std::string> g_table;
std::string                                  g_currentLang;
std::mutex                                   g_mutex;
bool                                         g_initialized = false;

// Maps Switch SetLanguage to our JSON file name (without .json).
// Use the global-scope name so our `i18n::ApplyLanguage` does not shadow it.
const char* MapSystemLanguage(::SetLanguage lang) {
    switch (lang) {
        case SetLanguage_JA:    return "ja";
        case SetLanguage_ENUS:
        case SetLanguage_ENGB:  return "en";
        case SetLanguage_FR:
        case SetLanguage_FRCA:  return "fr";
        case SetLanguage_DE:    return "de";
        case SetLanguage_IT:    return "it";
        case SetLanguage_ES:
        case SetLanguage_ES419: return "es";
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS: return "zh-cn";
        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT: return "zh-tw";
        case SetLanguage_KO:    return "ko";
        case SetLanguage_NL:    return "nl";
        case SetLanguage_PT:
        case SetLanguage_PTBR:  return "pt";
        case SetLanguage_RU:    return "ru";
        default:                return "ru"; // policy: default to Russian
    }
}

// ----- Minimal INI helpers (read/write a single key under [config]) -------

bool ReadConfigLanguage(std::string& out) {
    FILE* f = std::fopen(kConfigPath, "r");
    if (!f) return false;
    char line[256];
    bool inConfig = false;
    bool found = false;
    while (std::fgets(line, sizeof(line), f)) {
        // Strip newline
        size_t n = std::strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        // Trim leading whitespace
        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '#' || *p == ';' || *p == 0) continue;
        if (*p == '[') {
            inConfig = (std::strncmp(p, "[config]", 8) == 0);
            continue;
        }
        if (!inConfig) continue;
        char* eq = std::strchr(p, '=');
        if (!eq) continue;
        *eq = 0;
        // Trim key
        char* keyEnd = eq;
        while (keyEnd > p && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t')) --keyEnd;
        *keyEnd = 0;
        if (std::strcmp(p, "language") != 0) continue;
        char* val = eq + 1;
        while (*val == ' ' || *val == '\t') ++val;
        out.assign(val);
        found = true;
        break;
    }
    std::fclose(f);
    return found && !out.empty();
}

void WriteConfigLanguage(const std::string& lang) {
    // Read the file (if any), copy preserving everything except old language= line,
    // and ensure [config] section exists with the new language line.
    std::string content;
    {
        FILE* f = std::fopen(kConfigPath, "r");
        if (f) {
            char buf[1024];
            size_t n;
            while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
                content.append(buf, n);
            std::fclose(f);
        }
    }

    std::string out;
    out.reserve(content.size() + 64);

    bool inConfig = false;
    bool wroteLang = false;
    size_t i = 0;
    while (i < content.size()) {
        size_t lineEnd = content.find('\n', i);
        if (lineEnd == std::string::npos) lineEnd = content.size();
        std::string line = content.substr(i, lineEnd - i);
        i = lineEnd + 1;

        std::string trimmed = line;
        size_t a = 0;
        while (a < trimmed.size() && (trimmed[a]==' ' || trimmed[a]=='\t')) ++a;
        trimmed.erase(0, a);

        if (!trimmed.empty() && trimmed.front() == '[') {
            // Flush language= into the previous [config] section if not done yet.
            if (inConfig && !wroteLang) {
                out += "language=" + lang + "\n";
                wroteLang = true;
            }
            inConfig = (trimmed.rfind("[config]", 0) == 0);
            out += line + "\n";
            continue;
        }

        if (inConfig) {
            // Skip any existing language= line.
            if (trimmed.rfind("language", 0) == 0) {
                size_t eq = trimmed.find('=');
                if (eq != std::string::npos) {
                    std::string key = trimmed.substr(0, eq);
                    // Trim
                    while (!key.empty() && (key.back()==' ' || key.back()=='\t')) key.pop_back();
                    if (key == "language") continue;
                }
            }
        }

        out += line;
        if (lineEnd != content.size()) out += "\n";
    }

    if (!wroteLang) {
        if (!out.empty() && out.back() != '\n') out += "\n";
        if (!inConfig && content.find("[config]") == std::string::npos) {
            out += "[config]\n";
        }
        out += "language=" + lang + "\n";
    }

    // Ensure directory exists, then write atomically-ish (write+rename).
    mkdir("sdmc:/config", 0777);
    mkdir("sdmc:/config/ryazha-clk", 0777);

    std::string tmp = std::string(kConfigPath) + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "w");
    if (!f) return;
    std::fwrite(out.data(), 1, out.size(), f);
    std::fclose(f);
    std::remove(kConfigPath);
    std::rename(tmp.c_str(), kConfigPath);
}

// ----- JSON parsing (single-pass, no deps) --------------------------------

void SkipWhitespace(const char*& p, const char* end) {
    while (p < end) {
        const unsigned char c = (unsigned char)*p;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++p; continue; }
        if ((end - p) >= 3 && (unsigned char)p[0] == 0xEF
                          && (unsigned char)p[1] == 0xBB
                          && (unsigned char)p[2] == 0xBF) { p += 3; continue; }
        break;
    }
}

bool ParseString(const char*& p, const char* end, std::string& out) {
    if (p >= end || *p != '"') return false;
    ++p;
    out.clear();
    while (p < end && *p != '"') {
        if (*p == '\\' && (p + 1) < end) {
            char esc = p[1];
            p += 2;
            switch (esc) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                case '"': out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'u': {
                    if ((end - p) < 4) return false;
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char c = p[i];
                        cp <<= 4;
                        if      (c >= '0' && c <= '9') cp |= (c - '0');
                        else if (c >= 'a' && c <= 'f') cp |= (c - 'a' + 10);
                        else if (c >= 'A' && c <= 'F') cp |= (c - 'A' + 10);
                        else return false;
                    }
                    p += 4;
                    if (cp < 0x80) {
                        out.push_back((char)cp);
                    } else if (cp < 0x800) {
                        out.push_back((char)(0xC0 | (cp >> 6)));
                        out.push_back((char)(0x80 | (cp & 0x3F)));
                    } else if (cp >= 0xD800 && cp <= 0xDFFF) {
                        out.push_back('?');
                    } else {
                        out.push_back((char)(0xE0 | (cp >> 12)));
                        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                        out.push_back((char)(0x80 | (cp & 0x3F)));
                    }
                    break;
                }
                default: out.push_back(esc); break;
            }
        } else {
            out.push_back(*p++);
        }
    }
    if (p >= end || *p != '"') return false;
    ++p;
    return true;
}

bool ParseJsonObject(const char* data, size_t size,
                     std::unordered_map<std::string, std::string>& out) {
    const char* p   = data;
    const char* end = data + size;
    SkipWhitespace(p, end);
    if (p >= end || *p != '{') return false;
    ++p;
    SkipWhitespace(p, end);
    while (p < end && *p != '}') {
        std::string key, value;
        if (!ParseString(p, end, key)) return false;
        SkipWhitespace(p, end);
        if (p >= end || *p != ':') return false;
        ++p;
        SkipWhitespace(p, end);
        if (!ParseString(p, end, value)) return false;
        // operator[]= вместо emplace -- если в JSON встречается дубль,
        // последнее значение wins (логично: автор переписывал перевод).
        // Раньше emplace игнорировал дубль -> непредсказуемо.
        if (!key.empty()) out[std::move(key)] = std::move(value);
        SkipWhitespace(p, end);
        if (p < end && *p == ',') { ++p; SkipWhitespace(p, end); }
    }
    return true;
}

bool LoadLanguageFile(const std::string& code) {
    std::string path = std::string(kLangDir) + code + ".json";
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) return false;

    std::fseek(fp, 0, SEEK_END);
    long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > (1 << 20)) { std::fclose(fp); return false; }

    std::string buf((size_t)size, 0);
    size_t got = std::fread(buf.data(), 1, (size_t)size, fp);
    std::fclose(fp);
    if (got != (size_t)size) return false;

    std::unordered_map<std::string, std::string> tmp;
    if (!ParseJsonObject(buf.data(), buf.size(), tmp)) return false;

    g_table = std::move(tmp);
    g_currentLang = code;
    return true;
}

void InitializeLocked() {
    if (g_initialized) return;
    g_initialized = true;

    // 1) Config preference wins.
    std::string lang;
    if (ReadConfigLanguage(lang) && !lang.empty()) {
        if (LoadLanguageFile(lang)) return;
    }

    // 2) Otherwise, derive from system language.
    u64 langCode = 0;
    ::SetLanguage setLang = SetLanguage_ENUS;
    if (R_SUCCEEDED(setInitialize())) {
        if (R_SUCCEEDED(setGetSystemLanguage(&langCode))) {
            (void)setMakeLanguage(langCode, &setLang);
        }
        setExit();
    }
    if (LoadLanguageFile(MapSystemLanguage(setLang))) return;

    // 3) Last resort: try ru, then en.
    if (LoadLanguageFile("ru")) return;
    LoadLanguageFile("en");
}

} // namespace

void Initialize() {
    std::lock_guard<std::mutex> lk(g_mutex);
    InitializeLocked();
}

void ApplyLanguage(const std::string& code) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!LoadLanguageFile(code)) return;
    WriteConfigLanguage(code);
}

std::string CurrentLanguage() {
    std::lock_guard<std::mutex> lk(g_mutex);
    InitializeLocked();
    return g_currentLang;
}

std::string t(const std::string& key) {
    std::lock_guard<std::mutex> lk(g_mutex);
    InitializeLocked();
    auto it = g_table.find(key);
    if (it != g_table.end()) return it->second;
    return key;
}

const std::vector<LangEntry>& AvailableLanguages() {
    // Plain string literals (no u8 prefix) so they are const char* in C++20.
    static const std::vector<LangEntry> kLangs = {
        {"ru",    "Русский"},
        {"en",    "English"},
        {"uk",    "Українська"},
        {"de",    "Deutsch"},
        {"es",    "Español"},
        {"fr",    "Français"},
        {"it",    "Italiano"},
        {"nl",    "Nederlands"},
        {"pl",    "Polski"},
        {"pt",    "Português"},
        {"ja",    "日本語"},
        {"ko",    "한국어"},
        {"zh-cn", "简体中文"},
        {"zh-tw", "繁體中文"},
    };
    return kLangs;
}

} // namespace i18n
