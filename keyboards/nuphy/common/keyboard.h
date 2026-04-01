#include "quantum.h"

void housekeeping_task_nuphy(void);
void keyboard_post_init_nuphy(void);
bool rgb_matrix_indicators_nuphy(void);

void set_indicator_on_side(uint8_t r, uint8_t g, uint8_t b);
void set_side_rgb(uint8_t r, uint8_t g, uint8_t b);
void side_rgb_refresh(void);
