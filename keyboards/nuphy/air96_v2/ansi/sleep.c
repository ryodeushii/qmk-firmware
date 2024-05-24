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

#include "user_kb.h"
#include "ansi.h"
#include "hal_usb.h"
#include "usb_main.h"
#include "mcu_pwr.h"
#include "rf_queue.h"

void deep_sleep_handle(void) {
    signal_sleep(0x00, dev_info.link_mode == LINK_RF_24 ? 0x80 : 0x00, dev_info.link_mode == LINK_RF_24 ? 0x00 : 0x80);
    // Sync again before sleeping
    dev_sts_sync();
    enter_deep_sleep(); // puts the board in WFI mode and pauses the MCU
#if (MCU_SLEEP_ENABLE)
    exit_deep_sleep();  // This gets called when there is an interrupt (wake) event.
    no_act_time = 0;    // required to not cause an immediate sleep on first wake
#endif
}

/**
 * @brief  Sleep Handle.
 */
void sleep_handle(void) {
    static uint32_t delay_step_timer = 0;
    static uint8_t  usb_suspend_debounce = 0;
    static uint32_t rf_disconnect_time = 0;

    if (user_config.sleep_mode == 0 || USB_ACTIVE) return;
    if (timer_elapsed32(delay_step_timer) > (60 * 1000)) no_act_time = 0;

    /* 500ms interval */
    if (timer_elapsed32(delay_step_timer) < 500) return;
        delay_step_timer = timer_read32();

    if (user_config.sleep_mode != 1 || f_rf_sleep)
        f_goto_deepsleep = 0;
    else if (no_act_time >= deep_sleep_delay)
        f_goto_deepsleep = 1;

    if (f_goto_deepsleep != 0) {
        f_goto_deepsleep   = 0;
        f_goto_sleep       = 0;
        f_wakeup_prepare   = 1;
        f_rf_sleep         = 1;
        deep_sleep_handle();
        return;
    }

    // sleep process
    if (f_goto_sleep) {
        f_goto_sleep       = 0;
        enter_light_sleep();
        f_wakeup_prepare = 1;
    }

    // sleep check
    if (f_goto_sleep || f_wakeup_prepare)
        return;
    if (dev_info.link_mode == LINK_USB) {
        if (USB_DRIVER.state == USB_SUSPENDED) {
            usb_suspend_debounce++;
            if (usb_suspend_debounce >= 2) {
                f_goto_sleep = 1;
            }
        } else {
            usb_suspend_debounce = 0;
        }
    } else if (no_act_time >= sleep_time_delay) {
        f_goto_sleep     = 1;
    } else if (rf_linking_time >= (dev_info.link_mode == LINK_RF_24 ? (link_timeout / 4) : link_timeout)) {
        f_goto_deepsleep = 1;
        f_goto_sleep     = 1;
        rf_linking_time  = 0;
    } else if (dev_info.rf_state == RF_DISCONNECT) {
            rf_disconnect_time++;
        if (rf_disconnect_time > 5 * 2) {
            f_goto_deepsleep   = 1;
            f_goto_sleep       = 1;
            rf_disconnect_time = 0;
        }
    } else if (dev_info.rf_state == RF_CONNECT) {
            rf_disconnect_time = 0;
    }
}
