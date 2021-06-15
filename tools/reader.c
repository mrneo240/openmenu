/*
 * File: reader.c
 * Project: dat_builder
 * File Created: Tuesday, 8th June 2021 2:47:35 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#ifdef COSMO
#include "cosmo/cosmopolitan.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "../inc/dat_format.h"
#include "../inc/uthash.h"

/* Called:
./datread input.dat

Dumps all info about the container
*/

#define NUM_ARGS (1)
int main(int argc, char **argv) {
  if (argc < NUM_ARGS + 1 /*binary itself*/) {
    printf("Incorrect usage!\n\t./datread input.dat\n");
    return 1;
  }

  /* Basic Usage */
  dat_file input_bin;
  DAT_init(&input_bin);
  DAT_load_parse(&input_bin, argv[1]);

  /* Dump info and files */
  DAT_dump(&input_bin);

  /* Hashmap test */
  printf("\nSearching known:\n");
  const char *search = "T40502N";
  printf("Found %s at %X\n", search, DAT_get_offset_by_ID(&input_bin, search));

  printf("\nSearching missing:\n");
  const char *missing = "MISSING";
  printf("Found %s at %X\n", missing, DAT_get_offset_by_ID(&input_bin, missing));
}
