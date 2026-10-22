// Układ ekranów 480x320 (używany przez ui.cpp i scenariusz symulatora).
// Kolumny gałek i soft-keys leżą nad 3 enkoderami (od lewej E1, E2, E3).
#pragma once

// --- pasek górny ----------------------------------------------------------
#define TOP_Y0        6
#define TOP_Y1        34
#define PRE_X0        6
#define PRE_X1        186
#define IN_X0         192
#define IN_X1         316
#define VOL_X0        322
#define VOL_X1        398
#define TUN_X0        404
#define TUN_X1        436
#define SET_X0        442
#define SET_X1        474

// --- kafle efektów ------------------------------------------------------
#define TILE_Y0       42
#define TILE_Y1       106
#define TILE_W        152
#define TILE_GAP      8
#define TILE_X0(i)    (6 + (i) * (TILE_W + TILE_GAP))

// --- panel efektu w fokusie: 3 suwaki (enkodery 1..3) + grafika ---------------
#define FX_X0         6
#define FX_X1         474
#define FX_Y0         116
#define FX_Y1         270
#define FX_TITLE_Y1   (FX_Y0 + 60)          // dotknięcie nazwy = zmiana efektu
#define BAR_X0        136
#define BAR_X1        320
#define BAR_CY(p)     (FX_Y0 + 82 + (p) * 26)   // środek paska p
#define VAL_X1        364
#define BIG_ICON_X0   (FX_X1 - 16 - 80)
#define BIG_ICON_Y0   (FX_Y0 + (FX_Y1 - FX_Y0 - 96) / 2)

// --- soft-keys: co robi wciśnięcie enkodera ----------------------------------
#define SK_Y0         276
#define SK_Y1         314
#define SK_W          150
#define SK_X0(i)      (6 + (i) * 158)

// --- podekrany ------------------------------------------------------------
#define BACK_X0       384
#define BACK_X1       474

#define MN_X0         6
#define MN_X1         474
#define MN_H          46
#define MN_Y(i)       (50 + (i) * 54)

#define PICK_COLS     3
#define PICK_W        152
#define PICK_H        42
#define PICK_X0(k)    (6 + ((k) % PICK_COLS) * (PICK_W + 8))
#define PICK_Y(k)     (46 + ((k) / PICK_COLS) * (PICK_H + 4))

#define PR_W          230
#define PR_H          48
#define PR_X0(i)      (6 + ((i) % 2) * (PR_W + 8))
#define PR_Y(i)       (46 + ((i) / 2) * (PR_H + 6))

#define ST_X0         6
#define ST_X1         474
#define ST_ROW_H      48
#define ST_Y(r)       (46 + (r) * (ST_ROW_H + 8))
#define ST_CTL_X0     170
#define ST_BAR_X1     392
#define FS_X0         170
#define FS_MID        319
#define FS_X1         468
