#pragma once
#include <stdint.h>
// External types

struct btl_enemy {
  uint8_t spriteid;
  uint8_t sptype;
  uint8_t fly;
  uint8_t flip;
  int16_t maxhp, maxmp, str, def, agi, mag;
  int16_t gem_accum;
  uint8_t equiptype;
  uint8_t eqatt[4];
  uint8_t weak[4];
  uint8_t spdata[8];
  char mname[32];
  char sname[32];
  char ename[32];
  char sdesc[128];
};
