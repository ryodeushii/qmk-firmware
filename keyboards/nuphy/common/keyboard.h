#include "quantum.h"

void housekeeping_task_nuphy(void);
void keyboard_post_init_nuphy(void);
bool rgb_matrix_indicators_nuphy(void);

__attribute((weak)) void set_indicator_on_side(uint8_t r, uint8_t g, uint8_t b) {}
__attribute((weak)) void side_rgb_refresh(void) {} // use stub to avoid linking error for non gem80/air75v2 boards
