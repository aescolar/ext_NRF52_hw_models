/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note that the function prototypes are taken from the NRFx HAL
 */

#include "hal/nrf_power.h"

NRF_STATIC_INLINE void nrf_power_event_clear(NRF_POWER_Type * p_reg, nrf_power_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
}

NRF_STATIC_INLINE bool nrf_power_event_check(NRF_POWER_Type const * p_reg, nrf_power_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_power_int_enable_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->INTENSET;
}

NRF_STATIC_INLINE bool nrf_power_event_get_and_clear(NRF_POWER_Type *  p_reg,
                                                     nrf_power_event_t event)
{
    bool ret = nrf_power_event_check(p_reg, event);
    if (ret)
    {
        nrf_power_event_clear(p_reg, event);
    }
    return ret;
}
