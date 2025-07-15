#include "quantum.h"
#include "keys.h"
#include "config.h"
#include "rf_driver.h"

extern DEV_INFO_STRUCT dev_info;
extern uint16_t      rf_link_show_time;
extern host_driver_t rf_host_driver;

host_driver_t  *m_host_driver  = 0;
uint8_t         host_mode      = 0;
bool            f_send_channel = 0;
/**
 * @brief  switch device link mode.
 * @param mode : link mode
 */
void switch_dev_link(uint8_t mode) {
    if (mode > LINK_USB) return;

    break_all_key();

    dev_info.link_mode = mode;

    dev_info.rf_state = RF_IDLE;
    f_send_channel    = 1;

    if (mode == LINK_USB) {
        host_mode = HOST_USB_TYPE;
        host_set_driver(m_host_driver);
        rf_link_show_time = 0;
    } else {
        host_mode = HOST_RF_TYPE;
        host_set_driver(&rf_host_driver);
    }
}
