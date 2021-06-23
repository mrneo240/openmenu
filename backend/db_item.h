/*
 * File: db_item.h
 * Project: backend
 * File Created: Wednesday, 16th June 2021 5:44:34 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once
typedef struct db_item {
  char num_players;
  char vmu_blocks;
  char accessories;     /* @Todo: describe how to use this */
  char network;         /* 0, none. 1, modem. 2, bba. 3, both */
  unsigned short genre; /* Genres Described in metapacker.c */
  char padding1;        /*Expansion */
  char padding2;        /*Expansion */
  /*char description[364];*/
  char description[376];
} db_item;