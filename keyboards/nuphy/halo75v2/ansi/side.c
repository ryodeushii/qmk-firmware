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

#include "ansi.h"
#include "common/config.h"
#include "common/wireless.h"
#include "host.h"
#include "rgb_matrix.h"
#include "side.h"
#include "timer.h"

#define SIDE_INDEX 83

#define SIDE_LED_COUNT 5
#define AMBIENT_LED_COUNT 40
#define HALO_LED_COUNT (SIDE_LED_COUNT + AMBIENT_LED_COUNT)

#define SIDE_WAVE EFFECT_WAVE
#define SIDE_MIX EFFECT_MIX
#define SIDE_STATIC EFFECT_STATIC
#define SIDE_BREATH EFFECT_BREATH
#define SIDE_OFF EFFECT_OFF

#define LIGHT_COLOR_MAX 8
#define LIGHT_SPEED_MAX 4

#define LOW_BAT_BLINK_PRIOD 500

static const uint8_t side_speed_table[5][5] = {
    [SIDE_WAVE] = {10, 20, 25, 30, 45}, [SIDE_MIX] = {25, 30, 40, 50, 60}, [SIDE_STATIC] = {50, 50, 50, 50, 50}, [SIDE_BREATH] = {25, 30, 40, 50, 60}, [SIDE_OFF] = {50, 50, 50, 50, 50},
};

static const uint8_t side_light_table[6] = {
    0, 16, 32, 64, 128, 255,
};

static const uint8_t side_led_index_tab[SIDE_LED_COUNT] = {
    SIDE_INDEX + 0, SIDE_INDEX + 1, SIDE_INDEX + 2, SIDE_INDEX + 3, SIDE_INDEX + 4,
};

static const uint8_t power_led_index_tab[HALO_LED_COUNT] = {
    SIDE_INDEX + 10, SIDE_INDEX + 11, SIDE_INDEX + 12, SIDE_INDEX + 13, SIDE_INDEX + 14, SIDE_INDEX + 15, SIDE_INDEX + 16, SIDE_INDEX + 17, SIDE_INDEX + 18, SIDE_INDEX + 19, SIDE_INDEX + 20, SIDE_INDEX + 21, SIDE_INDEX + 22, SIDE_INDEX + 23, SIDE_INDEX + 24, SIDE_INDEX + 25, SIDE_INDEX + 26, SIDE_INDEX + 27, SIDE_INDEX + 0, SIDE_INDEX + 1, SIDE_INDEX + 2, SIDE_INDEX + 3, SIDE_INDEX + 4, SIDE_INDEX + 28, SIDE_INDEX + 29, SIDE_INDEX + 30, SIDE_INDEX + 31, SIDE_INDEX + 32, SIDE_INDEX + 33, SIDE_INDEX + 34, SIDE_INDEX + 35, SIDE_INDEX + 36, SIDE_INDEX + 37, SIDE_INDEX + 38, SIDE_INDEX + 39, SIDE_INDEX + 40, SIDE_INDEX + 41, SIDE_INDEX + 42, SIDE_INDEX + 43, SIDE_INDEX + 44, SIDE_INDEX + 9, SIDE_INDEX + 8, SIDE_INDEX + 7, SIDE_INDEX + 6, SIDE_INDEX + 5,
};

bool     f_charging        = true;
uint8_t  side_play_point   = 0;
uint16_t side_play_cnt     = 0;
uint32_t led_play_timer    = 0;
uint8_t  low_bat_blink_cnt = 6;
uint8_t  r_temp, g_temp, b_temp;

static uint8_t key_pwm_tab[HALO_LED_COUNT] = {0};
static uint8_t power_play_index            = 0;
static bool    f_power_show                = true;

extern DEV_INFO_STRUCT dev_info;
extern bool            f_bat_hold;
extern bool            f_dial_sw_init_ok;
extern uint16_t        rf_link_show_time;
extern void            kb_config_reset(void);
void                   os_mode_led_show(void);
void                   sleep_indicator_show(void);
void                   wireless_mode_show(void);

static void light_point_playing(uint8_t trend, uint8_t step, uint8_t len, uint8_t *point) {
    if (trend) {
        *point += step;
        if (*point >= len) {
            *point -= len;
        }
    } else {
        *point -= step;
        if (*point >= len) {
            *point = len - (255 - *point) - 1;
        }
    }
}

static void count_rgb_light(uint8_t light_temp) {
    uint16_t temp;

    temp   = light_temp * r_temp + r_temp;
    r_temp = temp >> 8;

    temp   = light_temp * g_temp + g_temp;
    g_temp = temp >> 8;

    temp   = light_temp * b_temp + b_temp;
    b_temp = temp >> 8;
}

static uint8_t clamp_speed(uint8_t speed) {
    if (speed > LIGHT_SPEED_MAX) {
        return LIGHT_SPEED_MAX / 2;
    }
    return speed;
}

static uint8_t clamp_brightness(uint8_t brightness) {
    if (brightness > 5) {
        return 5;
    }
    return brightness;
}

static void set_segment_rgb(const uint8_t *indices, uint8_t count, uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < count; i++) {
        rgb_matrix_set_color(indices[i], r, g, b);
    }
}

static void set_all_halo_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_segment_rgb(power_led_index_tab, HALO_LED_COUNT, r, g, b);
}

static void set_side_led_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    rgb_matrix_set_color(side_led_index_tab[index], r, g, b);
}

static void adjust_brightness(uint8_t *brightness, uint8_t brighten) {
    if (brighten) {
        if (*brightness == 5) {
            return;
        }
        (*brightness)++;
    } else {
        if (*brightness == 0) {
            return;
        }
        (*brightness)--;
    }

    save_config_to_eeprom();
}

static void adjust_speed(uint8_t *speed, uint8_t faster) {
    *speed = clamp_speed(*speed);

    if (faster) {
        if (*speed) {
            (*speed)--;
        }
    } else {
        if (*speed < LIGHT_SPEED_MAX) {
            (*speed)++;
        }
    }

    save_config_to_eeprom();
}

static void adjust_color(uint8_t mode, uint8_t *rgb_mode, uint8_t *color, uint8_t dir) {
    if (mode == SIDE_WAVE || mode == SIDE_BREATH || mode == SIDE_STATIC) {
        uint8_t delta = dir ? RGB_MATRIX_HUE_STEP : (uint8_t)(-RGB_MATRIX_HUE_STEP);

        keyboard_config.lights.side_static_color.hue += delta;
        save_config_to_eeprom();
        return;
    }

    if (mode != SIDE_WAVE) {
        if (*rgb_mode) {
            *rgb_mode = 0;
            *color    = 0;
        }
    }

    if (dir) {
        if (*rgb_mode) {
            *rgb_mode = 0;
            *color    = 0;
        } else {
            (*color)++;
            if (*color >= LIGHT_COLOR_MAX) {
                *rgb_mode = 1;
                *color    = 0;
            }
        }
    } else {
        if (*rgb_mode) {
            *rgb_mode = 0;
            *color    = LIGHT_COLOR_MAX - 1;
        } else {
            (*color)--;
            if (*color >= LIGHT_COLOR_MAX) {
                *rgb_mode = 1;
                *color    = 0;
            }
        }
    }

    save_config_to_eeprom();
}

static void adjust_mode(uint8_t *mode, uint8_t dir, uint8_t *play_point) {
    if (dir) {
        (*mode)++;
        if (*mode > SIDE_OFF) {
            *mode = 0;
        }
    } else {
        if (*mode > 0) {
            (*mode)--;
        } else {
            *mode = SIDE_OFF;
        }
    }

    *play_point = 0;
    save_config_to_eeprom();
}

static bool consume_animation_step(uint8_t mode, uint8_t speed, uint16_t *play_cnt) {
    speed = clamp_speed(speed);

    if (*play_cnt <= side_speed_table[mode][speed]) {
        return false;
    }

    *play_cnt -= side_speed_table[mode][speed];
    if (*play_cnt > 20) {
        *play_cnt = 0;
    }

    return true;
}

static void render_wave_segment(const uint8_t *indices, uint8_t count, uint8_t brightness, uint8_t speed, uint8_t *play_point, uint16_t *play_cnt) {
    uint8_t play_index;
    uint8_t wave_step = count > SIDE_LED_COUNT ? 5 : 12;

    if (!consume_animation_step(SIDE_WAVE, speed, play_cnt)) {
        return;
    }

    brightness = clamp_brightness(brightness);

    light_point_playing(0, count > SIDE_LED_COUNT ? 1 : 2, WAVE_TAB_LEN, play_point);

    play_index = *play_point;

    for (uint8_t i = 0; i < count; i++) {
        rgb_t rgb = nuphy_picker_hsv_rgb(keyboard_config.lights.side_static_color.hue, keyboard_config.lights.side_static_color.sat, 255);

        r_temp = rgb.r;
        g_temp = rgb.g;
        b_temp = rgb.b;
        light_point_playing(1, wave_step, WAVE_TAB_LEN, &play_index);
        count_rgb_light(wave_data_tab[play_index]);

        count_rgb_light(side_light_table[brightness]);
        rgb_matrix_set_color(indices[i], r_temp, g_temp, b_temp);
    }
}

static void render_mix_segment(const uint8_t *indices, uint8_t count, uint8_t brightness, uint8_t speed, uint8_t *play_point, uint16_t *play_cnt) {
    if (!consume_animation_step(SIDE_MIX, speed, play_cnt)) {
        return;
    }

    brightness = clamp_brightness(brightness);

    light_point_playing(1, 1, FLOW_COLOR_TAB_LEN, play_point);

    r_temp = flow_rainbow_color_tab[*play_point][0];
    g_temp = flow_rainbow_color_tab[*play_point][1];
    b_temp = flow_rainbow_color_tab[*play_point][2];

    count_rgb_light(side_light_table[brightness]);
    set_segment_rgb(indices, count, r_temp, g_temp, b_temp);
}

static void render_breathe_segment(const uint8_t *indices, uint8_t count, uint8_t brightness, uint8_t speed, uint8_t *play_point, uint16_t *play_cnt) {
    if (!consume_animation_step(SIDE_BREATH, speed, play_cnt)) {
        return;
    }

    brightness = clamp_brightness(brightness);

    light_point_playing(0, 1, BREATHE_TAB_LEN, play_point);

    rgb_t rgb = nuphy_picker_hsv_rgb(keyboard_config.lights.side_static_color.hue, keyboard_config.lights.side_static_color.sat, 255);

    r_temp = rgb.r;
    g_temp = rgb.g;
    b_temp = rgb.b;

    count_rgb_light(breathe_data_tab[*play_point]);
    count_rgb_light(side_light_table[brightness]);
    set_segment_rgb(indices, count, r_temp, g_temp, b_temp);
}

static void render_static_segment(const uint8_t *indices, uint8_t count, uint8_t brightness) {
    rgb_t rgb = nuphy_static_picker_rgb(keyboard_config.lights.side_static_color.hue, keyboard_config.lights.side_static_color.sat, brightness);

    r_temp = rgb.r;
    g_temp = rgb.g;
    b_temp = rgb.b;

    count_rgb_light(side_light_table[clamp_brightness(brightness)]);
    set_segment_rgb(indices, count, r_temp, g_temp, b_temp);
}

static void render_halo_effect(void) {
    switch (keyboard_config.lights.side_mode) {
        case SIDE_WAVE:
            render_wave_segment(power_led_index_tab, HALO_LED_COUNT, keyboard_config.lights.side_brightness, keyboard_config.lights.side_speed, &side_play_point, &side_play_cnt);
            break;
        case SIDE_MIX:
            render_mix_segment(power_led_index_tab, HALO_LED_COUNT, keyboard_config.lights.side_brightness, keyboard_config.lights.side_speed, &side_play_point, &side_play_cnt);
            break;
        case SIDE_STATIC:
            render_static_segment(power_led_index_tab, HALO_LED_COUNT, keyboard_config.lights.side_brightness);
            break;
        case SIDE_BREATH:
            render_breathe_segment(power_led_index_tab, HALO_LED_COUNT, keyboard_config.lights.side_brightness, keyboard_config.lights.side_speed, &side_play_point, &side_play_cnt);
            break;
        case SIDE_OFF:
        default:
            set_all_halo_rgb(0, 0, 0);
            break;
    }
}

void side_rgb_refresh(void) {
    rgb_matrix_update_pwm_buffers();
}

void side_brightness_control(uint8_t brighten) {
    adjust_brightness(&keyboard_config.lights.side_brightness, brighten);
}

void side_speed_control(uint8_t fast) {
    adjust_speed(&keyboard_config.lights.side_speed, fast);
}

void side_color_control(uint8_t dir) {
    adjust_color(keyboard_config.lights.side_mode, &keyboard_config.lights.side_rgb, &keyboard_config.lights.side_color, dir);
}

void side_mode_control(uint8_t dir) {
    adjust_mode(&keyboard_config.lights.side_mode, dir, &side_play_point);
}

void ambient_brightness_control(uint8_t brighten) {
    adjust_brightness(&keyboard_config.lights.side_brightness, brighten);
}

void ambient_speed_control(uint8_t fast) {
    adjust_speed(&keyboard_config.lights.side_speed, fast);
}

void ambient_color_control(uint8_t dir) {
    adjust_color(keyboard_config.lights.side_mode, &keyboard_config.lights.side_rgb, &keyboard_config.lights.side_color, dir);
}

void ambient_mode_control(uint8_t dir) {
    adjust_mode(&keyboard_config.lights.side_mode, dir, &side_play_point);
}

void set_side_rgb(uint8_t r, uint8_t g, uint8_t b) {
    set_segment_rgb(side_led_index_tab, SIDE_LED_COUNT, r, g, b);
}

void set_indicator_on_side(uint8_t r, uint8_t g, uint8_t b) {
    set_side_rgb(r, g, b);
}

static void halo_side_indicators_show(void) {
    bool show_caps_lock = false;

    if (dev_info.link_mode == LINK_USB) {
        show_caps_lock = host_keyboard_led_state().caps_lock;
    } else {
        show_caps_lock = dev_info.rf_led & 0x02;
    }

    if (show_caps_lock) {
        switch (keyboard_config.common.caps_indicator_type) {
            case CAPS_INDICATOR_SIDE:
            case CAPS_INDICATOR_BOTH:
                set_indicator_on_side(0x00, 0x80, 0x80);
                break;
            case CAPS_INDICATOR_UNDER_KEY:
            case CAPS_INDICATOR_OFF:
            default:
                break;
        }
    }

    os_mode_led_show();
    sleep_indicator_show();

#if (WORK_MODE == THREE_MODE)
    wireless_mode_show();
#endif
}

void bat_charging_breathe(void) {
    static uint32_t interval_timer = 0;
    static uint8_t  play_point     = 0;

    if (timer_elapsed32(interval_timer) > 30) {
        interval_timer = timer_read32();
        light_point_playing(0, 2, BREATHE_TAB_LEN, &play_point);
    }

    r_temp = 0xFF;
    g_temp = 0x40;
    b_temp = 0x00;
    count_rgb_light(breathe_data_tab[play_point]);
    set_side_rgb(r_temp, g_temp, b_temp);
}

void bat_charging_design(uint8_t init, uint8_t r, uint8_t g, uint8_t b) {
    static uint32_t interval_timer = 0;
    static uint16_t show_mask      = 0x00;
    static bool     f_move_trend   = false;
    uint16_t        bit_mask       = 1;

    if (timer_elapsed32(interval_timer) > 100) {
        interval_timer = timer_read32();

        if (f_move_trend) {
            show_mask >>= 1;
            if (show_mask == (0x1F >> (SIDE_LED_COUNT - init))) {
                f_move_trend = false;
            }
        } else {
            show_mask <<= 1;
            show_mask |= 1;
            if (show_mask == 0x7F) {
                f_move_trend = true;
            }
        }
    }

    for (uint8_t i = 0; i < SIDE_LED_COUNT; i++) {
        if (show_mask & bit_mask) {
            set_side_led_color(i, r, g, b);
        } else {
            set_side_led_color(i, 0x00, 0x00, 0x00);
        }
        bit_mask <<= 1;
    }
}

void low_bat_show(void) {
    static uint32_t interval_timer = 0;

    r_temp = 0x80;
    g_temp = 0x00;
    b_temp = 0x00;

    if (low_bat_blink_cnt) {
        if (timer_elapsed32(interval_timer) > (LOW_BAT_BLINK_PRIOD >> 1)) {
            r_temp = 0x00;
            g_temp = 0x00;
            b_temp = 0x00;
        }

        if (timer_elapsed32(interval_timer) >= LOW_BAT_BLINK_PRIOD) {
            interval_timer = timer_read32();
            low_bat_blink_cnt--;
        }
    }

    set_side_rgb(r_temp, g_temp, b_temp);
}

void bat_percent_led(uint8_t bat_percent) {
    uint8_t bat_end_led = 0;
    uint8_t bat_r;
    uint8_t bat_g;
    uint8_t bat_b;

    if (bat_percent <= 20) {
        bat_end_led = 0;
        bat_r       = side_color_lib[0][0];
        bat_g       = side_color_lib[0][1];
        bat_b       = side_color_lib[0][2];
    } else if (bat_percent <= 40) {
        bat_end_led = 1;
        bat_r       = side_color_lib[1][0];
        bat_g       = side_color_lib[1][1];
        bat_b       = side_color_lib[1][2];
    } else if (bat_percent <= 60) {
        bat_end_led = 2;
        bat_r       = side_color_lib[2][0];
        bat_g       = side_color_lib[2][1];
        bat_b       = side_color_lib[2][2];
    } else if (bat_percent <= 80) {
        bat_end_led = 3;
        bat_r       = side_color_lib[4][0];
        bat_g       = side_color_lib[4][1];
        bat_b       = side_color_lib[4][2];
    } else {
        bat_end_led = 4;
        bat_r       = side_color_lib[3][0];
        bat_g       = side_color_lib[3][1];
        bat_b       = side_color_lib[3][2];
    }

    bat_r = bat_r * keyboard_config.custom.battery_indicator_brightness / 100;
    bat_g = bat_g * keyboard_config.custom.battery_indicator_brightness / 100;
    bat_b = bat_b * keyboard_config.custom.battery_indicator_brightness / 100;

    if (f_charging) {
        low_bat_blink_cnt = 6;
#if (CHARGING_SHIFT)
        bat_charging_design(bat_end_led, bat_r, bat_g, bat_b);
#else
        bat_charging_breathe();
#endif
    } else if (bat_percent < 10) {
        low_bat_show();
    } else {
        low_bat_blink_cnt = 6;
        for (uint8_t i = 0; i < SIDE_LED_COUNT; i++) {
            if (i <= bat_end_led) {
                set_side_led_color(i, bat_r, bat_g, bat_b);
            } else {
                set_side_led_color(i, 0x00, 0x00, 0x00);
            }
        }
    }
}

void bat_led_show(void) {
    static bool     bat_show_flag    = true;
    static uint32_t bat_show_time    = 0;
    static uint32_t bat_sts_debounce = 0;
    static uint32_t bat_per_debounce = 0;
    static uint8_t  charge_state     = 0;
    static uint8_t  bat_percent      = 0;
    static bool     f_init           = true;

    if (dev_info.link_mode != LINK_USB) {
        if (rf_link_show_time < RF_LINK_SHOW_TIME) {
            return;
        }

        if (dev_info.rf_state != RF_CONNECT) {
            return;
        }
    }

    if (f_init) {
        f_init        = false;
        bat_show_time = timer_read32();
        charge_state  = dev_info.rf_charge;
        bat_percent   = dev_info.rf_battery;
    }

    if (charge_state != dev_info.rf_charge) {
        if (timer_elapsed32(bat_sts_debounce) > 1000) {
            if (((charge_state & 0x01) == 0) && ((dev_info.rf_charge & 0x01) != 0)) {
                bat_show_flag = true;
                f_charging    = true;
                bat_show_time = timer_read32();
            }
            charge_state = dev_info.rf_charge;
        }
    } else {
        bat_sts_debounce = timer_read32();

        if (f_charging) {
            if (timer_elapsed32(bat_show_time) > 10000) {
                bat_show_flag = false;
                f_charging    = false;
            }
        } else if (timer_elapsed32(bat_show_time) > 5000) {
            bat_show_flag = false;
        }

        if (charge_state == 0x03) {
            f_charging = true;
        } else if (!(charge_state & 0x01)) {
            f_charging = false;
        }
    }

    if (bat_percent != dev_info.rf_battery) {
        if (timer_elapsed32(bat_per_debounce) > 1000) {
            bat_percent = dev_info.rf_battery;
        }
    } else {
        bat_per_debounce = timer_read32();

        if ((bat_percent < 10) && (!(charge_state & 0x01))) {
            bat_show_flag = true;
            bat_show_time = timer_read32();

            if (rgb_matrix_config.hsv.v > RGB_MATRIX_VAL_STEP) {
                rgb_matrix_config.hsv.v = RGB_MATRIX_VAL_STEP;
            }

            if (keyboard_config.lights.side_brightness > 1) {
                keyboard_config.lights.side_brightness = 1;
            }

            keyboard_config.lights.ambient_brightness = keyboard_config.lights.side_brightness;
        }
    }

    if (f_bat_hold || bat_show_flag) {
        bat_percent_led(bat_percent);
    }
}

void device_reset_show(void) {
    gpio_write_pin_high(DC_BOOST_PIN);
    gpio_write_pin_high(RGB_DRIVER_SDB1);
    gpio_write_pin_high(RGB_DRIVER_SDB2);

    for (int blink_cnt = 0; blink_cnt < 3; blink_cnt++) {
        rgb_matrix_set_color_all(0xFF, 0xFF, 0xFF);
        rgb_matrix_update_pwm_buffers();
        wait_ms(200);

        rgb_matrix_set_color_all(0x00, 0x00, 0x00);
        rgb_matrix_update_pwm_buffers();
        wait_ms(200);
    }
}

void device_reset_init(void) {
    side_play_point  = 0;
    side_play_cnt    = 0;
    led_play_timer   = timer_read32();
    power_play_index = 0;
    f_power_show     = true;
    f_bat_hold       = false;

    kb_config_reset();
}

void rgb_test_show(void) {
    gpio_write_pin_high(DC_BOOST_PIN);
    gpio_write_pin_high(RGB_DRIVER_SDB1);
    gpio_write_pin_high(RGB_DRIVER_SDB2);

    rgb_matrix_set_color_all(0xFF, 0x00, 0x00);
    rgb_matrix_update_pwm_buffers();
    wait_ms(1000);

    rgb_matrix_set_color_all(0x00, 0xFF, 0x00);
    rgb_matrix_update_pwm_buffers();
    wait_ms(1000);

    rgb_matrix_set_color_all(0x00, 0x00, 0xFF);
    rgb_matrix_update_pwm_buffers();
    wait_ms(1000);
}

static void side_power_mode_show(void) {
    if (!consume_animation_step(SIDE_WAVE, keyboard_config.lights.side_speed, &side_play_cnt)) {
        return;
    }

    if (power_play_index < HALO_LED_COUNT) {
        key_pwm_tab[power_play_index] = 0xFF;
        power_play_index++;
    }

    for (uint8_t i = 0; i < HALO_LED_COUNT; i++) {
        if (keyboard_config.lights.side_mode == SIDE_MIX) {
            r_temp = flow_rainbow_color_tab[side_play_point % FLOW_COLOR_TAB_LEN][0];
            g_temp = flow_rainbow_color_tab[side_play_point % FLOW_COLOR_TAB_LEN][1];
            b_temp = flow_rainbow_color_tab[side_play_point % FLOW_COLOR_TAB_LEN][2];
        } else {
            rgb_t rgb = nuphy_picker_hsv_rgb(keyboard_config.lights.side_static_color.hue, keyboard_config.lights.side_static_color.sat, 255);

            r_temp = rgb.r;
            g_temp = rgb.g;
            b_temp = rgb.b;
        }

        count_rgb_light(key_pwm_tab[i]);
        count_rgb_light(side_light_table[2]);
        rgb_matrix_set_color(power_led_index_tab[i], r_temp, g_temp, b_temp);
    }

    for (uint8_t i = 0; i < HALO_LED_COUNT; i++) {
        if (key_pwm_tab[i] & 0x80) {
            key_pwm_tab[i] -= 8;
        } else if (key_pwm_tab[i] & 0x40) {
            key_pwm_tab[i] -= 6;
        } else if (key_pwm_tab[i] & 0x20) {
            key_pwm_tab[i] -= 4;
        } else if (key_pwm_tab[i] & 0x10) {
            key_pwm_tab[i] -= 3;
        } else if (key_pwm_tab[i] & 0x08) {
            key_pwm_tab[i] -= 2;
        } else if (key_pwm_tab[i]) {
            key_pwm_tab[i]--;
        }
    }

    if (key_pwm_tab[HALO_LED_COUNT - 1] == 1) {
        f_power_show      = false;
        rf_link_show_time = 0;
        f_charging        = true;
    }
}

void side_led_show(void) {
    static bool flag_power_on = true;
    uint32_t    elapsed;

    if (flag_power_on) {
        if (!f_dial_sw_init_ok) {
            return;
        }
        flag_power_on = false;
    }

    elapsed        = timer_elapsed32(led_play_timer);
    led_play_timer = timer_read32();
    side_play_cnt += elapsed;

    if (!keyboard_config.common.power_on_animation) {
        f_power_show = false;
    }

    if (f_power_show) {
        side_power_mode_show();
        return;
    }

    keyboard_config.lights.ambient_mode       = keyboard_config.lights.side_mode;
    keyboard_config.lights.ambient_brightness = keyboard_config.lights.side_brightness;
    keyboard_config.lights.ambient_speed      = keyboard_config.lights.side_speed;
    render_halo_effect();

#if (WORK_MODE == THREE_MODE)
    bat_led_show();
#endif

    halo_side_indicators_show();
}
