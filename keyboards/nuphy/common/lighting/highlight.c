#include "highlight.h"

#include "color.h"
#include "common/config.h"
#include "common/config/config.h"
#include "common/keys.h"
#include "common/wireless.h"
#include "config.h"
#include "quantum.h"
#include "rgb_matrix.h"

__attribute__((weak)) bool nuphy_highlight_custom_key_extra(uint8_t index, uint16_t keycode) {
    return false;
}

static bool is_media_keycode(uint16_t keycode) {
    switch (keycode) {
        case KC_BRID:
        case KC_BRIU:
        case KC_BRMD:
        case KC_BRMU:
        case KC_MPRV:
        case KC_MPLY:
        case KC_MNXT:
        case KC_MUTE:
        case KC_VOLD:
        case KC_VOLU:
        case MAC_TASK:
        case MAC_SEARCH:
        case MAC_VOICE:
        case MAC_CONSOLE:
        case MAC_DND:
        case MAC_GLOBE:
            return true;
        default:
            return false;
    }
}

void nuphy_highlight_custom_keys(uint8_t led_min, uint8_t led_max) {
    if (!CUSTOM_KEYS_HIGHLIGHT_ENABLED || !keyboard_config.custom.toggle_custom_keys_highlight) {
        return;
    }

    uint8_t layer = get_highest_layer(layer_state);
    if (layer == 0 || layer == 2) {
        return;
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];

            if (index == NO_LED || index < led_min || index > led_max) {
                continue;
            }

            uint16_t keycode = keymap_key_to_keycode(layer, (keypos_t){col, row});

            if (keycode >= AMBIENT_VAI && keycode <= AMBIENT_SPD) {
                rgb_matrix_set_color(index, RGB_WHITE);
            } else if (keycode >= SIDE_VAI && keycode <= SIDE_SPD) {
                rgb_matrix_set_color(index, RGB_YELLOW);
            } else if (keycode >= DEBOUNCE_PRESS_INC && keycode <= DEBOUNCE_PRESS_SHOW) {
                rgb_matrix_set_color(index, 0, 255, 0);
            } else if (keycode >= DEBOUNCE_RELEASE_INC && keycode <= DEBOUNCE_RELEASE_SHOW) {
                rgb_matrix_set_color(index, 255, 0, 0);
            } else if (keycode == DEV_RESET) {
                rgb_matrix_set_color(index, RGB_RED);
            } else if (keycode == SLEEP_MODE || keycode == TOG_USB_SLP || keycode == TOG_DEEP_SLEEP || (keycode >= SLEEP_TIMEOUT_INC && keycode <= SLEEP_TIMEOUT_SHOW)) {
                rgb_matrix_set_color(index, RGB_CYAN);
            } else if (keycode >= LNK_USB && keycode <= LNK_BLE3) {
                if (dev_info.link_mode != LINK_USB) {
                    rgb_matrix_set_color(index, RGB_BLUE);
                }
            } else if (is_media_keycode(keycode)) {
                rgb_matrix_set_color(index, 225, 65, 140);
            } else if (nuphy_highlight_custom_key_extra(index, keycode)) {
                continue;
            }
        }
    }
}
