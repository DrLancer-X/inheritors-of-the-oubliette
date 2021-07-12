#pragma once
#include <stdint.h>

#include "lvl0_palette_bin.h"
#include "lvl0_colmap_bin.h"
#include "lvl0_flats_bin.h"
#include "lvl0_walls_bin.h"
#include "lvl0_twalls_bin.h"
#include "lvl0_backdrop_bin.h"
#include "lvl0_bin.h"
#include "lvl0_vis_bin.h"

#include "lvl1_palette_bin.h"
#include "lvl1_colmap_bin.h"
#include "lvl1_flats_bin.h"
#include "lvl1_walls_bin.h"
#include "lvl1_twalls_bin.h"
#include "lvl1_backdrop_bin.h"
#include "lvl1_bin.h"
#include "lvl1_vis_bin.h"


struct mapinfo {
  const uint8_t *map;
  const uint8_t *vis;
  const uint8_t *backdrop;
  const uint8_t *palette;
  const uint8_t *colmap;
  const uint8_t *flats;
  const uint8_t *walls;
  const uint8_t *twalls;
};

const struct mapinfo MAPS[] = {
  {lvl0_bin, lvl0_vis_bin, lvl0_backdrop_bin, lvl0_palette_bin, lvl0_colmap_bin, lvl0_flats_bin, lvl0_walls_bin, lvl0_twalls_bin},
  {lvl1_bin, lvl1_vis_bin, lvl1_backdrop_bin, lvl1_palette_bin, lvl1_colmap_bin, lvl1_flats_bin, lvl1_walls_bin, lvl1_twalls_bin}
};
