/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
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

#include "base_gui.h"

#include "../elements/base_frame.h"

#include <tesla.hpp>
#include <math.h>

#define LOGO_X 20
#define LOGO_Y 50
#define LOGO_LABEL_FONT_SIZE 45

#define BADGE_LEFT_PAD        18
#define BADGE_PAD_X           12
#define BADGE_PAD_Y            5
#define BADGE_RADIUS           7
#define BADGE_BORDER           1
#define BADGE_VER_FONT_SIZE   16
#define BADGE_LABEL_FONT_SIZE 17
#define BADGE_LINE_GAP         1
#define RYAZHA_BADGE_LABEL    "RCU-Mode"

std::string getVersionString() {
    char buf[0x100] = "";
    Result rc = hocclkIpcGetVersionString(buf, sizeof(buf));
    if (R_FAILED(rc) || buf[0] == '\0') {
        return "Unknown";
    }
    return std::string(buf);
}

static constexpr tsl::Color kLogoPalette[] = {
    tsl::Color(15, 0, 0, 15),   // red
    tsl::Color(15, 15, 0, 15),  // yellow
    tsl::Color(15, 15, 15, 15), // white
    tsl::Color(15, 8, 0, 15),   // orange
};
static constexpr tsl::Color STATIC_GREEN     = tsl::Color(0, 15, 0, 15);
const std::string name = "Ryazha-clk";

static s32 drawDynamicUltraText(
    tsl::gfx::Renderer* renderer,
    s32 startX,
    s32 y,
    u32 fontSize,
    const tsl::Color& staticColor,
    bool useNotificationMethod = false)
{
    s32 currentX = startX;

    const u64 currentTime_ns = armTicksToNs(armGetSystemTick());
    const double timeNow = static_cast<double>(currentTime_ns) / 1e9;
    const double center = (static_cast<double>(name.size()) - 1.0) * 0.5;
    const double dropSpeed = 2.8;
    const double rippleFreq = 2.6;

    for (size_t i = 0; i < name.size(); i++)
    {
        char letter = name[i];
        if (letter == '\0') break;

        const double x = static_cast<double>(i) - center;
        const double dist = fabs(x);
        const double phase = dist * rippleFreq - timeNow * dropSpeed;
        const double ringA = sin(phase);
        const double ringB = 0.55 * sin((dist * 1.25) * rippleFreq - timeNow * (dropSpeed * 1.45));
        const double ripple = ringA + ringB;

        const double blend = std::clamp(0.5 + ripple * 0.25, 0.0, 1.0);
        const double brightness = std::clamp(0.75 + fabs(ringA) * 0.35, 0.55, 1.0);

        constexpr size_t kPaletteCount = sizeof(kLogoPalette) / sizeof(kLogoPalette[0]);
        const double pos = blend * (double)kPaletteCount;
        const size_t seg = ((size_t)pos) % kPaletteCount;
        const size_t nxt = (seg + 1) % kPaletteCount;
        const double t = pos - std::floor(pos);

        u8 r = static_cast<u8>((kLogoPalette[seg].r + (kLogoPalette[nxt].r - kLogoPalette[seg].r) * t) * brightness);
        u8 g = static_cast<u8>((kLogoPalette[seg].g + (kLogoPalette[nxt].g - kLogoPalette[seg].g) * t) * brightness);
        u8 b = static_cast<u8>((kLogoPalette[seg].b + (kLogoPalette[nxt].b - kLogoPalette[seg].b) * t) * brightness);

        r = std::clamp<u8>(r, 0, 15);
        g = std::clamp<u8>(g, 0, 15);
        b = std::clamp<u8>(b, 0, 15);

        // bool lightning = (fmod(timeNow, 5.0) < 0.15);
        // if (lightning) {
        //     r = std::min<u8>(r + 4, 15);
        //     g = std::min<u8>(g + 4, 15);
        //     b = std::min<u8>(b + 15, 15);
        // }

        tsl::Color color(r, g, b, 15);

        std::string ls(1, letter);

        if (useNotificationMethod)
            currentX += renderer->drawNotificationString(ls, false, currentX, y, fontSize, color).first;
        else
            currentX += renderer->drawString(ls, false, currentX, y, fontSize, color).first;
    }

    return currentX;
}

static void drawRcuModeBadge(tsl::gfx::Renderer* renderer, s32 titleRightX, s32 titleY) {
    static constexpr const char* kBadgeVersion = "3.0.0";
    const std::string versionLabel = kBadgeVersion;
    const s32 verW   = renderer->getTextDimensions(versionLabel.c_str(), false, BADGE_VER_FONT_SIZE).first;
    const s32 labelW = renderer->getTextDimensions(RYAZHA_BADGE_LABEL, false, BADGE_LABEL_FONT_SIZE).first;
    const s32 contentW = std::max(verW, labelW);

    const s32 badgeW = contentW + BADGE_PAD_X * 2;
    const s32 badgeH = BADGE_VER_FONT_SIZE + BADGE_LINE_GAP + BADGE_LABEL_FONT_SIZE + BADGE_PAD_Y * 2;
    const s32 badgeX = titleRightX + BADGE_LEFT_PAD;
    const s32 badgeY = titleY - badgeH + 4;

    const tsl::Color glowColor     = tsl::Color(15, 10, 1, 6);
    const tsl::Color borderColor   = tsl::Color(15, 13, 3, 15);
    const tsl::Color borderInner   = tsl::Color(15, 8, 1, 13);
    const tsl::Color innerColor    = tsl::Color(1, 1, 1, 9);
    const tsl::Color highlightFill = tsl::Color(15, 8, 1, 2);
    const tsl::Color verColor      = tsl::Color(15, 15, 10, 15);
    const tsl::Color labelColor    = tsl::Color(15, 4, 4, 15);

    renderer->drawRoundedRect(badgeX - 1, badgeY - 1, badgeW + 2, badgeH + 2, BADGE_RADIUS + 1, glowColor);
    renderer->drawRoundedRect(badgeX, badgeY, badgeW, badgeH, BADGE_RADIUS, borderColor);
    renderer->drawRoundedRect(
        badgeX + BADGE_BORDER,
        badgeY + BADGE_BORDER,
        badgeW - BADGE_BORDER * 2,
        badgeH - BADGE_BORDER * 2,
        BADGE_RADIUS - 1,
        borderInner
    );
    renderer->drawRoundedRect(
        badgeX + BADGE_BORDER + 1,
        badgeY + BADGE_BORDER + 1,
        badgeW - (BADGE_BORDER + 1) * 2,
        badgeH - (BADGE_BORDER + 1) * 2,
        BADGE_RADIUS - 2,
        innerColor
    );
    renderer->drawRoundedRect(badgeX + 2, badgeY + 2, badgeW - 4, 8, BADGE_RADIUS - 3, highlightFill);

    const s32 verBaselineY = badgeY + BADGE_PAD_Y + BADGE_VER_FONT_SIZE;
    const s32 labelBaselineY = verBaselineY + BADGE_LINE_GAP + BADGE_LABEL_FONT_SIZE;
    const s32 verX = badgeX + (badgeW - verW) / 2;
    const s32 labelX = badgeX + (badgeW - labelW) / 2;

    renderer->drawString(versionLabel.c_str(), false, verX + 1, verBaselineY + 1, BADGE_VER_FONT_SIZE, tsl::Color(0, 0, 0, 10));
    renderer->drawString(RYAZHA_BADGE_LABEL, false, labelX + 1, labelBaselineY + 1, BADGE_LABEL_FONT_SIZE, tsl::Color(0, 0, 0, 10));
    renderer->drawString(versionLabel.c_str(), false, verX, verBaselineY, BADGE_VER_FONT_SIZE, verColor);
    renderer->drawString(RYAZHA_BADGE_LABEL, false, labelX, labelBaselineY, BADGE_LABEL_FONT_SIZE, labelColor);
}

void BaseGui::preDraw(tsl::gfx::Renderer* renderer) {
    s32 titleRightX = drawDynamicUltraText(
        renderer,
        LOGO_X,
        LOGO_Y,
        LOGO_LABEL_FONT_SIZE,
        STATIC_GREEN,
        false
    );
    drawRcuModeBadge(renderer, titleRightX, LOGO_Y);
}

tsl::elm::Element* BaseGui::createUI()
{
    BaseFrame* rootFrame = new BaseFrame(this);
    rootFrame->setContent(this->baseUI());
    return rootFrame;
}

void BaseGui::update()
{
    this->refresh();
}