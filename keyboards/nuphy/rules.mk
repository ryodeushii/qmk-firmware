# common rules for nuphy keyboards
SRC += common/features/socd_cleaner.c \
	   common/wireless/rf_driver.c  \
	   common/wireless/rf_queue.c  \
	   common/wireless/rf.c  \
	   common/config/config.c  \
	   common/config/via.c  \
	   common/core/debounce.c  \
	   common/wireless/links.c  \
	   common/system/timer.c  \
	   common/power/sleep.c  \
	   common/core/keyboard.c  \
	   common/core/keys.c


LTO_ENABLE = no
