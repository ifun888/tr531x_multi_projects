/**
 * Copyright (c) Triductor. 2022-2023. All rights reserved.
 *
 * Description: Provides hal sio \n
 *
 * History: \n
 * 2022-08-01, Create file. \n
 */
#include "common_def.h"
#include "hal_sio.h"

uintptr_t g_hal_sio_regs[I2S_MAX_NUMBER] = { 0 };

errcode_t hal_sio_regs_init(sio_bus_t bus)
{
    if (sio_porting_base_addr_get(bus) == 0) {
        return ERRCODE_SIO_REG_ADDR_INVALID;
    }
    g_hal_sio_regs[bus] = sio_porting_base_addr_get(bus);
    return ERRCODE_SUCC;
}

void hal_sio_regs_deinit(sio_bus_t bus)
{
    g_hal_sio_regs[bus] = 0;
}
