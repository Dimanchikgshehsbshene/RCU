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

#include <switch.h>

namespace i18n {

namespace {

constexpr const char* kLangDir = "sdmc:/config/ryazha-clk/lang/";

std::unordered_map<std::string, std::string> g_table;
std::once_flag g_initFlag;

// Maps Switch SetLanguage to our JSON file name (without .json).
const char* MapLanguage(SetLanguage lang) {
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
        default:                return "en";
    }
}

// Strip leading whitespace and any UTF-8 BOM in-place.
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

// Parse a JSON string literal at *p, advancing p past the closing quote.
// Returns true and fills out on success; false on malformed input.
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
                    // Decode \uXXXX as UTF-8. Only BMP code points; surrogate
                    // pairs collapse to '?' to keep the parser dependency-free.
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char c = p[i];
                        cp <<= 4;
                        if (c >= '0' && c <= '9') cp |= (c - '0');
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
        if (!key.empty()) {
            out.emplace(std::move(key), std::move(value));
        }
        SkipWhitespace(p, end);
        if (p < end && *p == ',') { ++p; SkipWhitespace(p, end); }
    }
    return true;
}

void LoadFile(const char* path) {
    FILE* fp = std::fopen(path, "rb");
    if (!fp) return;

    std::fseek(fp, 0, SEEK_END);
    long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > (1 << 20)) { std::fclose(fp); return; } // hard cap 1 MiB

    std::string buf;
    buf.resize((size_t)size);
    size_t got = std::fread(buf.data(), 1, (size_t)size, fp);
    std::fclose(fp);
    if (got != (size_t)size) return;

    ParseJsonObject(buf.data(), buf.size(), g_table);
}

void InitializeOnce() {
    u64 langCode = 0;
    SetLanguage setLang = SetLanguage_ENUS;

    Result rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        if (R_SUCCEEDED(setGetSystemLanguage(&langCode))) {
            (void)setMakeLanguage(langCode, &setLang);
        }
        setExit();
    }

    char path[128];
    std::snprintf(path, sizeof(path), "%s%s.json", kLangDir, MapLanguage(setLang));
    LoadFile(path);

    // If user's language file isn't present, fall back to English so we still
    // pick up the canonical English overrides rather than the in-code defaults.
    if (g_table.empty() && setLang != SetLanguage_ENUS) {
        std::snprintf(path, sizeof(path), "%sen.json", kLangDir);
        LoadFile(path);
    }
}

} // namespace

void Initialize() {
    std::call_once(g_initFlag, &InitializeOnce);
}

std::string t(const std::string& key) {
    Initialize();
    auto it = g_table.find(key);
    if (it != g_table.end()) return it->second;
    return key;
}

} // namespace i18n
