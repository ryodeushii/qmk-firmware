#pragma once

#include "quantum/quantum.h"

#define RF_LINK_SHOW_TIME 300

#define LINK_TIMEOUT (100 * 120)
#define LINK_TIMEOUT_ALT (100 * 5)
#define POWER_DOWN_DELAY (24)

#define LINK_RF_24 0
#define LINK_BT_1 1
#define LINK_BT_2 2
#define LINK_BT_3 3
#define LINK_USB 4

#define RF_IDLE 0
#define RF_PAIRING 1
#define RF_LINKING 2
#define RF_CONNECT 3
#define RF_DISCONNECT 4
#define RF_SLEEP 5
#define RF_SNIF 6
#define RF_INVAILD 0XFE
#define RF_ERR_STATE 0XFF
#define RF_WAKE 0XA5

#define RF_LONG_PRESS_DELAY 30
#define DEV_RESET_PRESS_DELAY 30
#define RGB_TEST_PRESS_DELAY 30

typedef struct {
    uint8_t link_mode;
    uint8_t rf_channel;
    uint8_t ble_channel;
    uint8_t rf_state;
    uint8_t rf_charge;
    uint8_t rf_led;
    uint8_t rf_battery;
    uint8_t sys_sw_state;
} DEV_INFO_STRUCT;

extern DEV_INFO_STRUCT dev_info;

void nuphy_rf_sync_status(void);
void nuphy_rf_transport_init(void);
void nuphy_rf_device_init(void);
void nuphy_rf_repeat_reports(void);
void nuphy_rf_poll(void);
void nuphy_rf_enter_dfu(void);
void nuphy_rf_set_link(uint8_t mode, uint8_t ack_count, uint8_t delay_ms);
bool nuphy_rf_request_new_adv(uint8_t retries);
void nuphy_rf_factory_reset(void);
void nuphy_rf_prepare_wakeup(void);
void nuphy_rf_request_sleep(bool keep_connection);
