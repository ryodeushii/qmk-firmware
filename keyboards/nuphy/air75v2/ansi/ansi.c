/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "action.h"
#include "color.h"
#include "common/wireless.h"
#include "common/config.h"
#include "common/keyboard.h"
#include "common/keys.h"
#include "common/lighting/highlight.h"
#include "config.h"
#include "host.h"
#include "keyboard.h"
#include "mcu_pwr.h"
#include "quantum.h"
#include "rgb_matrix.h"

#ifdef VIA_ENABLE
#    include "eeprom.h"
#    include "via.h"
#else
#    include "eeconfig.h"
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    return process_record_nuphy(keycode, record);
}

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
    return rgb_matrix_indicators_user();
}

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    rgb_matrix_set_color(RGB_MATRIX_LED_COUNT, 0, 0, 0);
    nuphy_highlight_custom_keys(led_min, led_max);

    return rgb_matrix_indicators_advanced_user(led_min, led_max);
}

void gpio_init(void) {
    pwr_rgb_led_on();
    pwr_side_led_on();

    gpio_set_pin_output(DRIVER_SIDE_DI_PIN);
    gpio_write_pin_low(DRIVER_SIDE_DI_PIN);

#if (WORK_MODE == THREE_MODE)
    gpio_set_pin_output(NRF_WAKEUP_PIN);
    gpio_write_pin_high(NRF_WAKEUP_PIN);

    gpio_set_pin_input_high(NRF_TEST_PIN);

    gpio_set_pin_output(NRF_RESET_PIN);
    gpio_write_pin_low(NRF_RESET_PIN);
    wait_ms(50);
    gpio_write_pin_high(NRF_RESET_PIN);

    gpio_set_pin_input_high(DEVICE_MODE_PIN);
#endif
    gpio_set_pin_input_high(OS_MODE_PIN);

    gpio_set_pin_output(DC_BOOST_PIN);
    gpio_write_pin_high(DC_BOOST_PIN);

    gpio_set_pin_output(DRIVER_MATRIX_CS_PIN);
    gpio_write_pin_low(DRIVER_MATRIX_CS_PIN);

    gpio_set_pin_output(DRIVER_SIDE_CS_PIN);
    gpio_write_pin_low(DRIVER_SIDE_CS_PIN);
}
