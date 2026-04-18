/*
Asymetric per-key algorithm. After pressing a key, it immediately changes state,
with no further inputs accepted until DEBOUNCE milliseconds have occurred. After
releasing a key, that state is pushed after no changes occur for DEBOUNCE milliseconds.
*/

#include "quantum/debounce.h"
#include "debounce.h"
#include "../config/config.h"
#include "timer.h"
#include <stdlib.h>

#ifdef PROTOCOL_CHIBIOS
#    if CH_CFG_USE_MEMCORE == FALSE
#        error ChibiOS is configured without a memory allocator. Your keyboard may have set `#define CH_CFG_USE_MEMCORE FALSE`, which is incompatible with this debounce algorithm.
#    endif
#endif

#define ROW_SHIFTER ((matrix_row_t)1)
#define DEBOUNCE_ELAPSED 0

typedef struct {
    bool    pressed : 1;
    uint8_t time : 7;
} debounce_counter_t;

static debounce_counter_t *debounce_counters;
static fast_timer_t        last_time;
static bool                counters_need_update;
static bool                matrix_need_update;
static bool                cooked_changed;

static void update_debounce_counters_and_transfer_if_expired(matrix_row_t raw[], matrix_row_t cooked[], uint8_t elapsed_time);
static void transfer_matrix_values(matrix_row_t raw[], matrix_row_t cooked[]);

void debounce_init(void) {
    const uint8_t rows = matrix_rows();
    const uint8_t cols = matrix_cols();

    debounce_counters = malloc(rows * cols * sizeof(debounce_counter_t));
    int i             = 0;
    for (uint8_t r = 0; r < rows; r++) {
        for (uint8_t c = 0; c < cols; c++) {
            debounce_counters[i++].time = DEBOUNCE_ELAPSED;
        }
    }
}

void debounce_free(void) {
    free(debounce_counters);
    debounce_counters = NULL;
}

bool debounce(matrix_row_t raw[], matrix_row_t cooked[], bool changed) {
    bool updated_last = false;
    cooked_changed    = false;

    if (counters_need_update) {
        fast_timer_t now          = timer_read_fast();
        fast_timer_t elapsed_time = TIMER_DIFF_FAST(now, last_time);

        last_time    = now;
        updated_last = true;
        if (elapsed_time > UINT8_MAX) {
            elapsed_time = UINT8_MAX;
        }

        if (elapsed_time > 0) {
            update_debounce_counters_and_transfer_if_expired(raw, cooked, elapsed_time);
        }
    }

    if (changed || matrix_need_update) {
        if (!updated_last) {
            last_time = timer_read_fast();
        }

        transfer_matrix_values(raw, cooked);
    }

    return cooked_changed;
}

static void update_debounce_counters_and_transfer_if_expired(matrix_row_t raw[], matrix_row_t cooked[], uint8_t elapsed_time) {
    debounce_counter_t *debounce_pointer = debounce_counters;
    const uint8_t       rows             = matrix_rows();
    const uint8_t       cols             = matrix_cols();

    counters_need_update = false;
    matrix_need_update   = false;

    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < cols; col++) {
            matrix_row_t col_mask = (ROW_SHIFTER << col);

            if (debounce_pointer->time != DEBOUNCE_ELAPSED) {
                if (debounce_pointer->time <= elapsed_time) {
                    debounce_pointer->time = DEBOUNCE_ELAPSED;

                    if (debounce_pointer->pressed) {
                        // key-down: eager
                        matrix_need_update = true;
                    } else {
                        // key-up: defer
                        matrix_row_t cooked_next = (cooked[row] & ~col_mask) | (raw[row] & col_mask);
                        cooked_changed |= cooked_next ^ cooked[row];
                        cooked[row] = cooked_next;
                    }
                } else {
                    debounce_pointer->time -= elapsed_time;
                    counters_need_update = true;
                }
            }
            debounce_pointer++;
        }
    }
}

static void transfer_matrix_values(matrix_row_t raw[], matrix_row_t cooked[]) {
    debounce_counter_t *debounce_pointer = debounce_counters;
    const uint8_t       rows             = matrix_rows();
    const uint8_t       cols             = matrix_cols();

    matrix_need_update = false;

    for (uint8_t row = 0; row < rows; row++) {
        matrix_row_t delta = raw[row] ^ cooked[row];
        for (uint8_t col = 0; col < cols; col++) {
            matrix_row_t col_mask = (ROW_SHIFTER << col);

            if (delta & col_mask) {
                if (debounce_pointer->time == DEBOUNCE_ELAPSED) {
                    debounce_pointer->pressed = (raw[row] & col_mask);
                    // debounce_pointer->time    = g_config.debounce_press_ms; // FIXME: original place of debounce
                    counters_need_update = true;

                    if (debounce_pointer->pressed) {
                        // key-down: eager
                        cooked[row] ^= col_mask;
                        cooked_changed         = true;
                        debounce_pointer->time = keyboard_config.common.debounce_press_ms;
                    } else {
                        debounce_pointer->time = keyboard_config.common.debounce_release_ms;
                    }
                }
            } else if (debounce_pointer->time != DEBOUNCE_ELAPSED) {
                if (!debounce_pointer->pressed) {
                    // key-up: defer
                    debounce_pointer->time = DEBOUNCE_ELAPSED;
                }
            }
            debounce_pointer++;
        }
    }
}

void adjust_debounce(uint8_t dir, DEBOUNCE_EVENT debounce_event) {
#if DEBOUNCE > 0
    if (dir) {
        if (debounce_event == DEBOUNCE_PRESS && keyboard_config.common.debounce_press_ms < 99) {
            keyboard_config.common.debounce_press_ms += DEBOUNCE_STEP;
        } else if (debounce_event == DEBOUNCE_RELEASE && keyboard_config.common.debounce_release_ms < 99) {
            keyboard_config.common.debounce_release_ms += DEBOUNCE_STEP;
        }
    } else if (!dir) {
        if (debounce_event == DEBOUNCE_PRESS && keyboard_config.common.debounce_press_ms > 0) {
            keyboard_config.common.debounce_press_ms -= DEBOUNCE_STEP;
        } else if (debounce_event == DEBOUNCE_RELEASE && keyboard_config.common.debounce_release_ms > 0) {
            keyboard_config.common.debounce_release_ms -= DEBOUNCE_STEP;
        }
    }
    save_config_to_eeprom();
#endif
}
