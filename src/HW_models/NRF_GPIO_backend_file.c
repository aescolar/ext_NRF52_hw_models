/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File-input backend for GPIO pins.
 * Drives GPIO inputs from stimuli files in CSV format.
 *
 * Check docs/GPIO.md for more info.
 */

#include <stdbool.h>
#include <string.h>
#include "bs_cmd_line.h"
#include "bs_dynargs.h"
#include "bs_oswrap.h"
#include "bs_tracing.h"
#include "bs_types.h"
#include "HW_utils.h"
#include "NHW_config.h"
#include "NRF_GPIO.h"
#include "nsi_hw_scheduler.h"
#include "nsi_tasks.h"
#include "nsi_hws_models_if.h"

#define MAXLINESIZE 2048

static bs_time_t Timer_GPIO_input = TIME_NEVER;
static char *gpio_in_file_path; /* Possible file for input stimuli */

/* GPIO input status */
static struct {
  FILE *input_file_ptr;
  /* Next event port.pin & level: */
  unsigned int port;
  unsigned int pin;
  bool level; /* true: high; false: low*/
} gpio_input_file_st;

/*
 * Process next (valid) line in the input stimuly file, and program
 * the next input update event
 * (or close down the file if it ended or is corrupted)
 */
static void nrf_gpio_input_process_next_time(char *buf)
{
  bs_time_t time;
  unsigned int port;
  unsigned int pin;
  unsigned int level;
  int n;

  n = sscanf(buf, "%"SCNtime",%u,%u,%u", &time, &port, &pin, &level);
  if (n > 0 && n < 4) {
    bs_trace_warning_time_line("File %s seems corrupted. Ignoring rest of file. "
        "Expected \""
        "<uin64_t time>,<uint port>,<uint pin>,<uint level>\". "
        "Line was:%s\n",
        gpio_in_file_path, buf);
  }
  if (n < 4) { /* End of file, or corrupted => we are done */
    fclose(gpio_input_file_st.input_file_ptr);
    gpio_input_file_st.input_file_ptr = NULL;
    Timer_GPIO_input = TIME_NEVER;
  } else {
    if (time < nsi_hws_get_time()) {
      bs_trace_error_time_line("%s: GPIO input file went back in time(%s)\n",
          __func__, buf);
    }
    if (port >= NHW_GPIO_TOTAL_INST) {
      bs_trace_error_time_line("%s: GPIO input file attempted to access not "
          "existing GPIO port (%u>=%u) (%s)\n",
          __func__, port, NHW_GPIO_TOTAL_INST, buf);
    }
    unsigned int max_pins = nrf_gpio_get_number_pins_in_port(port);
    if (pin >= max_pins) {
      bs_trace_error_time_line("%s: GPIO input file attempted to access not "
          "existing GPIO pin in port %i (%u>=%u) (%s)\n",
          __func__, port, pin, max_pins, buf);
    }
    if (level != 0 && level != 1) {
      bs_trace_error_time_line("%s: level can only be 0 (for low) or 1 (for high)"
          "(%u) (%s)\n",
          __func__, level, buf);
    }
    gpio_input_file_st.level = level;
    gpio_input_file_st.pin = pin;
    gpio_input_file_st.port = port;
    Timer_GPIO_input = time;
  }

  nsi_hws_find_next_event();
}

/*
 * Initialize GPIO input from file, and queue next input event change
 */
static void nrf_gpio_init_input_file(void)
{
  gpio_input_file_st.input_file_ptr = NULL;

  if (gpio_in_file_path == NULL) {
    return;
  }

  char line_buf[MAXLINESIZE];
  int read;

  gpio_input_file_st.input_file_ptr = bs_fopen(gpio_in_file_path, "r");

  read = hwu_readline(line_buf, MAXLINESIZE, gpio_input_file_st.input_file_ptr);
  if (strncmp(line_buf,"time",4) == 0) { /* Let's skip a possible csv header line */
    read = hwu_readline(line_buf, MAXLINESIZE, gpio_input_file_st.input_file_ptr);
  }
  if (read == 0) {
    bs_trace_warning_line("%s: Input file %s seems empty\n",
        __func__, gpio_in_file_path);
  }

  nrf_gpio_input_process_next_time(line_buf);
}

NSI_TASK(nrf_gpio_init_input_file, HW_INIT, 101); /* After nrf_gpio_init() */

/*
 * Event timer handler for the GPIO input
 */
static void nrf_gpio_input_event_triggered(void)
{
  char line_buf[MAXLINESIZE];

  nrf_gpio_eval_input(gpio_input_file_st.port, gpio_input_file_st.pin,
      gpio_input_file_st.level);

  (void)hwu_readline(line_buf, MAXLINESIZE, gpio_input_file_st.input_file_ptr);

  nrf_gpio_input_process_next_time(line_buf);
}

NSI_HW_EVENT(Timer_GPIO_input, nrf_gpio_input_event_triggered, 50);

static void nrf_gpio_input_file_backend_cleaup(void)
{
  if (gpio_input_file_st.input_file_ptr != NULL) {
    fclose(gpio_input_file_st.input_file_ptr);
    gpio_input_file_st.input_file_ptr = NULL;
  }
}

NSI_TASK(nrf_gpio_input_file_backend_cleaup, ON_EXIT_PRE, 100);

static void nrf_gpio_input_file_register_cmd_args(void) {

  static bs_args_struct_t args_struct_toadd[] = {
      {
          .option="gpio_in_file",
          .name="path",
          .type='s',
          .dest=(void *)&gpio_in_file_path,
          .descript="Optional path to a file containing GPIOs inputs activity",
      },
      ARG_TABLE_ENDMARKER
  };

  bs_add_extra_dynargs(args_struct_toadd);
}

NSI_TASK(nrf_gpio_input_file_register_cmd_args, PRE_BOOT_1, 100);
