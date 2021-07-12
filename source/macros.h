#pragma once
#include <stdint.h>
#include <tonc_memmap.h>
#include <tonc_math.h>

#define OBJ_PAL(n) (((volatile uint16_t *)MEM_PAL_OBJ + ((n) << 4)))
#define OBJ_CHR(x, y) (((volatile uint16_t *)0x6014000 + ((x) << 4) + ((y) << 9)))

#define OBJMEM ((volatile OBJ_ATTR*)MEM_OAM)
#define AFFMEM ((volatile OBJ_AFFINE*)MEM_OAM)

#define OBJSET(i, cx, cy, x, y, p, a0, a1, a2) { \
  OBJMEM[i].attr0 = ATTR0_Y(y) | ATTR0_4BPP | (a0); \
  OBJMEM[i].attr1 = ATTR1_X(x) | (a1); \
  OBJMEM[i].attr2 = ATTR2_ID(512 + ((cy) << 5) + (cx)) | ATTR2_PALBANK(p) | (a2); \
}

#define AFFSET(i, a, b, c, d) { \
  AFFMEM[i].pa = (a); \
  AFFMEM[i].pb = (b); \
  AFFMEM[i].pc = (c); \
  AFFMEM[i].pd = (d); \
}

//#define min(x, y) ((x) < (y) ? (x) : (y))
//#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(x, a, b) (min(max((x), (a)), (b)))
