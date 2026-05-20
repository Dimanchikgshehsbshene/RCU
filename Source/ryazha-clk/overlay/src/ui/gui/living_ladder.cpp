/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * Тонкий IPC-прокси: вся логика теперь в sysmodule.
 *
 */

#include "living_ladder.h"
#include "../../ipc.h"

void LivingLadderProxy::ensureLoaded() const {
    if (loaded_) return;
    RClkLadderConfig tmp{};
    if (R_SUCCEEDED(rclkIpcGetLadderConfig(&tmp))) {
        cfg_ = tmp;
    }
    loaded_ = true;
}

LadderConfig& LivingLadderProxy::config() {
    ensureLoaded();
    return cfg_;
}

const LadderConfig& LivingLadderProxy::config() const {
    ensureLoaded();
    return cfg_;
}

void LivingLadderProxy::push() {
    ensureLoaded();
    (void)rclkIpcSetLadderConfig(&cfg_);
}

void LivingLadderProxy::pull() {
    RClkLadderConfig tmp{};
    if (R_SUCCEEDED(rclkIpcGetLadderConfig(&tmp))) {
        cfg_ = tmp;
    }
    loaded_ = true;
}

LivingLadderProxy& livingLadder() {
    static LivingLadderProxy g;
    return g;
}
