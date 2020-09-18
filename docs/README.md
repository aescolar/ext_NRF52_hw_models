**Models of some of the HW present in a NRF52xxx.**<br>
Where relevant differences exist, these models try to align with a NRF52382.

This repo models the nRF peripheral by implementing of the nrfx HAL functions.
Therefore, any code using this these HAL functions should work without needing
futher changes. This includes Zephyr SW.
The user of the HW models must provide the path to a nrfx checkout
of version 2.3.0 or later. This is done by setting the environment
variable NRFX_BASE.
See the [nrfx/hal/README.md](../src/nrfx/hal/README.md) for more details.

This models can be used directly with
[Zephyr's nrf52_bsim target](https://docs.zephyrproject.org/latest/boards/posix/nrf52_bsim/doc/index.html).

The NRF_RADIO peripheral model uses [BabbleSim](http://babblesim.github.io)
for the radio environment simulation.

For more information about the HW models, or how to use them without the
nrf52_bsim please refer to [README_HW_models.md](./README_HW_models.md)

## Faithfullness of these models

These models are accurate enough to allow the current Zephyr to run, and
its BLE stack to function. For more details please see the notes on the source
files for each peripheral model.

These models are based solely on
[the public SOC specifications](https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52832.html?cp=3_1)

They have been developed without any other knowledge and probably contain
inacuracies, and defects.

Several of the peripherals which are not necessary for typical BLE applications
are not modelled. Moreover, in the modelled peripherals, functionality which is
not used in the Zephyr drivers is normally not modelled either.
The [TODO](../TODO.txt) file list some mayor omissions.
It is certainly possible to expand these models to include the rest.
