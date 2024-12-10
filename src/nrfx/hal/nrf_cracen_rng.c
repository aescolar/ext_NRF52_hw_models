/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note that the function prototypes are taken from the NRFx HAL
 */

#include <stdint.h>
#include "hal/nrf_cracen_rng.h"
#include "bs_tracing.h"
#include "NHW_CRACEN_RNG.h"

NRF_STATIC_INLINE void nrf_cracen_rng_control_set(NRF_CRACENCORE_RNGCONTROL_Type * p_reg,
                                                  uint32_t value)
{
    p_reg->CONTROL = value;
    nhw_CRACEN_RNG_regw_sideeffects_CONTROL();
}

NRF_STATIC_INLINE uint32_t nrf_cracen_rng_fifo_read(NRF_CRACENCORE_RNGCONTROL_Type const * p_reg)
{
    nhw_CRACEN_RNG_regr_sideeffects_FIFO();
    return p_reg->FIFO[0];
}
