// Copyright 2026 Ryodeushii
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rgb_matrix_drivers.h"
#include "is31fl3763.h"

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = is31fl3763_init_drivers,
    .flush         = is31fl3763_flush,
    .set_color     = is31fl3763_set_color,
    .set_color_all = is31fl3763_set_color_all,
};
