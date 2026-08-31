/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bs_oswrap.h"
#include "bs_tracing.h"

void hwu_reverse_byte_order(const unsigned char *in_data, unsigned char *out_data, size_t len)
{
    unsigned int i;
    in_data += len - 1;
    for (i = 0; i < len; i++)
    {
        *out_data++ = *in_data--;
    }
}

/*
 * Read a line from a file into a buffer (s), while
 * skipping duplicate spaces (unless they are quoted), comments (#...),
 * and empty lines.
 * The string will be null terminated (even if nothing is copied in).
 *
 * Return: The number of characters copied into s (apart from the termination 0 byte)
 */
int hwu_readline(char *s, int size, FILE *stream)
{
  int c = 0, i = 0;
  bool was_a_space = true;
  bool in_a_string = false;

  while ((i == 0) && (c != EOF)) {
    while ((i < size - 1) && ((c = getc(stream)) != EOF) && c != '\n') {
      if (isspace(c) && (!in_a_string)) {
        if (was_a_space) {
          continue;
        }
        was_a_space = true;
      } else {
        was_a_space = false;
      }
      if (c == '"') {
        in_a_string = !in_a_string;
      }
      if (c == '#') {
        bs_skipline(stream);
        break;
      }
      s[i++] = c;
    }
  }
  s[i] = 0;

  if (i >= size - 1) {
    bs_trace_warning_line("Truncated line while reading from file after %i chars\n",
              size - 1);
  }
  return i;
}
