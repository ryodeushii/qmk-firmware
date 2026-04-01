SRC += side.c

SRC += mcu_pwr.c

UART_DRIVER_REQUIRED = yes

OPT ?= 2
CUSTOM_MATRIX = lite
SRC += matrix.c

SLEEP_LED_ENABLE = no
# RGB driver
COMMON_VPATH += $(DRIVER_PATH)/led/issi
SRC += is31fl3763.c
I2C_DRIVER_REQUIRED = yes
