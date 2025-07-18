/* Copyright 2018 James Laird-Wah
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rgb_matrix_drivers.h"

#include <stdbool.h>
#include "keyboard.h"
#include "color.h"
#include "util.h"

/* Each driver needs to define the struct
 *    const rgb_matrix_driver_t rgb_matrix_driver;
 * All members must be provided.
 * Keyboard custom drivers can define this in their own files, it should only
 * be here if shared between boards.
 */

#if defined(RGB_MATRIX_IS31FL3218)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3218_init,
    .flush         = is31fl3218_update_pwm_buffers,
    .set_color     = is31fl3218_set_color,
    .set_color_all = is31fl3218_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3236)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3236_init_drivers,
    .flush         = is31fl3236_flush,
    .set_color     = is31fl3236_set_color,
    .set_color_all = is31fl3236_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3729)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3729_init_drivers,
    .flush         = is31fl3729_flush,
    .set_color     = is31fl3729_set_color,
    .set_color_all = is31fl3729_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3731)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3731_init_drivers,
    .flush         = is31fl3731_flush,
    .set_color     = is31fl3731_set_color,
    .set_color_all = is31fl3731_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3733)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3733_init_drivers,
    .flush         = is31fl3733_flush,
    .set_color     = is31fl3733_set_color,
    .set_color_all = is31fl3733_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3736)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3736_init_drivers,
    .flush         = is31fl3736_flush,
    .set_color     = is31fl3736_set_color,
    .set_color_all = is31fl3736_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3737)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3737_init_drivers,
    .flush         = is31fl3737_flush,
    .set_color     = is31fl3737_set_color,
    .set_color_all = is31fl3737_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3741)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3741_init_drivers,
    .flush         = is31fl3741_flush,
    .set_color     = is31fl3741_set_color,
    .set_color_all = is31fl3741_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3742A)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3742a_init_drivers,
    .flush         = is31fl3742a_flush,
    .set_color     = is31fl3742a_set_color,
    .set_color_all = is31fl3742a_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3743A)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3743a_init_drivers,
    .flush         = is31fl3743a_flush,
    .set_color     = is31fl3743a_set_color,
    .set_color_all = is31fl3743a_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3745)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3745_init_drivers,
    .flush         = is31fl3745_flush,
    .set_color     = is31fl3745_set_color,
    .set_color_all = is31fl3745_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3746A)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3746a_init_drivers,
    .flush         = is31fl3746a_flush,
    .set_color     = is31fl3746a_set_color,
    .set_color_all = is31fl3746a_set_color_all,
};

#elif defined(RGB_MATRIX_IS31FL3763)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3763_init_drivers,
    .flush         = is31fl3763_flush,
    .set_color     = is31fl3763_set_color,
    .set_color_all = is31fl3763_set_color_all,
};

#elif defined(RGB_MATRIX_SNLED27351)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = snled27351_init_drivers,
    .flush         = snled27351_flush,
    .set_color     = snled27351_set_color,
    .set_color_all = snled27351_set_color_all,
};

#elif defined(RGB_MATRIX_AW20216S)
const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = aw20216s_init_drivers,
    .flush         = aw20216s_flush,
    .set_color     = aw20216s_set_color,
    .set_color_all = aw20216s_set_color_all,
};

#elif defined(RGB_MATRIX_WS2812)
#    if defined(RGBLIGHT_WS2812)
#        pragma message "Cannot use RGBLIGHT and RGB Matrix using WS2812 at the same time."
#        pragma message "You need to use a custom driver, or re-implement the WS2812 driver to use a different configuration."
#    endif

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = ws2812_init,
    .flush         = ws2812_flush,
    .set_color     = ws2812_set_color,
    .set_color_all = ws2812_set_color_all,
};

#endif
