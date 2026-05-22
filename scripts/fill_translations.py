#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Fills all language JSON files in Source/ryazha-clk/overlay/lang/ with every
i18n::t("...") key referenced in C++ source. Missing keys get a best-effort
fallback (English where known, otherwise = key).

Run from repo root: python3 scripts/fill_translations.py
"""

import json
import os
import re
import sys

SRC_ROOT = 'Source/ryazha-clk/overlay/src'
LANG_DIR = 'Source/ryazha-clk/overlay/lang'

# Match  i18n::t("...")  including escaped quotes/backslashes inside.
KEY_RE = re.compile(r'i18n::t\(\s*"((?:\\.|[^"\\])*)"\s*\)')

# Russian -> English (best-effort manual)
RU_TO_EN = {
    'Говернер': 'Governor',
    'Профиль дока на Switch Lite не используется': 'Switch Lite does not use the docked profile',
    'Ryazha-Авто': 'Ryazha-Auto',
    'Включено': 'Enabled',
    'Целевой FPS': 'Target FPS',
    'Алгоритм': 'Algorithm',
    'Лимиты': 'Limits',
    'Пределы ЦП / ГП': 'CPU / GPU bounds',
    'VRR (частота дисплея)': 'VRR (display refresh rate)',
    'Предиктор': 'Predictor',
    'Окно предиктора': 'Predictor window',
    'Порог скачка': 'Spike threshold',
    'Авто гамма (46-55 FPS)': 'Auto gamma (46-55 FPS)',
    'Профиль приложения': 'App profile',
    'Глобальный профиль': 'Global profile',
    'Временный профиль': 'Temporary profile',
    'Настройки': 'Settings',
    'Сброс': 'Reset',
    'OLED': 'OLED',
}

# Russian -> Ukrainian (close-but-different)
RU_TO_UK = {
    'Говернер': 'Регулятор',
    'Профиль дока на Switch Lite не используется':
        'Switch Lite не використовує профіль дока',
    'Ryazha-Авто': 'Ryazha-Авто',
    'Включено': 'Увімкнено',
    'Целевой FPS': 'Цільовий FPS',
    'Алгоритм': 'Алгоритм',
    'Лимиты': 'Ліміти',
    'Пределы ЦП / ГП': 'Межі ЦП / ГП',
    'VRR (частота дисплея)': 'VRR (частота дисплея)',
    'Предиктор': 'Предиктор',
    'Окно предиктора': 'Вікно предиктора',
    'Порог скачка': 'Поріг сплеску',
    'Авто гамма (46-55 FPS)': 'Авто гамма (46-55 FPS)',
    'Профиль приложения': 'Профіль додатка',
    'Глобальный профиль': 'Глобальний профіль',
    'Временный профиль': 'Тимчасовий профіль',
    'Настройки': 'Налаштування',
    'Сброс': 'Скидання',
    'OLED': 'OLED',
}


def is_cyrillic(text):
    return any('Ѐ' <= c <= 'ӿ' for c in text)


def main():
    keys_used = set()
    for root, _, files in os.walk(SRC_ROOT):
        for f in files:
            if not f.endswith(('.cpp', '.h', '.hpp', '.cc', '.cxx')):
                continue
            path = os.path.join(root, f)
            with open(path, encoding='utf-8', errors='replace') as fh:
                content = fh.read()
            for m in KEY_RE.finditer(content):
                # Unescape backslash sequences embedded in the C++ literal.
                key = m.group(1).encode('utf-8').decode('unicode_escape')
                keys_used.add(key)

    print(f'Found {len(keys_used)} unique i18n::t() keys in source.')

    langs = {}
    for f in os.listdir(LANG_DIR):
        if f.endswith('.json'):
            with open(os.path.join(LANG_DIR, f), encoding='utf-8') as fh:
                langs[f[:-5]] = json.load(fh)

    added = 0
    for code, d in langs.items():
        for key in keys_used:
            if key in d:
                continue
            if code == 'ru':
                # Russian keys: leave as-is (key IS Russian text).
                # English keys without a Russian translation: still better
                # to show English than nothing, until someone translates.
                d[key] = key
            elif code == 'uk':
                d[key] = RU_TO_UK.get(key, key)
            else:
                # All other langs default to English fallback.
                d[key] = RU_TO_EN.get(key, key)
            added += 1

    print(f'Added {added} fallback entries.')

    for code, d in langs.items():
        with open(os.path.join(LANG_DIR, f'{code}.json'),
                  'w', encoding='utf-8') as fh:
            json.dump(d, fh, ensure_ascii=False, indent=2, sort_keys=True)

    union = set()
    for d in langs.values():
        union.update(d.keys())
    print(f'Total unique keys across all langs: {len(union)}')
    for code in sorted(langs):
        print(f'  {code}: {len(langs[code])}')


if __name__ == '__main__':
    main()
