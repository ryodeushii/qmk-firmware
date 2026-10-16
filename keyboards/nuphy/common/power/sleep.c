/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quantum.h"
#include "hal_usb.h"
#include "usb_main.h"
#include "mcu_pwr.h"
#include "../debounce.h"
#include "../config/config.h"
#include "../wireless.h"
#include "../wireless/internal.h"
#include "sleep_policy.h"
extern uint16_t        rf_linking_time;
extern uint16_t        rf_link_timeout;
extern uint32_t        no_act_time;
extern bool            f_goto_sleep;
extern bool            f_wakeup_prepare;

void deep_sleep_handle(void) {
    // Sync again before sleeping. Without this, the wake keystroke is more likely to be lost.
    nuphy_rf_sync_status();

    enter_deep_sleep(); // puts the board in WFI mode and pauses the MCU
    exit_deep_sleep();  // This gets called when there is an interrupt (wake) event.

    no_act_time = 0; // required to not cause an immediate sleep on first wake
}

/**
 * @brief  Sleep Handle.
 */
void sleep_handle(void) {
    static uint32_t delay_step_timer     = 0;
    static uint8_t  usb_suspend_debounce = 0;
#if (WORK_MODE == THREE_MODE)
    static uint32_t rf_disconnect_time = 0;
#endif

    /* 50ms interval */
    if (timer_elapsed32(delay_step_timer) < 50) return;
    delay_step_timer = timer_read32();

    nuphy_wireless_status_t wireless_status;
    nuphy_wireless_get_status(&wireless_status);

    nuphy_sleep_policy_state_t state = {
        .sleep_toggle     = keyboard_config.common.sleep_toggle,
        .usb_sleep_toggle = keyboard_config.common.usb_sleep_toggle,
        .deep_sleep_toggle = keyboard_config.common.deep_sleep_toggle,
        .transport        = wireless_status.link_mode == LINK_USB ? NUPHY_SLEEP_TRANSPORT_USB : NUPHY_SLEEP_TRANSPORT_WIRELESS,
        .charging         = (wireless_status.rf_charge & 0x01) != 0 || wireless_status.rf_charge == 0x03,
        .usb_suspended    = USB_DRIVER.state == USB_SUSPENDED,
        .inactivity_time  = no_act_time,
        .sleep_timeout    = get_sleep_timeout(),
        .usb_suspend_debounce = usb_suspend_debounce,
        .goto_sleep       = f_goto_sleep,
        .wakeup_prepare   = f_wakeup_prepare,
    };
#if (WORK_MODE == THREE_MODE)
    state.rf_connected       = wireless_status.rf_state == NUPHY_WIRELESS_STATE_CONNECTED;
    state.rf_disconnected    = wireless_status.rf_state == NUPHY_WIRELESS_STATE_DISCONNECTED;
    state.rf_linking_time    = rf_linking_time;
    state.rf_disconnect_time = rf_disconnect_time;
#endif

    nuphy_sleep_policy_result_t result;
    nuphy_sleep_policy_step(&state, &result);

    no_act_time            = result.inactivity_time;
    usb_suspend_debounce   = result.usb_suspend_debounce;
    f_goto_sleep           = result.goto_sleep;
    f_wakeup_prepare       = result.wakeup_prepare;
#if (WORK_MODE == THREE_MODE)
    rf_linking_time    = (uint16_t)result.rf_linking_time;
    rf_disconnect_time = result.rf_disconnect_time;
#endif

    switch (result.action) {
        case NUPHY_SLEEP_ACTION_LIGHT_SLEEP:
            break_all_key();
            enter_light_sleep();
            break;
        case NUPHY_SLEEP_ACTION_DEEP_SLEEP:
            break_all_key();
            deep_sleep_handle();
            break;
        case NUPHY_SLEEP_ACTION_WAKE_PREPARATION:
        case NUPHY_SLEEP_ACTION_NONE:
        case NUPHY_SLEEP_ACTION_SLEEP_REQUEST:
            break;
    }
}
