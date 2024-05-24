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
#include "rgb_table.h"
#include "mcu_pwr.h"
#include "usb_main.h"
#include "is31fl3733.h"

/* side rgb mode */
enum {
    SIDE_WAVE = 0,
    SIDE_MIX,
    SIDE_STATIC,
    SIDE_BREATH,
    SIDE_OFF,
};

uint8_t side_play_point       = 0;
uint32_t side_one_timer       = 0;
uint8_t sys_light             = 0;
bool rgb_state                = 0;
uint8_t rgb_start_led         = 0;
uint8_t rgb_end_led           = 0;
uint16_t rgb_show_time        = 0;
uint32_t rgb_indicator_timer  = 0;

HSV hsv = { .h = 0, .s = 255, .v = 255};
RGB current_rgb = {.r = 0x00, .g = 0x00, .b = 0x00};

const uint8_t side_speed_table[5][5] = {
    [SIDE_WAVE]   = {14, 19, 25, 32, 40}, // [SIDE_WAVE]   = {10, 14, 20, 28, 38},
    [SIDE_MIX]    = {14, 19, 25, 32, 40}, // [SIDE_MIX]    = {10, 14, 20, 28, 38},
    [SIDE_STATIC] = {50, 50, 50, 50, 50}, // [SIDE_STATIC] = {50, 50, 50, 50, 50},
    [SIDE_BREATH] = {14, 19, 25, 32, 40}, // [SIDE_BREATH] = {10, 14, 20, 28, 38},
    [SIDE_OFF]    = {50, 50, 50, 50, 50}, // [SIDE_OFF]    = {50, 50, 50, 50, 50},
};

const uint8_t side_light_table[7] = {
    0,
    16,
    32,
    64,
    96,
    128,
    160,
};

const uint8_t side_led_index_tab[SIDE_LINE][2] =
    {
        {SIDE_INDEX + 4, SIDE_INDEX + 5},
        {SIDE_INDEX + 3, SIDE_INDEX + 6},
        {SIDE_INDEX + 2, SIDE_INDEX + 7},
        {SIDE_INDEX + 1, SIDE_INDEX + 8},
        {SIDE_INDEX + 0, SIDE_INDEX + 9},
};

void set_sys_light(void) {
    sys_light = user_config.ee_side_light > 5 ? 1 : (3 - user_config.ee_side_light / 2);
}

static void side_off_mode_show(void);
void rgb_matrix_update_pwm_buffers(void);

typedef struct is31fl3733_driver_t {
    uint8_t pwm_buffer[192];
    bool    pwm_buffer_dirty;
    uint8_t led_control_buffer[24];
    bool    led_control_buffer_dirty;
} PACKED is31fl3733_driver_t;

extern is31fl3733_driver_t driver_buffers[DRIVER_COUNT];

/**
 * @brief is_side_rgb_off
 */

bool is_side_rgb_off(void)
{
    is31fl3733_led_t led;
    for (int i = SIDE_INDEX; i < SIDE_INDEX + 10; i++) {
        memcpy_P(&led, (&g_is31fl3733_leds[i]), sizeof(led));
        if (driver_buffers[led.driver].pwm_buffer[led.r] != 0 || driver_buffers[led.driver].pwm_buffer[led.g] != 0 || driver_buffers[led.driver].pwm_buffer[led.b] != 0) {
            return false;
        }
    }
    return true;
}


void side_rgb_refresh(void) {
    if (!is_side_rgb_off() || user_config.ee_side_light > 0) {
        pwr_led_on(); // power on side LED before refresh
    }
}

/**
 * @brief  Adjusting the brightness of side lights.
 * @param  bright: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_light_control(uint8_t bright) {
    if (bright) {
        if (user_config.ee_side_light < SIDE_BRIGHT_MAX) user_config.ee_side_light++;
    } else {
        if (user_config.ee_side_light > 0) user_config.ee_side_light--;
    }
    set_sys_light();
#ifndef NO_DEBUG
    dprintf("side matrix light_control [NOEEPROM]: %d\n", user_config.ee_side_light);
#endif
}

/**
 * @brief  Adjusting the speed of side lights.
 * @param  fast: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_speed_control(uint8_t fast) {
    if (fast) {
        if (user_config.ee_side_speed > 0) user_config.ee_side_speed--;
    } else {
        if (user_config.ee_side_speed < SIDE_SPEED_MAX) user_config.ee_side_speed++;
    }
#ifndef NO_DEBUG
    dprintf("side matrix speed_control [NOEEPROM]: %d\n", user_config.ee_side_speed);
#endif
}

/**
 * @brief  Switch to the next color of side lights.
 * @param  color: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_colour_control(uint8_t color) {
    if (user_config.ee_side_mode != SIDE_WAVE) {
        if (user_config.ee_side_rgb) {
            user_config.ee_side_rgb    = 0;
            user_config.ee_side_colour = game_mode_enable;
        }
    }

    if (user_config.ee_side_rgb) {
        user_config.ee_side_rgb    = 0;
        user_config.ee_side_colour = color ? game_mode_enable : SIDE_COLOUR_MAX - 1;
    } else {
        if (color) user_config.ee_side_colour++;
        else user_config.ee_side_colour--;
        if (user_config.ee_side_colour >= SIDE_COLOUR_MAX) {
            user_config.ee_side_rgb    = 1;
            user_config.ee_side_colour = game_mode_enable;
        }
    }
#ifndef NO_DEBUG
    dprintf("side matrix colour_control [NOEEPROM]: %d rgb: %d\n", user_config.ee_side_colour, user_config.ee_side_rgb);
#endif
}

/**
 * @brief  Change the color mode of side lights.
 * @param  dir: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_mode_control(uint8_t dir) {
    if (dir) {
        user_config.ee_side_mode++;
        if (user_config.ee_side_mode > SIDE_OFF) user_config.ee_side_mode = 0;
    } else {
        if (user_config.ee_side_mode > 0) user_config.ee_side_mode--;
        else user_config.ee_side_mode = SIDE_OFF;
    }
    side_play_point          = 0;
#ifndef NO_DEBUG
    dprintf("side matrix mode_control [NOEEPROM]: %d\n", user_config.ee_side_mode);
#endif
}

void set_side_rgb(uint8_t side, uint8_t r, uint8_t g, uint8_t b) {
    // side = 1 => left
    // side = 2 => right
    // side = 3 => both
    uint8_t start = 0;
    uint8_t end = SIDE_LINE * 2;
    if (side > SYS_MARK) {
        r = r / sys_light;
        g = g / sys_light;
        b = b / sys_light;
    }
    if (side % SYS_MARK == LEFT_SIDE) end = end - SIDE_LINE;
    if (side % SYS_MARK == RIGHT_SIDE) start = start + SIDE_LINE;

    for (uint8_t i = start; i < end; i++) {
        rgb_matrix_set_color(SIDE_INDEX + i, r, g, b);
    }
}

/**
 * @brief  Visual cue for sleep on side LED.
*/
void signal_sleep(uint8_t r, uint8_t g, uint8_t b) {
    pwr_led_on();
    wait_ms(50); // give some time to ensure LED powers on.
    set_side_rgb(LEFT_SIDE + RIGHT_SIDE, r, g, b);
    rgb_matrix_update_pwm_buffers();
    wait_ms(300);
}

/**
 * @brief   system switch led show
 */
void sys_sw_led_show(void) {
    if (sys_show_timer != 0) {
        if (dev_info.sys_sw_state == SYS_SW_MAC) {
            current_rgb.r = SIDE_BLINK_LIGHT;
            current_rgb.g = SIDE_BLINK_LIGHT;
            current_rgb.b = SIDE_BLINK_LIGHT;
        } else {
            current_rgb.r = 0x00;
            current_rgb.g = 0x00;
            current_rgb.b = SIDE_BLINK_LIGHT;
        }
        if ((timer_elapsed32(sys_show_timer) / 500) % 2 == 0) {
            set_side_rgb(RIGHT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
        } else {
            set_side_rgb(RIGHT_SIDE, RGB_OFF);
        }
        if (timer_elapsed32(sys_show_timer) >= 3000) {
            sys_show_timer = 0;
        }
    }
}

/**
 * @brief  sleep_sw_led_show
 */
void sleep_sw_led_show(void) {
    if (sleep_show_timer != 0) {
        current_rgb.r = 0x00, current_rgb.g = 0x00, current_rgb.b = 0x00;
        switch (user_config.sleep_mode) {
            case 0:
                current_rgb.r = SIDE_BLINK_LIGHT;
                break;
            case 1:
                current_rgb.g = SIDE_BLINK_LIGHT;
                break;
            case 2:
                current_rgb.r = SIDE_BLINK_LIGHT;
                current_rgb.g = SIDE_BLINK_LIGHT;
                break;
        }

        if ((timer_elapsed32(sleep_show_timer) / 500) % 2 == 0) {
            set_side_rgb(RIGHT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
        } else {
            set_side_rgb(RIGHT_SIDE, RGB_OFF);
        }
        if (timer_elapsed32(sleep_show_timer) >= 3000) {
            sleep_show_timer = 0;
        }
    }
}

/**
 * @brief  host system led indicate.
 */
void sys_led_show(void) {
    current_rgb.g = SIDE_BLINK_LIGHT;
    current_rgb.b = SIDE_BLINK_LIGHT;
    current_rgb.r = 0x00;
    uint8_t led_side = LEFT_SIDE;

    if (host_keyboard_led_state().caps_lock && user_config.sys_ind > 0) {
        led_side = RIGHT_SIDE;
        set_side_rgb(LEFT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
    }

    if (host_keyboard_led_state().num_lock && user_config.sys_ind > 1) {
        current_rgb.r = SIDE_BLINK_LIGHT;
        set_side_rgb(led_side + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
    }
}

/**
 * @brief  light_point_playing.
 * @param trend:
 * @param step:
 * @param len:
 * @param point:
 */
static void light_point_playing(uint8_t trend, uint8_t step, uint8_t len, uint8_t *point) {
    if (trend) {
        *point += step;
        if (*point > 254 && len == 0) *point = 0;
        else if (*point >= len) *point -= len;
    } else {
        *point -= step;
        if (*point < 1 && len == 0) *point = 255;
        else if (*point >= len) *point = len - (255 - *point) - 1;
    }
}


/**
 * @brief  count_rgb_light.
 * @param light_temp:
 */
static void count_rgb_light(uint8_t light_temp) {
    uint16_t temp;

    temp   = (light_temp)*current_rgb.r + current_rgb.r;
    current_rgb.r = temp >> 8;

    temp   = (light_temp)*current_rgb.g + current_rgb.g;
    current_rgb.g = temp >> 8;

    temp   = (light_temp)*current_rgb.b + current_rgb.b;
    current_rgb.b = temp >> 8;
}

/**
 * @brief  side_wave_mode_show.
 */
static void side_wave_mode_show(void) {
    uint8_t play_index;

    if (user_config.ee_side_rgb)
        light_point_playing(0, 6, 0, &side_play_point);
    else
        light_point_playing(0, 2, BREATHE_TAB_LEN, &side_play_point);

    play_index = side_play_point;
    for (uint8_t i = 0; i < SIDE_LINE; i++) {
        if (user_config.ee_side_rgb) {
            hsv.h = play_index;
            current_rgb = hsv_to_rgb_nocie(hsv);

            light_point_playing(1, 32, 0, &play_index);
        } else {
            current_rgb.r = colour_lib[user_config.ee_side_colour][0];
            current_rgb.g = colour_lib[user_config.ee_side_colour][1];
            current_rgb.b = colour_lib[user_config.ee_side_colour][2];
            light_point_playing(1,12,BREATHE_TAB_LEN,&play_index);
            count_rgb_light(breathe_data_tab[play_index]);
        }
        count_rgb_light(side_light_table[user_config.ee_side_light]);

        for (uint8_t j = 0; j < 2; j++) {
            rgb_matrix_set_color(side_led_index_tab[i][j], current_rgb.r, current_rgb.g, current_rgb.b);
        }
    }
}

/**
 * @brief  side_spectrum_mode_show.
 */
static void side_spectrum_mode_show(void) {
    light_point_playing(0, 2, 0, &side_play_point);
    hsv.h = side_play_point;
    current_rgb   = hsv_to_rgb_nocie(hsv);

    count_rgb_light(side_light_table[user_config.ee_side_light]);
    set_side_rgb(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}


/**
 * @brief  side_breathe_mode_show.
 */
static void side_breathe_mode_show(void) {
    light_point_playing(1,1,BREATHE_TAB_LEN,&side_play_point);

    current_rgb.r = colour_lib[user_config.ee_side_colour][0];
    current_rgb.g = colour_lib[user_config.ee_side_colour][1];
    current_rgb.b = colour_lib[user_config.ee_side_colour][2];

    count_rgb_light(breathe_data_tab[side_play_point]);
    count_rgb_light(side_light_table[user_config.ee_side_light]);
    set_side_rgb(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}

/**
 * @brief  side_static_mode_show.
 */
static void side_static_mode_show(void) {
    current_rgb.r = colour_lib[user_config.ee_side_colour][0];
    current_rgb.g = colour_lib[user_config.ee_side_colour][1];
    current_rgb.b = colour_lib[user_config.ee_side_colour][2];

    count_rgb_light(side_light_table[user_config.ee_side_light]);
    set_side_rgb(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}

/**
 * @brief  side_off_mode_show.
 */
static void side_off_mode_show(void) {
    set_side_rgb(LEFT_SIDE + RIGHT_SIDE, RGB_OFF);
}

/**
 * @brief  side_one_control
 */
void side_one_control(uint8_t adjust)
{
    if (adjust) {
        if (user_config.ee_side_one == LEFT_SIDE + RIGHT_SIDE) {
            user_config.ee_side_one = 0;
        } else {
            user_config.ee_side_one++;
            side_one_timer = 1;
        }
    }
#ifndef NO_DEBUG
    dprintf("side matrix side_control [NOEEPROM]: %d\n", user_config.ee_side_one);
#endif
}

/**
 * @brief  side_one_show
 */
static void side_one_show(void)
{
    static uint8_t my_side = 2;
    if (user_config.ee_side_one == 0) return;

    if (user_config.ee_side_one != LEFT_SIDE + RIGHT_SIDE)
        set_side_rgb(user_config.ee_side_one, RGB_OFF);
    else {
        if (side_one_timer == 1) {
            my_side = 2;
    }
        if (side_one_timer <= 1)
            side_one_timer = timer_read32();

        if (timer_elapsed32(side_one_timer) < 500 ) {
            set_side_rgb(LEFT_SIDE + RIGHT_SIDE, RGB_OFF);
            return;
        } else if (timer_elapsed32(side_one_timer) >= (1000 * 60 * 1)) {
            side_one_timer = 0;
            my_side = my_side == 2 ? 1 : 2;
        }
        set_side_rgb(my_side, RGB_OFF);
    }
}

/**
 * @brief  rf state indicate
 */
void rf_led_show(void) {
    static uint32_t rf_blink_timer = 0;
    uint16_t        rf_blink_period = 0;

    if (rf_blink_cnt || (rf_link_show_time < RF_LINK_SHOW_TIME)) {
        current_rgb.r = dev_info.link_mode == LINK_USB ? SIDE_BLINK_LIGHT : 0x00; // LINK_USB
        current_rgb.g = dev_info.link_mode % LINK_USB == 0 ? SIDE_BLINK_LIGHT : 0x00; // LINK_USB || LINK_RF_24
        current_rgb.b = dev_info.link_mode % LINK_USB != 0 ? SIDE_BLINK_LIGHT : 0x00; // LINK_BT
    } else {
        rf_blink_timer = timer_read32();
        return;
    }

    if (rf_blink_cnt) {
        if (dev_info.rf_state == RF_PAIRING)
            rf_blink_period = RF_LED_PAIR_PERIOD;
        else
            rf_blink_period = RF_LED_LINK_PERIOD;

        if (timer_elapsed32(rf_blink_timer) > (rf_blink_period >> 1)) {
            current_rgb.r = 0x00, current_rgb.g = 0x00, current_rgb.b = 0x00;
        }

        if (timer_elapsed32(rf_blink_timer) >= rf_blink_period) {
            rf_blink_cnt--;
            rf_blink_timer = timer_read32();
        }
    }

    set_side_rgb(LEFT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);

    // light up corresponding BT/RF key
    if (dev_info.link_mode <= LINK_BT_3) {
        uint8_t my_pos = dev_info.link_mode == LINK_RF_24 ? 23 : (19 + dev_info.link_mode);
        rgb_required = 1;
        rgb_matrix_set_color(my_pos, current_rgb.r, current_rgb.g, current_rgb.b);
    }
}

/**
 * @brief  bat_num_led.
 */
void bat_num_led(void) {
    uint8_t bat_percent = dev_info.rf_battery;
    uint8_t bat_pct = bat_percent / 10;
    uint8_t r, g, b;

    rgb_required = 1;

    // set color
    if (bat_percent <= 15) {
        r = 0xff; g = 0x00; b = 0x00;
    }
    else if (bat_percent <= 50) {
        r = 0xff; g = 0x40; b = 0x00;
    }
    else if (bat_percent <= 80) {
        r = 0xff; g = 0xff; b = 0x00;
    } else {
        r = 0x00; g = 0xff; b = 0x00;
    }

    if (bat_percent % 10 == 0) bat_pct--;

    for(uint8_t i=0; i < bat_pct; i++)
        rgb_matrix_set_color(20 + i, r, g, b);
    // set percent

    if (bat_percent % 10 == 0) {
        r = 0x00; g = 0xff; b = 0x00;
    } else if (bat_percent % 10 <= 3) {
        r = 0xff; g = 0x00; b = 0x00;
    } else if (bat_percent % 10 <= 6) {
        r = 0xff; g = 0x40; b = 0x00;
    } else {
        r = 0x00; g = 0xff; b = 0x00;
    }

    rgb_matrix_set_color(20 + bat_pct, r, g, b);

}

/**
 * @brief  Battery level indicator
 */
void bat_percent_led(void)
{
    uint8_t bat_percent = dev_info.rf_battery;
    uint8_t bat_end_led = 4;
    current_rgb.r = SIDE_BLINK_LIGHT, current_rgb.g = SIDE_BLINK_LIGHT / 2, current_rgb.b = 0x00;

    if (bat_percent <= 30) {
        bat_end_led = 1;
        current_rgb.r = SIDE_BLINK_LIGHT, current_rgb.g = 0x00, current_rgb.b = 0x00;
    } else if (bat_percent <= 40) {
        bat_end_led = 1;
    } else if (bat_percent <= 60) {
        bat_end_led = 2;
    } else if (bat_percent <= 75) {
        bat_end_led = 3;
    } else if (bat_percent <= 90) {
        bat_end_led = 4;
    } else {
        current_rgb.r = 0x00, current_rgb.g = SIDE_BLINK_LIGHT, current_rgb.b = 0x00;
    }

    set_side_rgb(RIGHT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
    for (uint8_t i = bat_end_led + 1; i < SIDE_LINE; i++)
        rgb_matrix_set_color(SIDE_INDEX + 9 - i, RGB_OFF);

}

/**
 * @brief  battery state indicate
 */
void bat_led_show(void)
{
    static bool bat_show_flag   = 1;
    static bool bat_show_breath = 0;
    static bool f_init          = 1;
    static uint8_t  bat_play_point = 0;
    static uint32_t bat_play_timer = 0;
    static uint32_t bat_show_time  = 0;

    static uint32_t bat_sts_debounce = 0;
    static uint8_t  charge_state     = 0;

    if (f_init) {
        f_init        = 0;
        bat_show_time = timer_read32();
        charge_state  = dev_info.rf_charge;
    }

    if (charge_state != dev_info.rf_charge) {
        if (timer_elapsed32(bat_sts_debounce) > 1500) {
            if ((charge_state & 0x01) == 0 && (dev_info.rf_charge & 0x01) != 0) {
                bat_show_flag   = true;
                bat_show_breath = true;
                bat_show_time   = timer_read32();
            }
            charge_state = dev_info.rf_charge;
        }
    } else {
        bat_sts_debounce = timer_read32();
        if (timer_elapsed32(bat_show_time) > 5000) {
            bat_show_flag   = false;
            bat_show_breath = false;
        }
        if (charge_state == 0x03) {
            bat_show_breath = true;
        } else if (charge_state & 0x01) {
            dev_info.rf_battery = 100;
        }
    }

    if (dev_info.rf_battery < 15) {
        bat_show_flag = true;
        bat_show_breath = true;
        bat_show_time = timer_read32();
    }

    if (f_bat_hold || bat_show_flag) {

        if (bat_show_breath) {

            if (timer_elapsed32(bat_play_timer) > 15) {
                bat_play_timer = timer_read32();
                light_point_playing(0, 1, BREATHE_TAB_LEN, &bat_play_point);
            }
            current_rgb.r = SIDE_BLINK_LIGHT, current_rgb.g = dev_info.rf_battery < 15 ? 0x00 : SIDE_BLINK_LIGHT / 2, current_rgb.b = 0x00;
            count_rgb_light(breathe_data_tab[bat_play_point]);
            set_side_rgb(RIGHT_SIDE + SYS_MARK, current_rgb.r, current_rgb.g, current_rgb.b);
        } else {
            bat_percent_led();
        }
    }
}

/**
 * @brief  device_reset_show.
 */
void device_reset_show(void)
{
    pwr_led_on();
    for (uint8_t blink_cnt = 0; blink_cnt < 3; blink_cnt++) {
        rgb_matrix_set_color_all(RGB_WHITE);
        rgb_matrix_update_pwm_buffers();
        wait_ms(200);

        rgb_matrix_set_color_all(RGB_OFF);
        rgb_matrix_update_pwm_buffers();
        wait_ms(200);
    }
}

/**
 * @brief  device_reset_init.
 */
void device_reset_init(void)
{
    side_play_point  = 0;
    game_mode_enable = 0;
    f_bat_hold       = false;

    rgb_matrix_enable_noeeprom();
    user_config_reset();
}

/**
 * @brief  rgb test
 */

void rgb_test_show(void) {
    // open power control
    pwr_led_on();

    for (uint8_t i = 0; i < SIDE_COLOUR_MAX; i++) {
        rgb_matrix_set_color_all(colour_lib[i][0], colour_lib[i][1], colour_lib[i][2]);
        set_side_rgb(LEFT_SIDE + RIGHT_SIDE, colour_lib[i][0], colour_lib[i][1], colour_lib[i][2]);
        rgb_matrix_update_pwm_buffers();
        wait_ms(500);
    }
}

void signal_rgb_led(uint8_t state, uint8_t start_led, uint8_t end_led, uint16_t show_time) {
    rgb_state     = state > 0 ? 1 : 0;
    rgb_start_led = start_led;
    rgb_end_led   = end_led;
    rgb_show_time = show_time;
}

void rgb_led_indicator(void) {
    if (rgb_show_time == 0) return;
    if (rgb_indicator_timer == 0) { rgb_indicator_timer = timer_read32(); }
    if (timer_elapsed32(rgb_indicator_timer) < rgb_show_time) {
        current_rgb.r = 0xFF;
        current_rgb.g = 0x00;
        current_rgb.b = 0x00;
        if (rgb_state) {
            current_rgb.r = 0x00;
            current_rgb.g = 0xFF;
        }

        rgb_required = 1;
        for (uint8_t i = rgb_start_led; i <= rgb_end_led; i++) {
            rgb_matrix_set_color(i, current_rgb.r, current_rgb.g, current_rgb.b);
        }
    } else {
        rgb_indicator_timer = 0;
        rgb_show_time       = 0;
    }
}

void caps_word_show(void) {
    if (game_mode_enable || !user_config.caps_word_enable) return;
    if (is_caps_word_on()) {
        rgb_required = 1;
        rgb_matrix_set_color(CAPS_LED, RGB_CYAN);
    }
}


/**
 * @brief  side_led_show.
 */
void normal_led_process(void) {
    static uint32_t side_update_time  = 0;
    // side_mode & side_speed should always be valid...
    // refresh side LED animation based on speed.

    uint32_t update_interval = game_mode_enable ? 500 : side_speed_table[user_config.ee_side_mode][user_config.ee_side_speed];

    if (timer_elapsed32(side_update_time) < update_interval) return;
    side_update_time = timer_read32();
    if (!sys_light) { set_sys_light(); }

    switch (user_config.ee_side_mode) {
        case SIDE_WAVE:
            side_wave_mode_show();
            break;
        case SIDE_MIX:
            side_spectrum_mode_show();
            break;
        case SIDE_BREATH:
            side_breathe_mode_show();
            break;
        case SIDE_STATIC:
            side_static_mode_show();
            break;
        case SIDE_OFF:
            side_off_mode_show();
            break;
    }

    if (!game_mode_enable) {
        side_one_show();
        bat_led_show();
        sleep_sw_led_show();
    } else if (dev_info.rf_battery < 15 && !USB_ACTIVE) { set_side_rgb(RIGHT_SIDE, 0x40, 0x00, 0x00); }

    sys_sw_led_show();
    sys_led_show();
}

void realtime_led_process(void) {
    rf_led_show();
    rgb_led_indicator();
    caps_word_show();
}

void led_show(void) {
    if (f_wakeup_prepare) return;
    normal_led_process();
    realtime_led_process();
    side_rgb_refresh();
}
