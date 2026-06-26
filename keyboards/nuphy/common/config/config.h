#pragma once

#include <stdint.h>
#include "color.h"
#include "config_size.h"

#define USB_MODE 0
#define THREE_MODE 1

#ifndef WORK_MODE
#    define WORK_MODE THREE_MODE
#endif
#ifndef DEFAULT_SLEEP_TOGGLE
#    define DEFAULT_SLEEP_TOGGLE true
#endif

#ifndef DEFAULT_USB_SLEEP_TOGGLE
#    define DEFAULT_USB_SLEEP_TOGGLE false
#endif

#ifndef DEFAULT_DEEP_SLEEP_TOGGLE
#    define DEFAULT_DEEP_SLEEP_TOGGLE true
#endif

#ifndef DEFAULT_POWER_ON_ANIMATION
#    ifdef DEFAULT_TOGGLE_POWER_ON_ANIMATION
#        define DEFAULT_POWER_ON_ANIMATION DEFAULT_TOGGLE_POWER_ON_ANIMATION
#    else
#        define DEFAULT_POWER_ON_ANIMATION true
#    endif
#endif

#ifndef DEFAULT_SLEEP_TIMEOUT
#    define DEFAULT_SLEEP_TIMEOUT 15
#endif

#ifndef PRESS_DEBOUNCE
#    define PRESS_DEBOUNCE (DEBOUNCE)
#endif

#ifndef RELEASE_DEBOUNCE
#    define RELEASE_DEBOUNCE (DEBOUNCE)
#endif

#ifndef DEFAULT_CAPS_INDICATOR_TYPE
#    define DEFAULT_CAPS_INDICATOR_TYPE CAPS_INDICATOR_SIDE
#endif

#ifndef DEFAULT_SIDE_MODE
#    ifdef DEFAULT_EFFECT_MODE
#        define DEFAULT_SIDE_MODE DEFAULT_EFFECT_MODE
#    else
#        define DEFAULT_SIDE_MODE EFFECT_WAVE
#    endif
#endif

#ifndef DEFAULT_SIDE_BRIGHTNESS
#    ifdef DEFAULT_EFFECT_BRIGHTNESS
#        define DEFAULT_SIDE_BRIGHTNESS DEFAULT_EFFECT_BRIGHTNESS
#    else
#        define DEFAULT_SIDE_BRIGHTNESS 4
#    endif
#endif

#ifndef DEFAULT_SIDE_SPEED
#    ifdef DEFAULT_EFFECT_SPEED
#        define DEFAULT_SIDE_SPEED DEFAULT_EFFECT_SPEED
#    else
#        define DEFAULT_SIDE_SPEED 2
#    endif
#endif

#ifndef DEFAULT_SIDE_RGB
#    ifdef DEFAULT_EFFECT_RGB
#        define DEFAULT_SIDE_RGB DEFAULT_EFFECT_RGB
#    else
#        define DEFAULT_SIDE_RGB 1
#    endif
#endif

#ifndef DEFAULT_SIDE_COLOR
#    ifdef DEFAULT_EFFECT_COLOR
#        define DEFAULT_SIDE_COLOR DEFAULT_EFFECT_COLOR
#    else
#        define DEFAULT_SIDE_COLOR 6 // FIXME: what is this value?
#    endif
#endif

#ifndef DEFAULT_AMBIENT_MODE
#    define DEFAULT_AMBIENT_MODE EFFECT_WAVE
#endif

#ifndef DEFAULT_AMBIENT_BRIGHTNESS
#    define DEFAULT_AMBIENT_BRIGHTNESS 4
#endif

#ifndef DEFAULT_AMBIENT_SPEED
#    define DEFAULT_AMBIENT_SPEED 2
#endif

#ifndef DEFAULT_AMBIENT_RGB
#    define DEFAULT_AMBIENT_RGB 1
#endif

#ifndef DEFAULT_AMBIENT_COLOR
#    define DEFAULT_AMBIENT_COLOR 6 //  FIXME: what? is this value?
#endif

#ifndef DEFAULT_RGB_MATRIX_BRIGHTNESS
#    define DEFAULT_RGB_MATRIX_BRIGHTNESS (RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2)
#endif

#ifndef DEFAULT_STATIC_SATURATION
#    define DEFAULT_STATIC_SATURATION UINT8_MAX
#endif

#ifndef DEFAULT_SIDE_STATIC_HUE
#    ifdef RGB_DEFAULT_COLOR
#        define DEFAULT_SIDE_STATIC_HUE RGB_DEFAULT_COLOR
#    else
#        define DEFAULT_SIDE_STATIC_HUE 0
#    endif
#endif

#ifndef DEFAULT_SIDE_STATIC_SAT
#    define DEFAULT_SIDE_STATIC_SAT DEFAULT_STATIC_SATURATION
#endif

#ifndef DEFAULT_AMBIENT_STATIC_HUE
#    define DEFAULT_AMBIENT_STATIC_HUE DEFAULT_SIDE_STATIC_HUE
#endif

#ifndef DEFAULT_AMBIENT_STATIC_SAT
#    define DEFAULT_AMBIENT_STATIC_SAT DEFAULT_STATIC_SATURATION
#endif

#ifndef NUPHY_CONFIG_INIT_MAGIC
#    define NUPHY_CONFIG_INIT_MAGIC 0x46
#endif

#ifndef NUPHY_CONFIG_LEGACY_MAGIC
#    define NUPHY_CONFIG_LEGACY_MAGIC 0x45
#endif

typedef struct {
    uint8_t sleep_toggle : 1;
    uint8_t usb_sleep_toggle : 1;
    uint8_t deep_sleep_toggle : 1;
    uint8_t debounce_press_ms;
    uint8_t debounce_release_ms;
    uint8_t sleep_timeout;
    uint8_t caps_indicator_type;
    uint8_t power_on_animation : 1;
} common_config_t;

typedef struct {
    uint8_t toggle_custom_keys_highlight : 1;
    uint8_t detect_numlock_state : 1;
    uint8_t battery_indicator_numeric : 1;
    uint8_t battery_indicator_brightness;
} custom_config_t;

typedef struct {
    uint8_t hue;
    uint8_t sat;
} static_color_config_t;

typedef struct {
    // (top) side light config
    uint8_t               side_mode;
    uint8_t               side_brightness;
    uint8_t               side_speed;
    uint8_t               side_rgb;
    uint8_t               side_color;
    static_color_config_t side_static_color;

    // ambient light config
    uint8_t               ambient_mode;
    uint8_t               ambient_brightness;
    uint8_t               ambient_speed;
    uint8_t               ambient_rgb;
    uint8_t               ambient_color;
    static_color_config_t ambient_static_color;
} lights_config_t;

typedef struct {
    uint8_t         been_initiated;
    common_config_t common;
    custom_config_t custom;
    lights_config_t lights;
} keyboard_config_t;

_Static_assert(sizeof(keyboard_config_t) == NUPHY_VIA_EEPROM_CUSTOM_CONFIG_SIZE, "keyboard_config_t size changed; update NUPHY_VIA_EEPROM_CUSTOM_CONFIG_SIZE");

keyboard_config_t keyboard_config;

enum via_common_ids {
    id_usb_sleep_toggle    = 0,
    id_debounce_press      = 1,
    id_debounce_release    = 2,
    id_sleep_timeout       = 3,
    id_caps_indicator_type = 4,
    id_sleep_toggle        = 5,
    id_deep_sleep_toggle   = 6,
    id_power_on_animation  = 7,
};

enum via_custom_ids { id_battery_indicator_brightness = 31, id_toggle_custom_keys_highlight = 32, id_toggle_detect_numlock_state = 33, id_battery_indicator_numeric = 34 };

enum via_non_matrix_lights_ids {
    // side light controls
    id_side_light_mode         = 10,
    id_side_light_speed        = 11,
    id_side_light_color        = 12,
    id_side_light_brightness   = 13,
    id_side_light_static_color = 14,
    // ambient light controls
    id_ambient_light_mode         = 20,
    id_ambient_light_speed        = 21,
    id_ambient_light_color        = 22,
    id_ambient_light_brightness   = 23,
    id_ambient_light_static_color = 24,
};

enum work_mode {
    WIRED    = 0,
    WIRELESS = 1,
};

typedef enum {
    CAPS_INDICATOR_SIDE = 0,
    CAPS_INDICATOR_UNDER_KEY,
    CAPS_INDICATOR_BOTH,
    CAPS_INDICATOR_OFF,
} CAPS_LOCK_INDICATION;

enum NonMatrixEffects {
    EFFECT_WAVE = 0,
    EFFECT_MIX,
    EFFECT_STATIC,
    EFFECT_BREATH,
    EFFECT_OFF,
} non_matrix_effects;

void     custom_eeprom_init(void);
void     init_keyboard_config(void);
void     save_config_to_eeprom(void);
void     load_config_from_eeprom(void);
uint32_t get_sleep_timeout(void);
void     user_set_rgb_color(int index, uint8_t red, uint8_t green, uint8_t blue);
uint8_t  get_led_index(uint8_t row, uint8_t col);
uint8_t  two_digit_decimals_led(uint8_t value);
uint8_t  two_digit_ones_led(uint8_t value);
rgb_t    nuphy_static_picker_rgb(uint8_t hue, uint8_t sat, uint8_t brightness);
rgb_t    nuphy_picker_hsv_rgb(uint8_t hue, uint8_t sat, uint8_t value);
void     toggle_usb_sleep(void);
void     toggle_deep_sleep(void);
void     toggle_caps_indication(void);
void     adjust_sleep_timeout(uint8_t dir);

#define SYS_SW_WIN 0xa1
#define SYS_SW_MAC 0xa2
