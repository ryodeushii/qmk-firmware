/*
Copyright 2023 NuPhy, Persama (@Persama) & jincao1

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Shared implementation body. Include this from a board-local mcu_pwr.c that
// already pulled in the board's config.h, mcu_pwr.h, and related dependencies.

static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

extern DEV_INFO_STRUCT dev_info;

static bool tim6_enabled = false;
static bool sleeping     = false;

static bool rgb_led_on  = 0;
static bool side_led_on = 0;

void rgb_matrix_update_pwm_buffers(void);
void clear_report_buffer_and_queue(void);

void m_deinit_usb_072(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB1PeriphResetCmd(RCC_APB1RSTR_USBRST, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1RSTR_USBRST, DISABLE);
    wait_ms(10);

    RCC_APB1PeriphClockCmd(RCC_APB1ENR_USBEN, DISABLE);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init((GPIO_TypeDef *)GPIOA, &GPIO_InitStructure);
}

#include "hal_usb.h"
#include "usb_main.h"

void SYSCFG_EXTILineConfig(uint8_t EXTI_PortSourceGPIOx, uint8_t EXTI_PinSourcex) {
    uint32_t tmp = 0x00;

    tmp = ((uint32_t)0x0F) << (0x04 * (EXTI_PinSourcex & (uint8_t)0x03));
    SYSCFG->EXTICR[EXTI_PinSourcex >> 0x02] &= ~tmp;
    SYSCFG->EXTICR[EXTI_PinSourcex >> 0x02] |= (((uint32_t)EXTI_PortSourceGPIOx) << (0x04 * (EXTI_PinSourcex & (uint8_t)0x03)));
}

#include "hal_lld.h"

#define EXTI_PortSourceGPIOA ((uint8_t)0x00)
#define EXTI_PortSourceGPIOB ((uint8_t)0x01)
#define EXTI_PortSourceGPIOC ((uint8_t)0x02)
#define EXTI_PortSourceGPIOD ((uint8_t)0x03)

void enter_deep_sleep(void) {
    if (dev_info.rf_state == RF_CONNECT)
        uart_send_cmd(CMD_SET_CONFIG, 5, 5);
    else
        uart_send_cmd(CMD_SLEEP, 5, 5);

    if (tim6_enabled) TIM_Cmd(TIM6, DISABLE);

    for (int i = 0; i < ARRAY_SIZE(col_pins); i++) {
        gpio_set_pin_output(col_pins[i]);
        gpio_write_pin_high(col_pins[i]);
    }

    for (int i = 0; i < ARRAY_SIZE(row_pins); i++) {
        gpio_set_pin_input_low(row_pins[i]);
    }

    SYSCFG_EXTILineConfig(EXTI_PORT_R0, EXTI_PIN_R0);
    SYSCFG_EXTILineConfig(EXTI_PORT_R1, EXTI_PIN_R1);
    SYSCFG_EXTILineConfig(EXTI_PORT_R2, EXTI_PIN_R2);
    SYSCFG_EXTILineConfig(EXTI_PORT_R3, EXTI_PIN_R3);
    SYSCFG_EXTILineConfig(EXTI_PORT_R4, EXTI_PIN_R4);
    SYSCFG_EXTILineConfig(EXTI_PORT_R5, EXTI_PIN_R5);

    EXTI_InitTypeDef m_exti;
    EXTI_StructInit(&m_exti);
    m_exti.EXTI_Line    = 0XFFFF;
    m_exti.EXTI_LineCmd = ENABLE;
    m_exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    m_exti.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&m_exti);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel         = EXTI4_15_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd      = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_1_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_3_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    led_pwr_sleep_handle();

    gpio_set_pin_output(DEVICE_MODE_PIN);
    gpio_write_pin_low(DEVICE_MODE_PIN);

    gpio_set_pin_output(OS_MODE_PIN);
    gpio_write_pin_low(OS_MODE_PIN);

    gpio_set_pin_output(DRIVER_SIDE_DI_PIN);
    gpio_write_pin_low(DRIVER_SIDE_DI_PIN);

    gpio_set_pin_output(NRF_TEST_PIN);
    gpio_write_pin_high(NRF_TEST_PIN);

    gpio_set_pin_input(NRF_WAKEUP_PIN);
    gpio_write_pin_low(NRF_WAKEUP_PIN);

    clear_report_buffer_and_queue();

    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

void exit_deep_sleep(void) {
    extern void matrix_init_custom(void);
    matrix_init_custom();

#if (WORK_MODE == THREE_MODE)
    gpio_set_pin_input_high(DEVICE_MODE_PIN);
#endif
    gpio_set_pin_input_high(OS_MODE_PIN);

#if (WORK_MODE == THREE_MODE)
    gpio_set_pin_output(NRF_WAKEUP_PIN);
    gpio_write_pin_high(NRF_WAKEUP_PIN);
#endif

    led_pwr_wake_handle();

    stm32_clock_init();

    if (tim6_enabled) TIM_Cmd(TIM6, ENABLE);

#if (WORK_MODE == THREE_MODE)
    uart_send_cmd(CMD_HAND, 0, 1);
#endif
    if (dev_info.link_mode == LINK_USB) {
        usb_lld_wakeup_host(&USB_DRIVER);
        restart_usb_driver(&USB_DRIVER);
    }

    dev_info.rf_state = RF_WAKE;
}

void enter_light_sleep(void) {
#if (WORK_MODE == THREE_MODE)
    dev_sts_sync();

    if (dev_info.rf_state == RF_CONNECT)
        uart_send_cmd(CMD_SET_CONFIG, 5, 5);
    else
        uart_send_cmd(CMD_SLEEP, 5, 5);

#endif
    led_pwr_sleep_handle();

#if (WORK_MODE == THREE_MODE)
    clear_report_buffer_and_queue();
#endif
    sleeping = true;
}

void exit_light_sleep(void) {
    sleeping = false;

    led_pwr_wake_handle();
#if (WORK_MODE == THREE_MODE)
    uart_send_cmd(CMD_HAND, 0, 1);
#endif
    if (dev_info.link_mode == LINK_USB) {
        if (USB_DRIVER.state == USB_SUSPENDED) {
            usb_lld_wakeup_host(&USB_DRIVER);
        }
    }

#if (WORK_MODE == THREE_MODE)
    dev_info.rf_state = RF_WAKE;
#endif
}

void led_pwr_sleep_handle(void) {
    if (rgb_led_on) {
        pwr_rgb_led_off();
    }
    if (side_led_on) {
        pwr_side_led_off();
    }
}

void side_rgb_refresh(void);

void led_pwr_wake_handle(void) {
    if (!rgb_led_on) {
        pwr_rgb_led_on();
        rgb_matrix_update_pwm_buffers();
    }
    if (!side_led_on) {
        pwr_side_led_on();
        side_rgb_refresh();
    }
}

void pwr_rgb_led_off(void) {
    if (!rgb_led_on) return;

    gpio_set_pin_output(DC_BOOST_PIN);
    gpio_write_pin_low(DC_BOOST_PIN);
    gpio_set_pin_input(DRIVER_MATRIX_CS_PIN);
    rgb_led_on = 0;
}

void pwr_rgb_led_on(void) {
    if (sleeping || rgb_led_on) return;

    gpio_set_pin_output(DC_BOOST_PIN);
    gpio_write_pin_high(DC_BOOST_PIN);
    gpio_set_pin_output(DRIVER_MATRIX_CS_PIN);
    gpio_write_pin_low(DRIVER_MATRIX_CS_PIN);
    rgb_led_on = 1;
}

void pwr_side_led_off(void) {
    if (!side_led_on) return;

    gpio_set_pin_input(DRIVER_SIDE_CS_PIN);
    side_led_on = 0;
}

void pwr_side_led_on(void) {
    if (sleeping || side_led_on) return;

    gpio_set_pin_output(DRIVER_SIDE_CS_PIN);
    gpio_write_pin_low(DRIVER_SIDE_CS_PIN);
    side_led_on = 1;
}

bool is_rgb_led_on(void) {
    return rgb_led_on;
}

bool is_side_led_on(void) {
    return side_led_on;
}

void EXTI_ClearFlag(uint32_t EXTI_Line) {
    EXTI->PR = EXTI_Line;
}

#define STM32_EXTI_0_1_HANDLER Vector54
OSAL_IRQ_HANDLER(STM32_EXTI_0_1_HANDLER) {
    EXTI->PR = 0xffff;
}

#define STM32_EXTI_2_3_HANDLER Vector58
OSAL_IRQ_HANDLER(STM32_EXTI_2_3_HANDLER) {
    EXTI->PR = 0xffff;
}

#define STM32_EXTI_4_15_HANDLER Vector5C
OSAL_IRQ_HANDLER(STM32_EXTI_4_15_HANDLER) {
    EXTI->PR = 0xffff;
}

void EXTI_DeInit(void) {
    EXTI->IMR  = 0x0F940000;
    EXTI->EMR  = 0x00000000;
    EXTI->RTSR = 0x00000000;
    EXTI->FTSR = 0x00000000;
    EXTI->PR   = 0x006BFFFF;
}

void EXTI_Init(EXTI_InitTypeDef *EXTI_InitStruct) {
    uint32_t tmp = 0;

    tmp = (uint32_t)EXTI_BASE;

    if (EXTI_InitStruct->EXTI_LineCmd != DISABLE) {
        EXTI->IMR &= ~EXTI_InitStruct->EXTI_Line;
        EXTI->EMR &= ~EXTI_InitStruct->EXTI_Line;

        tmp += EXTI_InitStruct->EXTI_Mode;
        *(__IO uint32_t *)tmp |= EXTI_InitStruct->EXTI_Line;

        EXTI->RTSR &= ~EXTI_InitStruct->EXTI_Line;
        EXTI->FTSR &= ~EXTI_InitStruct->EXTI_Line;

        if (EXTI_InitStruct->EXTI_Trigger == EXTI_Trigger_Rising_Falling) {
            EXTI->RTSR |= EXTI_InitStruct->EXTI_Line;
            EXTI->FTSR |= EXTI_InitStruct->EXTI_Line;
        } else {
            tmp = (uint32_t)EXTI_BASE;
            tmp += EXTI_InitStruct->EXTI_Trigger;

            *(__IO uint32_t *)tmp |= EXTI_InitStruct->EXTI_Line;
        }
    } else {
        tmp += EXTI_InitStruct->EXTI_Mode;
        *(__IO uint32_t *)tmp &= ~EXTI_InitStruct->EXTI_Line;
    }
}

void EXTI_StructInit(EXTI_InitTypeDef *EXTI_InitStruct) {
    EXTI_InitStruct->EXTI_Line    = 0;
    EXTI_InitStruct->EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStruct->EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStruct->EXTI_LineCmd = DISABLE;
}

#if (0)
void mcu_timer6_init(void) {
    NVIC_InitTypeDef        NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel         = TIM6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd      = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_TimeBaseStructure.TIM_Period        = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler     = 48 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM6, ENABLE);

    tim6_enabled = true;
}

volatile uint8_t idle_sleep_cnt = 0;
OSAL_IRQ_HANDLER(STM32_TIM6_HANDLER) {
    if (TIM_GetFlagStatus(TIM6, TIM_FLAG_Update) != ST_RESET) {
        TIM_ClearFlag(TIM6, TIM_FLAG_Update);
        idle_sleep_cnt++;
    }
}

void idle_enter_sleep(void) {
    TIM6->CNT      = 0;
    idle_sleep_cnt = 0;
    while (idle_sleep_cnt < 1) {
        PWR_EnterSleepMode(PWR_SLEEPEntry_WFI);
    }
}
#endif
