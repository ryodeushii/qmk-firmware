/* Copyright 2026 NuPhy contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUPHY_SLEEP_POLICY_USB_SUSPEND_DEBOUNCE_LIMIT 20u
#define NUPHY_SLEEP_POLICY_RF_LINKING_TIMEOUT         500u
#define NUPHY_SLEEP_POLICY_RF_DISCONNECT_TIMEOUT      100u

typedef enum {
    NUPHY_SLEEP_TRANSPORT_USB = 0,
    NUPHY_SLEEP_TRANSPORT_WIRELESS,
} nuphy_sleep_policy_transport_t;

typedef enum {
    NUPHY_SLEEP_ACTION_NONE = 0,
    NUPHY_SLEEP_ACTION_SLEEP_REQUEST,
    NUPHY_SLEEP_ACTION_LIGHT_SLEEP,
    NUPHY_SLEEP_ACTION_DEEP_SLEEP,
    NUPHY_SLEEP_ACTION_WAKE_PREPARATION,
} nuphy_sleep_policy_action_t;

typedef struct {
    bool                               sleep_toggle;
    bool                               usb_sleep_toggle;
    bool                               deep_sleep_toggle;
    nuphy_sleep_policy_transport_t    transport;
    bool                               charging;
    bool                               usb_suspended;
    bool                               rf_connected;
    bool                               rf_disconnected;
    uint32_t                           inactivity_time;
    uint32_t                           sleep_timeout;
    uint32_t                           rf_linking_time;
    uint32_t                           rf_disconnect_time;
    uint8_t                            usb_suspend_debounce;
    bool                               goto_sleep;
    bool                               wakeup_prepare;
} nuphy_sleep_policy_state_t;

typedef struct {
    nuphy_sleep_policy_action_t action;
    uint32_t                    inactivity_time;
    uint32_t                    rf_linking_time;
    uint32_t                    rf_disconnect_time;
    uint8_t                     usb_suspend_debounce;
    bool                        goto_sleep;
    bool                        wakeup_prepare;
} nuphy_sleep_policy_result_t;

void nuphy_sleep_policy_step(const nuphy_sleep_policy_state_t *state, nuphy_sleep_policy_result_t *result);
