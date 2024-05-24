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
#include "usb_main.h"
#include "mcu_pwr.h"
#include "color.h"

user_config_t   user_config;
DEV_INFO_STRUCT dev_info = {
    .rf_battery = 100,
    .link_mode  = LINK_USB,
    .rf_state   = RF_IDLE,
};
bool f_bat_hold         = 0;
bool game_mode_enable   = 0;
bool rgb_required       = 0;
bool f_send_channel     = 0;
bool f_dial_sw_init_ok  = 0;
bool f_rf_sw_press      = 0;
bool f_dev_reset_press  = 0;
bool f_rgb_test_press   = 0;
bool f_bat_num_show     = 0;

uint8_t        rf_blink_cnt          = 0;
uint8_t        rf_sw_temp            = 0;
uint8_t        host_mode             = 0;
uint16_t       rf_linking_time       = 0;
uint16_t       rf_link_show_time     = 0;
uint32_t       no_act_time           = 0;
uint16_t       dev_reset_press_delay = 0;
uint16_t       rf_sw_press_delay     = 0;
uint16_t       rgb_test_press_delay  = 0;
uint32_t       sys_show_timer        = 0;
uint32_t       sleep_show_timer      = 0;

host_driver_t *m_host_driver         = 0;

uint16_t       link_timeout          = (100 * 60 * 1);
uint16_t       sleep_time_delay      = (100 * 60 * 2);
uint32_t       deep_sleep_delay      = (100 * 60 * 6);
uint32_t       eeprom_update_timer   = 0;
bool           user_update           = 0;
bool           rgb_update            = 0;

extern host_driver_t      rf_host_driver;

/**
 * @brief  gpio initial.
 */
void gpio_init(void) {
    /* power on all LEDs */
    pwr_led_on();
    /* config RF module pin */
    gpio_set_pin_output(NRF_WAKEUP_PIN);
    gpio_write_pin_high(NRF_WAKEUP_PIN);
    /* set RF module boot pin high */
    gpio_set_pin_input_high(NRF_BOOT_PIN);
    /* reset RF module */
    gpio_set_pin_output(NRF_RESET_PIN);
    gpio_write_pin_low(NRF_RESET_PIN);
    wait_ms(50);
    gpio_write_pin_high(NRF_RESET_PIN);
    /* config dial switch pin */
    gpio_set_pin_input_high(DEV_MODE_PIN);
    gpio_set_pin_input_high(SYS_MODE_PIN);
}

void set_link_mode(void) {
    f_rf_sw_press = 0;

    dev_info.link_mode   = rf_sw_temp;
    dev_info.rf_channel  = rf_sw_temp;
    dev_info.ble_channel = rf_sw_temp;
}

/**
 * @brief  long press key process.
 */
void custom_key_press(void) {
    static uint32_t long_press_timer = 0;

    if (timer_elapsed32(long_press_timer) < 100) return;
    long_press_timer = timer_read32();

    dial_sw_scan();

    // Open a new RF device
    if (f_rf_sw_press) {
        rf_sw_press_delay++;
        if (rf_sw_press_delay >= RF_LONG_PRESS_DELAY) {
            set_link_mode();
            uint8_t timeout = 5;
            while (timeout--) {
                uart_send_cmd(CMD_NEW_ADV, 0, 1);
                wait_ms(20);
                uart_receive_pro();
                if (f_rf_new_adv_ok) break;
            }
        }
    } else {
        rf_sw_press_delay = 0;
    }

    // The device is restored to factory Settings
    if (f_dev_reset_press) {
        dev_reset_press_delay++;
        if (dev_reset_press_delay >= DEV_RESET_PRESS_DELAY) {
            f_dev_reset_press = 0;

            if (dev_info.link_mode != LINK_RF_24) {
                dev_info.ble_channel = LINK_BT_1;
                dev_info.rf_channel  = LINK_BT_1;
            }

            if (dev_info.link_mode != LINK_USB) {
                if (dev_info.link_mode != LINK_RF_24) {
                    dev_info.link_mode   = LINK_BT_1;
                }
            } 

            uart_send_cmd(CMD_SET_LINK, 10, 10);
            wait_ms(500);
            uart_send_cmd(CMD_CLR_DEVICE, 10, 10);

            void device_reset_show(void);
            void device_reset_init(void);

            eeconfig_init();
            device_reset_show();
            device_reset_init();
            eeconfig_update_rgb_matrix_default();

            if (dev_info.sys_sw_state == SYS_SW_MAC) {
                default_layer_set(1 << 0);
                keymap_config.nkro = 0;
            } else {
                default_layer_set(1 << 2);
                keymap_config.nkro = 1;
            }
        }
    } else {
        dev_reset_press_delay = 0;
    }

    // Enter the RGB test mode
    if (f_rgb_test_press) {
        rgb_test_press_delay++;
        if (rgb_test_press_delay >= RGB_TEST_PRESS_DELAY) {
            f_rgb_test_press = 0;
            rgb_test_show();
        }
    } else {
        rgb_test_press_delay = 0;
    }

    // Trigger NumLock
    if (numlock_timer != 0 && timer_elapsed32(numlock_timer) > TAPPING_TERM) {
        tap_code(KC_NUM);
        numlock_timer = 0;
    }

    if (caps_word_timer != 0 && timer_elapsed32(caps_word_timer) > TAPPING_TERM * 4) {
        user_config.caps_word_enable = !user_config.caps_word_enable;
        caps_word_timer = 0;
#ifndef NO_DEBUG
        dprintf("caps_word_state: %s\n", user_config.caps_word_enable ? "ON" : "OFF");
#endif
        signal_rgb_led(user_config.caps_word_enable, CAPS_LED, CAPS_LED, CAPS_WORD_IDLE_TIMEOUT);
    }
}

/**
 * @brief  Release all keys, clear keyboard report.
 */
void break_all_key(void) {
    clear_keyboard();
    void clear_report_buffer_and_queue(void);
    clear_report_buffer_and_queue();
}

/**
 * @brief  switch device link mode.
 * @param mode : link mode
 */
void switch_dev_link(uint8_t mode) {
    if (mode > LINK_USB) return;
    no_act_time = 0;
    break_all_key();

    dev_info.link_mode = mode;

    dev_info.rf_state = RF_IDLE;
    f_send_channel    = 1;

    if (mode == LINK_USB) {
        host_mode = HOST_USB_TYPE;
        host_set_driver(m_host_driver);
        rf_link_show_time = 0;
    } else {
        host_mode = HOST_RF_TYPE;
        host_set_driver(&rf_host_driver);
    }
}

/**
 * @brief  read dial values
 */
uint8_t dial_read(void) {
    uint8_t dial_scan = 0;
    gpio_set_pin_input_high(DEV_MODE_PIN);
    gpio_set_pin_input_high(SYS_MODE_PIN);

    if (gpio_read_pin(DEV_MODE_PIN)) dial_scan |= 0X01;
    if (gpio_read_pin(SYS_MODE_PIN)) dial_scan |= 0X02;

    return dial_scan;
}

/**
 * @brief  set dial values based on input
 * @param dial_scan    : current dial input value
 * @param led_sys_show : show system led
 */
void dial_set(uint8_t dial_scan, bool led_sys_show) {

    if (dial_scan & 0x01) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

    if (dial_scan & 0x02) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            if (led_sys_show) sys_show_timer = timer_read32();
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            if (led_sys_show) sys_show_timer = timer_read32();
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
        }
    }
}

/**
 * @brief  scan dial switch.
 */
void dial_sw_scan(void) {
    uint8_t         dial_scan       = 0;
    static uint8_t  dial_save       = 0xf0;
    static uint8_t  debounce        = 0;

    dial_scan       = dial_read();

    if (dial_save != dial_scan) {
        break_all_key();

        no_act_time      = 0;
        rf_linking_time  = 0;
        f_wakeup_prepare = 0;

        dial_save         = dial_scan;
        debounce          = 5;
        f_dial_sw_init_ok = 0;
        return;
    } else if (debounce) {
        debounce--;
        return;
    }

    dial_set(dial_scan, true);

    if (f_dial_sw_init_ok == 0) {
        f_dial_sw_init_ok = 1;

        if (dev_info.link_mode != LINK_USB) {
            host_set_driver(&rf_host_driver);
        }
    }
}

/**
 * @brief  power on scan dial switch.
 */
void dial_sw_fast_scan(void) {
    uint8_t         dial_scan        = 0;
    uint8_t         dial_check       = 0xf0;
    uint8_t         debounce         = 0;

    // Debounce to get a stable state
    for (debounce = 0; debounce < 10; debounce++) {
        dial_scan       = dial_read();
        if (dial_check != dial_scan) {
            dial_check = dial_scan;
            debounce       = 0;
        }
        wait_ms(1);
    }

    dial_set(dial_scan, false);
}

/**
 * @brief  timer process.
 */
void timer_pro(void) {
    static uint32_t interval_timer = 0;
    static bool     f_first        = true;

    if (f_first) {
        f_first        = false;
        interval_timer = timer_read32();
        m_host_driver  = host_get_driver();
    }

    // step 10ms
    if (timer_elapsed32(interval_timer) < 10) return;
    interval_timer = timer_read32();

    if (rf_link_show_time < RF_LINK_SHOW_TIME) rf_link_show_time++;

    if (no_act_time < 0xffffffff) no_act_time++;

    if (rf_linking_time < 0xffff) rf_linking_time++;

}

/**
 * @brief  load eeprom data.
 */
void load_eeprom_data(void) {
    eeconfig_read_kb_datablock(&user_config);
    if (user_config.init_layer < 100) user_config_reset();
}

void call_update_eeprom_data(bool* eeprom_update_init) {
    *eeprom_update_init = 1;
    eeprom_update_timer = 0;
}

/**
 * @brief User config update to eeprom with delay
 */
void delay_update_eeprom_data(void) {
    if (eeprom_update_timer == 0) {
        if (user_update || rgb_update) eeprom_update_timer = timer_read32();
        return;
    }
    if (timer_elapsed32(eeprom_update_timer) < (1000 * 30)) return;
    if (user_update) {
#ifndef NO_DEBUG
        dprint("Updating EEPROM: user_config\n");
#endif
        eeconfig_update_kb_datablock(&user_config);
        user_update         = 0;
    }
    if (rgb_update) {
#ifndef NO_DEBUG
        dprint("Updating EEPROM:  rgb_config\n");
#endif
        eeconfig_update_rgb_matrix();
        rgb_update          = 0;
    }
    eeprom_update_timer = 0;
}

void game_mode_tweak(void)
{
    if (game_mode_enable) {
        pwr_led_on();
        rgb_matrix_mode_noeeprom(RGB_MATRIX_GAME_MODE);
        rgb_matrix_config.hsv.v = RGB_MATRIX_GAME_MODE_VAL;
        user_config.ee_side_mode   = 2;
        user_config.ee_side_rgb    = 0;
        user_config.ee_side_light  = 2;
        user_config.ee_side_colour = SIDE_MATRIX_GAME_MODE;
    } else {
        rgb_matrix_reload_from_eeprom();
        eeconfig_read_kb_datablock(&user_config);
    }
}

#ifndef NO_DEBUG
void user_debug(void) {
    static uint32_t last_print = 0;
    if (no_act_time == 0 || no_act_time == last_print) return;
    if (no_act_time % 3000 == 0) {
        if (!USB_ACTIVE && debug_enable) { debug_enable = false; }
        last_print = no_act_time;
        dprintf("no_act_time: %lds\n", no_act_time / 100);
    }
}
#endif

/**
 * @brief User config to default setting.
 */
void user_config_reset(void) {
    /* first power on, set rgb matrix brightness at middle level*/
    // rgb_matrix_sethsv(RGB_HUE_INIT, 255, RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);

    user_config.init_layer              = 100;
    user_config.ee_side_mode            = 0;
    user_config.ee_side_light           = 1;
    user_config.ee_side_speed           = 2;
    user_config.ee_side_rgb             = 1;
    user_config.ee_side_colour          = 0;
    user_config.ee_side_one             = 0;
    user_config.sleep_mode              = 1;
    user_config.caps_word_enable        = 1;
    user_config.sys_ind                 = 2;
    eeconfig_update_kb_datablock(&user_config);
}

/**
 * @brief Handle LED power
 * @note Turn off LEDs if not used to save some power. This is ported
 *       from older Nuphy leaks.
 */

void matrix_io_delay(void) {
    if (MATRIX_IO_DELAY == 0 || game_mode_enable == 1) {
        __asm__ __volatile__("nop;nop;nop;\n\t" ::: "memory");
        return;
    }

    if (no_act_time > 3000) wait_us(1200);
    else if (no_act_time > 1000) wait_us(200);
    wait_us(MATRIX_IO_DELAY);
}

void led_power_handle(void) {
    static uint32_t interval = 0;
    static uint8_t led_debounce = 4;

    if (timer_elapsed32(interval) < 500 || f_wakeup_prepare || game_mode_enable) // only check once in a while, less flickering for unhandled cases
        return;

    interval = timer_read32();

    if ((rgb_matrix_is_enabled() && rgb_matrix_get_val() != 0) || !is_side_rgb_off() || rgb_required) { 
        rgb_required = 0;
        led_debounce = 4;
        pwr_led_on();
    } else if (led_debounce--) {
        return;
    } else { // brightness is 0 or RGB off.
        pwr_led_off();
    }
}
