/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note that the function prototypes are taken from the NRFx HAL
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <nrf.h>

bool nrf_dma_accessible_check(void const * p_reg, void const * p_object)
{
  (void)p_reg;
  (void)p_object;
  return true; /* No restrictions in simulation */
}

#if defined(ADDRESS_BUS_Msk) /* Only for 54 or newer devices */
uint8_t nrf_address_bus_get(uint32_t addr, size_t size)
{
    return nrf_hack_get_peripheral_bus(addr);
}
#endif
