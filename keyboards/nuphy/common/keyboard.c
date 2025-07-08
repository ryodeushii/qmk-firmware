#include "quantum.h"
#include "rf_driver.h"
#include "config.h"
#include "keys.h"

__attribute((weak)) extern void side_led_show(void) {}
__attribute((weak)) extern void rgb_test_show(void) {}

void switch_dev_link(uint8_t mode);
void sleep_handle(void);

extern DEV_INFO_STRUCT dev_info;

bool f_bat_hold              = 0;
bool f_sys_show              = 0;
bool f_sleep_show            = 0;
bool f_dial_sw_init_ok       = 0;
bool f_rf_sw_press           = 0;
bool f_dev_reset_press       = 0;
bool f_rgb_test_press        = 0;
bool f_debounce_press_show   = 0;
bool f_debounce_release_show = 0;
bool f_sleep_timeout_show    = 0;

uint8_t  rf_blink_cnt          = 0;
uint8_t  rf_sw_temp            = 0;
uint16_t dev_reset_press_delay = 0;
uint16_t rf_sw_press_delay     = 0;
uint16_t rgb_test_press_delay  = 0;

extern uint16_t           rf_linking_time;
extern uint32_t           no_act_time;
extern bool               f_rf_new_adv_ok;
extern report_keyboard_t *keyboard_report;
extern report_nkro_t     *nkro_report;
extern host_driver_t      rf_host_driver;

/**
 * @brief  long press key process.
 */
void long_press_key(void) {
    static uint32_t long_press_timer = 0;

    if (timer_elapsed32(long_press_timer) < 100) return;
    long_press_timer = timer_read32();

    // Open a new RF device
    if (f_rf_sw_press) {
#if (WORK_MODE == THREE_MODE)
        rf_sw_press_delay++;
        if (rf_sw_press_delay >= RF_LONG_PRESS_DELAY) {
            f_rf_sw_press = 0;

            dev_info.link_mode   = rf_sw_temp;
            dev_info.rf_channel  = rf_sw_temp;
            dev_info.ble_channel = rf_sw_temp;

            uint8_t timeout = 5;
            while (timeout--) {
                uart_send_cmd(CMD_NEW_ADV, 0, 1);
                wait_ms(20);
                uart_receive_pro();
                if (f_rf_new_adv_ok) break;
            }
        }
#endif
    } else {
        rf_sw_press_delay = 0;
    }

    // The device is restored to factory Settings
    if (f_dev_reset_press) {
        dev_reset_press_delay++;
        if (dev_reset_press_delay >= DEV_RESET_PRESS_DELAY) {
            f_dev_reset_press = 0;

            if (dev_info.link_mode != LINK_USB) {
                if (dev_info.link_mode != LINK_RF_24) {
                    dev_info.link_mode   = LINK_BT_1;
                    dev_info.ble_channel = LINK_BT_1;
                    dev_info.rf_channel  = LINK_BT_1;
                }
            } else {
                dev_info.ble_channel = LINK_BT_1;
                dev_info.rf_channel  = LINK_BT_1;
            }

            uart_send_cmd(CMD_SET_LINK, 10, 10);
            wait_ms(500);
            uart_send_cmd(CMD_CLR_DEVICE, 10, 10);

            void device_reset_show(void);
            void device_reset_init(void);

            eeconfig_init();
            device_reset_show();
            device_reset_init();

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
}

/**
 * @brief  scan dial switch.
 */
void dial_sw_scan(void) {
    uint8_t         dial_scan       = 0;
    static uint8_t  dial_save       = 0xf0;
    static uint8_t  debounce        = 0;
    static uint32_t dial_scan_timer = 0;
    static bool     f_first         = true;

    if (!f_first) {
        if (timer_elapsed32(dial_scan_timer) < 20) return;
    }
    dial_scan_timer = timer_read32();

#if (WORK_MODE == THREE_MODE)
    setPinInputHigh(DEVICE_MODE_PIN);
#endif
    setPinInputHigh(OS_MODE_PIN);
#if (WORK_MODE == THREE_MODE)
    if (readPin(DEVICE_MODE_PIN)) dial_scan |= 0X01;
#endif
    if (readPin(OS_MODE_PIN)) dial_scan |= 0X02;

    if (dial_save != dial_scan) {
        break_all_key();

        no_act_time       = 0;
        rf_linking_time   = 0;
        dial_save         = dial_scan;
        debounce          = 25;
        f_dial_sw_init_ok = 0;
        return;
    } else if (debounce) {
        debounce--;
        return;
    }

#if (WORK_MODE == THREE_MODE)
    if (dial_scan & 0x01) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }
#endif

    if (dial_scan & 0x02) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            f_sys_show = 1;
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            f_sys_show = 1;
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            break_all_key();
        }
    }

    if (f_dial_sw_init_ok == 0) {
        f_dial_sw_init_ok = 1;
        f_first           = false;

#if (WORK_MODE == THREE_MODE)
        if (dev_info.link_mode != LINK_USB) {
            host_set_driver(&rf_host_driver);
        }
#endif
    }
}

/**
 * @brief  power on scan dial switch.
 */
void dial_sw_fast_scan(void) {
    uint8_t dial_scan_dev  = 0;
    uint8_t dial_scan_sys  = 0;
    uint8_t dial_check_dev = 0;
    uint8_t dial_check_sys = 0;
    uint8_t debounce       = 0;
#if (WORK_MODE == THREE_MODE)
    setPinInputHigh(DEVICE_MODE_PIN);
#endif
    setPinInputHigh(OS_MODE_PIN);

    // Debounce to get a stable state
    for (debounce = 0; debounce < 10; debounce++) {
        dial_scan_dev = 0;
        dial_scan_sys = 0;
#if (WORK_MODE == THREE_MODE)
        if (readPin(DEVICE_MODE_PIN))
            dial_scan_dev = 0x01;
        else
            dial_scan_dev = 0;
#endif
        if (readPin(OS_MODE_PIN))
            dial_scan_sys = 0x01;
        else
            dial_scan_sys = 0;
        if ((dial_scan_dev != dial_check_dev) || (dial_scan_sys != dial_check_sys)) {
            dial_check_dev = dial_scan_dev;
            dial_check_sys = dial_scan_sys;
            debounce       = 0;
        }
        wait_ms(1);
    }

#if (WORK_MODE == THREE_MODE)
    // RF link mode
    if (dial_scan_dev) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }
#endif
    // Win or Mac
    if (dial_scan_sys) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            // f_sys_show = 1;
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            break_all_key();
        }
    }
}

/**
 * @brief User config to default setting.
 */
void kb_config_reset(void) {
    rgb_matrix_enable();
    rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
    rgb_matrix_set_speed(255 - RGB_MATRIX_SPD_STEP * 2);
    rgb_matrix_sethsv(RGB_DEFAULT_COLOR, 255, RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);

    init_keyboard_config();
    keyboard_config.been_initiated = 0x45;

    save_config_to_eeprom();
}

void load_eeprom_data(void) {
    load_config_from_eeprom();

    if (keyboard_config.been_initiated != 0x45) {
        kb_config_reset();
        return;
    }
}

/* qmk housekeeping task */
void housekeeping_task_nuphy(void) {
    timer_pro();

#if (WORK_MODE == THREE_MODE)
    uart_receive_pro();

    uart_send_report_repeat();

    dev_sts_sync();
#endif

    long_press_key();

    dial_sw_scan();
    // external from each keyboard
    side_led_show();

    sleep_handle();
}

extern void gpio_init(void);

void keyboard_post_init_nuphy(void) {
    gpio_init();

#if (WORK_MODE == THREE_MODE)
    rf_uart_init();
    wait_ms(500);
    rf_device_init();
#endif
    break_all_key();
    dial_sw_fast_scan();
    load_eeprom_data();
    keyboard_post_init_user();
}

__attribute((weak)) void set_indicator_on_side(uint8_t r, uint8_t g, uint8_t b) {}

bool rgb_matrix_indicators_nuphy(void) {
    uint8_t caps_key_led_idx = get_led_index(3, 0);
    bool    showCapsLock     = false;

    if (dev_info.link_mode == LINK_USB) {
        showCapsLock = host_keyboard_led_state().caps_lock;
    } else {
        showCapsLock = dev_info.rf_led & 0x02;
    }

    if (rf_blink_cnt) {
        uint8_t r = 0, g = 0x80, b = 0;

        uint8_t col = 4;
        if (dev_info.link_mode >= LINK_BT_1 && dev_info.link_mode <= LINK_BT_3) {
            col = dev_info.link_mode;
            g   = 0;
            b   = 0x80; // blue for BT
        } else if (dev_info.link_mode == LINK_RF_24) {
            col = 4;
            g   = 0x80;
            b   = 0; // green for RF
        }
        user_set_rgb_color(get_led_index(1, col), r, g, b);
    }

    if (showCapsLock) {
        switch (keyboard_config.common.caps_indicator_type) {
            case CAPS_INDICATOR_SIDE:
                set_indicator_on_side(0X00, 0x80, 0x80); // highlight top-left side led to indicate caps lock enabled state

                break;
            case CAPS_INDICATOR_UNDER_KEY:
                user_set_rgb_color(caps_key_led_idx, 0, 0x80, 0x80); // 63 is CAPS_LOCK position

                break;
            case CAPS_INDICATOR_BOTH:
                set_indicator_on_side(0X00, 0x80, 0x80);             // highlight top-left side led to indicate caps lock enabled state
                user_set_rgb_color(caps_key_led_idx, 0, 0x80, 0x80); // 63 is CAPS_LOCK position

                break;
            case CAPS_INDICATOR_OFF:
            default:
                break;
        }
    }

    return true;
}
