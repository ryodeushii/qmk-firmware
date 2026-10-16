#pragma once

#include "quantum/quantum.h"

#define LINK_RF_24 0
#define LINK_BT_1 1
#define LINK_BT_2 2
#define LINK_BT_3 3
#define LINK_USB 4

#define NUPHY_WIRELESS_STATE_PAIRING      1
#define NUPHY_WIRELESS_STATE_CONNECTED    3
#define NUPHY_WIRELESS_STATE_DISCONNECTED 4

typedef struct {
    uint8_t link_mode;
    uint8_t rf_state;
    uint8_t rf_charge;
    uint8_t rf_led;
    uint8_t rf_battery;
    uint8_t sys_sw_state;
} nuphy_wireless_status_t;

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

void nuphy_wireless_get_status(nuphy_wireless_status_t *status);
void nuphy_wireless_set_active_link(uint8_t mode);
void nuphy_wireless_set_system_mode(uint8_t mode);
