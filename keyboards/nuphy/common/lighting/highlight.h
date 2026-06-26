#pragma once

#include <stdbool.h>
#include <stdint.h>

void nuphy_highlight_custom_keys(uint8_t led_min, uint8_t led_max);
bool nuphy_highlight_custom_key_extra(uint8_t index, uint16_t keycode);
