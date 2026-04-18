#pragma once

#include "../wireless.h"

#include "rf_protocol.h"
#include "rf_queue.h"

extern host_driver_t rf_host_driver;

void clear_report_buffer_and_queue(void);
