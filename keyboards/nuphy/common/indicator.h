#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUPHY_LINK_USB 4

static inline bool nuphy_should_render_wireless_profile_indicator(uint8_t link_mode, uint8_t blink_count) {
    return blink_count > 0 && link_mode != NUPHY_LINK_USB;
}
