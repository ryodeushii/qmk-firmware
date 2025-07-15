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

    // step 10ms
    if (timer_elapsed32(interval_timer) < TIMER_STEP) return;
    interval_timer = timer_read32();

    if (rf_link_show_time < RF_LINK_SHOW_TIME) rf_link_show_time++;

    if (no_act_time < 0xffffffff) no_act_time++;
#if (WORK_MODE == THREE_MODE)
    if (rf_linking_time < 0xffff) rf_linking_time++;
#endif
    if (rgb_led_last_act < 0xffff) rgb_led_last_act++;

    if (side_led_last_act < 0xffff) side_led_last_act++;
}
