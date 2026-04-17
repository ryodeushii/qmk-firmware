SRC += side.c

SRC += mcu_pwr.c

SRC += rgb_matrix_driver.c is31fl3763.c

UART_DRIVER_REQUIRED = yes

OPT ?= 2
CUSTOM_MATRIX = lite
SRC += matrix.c

SLEEP_LED_ENABLE = no
I2C_DRIVER_REQUIRED = yes
