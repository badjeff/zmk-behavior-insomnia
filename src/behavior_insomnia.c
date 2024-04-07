/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_insomnia

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/poweroff.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>

#include <zmk/pm.h>

#include <zmk/activity.h>
int set_state(enum zmk_activity_state state);

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// NOTE: checked in Kconfig & CMakeLists.txt
// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <zmk/insomnia/behavior_insomnia_timer.h>
#include <dt-bindings/zmk/insomnia.h>

struct behavior_insomnia_config {
    uint8_t index;
    uint32_t sleep_delay_ms;
};

struct behavior_insomnia_data {
    struct k_work_delayable insomnia_idle_work;
    struct k_work_delayable insomnia_sleep_work;
};

static void insomnia_sleep_work_handler(struct k_work *work) {
    LOG_DBG("set state to ZMK_ACTIVITY_SLEEP");
    set_state(ZMK_ACTIVITY_SLEEP);
    if (zmk_pm_suspend_devices() < 0) {
        LOG_ERR("Failed to suspend all the devices");
        zmk_pm_resume_devices();
        return;
    }
    sys_poweroff();
}

static void insomnia_idle_work_handler(struct k_work *work) {
    LOG_DBG("set state to ZMK_ACTIVITY_IDLE");
    set_state(ZMK_ACTIVITY_IDLE);
}

static int on_insomnia_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_insomnia_data *data = dev->data;
    const struct behavior_insomnia_config *config = dev->config;

    bool is_active = insomnia_timer_get_active();
    switch (binding->param1) {
        case INSOMNIA_OFF_CMD:
        LOG_DBG("INSOMNIA_OFF_CMD");
        if (is_active) {
            insomnia_timer_set_active(false);
            LOG_DBG("insonmia timer is deactivated");
        }
        break;
        case INSOMNIA_ON_CMD:
        LOG_DBG("INSOMNIA_ON_CMD");
        if (!is_active) {
            insomnia_timer_set_active(true);
        }
        break;
        case INSOMNIA_SLEEP_CMD:
        LOG_DBG("INSOMNIA_SLEEP_CMD");
        if (is_active) {
            insomnia_timer_set_active(false);
        }
        k_work_schedule(&data->insomnia_idle_work, K_MSEC(1));
        k_work_schedule(&data->insomnia_sleep_work, K_MSEC(1 + config->sleep_delay_ms));
        break;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_insomnia_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_insomnia_driver_api = {
    .binding_pressed = on_insomnia_binding_pressed,
    .binding_released = on_insomnia_binding_released,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
};

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static int behavior_insomnia_init(const struct device *dev) {
    const struct behavior_insomnia_config *config = dev->config;
    devs[config->index] = dev;

    struct behavior_insomnia_data *data = dev->data;
    k_work_init_delayable(&data->insomnia_idle_work, insomnia_idle_work_handler);
    k_work_init_delayable(&data->insomnia_sleep_work, insomnia_sleep_work_handler);

    return 0;
}

#define KINS_INST(n)                                                                        \
    static struct behavior_insomnia_data behavior_insomnia_data_##n = {};                   \
    static struct behavior_insomnia_config behavior_insomnia_config_##n = {                 \
        .index = n,                                                                         \
        .sleep_delay_ms = DT_INST_PROP(n, sleep_delay_ms),                                  \
    };                                                                                      \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_insomnia_init, NULL, &behavior_insomnia_data_##n,   \
                            &behavior_insomnia_config_##n, POST_KERNEL,                     \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                            \
                            &behavior_insomnia_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KINS_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
