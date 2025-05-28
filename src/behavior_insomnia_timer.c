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

#include <zmk/events/sensor_event.h>

// for ble split central, or ble uni-body
#if (IS_ENABLED(CONFIG_ZMK_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
bool zmk_ble_active_profile_is_connected(void);
#define INSOMNIA_KEEP_AWAKE_FN zmk_ble_active_profile_is_connected
#endif

// for ble split peripheral
#if (IS_ENABLED(CONFIG_ZMK_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_BLE) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
bool zmk_split_bt_peripheral_is_connected(void);
#define INSOMNIA_KEEP_AWAKE_FN zmk_split_bt_peripheral_is_connected
#endif /* (IS_ENABLED(CONFIG_ZMK_SPLIT_BLE) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)) */

void insomnia_timer_work_handler(struct k_work *work) {
#ifdef INSOMNIA_KEEP_AWAKE_FN
    if (INSOMNIA_KEEP_AWAKE_FN()) {
        LOG_DBG("emit activity event");
        raise_zmk_sensor_event(
            (struct zmk_sensor_event){.sensor_index = UINT8_MAX,
                                    .channel_data_size = 1,
                                    .channel_data = {(struct zmk_sensor_channel_data){
                                        .channel = SENSOR_CHAN_GAUGE_STATE_OF_CHARGE}},
                                    .timestamp = k_uptime_get()});
    }
#endif
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
