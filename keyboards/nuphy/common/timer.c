#include "quantum.h"
#include "rf_driver.h"
#include "config.h"

extern host_driver_t *m_host_driver;
extern uint16_t       rf_link_show_time;
uint32_t              no_act_time = 0;
extern uint16_t       rf_linking_time;
extern uint16_t       rgb_led_last_act;
extern uint16_t       side_led_last_act;
/**
 * @brief  timer process.
 */
void timer_pro(void) {
    static uint32_t interval_timer = 0;
    static bool     f_first        = true;

    if (f_first) {
        f_first        = false;
        interval_timer = timer_read32();
        m_host_driver  = host_get_driver();
    }

    // Count all elapsed timer steps so slower housekeeping loops do not stretch timeouts.
    uint32_t elapsed = timer_elapsed32(interval_timer);
    if (elapsed < TIMER_STEP) return;

    uint32_t steps = elapsed / TIMER_STEP;
    interval_timer += steps * TIMER_STEP;

    if (rf_link_show_time < RF_LINK_SHOW_TIME) {
        uint32_t remaining = RF_LINK_SHOW_TIME - rf_link_show_time;
        rf_link_show_time += (steps < remaining) ? steps : remaining;
    }

    if (no_act_time <= 0xffffffffU - steps) {
        no_act_time += steps;
    } else {
        no_act_time = 0xffffffffU;
    }
#if (WORK_MODE == THREE_MODE)
    if (rf_linking_time < 0xffff) {
        uint32_t remaining = 0xffff - rf_linking_time;
        rf_linking_time += (steps < remaining) ? steps : remaining;
    }
#endif
    if (rgb_led_last_act < 0xffff) {
        uint32_t remaining = 0xffff - rgb_led_last_act;
        rgb_led_last_act += (steps < remaining) ? steps : remaining;
    }

    if (side_led_last_act < 0xffff) {
        uint32_t remaining = 0xffff - side_led_last_act;
        side_led_last_act += (steps < remaining) ? steps : remaining;
    }
}
