/* Copyright 2026 NuPhy contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sleep_policy.h"

static void copy_state_to_result(const nuphy_sleep_policy_state_t *state, nuphy_sleep_policy_result_t *result) {
    result->action                 = NUPHY_SLEEP_ACTION_NONE;
    result->inactivity_time        = state->inactivity_time;
    result->rf_linking_time        = state->rf_linking_time;
    result->rf_disconnect_time     = state->rf_disconnect_time;
    result->usb_suspend_debounce   = state->usb_suspend_debounce;
    result->goto_sleep             = state->goto_sleep;
    result->wakeup_prepare         = state->wakeup_prepare;
}

static void reset_sleep_counters(nuphy_sleep_policy_result_t *result) {
    result->goto_sleep           = false;
    result->rf_linking_time      = 0;
    result->rf_disconnect_time   = 0;
    result->usb_suspend_debounce = 0;
}

void nuphy_sleep_policy_step(const nuphy_sleep_policy_state_t *state, nuphy_sleep_policy_result_t *result) {
    copy_state_to_result(state, result);

    if (!state->sleep_toggle) {
        return;
    }

    if (state->wakeup_prepare) {
        result->action          = NUPHY_SLEEP_ACTION_WAKE_PREPARATION;
        result->inactivity_time = 0;
        result->goto_sleep      = false;
        return;
    }

    if (state->goto_sleep) {
        reset_sleep_counters(result);

        if (state->transport == NUPHY_SLEEP_TRANSPORT_USB) {
            if (state->usb_sleep_toggle || state->usb_suspended) {
                result->action         = NUPHY_SLEEP_ACTION_LIGHT_SLEEP;
                result->wakeup_prepare = true;
            }
        } else if (state->charging || !state->deep_sleep_toggle) {
            result->action         = NUPHY_SLEEP_ACTION_LIGHT_SLEEP;
            result->wakeup_prepare = true;
        } else {
            result->action = NUPHY_SLEEP_ACTION_DEEP_SLEEP;
        }
        return;
    }

    if (state->transport == NUPHY_SLEEP_TRANSPORT_USB) {
        if (state->usb_suspended) {
            if (result->usb_suspend_debounce < NUPHY_SLEEP_POLICY_USB_SUSPEND_DEBOUNCE_LIMIT) {
                result->usb_suspend_debounce++;
            }
            if (result->usb_suspend_debounce >= NUPHY_SLEEP_POLICY_USB_SUSPEND_DEBOUNCE_LIMIT) {
                result->goto_sleep = true;
                result->action     = NUPHY_SLEEP_ACTION_SLEEP_REQUEST;
            }
        } else {
            result->usb_suspend_debounce = 0;
            if (state->usb_sleep_toggle && state->inactivity_time >= state->sleep_timeout) {
                result->goto_sleep = true;
                result->action     = NUPHY_SLEEP_ACTION_SLEEP_REQUEST;
            }
        }
        return;
    }

    if (state->inactivity_time >= state->sleep_timeout) {
        result->goto_sleep = true;
        result->action     = NUPHY_SLEEP_ACTION_SLEEP_REQUEST;
    } else if (state->rf_linking_time >= NUPHY_SLEEP_POLICY_RF_LINKING_TIMEOUT) {
        result->rf_linking_time = 0;
        result->goto_sleep      = true;
        result->action          = NUPHY_SLEEP_ACTION_SLEEP_REQUEST;
    } else if (state->rf_disconnected) {
        result->rf_disconnect_time++;
        if (result->rf_disconnect_time > NUPHY_SLEEP_POLICY_RF_DISCONNECT_TIMEOUT) {
            result->goto_sleep = true;
            result->action     = NUPHY_SLEEP_ACTION_SLEEP_REQUEST;
        }
    } else if (state->rf_connected) {
        result->rf_disconnect_time = 0;
    }
}
