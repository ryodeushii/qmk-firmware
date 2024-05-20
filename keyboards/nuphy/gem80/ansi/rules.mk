SRC += user_kb.c
SRC += rf.c
SRC += side.c side_driver.c side_logo.c

VPATH += keyboards/nuphy/common
SRC += mcu_pwr.c sleep.c debounce.c rf_driver.c  rf_queue.c

UART_DRIVER_REQUIRED = yes
