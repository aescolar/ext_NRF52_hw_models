/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File-input backend for GPIO pins.
 * Drives GPIO inputs from stimuli files in CSV format.
 * Multiple instances can be configured, each reading from a separate file.
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

static bs_time_t Timer_gbfile = TIME_NEVER;
static unsigned int gbf_next_i;

/* GPIO input status */
struct gif_st {
  char *path;
  FILE *fp;
  bs_time_t timer;
  /* Next event port.pin & level: */
  unsigned int port;
  unsigned int pin;
  bool level; /* true: high; false: low*/
};

static struct gif_st *gbfile_p;
static int n_gbfile;

static void nhw_gbfile_init(void *n);

static const struct nrf_gpio_backend_if nhw_gbfile_callbacks = {
  .init = nhw_gbfile_init,
};

static void nhw_gpio_filebackend_instantiate(char *path) {
  int n = n_gbfile;
  size_t path_len = strlen(path) + 1;

  n_gbfile++;
  gbfile_p = bs_realloc(gbfile_p, n_gbfile*sizeof(struct gif_st));

  gbfile_p[n].fp = NULL;
  gbfile_p[n].path = bs_malloc(path_len);
  memcpy(gbfile_p[n].path, path, path_len);
  gbfile_p[n].timer = TIME_NEVER;

  nrf_gpio_backend_register(&nhw_gbfile_callbacks, (void *)(uintptr_t)n);
  bs_trace_info(5, "Instantiated GPIO input file backend[%i] with path %s\n",
                n, path);
}

static void nhw_gbfile_find_next(void) {
  Timer_gbfile = gbfile_p[0].timer;
  gbf_next_i = 0;
  for (int i = 1; i < n_gbfile; i++) {
    if (gbfile_p[i].timer < Timer_gbfile) {
      Timer_gbfile = gbfile_p[i].timer;
      gbf_next_i = i;
    }
  }
  nsi_hws_find_next_event();
}

int nhw_gpio_filebackend_process_config(char *path) {
  nhw_gpio_filebackend_instantiate(path);
  return 0;
}

/*
 * Process next (valid) line in the input stimuly file, and program
 * the next input update event
 * (or close down the file if it ended or is corrupted)
 */
static void nhw_gbfile_process_next_time(struct gif_st *st, char *buf)
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
        st->path, buf);
  }
  if (n < 4) { /* End of file, or corrupted => we are done */
    fclose(st->fp);
    st->fp = NULL;
    st->timer = TIME_NEVER;
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
    st->level = level;
    st->pin = pin;
    st->port = port;
    st->timer = time;
  }

  nhw_gbfile_find_next();
}

/*
 * Initialize GPIO input from file, and queue next input event change
 */
static void nhw_gbfile_init(void *n)
{
  struct gif_st *st = &gbfile_p[(uintptr_t)n];
  char line_buf[MAXLINESIZE];
  int read;

  st->fp = bs_fopen(st->path, "r");

  read = hwu_readline(line_buf, MAXLINESIZE, st->fp);
  if (strncmp(line_buf,"time",4) == 0) { /* Let's skip a possible csv header line */
    read = hwu_readline(line_buf, MAXLINESIZE, st->fp);
  }
  if (read == 0) {
    bs_trace_warning_line("%s: Input file %s seems empty\n",
        __func__, st->path);
  }

  nhw_gbfile_process_next_time(st, line_buf);
}

/*
 * Event timer handler for the GPIO input
 */
static void nhw_gbfile_input_event_triggered(void)
{
  struct gif_st *st = (struct gif_st *)&gbfile_p[gbf_next_i];
  char line_buf[MAXLINESIZE];

  nrf_gpio_eval_input(st->port, st->pin, st->level);

  (void)hwu_readline(line_buf, MAXLINESIZE, st->fp);

  nhw_gbfile_process_next_time(st, line_buf);
}

NSI_HW_EVENT(Timer_gbfile, nhw_gbfile_input_event_triggered, 50);

static void nhw_gbfile_cleaup(void)
{
  if (gbfile_p == NULL) {
    return;
  }
  for (int i = 0; i < n_gbfile; i++) {
    if (gbfile_p[i].fp) {
      fclose(gbfile_p[i].fp);
      gbfile_p[i].fp = NULL;
    }
    if (gbfile_p[i].path) {
      free(gbfile_p[i].path);
      gbfile_p[i].path = NULL;
    }
  }
  free(gbfile_p);
  gbfile_p = NULL;
}

NSI_TASK(nhw_gbfile_cleaup, ON_EXIT_PRE, 100);

static char *gpio_in_file_path; /* cmd line path being parsed */

static void nhw_gbfile_cmd_found(char *argv, int offset) {
  (void)argv;
  (void)offset;
  nhw_gpio_filebackend_instantiate(gpio_in_file_path);
}

static void nhw_gbfile_register_cmd_args(void) {

  static bs_args_struct_t args_struct_toadd[] = {
      {
          .option="gpio_in_file",
          .name="path",
          .type='s',
          .dest=(void *)&gpio_in_file_path,
          .call_when_found=nhw_gbfile_cmd_found,
          .descript="Optional path to a file containing GPIOs inputs activity",
      },
      ARG_TABLE_ENDMARKER
  };

  bs_add_extra_dynargs(args_struct_toadd);
}

NSI_TASK(nhw_gbfile_register_cmd_args, PRE_BOOT_1, 100);
