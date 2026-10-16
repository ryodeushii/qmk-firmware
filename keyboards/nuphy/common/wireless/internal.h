#pragma once

#include "../wireless.h"

#define RF_LINK_SHOW_TIME 300

#define LINK_TIMEOUT (100 * 120)
#define LINK_TIMEOUT_ALT (100 * 5)
#define POWER_DOWN_DELAY (24)

#define RF_IDLE 0
#define RF_PAIRING NUPHY_WIRELESS_STATE_PAIRING
#define RF_LINKING 2
#define RF_CONNECT NUPHY_WIRELESS_STATE_CONNECTED
#define RF_DISCONNECT NUPHY_WIRELESS_STATE_DISCONNECTED
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
extern uint16_t        rf_link_show_time;
extern uint16_t        rf_linking_time;

void nuphy_wireless_switch_to_saved_link(void);
