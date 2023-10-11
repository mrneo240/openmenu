/*
 * File: gd_list.h
 * Project: ini_parse
 * File Created: Wednesday, 19th May 2021 5:33:33 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

struct gd_item;
int list_read(const char *filename);
int list_read_default(void);
void list_destroy(void);
void list_print_slots(void);
void list_print_temp(void);
void list_print(const struct gd_item **list);

/* simple sorting methods */
const struct gd_item **list_get(void);
const struct gd_item **list_get_sort_name(void);
const struct gd_item **list_get_sort_region(void);
const struct gd_item **list_get_sort_genre(void);
const struct gd_item **list_get_sort_default(void);
/* complex filtering and sorting */
const struct gd_item **list_get_genre(int genre);
const struct gd_item **list_get_genre_sort(int genre, int sort);
/* Grab multidisc games */
void list_set_multidisc(const char *product_id);
const struct gd_item **list_get_multidisc(void);

int list_length(void);
int list_multidisc_length(void);
const struct gd_item *list_item_get(int idx);
