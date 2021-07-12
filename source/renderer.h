#pragma once
#include <stdint.h>

struct poly_t {
  uint16_t id, tex;
};

struct rclist_bin {
  signed char x, y, z;
  unsigned char t, p, c, flip_u, flip_v;
  int pos;
};

struct rc_bin {
  unsigned char x, y, c, z;
  //signed short u, v, du, dv;
  unsigned int uv, duv;
} __attribute__((packed));

struct lvl_tile {
  unsigned char texs[6]; // n e s w u d
  unsigned char flags;
  unsigned char special;
};

static const int SCR_W = 240;
static const int SCR_H = 160;
static const int TEX_W = 64;
static const int TEX_H = 64;
static const int TEX_SIZE = TEX_W * TEX_H;

static const int DIR_N = 0;
static const int DIR_E = 1;
static const int DIR_S = 2;
static const int DIR_W = 3;
static const int DIR_U = 4;
static const int DIR_D = 5;

static const int PLAT_CEIL = 0;
static const int PLAT_FRONT = 1;
static const int PLAT_SIDE = 2;
static const int PLAT_SPRITE = 3;

static const int PERSP_FWD = 0;
static const int PERSP_STEP1 = 1;
static const int PERSP_STEP2 = 2;
static const int PERSP_ROT1 = 3;
static const int PERSP_ROT2 = 4;
static const int PERSP_STRAFE1 = 5;
static const int PERSP_STRAFE2 = 6;
static const int PERSP_UP1 = 7;
static const int PERSP_UP2 = 8;
static const int PERSP_MAX = 9;

static const int POLYLIST_SIZE = 2048;

extern uint8_t COLSPACE;

__attribute__((section(".iwram"), long_call, target("arm")))
uint32_t render_begin(const struct lvl_tile *, const void *, int, int, int, int, int);

__attribute__((section(".iwram"), long_call, target("arm")))
uint32_t render_draw(const struct lvl_tile *lvltiles, volatile uint16_t *scr, uint32_t px, uint32_t py, uint32_t pz, uint32_t persp, uint32_t pd, uint32_t polylist_n, uint32_t lim);

void load_palette(const uint8_t *palette_bin, const uint8_t *colmap_bin);
void init_textures(const uint8_t *flats, const uint8_t *walls, const uint8_t *twalls);

void update_colspace();
