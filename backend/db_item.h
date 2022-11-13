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

typedef enum FLAGS_GENRE {
  GENRE_NONE = (0 << 0),        // 0
  GENRE_ACTION = (1 << 0),      // 1
  GENRE_RACING = (1 << 1),      // 2
  GENRE_SIMULATION = (1 << 2),  // 4
  GENRE_SPORTS = (1 << 3),      // 8
  GENRE_LIGHTGUN = (1 << 4),    // 16
  GENRE_FIGHTING = (1 << 5),    // 32
  GENRE_SHOOTER = (1 << 6),     // 64
  GENRE_SURVIVAL = (1 << 7),    // 128
  GENRE_ADVENTURE = (1 << 8),   // 256
  GENRE_PLATFORMER = (1 << 9),  // 512
  GENRE_RPG = (1 << 10),        // 1024
  GENRE_SHMUP = (1 << 11),      // 2048
  GENRE_STRATEGY = (1 << 12),   // 4096
  GENRE_PUZZLE = (1 << 13),     // 8192
  GENRE_ARCADE = (1 << 14),     // 16384
  GENRE_MUSIC = (1 << 15),      // 32768
} FLAGS_GENRE;

typedef enum FLAGS_ACCESORIES {
  ACCESORIES_NONE = (0 << 0),          // 0
  ACCESORIES_JUMP_PACK = (1 << 0),     // 1
  ACCESORIES_KEYBOARD = (1 << 1),      // 2
  ACCESORIES_VGA = (1 << 2),           // 4
  ACCESORIES_MOUSE = (1 << 3),         // 8
  ACCESORIES_MARACAS = (1 << 4),       // 16
  ACCESORIES_RACING_WHEEL = (1 << 5),  // 32
  ACCESORIES_MICROPHONE = (1 << 6),    // 64
  ACCESORIES_ARCADE_STICK = (1 << 7),  // 128
  ACCESORIES_LIGHTGUN = (1 << 8),      // 256
  ACCESORIES_BBA = (1 << 9),           // 512
  ACCESORIES_FISHING_ROD = (1 << 10),  // 1024
  ACCESORIES_ASCII_PAD = (1 << 11),    // 2048
  ACCESORIES_DREAMEYE = (1 << 12),     // 4096
  ACCESORIES_MODEM = (1 << 13),        // 8192
  ACCESORIES_UNUSED = (1 << 14),       // 16384
  ACCESORIES_UNUSED2 = (1 << 15),      // 32768
} FLAGS_ACCESORIES;

typedef struct db_item {
  unsigned char num_players;
  unsigned char vmu_blocks;
  unsigned char padding0;    /*Currently Unused, for expansion */
  unsigned char network;     /* 0, none. 1, modem. 2, bba. 3, both */
  unsigned short genre;      /* Genres Described above */
  unsigned short accessories;/* Accessories Described above */
  char description[376];
} db_item;
