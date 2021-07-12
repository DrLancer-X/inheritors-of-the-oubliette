#pragma once
#include <stdint.h>
#include "renderer.h"

void gameplay();
void loadMap();
int getDirX(uint8_t dir);
int getDirY(uint8_t dir);
int getDirZ(uint8_t dir);
uint8_t getTexDirFrom(uint16_t x, uint16_t y, uint16_t z, uint8_t dir);
int turnLeft();
int turnRight();
int after_boss();

struct lvldata {
  struct lvl_tile *map;
  const void *vis;
  const uint8_t *backdrop;
};
