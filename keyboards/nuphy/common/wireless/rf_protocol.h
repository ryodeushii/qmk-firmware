#pragma once

#include "quantum/quantum.h"

#define UART_HEAD 0x5A
#define FUNC_VALID_LEN 32
#define UART_MAX_LEN 64

typedef enum {
    RX_Idle,
    RX_Receiving,
    RX_Done,
    RX_Fail,
    RX_OV_ERR,
    RX_SUM_ERR,
    RX_CMD_ERR,
    RX_DATA_ERR,
    RX_DATA_OV,
    RX_FORMAT_ERR,

    TX_OK = 0XE0,
    TX_DONE,
    TX_BUSY,
    TX_TIMEOUT,
    TX_DATA_ERR,

} TYPE_RX_STATE;

typedef struct {
    uint8_t  RXDState;
    uint8_t  RXDLen;
    uint8_t  RXDOverTime;
    uint8_t  TXDLenBack;
    uint8_t  TXDOffset;
    uint8_t  RXCmd;
    uint32_t TXLastCmdTm;
    uint8_t  TXDBuf[UART_MAX_LEN];
    uint8_t  RXDBuf[UART_MAX_LEN];
} USART_MGR_STRUCT;

#define CMD_POWER_UP 0XF0
#define CMD_SLEEP 0XF1
#define CMD_HAND 0XF2
#define CMD_SNIF 0XF3
#define CMD_24G_SUSPEND 0XF4
#define CMD_IDLE_EXIT 0XFE

#define CMD_RPT_MS 0XE0
#define CMD_RPT_BYTE_KB 0XE1
#define CMD_RPT_BIT_KB 0XE2
#define CMD_RPT_CONSUME 0XE3
#define CMD_RPT_SYS 0XE4

#define CMD_SET_LINK 0XC0
#define CMD_SET_CONFIG 0XC1
#define CMD_GET_CONFIG 0XC2
#define CMD_SET_NAME 0XC3
#define CMD_GET_NAME 0XC4
#define CMD_CLR_DEVICE 0XC5
#define CMD_NEW_ADV 0XC7
#define CMD_RF_STS_SYSC 0XC9
#define CMD_SET_24G_NAME 0XCA
#define CMD_GO_TEST 0XCF
#define CMD_RF_DFU 0XB1
#define CMD_NULL 0X00

#define CMD_WRITE_DATA 0X80
#define CMD_READ_DATA 0X81

void    uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack_cnt, uint8_t delayms);
