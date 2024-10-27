/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/insomnia/behavior_insomnia_timer.h>

#define INSOMNIA_TIMER_K_MSEC K_MSEC(CONFIG_ZMK_INSOMNIA_PING_INTERVAL)

// #include <zmk/event_manager.h>
#include <zmk/activity.h>
int set_state(enum zmk_activity_state state);

#if (IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

// split peripheral
bool zmk_split_bt_peripheral_is_connected(void);
#define INSOMNIA_KEEP_AWAKE_FN zmk_split_bt_peripheral_is_connected

#endif /* (IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)) */

#ifndef INSOMNIA_KEEP_AWAKE_FN

// split central or uni-body sheild
bool zmk_ble_active_profile_is_connected(void);
#define INSOMNIA_KEEP_AWAKE_FN zmk_ble_active_profile_is_connected

#endif /* not defined INSOMNIA_KEEP_AWAKE_FN */

void insomnia_timer_work_handler(struct k_work *work) {
    if (INSOMNIA_KEEP_AWAKE_FN()) {
        LOG_DBG("emit activity event");
        set_state(ZMK_ACTIVITY_ACTIVE);
    }
}

K_WORK_DEFINE(insomnia_timer_work, insomnia_timer_work_handler);

void insomnia_timer_expiry_function(struct k_timer *_timer) {
    k_work_submit(&insomnia_timer_work);
}

K_TIMER_DEFINE(insomnia_timer, insomnia_timer_expiry_function, NULL);

static bool insomnia_is_active = false;

bool insomnia_timer_get_active() { return insomnia_is_active; }

void insomnia_timer_set_active(bool to_active) {
    if (insomnia_is_active == to_active) {
        return;
    }
    if (!insomnia_is_active) {
        k_timer_start(&insomnia_timer, INSOMNIA_TIMER_K_MSEC, INSOMNIA_TIMER_K_MSEC);
        LOG_DBG("true");
    } else {
        k_timer_stop(&insomnia_timer);
        LOG_DBG("false");
    }
    insomnia_is_active = to_active;
}

static int insomnia_init(void) {
#if IS_ENABLED(CONFIG_ZMK_INSOMNIA_PING_ON_START)
    insomnia_timer_set_active(true);
#endif
    return 0;
}

SYS_INIT(insomnia_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
