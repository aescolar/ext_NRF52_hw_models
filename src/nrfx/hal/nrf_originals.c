
#define NRF_STATIC_INLINE __attribute__((weak))
#define NRF_DONT_DECLARE_ONLY
#include "hal/nrf_aar.h"
#include "hal/nrf_ccm.h"
#include "hal/nrf_radio.h"
#undef NRF_STATIC_INLINE
#undef NRF_DONT_DECLARE_ONLY
