#include "quantum.h"

typedef enum { DEBOUNCE_PRESS = 0, DEBOUNCE_RELEASE } DEBOUNCE_EVENT;

void adjust_debounce(uint8_t dir, DEBOUNCE_EVENT debounce_event);

void break_all_key(void);
