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

#include "action.h"
#include "color.h"
#include "common/features/socd_cleaner.h"
#include "common/rf_driver.h"
#include "common/config.h"
#include "common/keyboard.h"
#include "common/keys.h"
#include "common/via.h"
#include "config.h"
#include "host.h"
#include "keyboard.h"
#include "nvm_eeprom_eeconfig_internal.h"
#include "quantum.h"
#include "rgb_matrix.h"
#include "user_kb.h"

#ifdef VIA_ENABLE
#    include "eeprom.h"
#    include "via.h"
#else
#    include "eeconfig.h"
#endif

extern bool            f_bat_hold;
extern bool            f_debounce_press_show;
extern bool            f_debounce_release_show;
extern bool            f_sleep_timeout_show;
extern DEV_INFO_STRUCT dev_info;

extern void exit_light_sleep(void);

bool pre_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // wakeup check for light sleep/no sleep - fire this immediately to not lose wake keys.
    if (f_wakeup_prepare) {
        f_wakeup_prepare = 0;
        if (keyboard_config.common.sleep_toggle) exit_light_sleep();
    }

    return pre_process_record_user(keycode, record);
}

/* qmk process record */
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    return process_record_nuphy(keycode, record);
}

/* qmk keyboard post init */
void keyboard_post_init_kb(void) {
    keyboard_post_init_nuphy();
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    housekeeping_task_nuphy();
    housekeeping_task_user();
}

bool rgb_matrix_indicators_kb(void) {
    rgb_matrix_indicators_nuphy();
    return rgb_matrix_indicators_user();;
}

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    rgb_matrix_set_color(RGB_MATRIX_LED_COUNT - 1, 0, 0, 0);

    if (keyboard_config.custom.toggle_custom_keys_highlight) {
        uint8_t layer = get_highest_layer(layer_state);
        switch (layer) {
            case 0:
            case 2:
                break;
            default: {
                for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
                    for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                        uint8_t index = g_led_config.matrix_co[row][col];

                        if (index >= led_min && index <= led_max && index != NO_LED) {
                            int keycode = keymap_key_to_keycode(layer, (keypos_t){col, row});

                            if (keycode >= AMBIENT_VAI && keycode <= AMBIENT_SPD) {
                                rgb_matrix_set_color(index, RGB_WHITE);
                            } else if (keycode >= SIDE_VAI && keycode <= SIDE_SPD) {
                                rgb_matrix_set_color(index, RGB_YELLOW);
                            } else if (keycode >= DEBOUNCE_PRESS_INC && keycode <= DEBOUNCE_PRESS_SHOW) {
                                rgb_matrix_set_color(index, 0, 255, 0);
                            } else if (keycode >= DEBOUNCE_RELEASE_INC && keycode <= DEBOUNCE_RELEASE_SHOW) {
                                rgb_matrix_set_color(index, 255, 0, 0);
                            } else if (keycode == SLEEP_MODE || keycode == TOG_USB_SLP || keycode == TOG_DEEP_SLEEP || (keycode >= SLEEP_TIMEOUT_INC && keycode <= SLEEP_TIMEOUT_SHOW)) {
                                rgb_matrix_set_color(index, RGB_CYAN);
                            } else if (keycode >= LNK_USB && keycode <= LNK_BLE3) {
                                if (dev_info.link_mode != LINK_USB) {
                                    rgb_matrix_set_color(index, RGB_BLUE);
                                }
                            } else if (keycode > KC_NUM_LOCK && keycode <= KC_KP_DOT) {
                                rgb_matrix_set_color(index, RGB_RED);
                            } else if (keycode >= SOCDON && keycode <= SOCDTOG) {
                                rgb_matrix_set_color(index, RGB_BLUE);
                            } else if (keycode > KC_TRNS) {
                                rgb_matrix_set_color(index, 225, 65, 140);
                            }
                        }
                    }
                }
            }
        }
    }

    return rgb_matrix_indicators_advanced_user(led_min, led_max);
}
