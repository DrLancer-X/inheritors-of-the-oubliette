// TODO: I've got the transparency data being copied to trcol but have nothing using it yet

#include "newrcd_bin.h"
#include "polylist_bin.h"
#include <stdint.h>
#include <string.h>

#include "renderer.h"
#include "fast_set.h"

#include <tonc_memmap.h>
#include <tonc_bios.h>

uint8_t COLSPACE = 3;

//static const struct rclist_bin *rclist = (const struct rclist_bin *)rcdlist_bin;
//static const struct rc_bin *rcbins = (const struct rc_bin *)rcd_bin;
//static const uint16_t *drawlist = ((const uint16_t *)drawlist_bin) + 13 * 2;
//static const uint32_t *drawlist_idx = (const uint32_t *)drawlist_bin;
static const uint16_t *polyinfo = (const uint16_t *)polylist_bin;

static const uint16_t *newrcd = ((const uint16_t *)newrcd_bin);
static const uint32_t *newrcd_lookup = ((const uint32_t *)newrcd_bin);

static const uint8_t *textures_flats_bin;
static const uint8_t *textures_walls_bin;
static const uint8_t *textures_twalls_bin;

uint8_t col_lut[256 * 32];

uint16_t poly_list[3500];
uint16_t *curr_poly_list;

const uint8_t *palette_data;

void load_palette(const uint8_t *palette_bin, const uint8_t *colmap_bin)
{
  palette_data = palette_bin;
  fast_copy((volatile void *)MEM_PAL_BG, palette_data + (COLSPACE << 9), 512);
  if (colmap_bin)
    fast_copy(col_lut, colmap_bin, 8192);
}
void init_textures(const uint8_t *flats, const uint8_t *walls, const uint8_t *twalls)
{
  textures_flats_bin = flats;
  textures_walls_bin = walls;
  textures_twalls_bin = twalls;
}
void update_colspace()
{
  fast_copy((volatile void *)MEM_PAL_BG, palette_data + (COLSPACE << 9), 512);
}

__attribute__((section(".iwram"), long_call, target("arm")))
uint32_t render_begin(const struct lvl_tile *lvltiles, const void *lvl_vis, int px, int py, int pz, int pp, int pd)
{
  const uint32_t *lvl_vis_lut = lvl_vis;
  const uint16_t *lvl_vis_data = lvl_vis;
  uint32_t lut_pos = lvl_vis_lut[pz * 256 + py * 16 + px];
  //uint32_t lut_size = lvl_vis_lut[pz * 256 + py * 16 + px + 1] - lut_pos;
  
  lvl_vis_data = &lvl_vis_data[lut_pos];
  LZ77UnCompWram(lvl_vis_data, poly_list);
  uint32_t poly_list_pos = poly_list[pd * PERSP_MAX + pp];
  uint32_t poly_list_size = poly_list[pd * PERSP_MAX + pp + 1] - poly_list_pos;
  curr_poly_list = &poly_list[poly_list_pos];
  
  return poly_list_size;
}

__attribute__((section(".iwram"), long_call, target("arm")))
static inline uint8_t remap(uint8_t v, uint8_t z)
{
  //return v;
  return col_lut[((z >> 3) << 8) | v];
}

__attribute__((section(".iwram"), long_call, target("arm")))
static inline uint32_t min(uint32_t x, uint32_t y)
{
  //return x < y ? x : y;
  return y ^ ((x ^ y) & -(x < y));
}

static uint32_t poly_i;
static uint16_t poly_id, polydat;
static uint32_t tex_dir, wx, wz, wy, vert, idx, texid, count, i, pos;
static uint16_t xy, compressed_uv, pack, xytarg;
static uint32_t uv, duv, uv2, duv2;
static uint32_t jump_to;
static uint8_t texcol[128];
static uint8_t trcol[128];
static uint8_t z, z2, c, j, texel, texel2;
static const uint8_t *tex;


__attribute__((section(".iwram"), long_call, target("arm")))
static inline void trcolu(char x, volatile uint16_t *scr)
{
  //const uint8_t PURPLE = 184;
  //const uint8_t GREEN = 71;
  //const uint8_t CYAN = 112;
  //const uint8_t YELLOW = 232;
  //scr[0] = 184; // Purple
  //scr[1] = 71; // Green
  //scr[2] = 112; // Cyan
  //scr[3] = 232; // Yellow
  const uint8_t *trcol_1 = &trcol[0];
  const uint8_t *trcol_2 = &trcol[64];
  const uint8_t *texcol_1 = &texcol[0];
  const uint8_t *texcol_2 = &texcol[64];
  while (xy < xytarg) {
    uint32_t uvtarg;
    uint8_t trc1, trc2;
    switch (x) {
      case 'L':
        trc1 = trcol_1[(uv & 0x7E00) >> 9];
        uv &= 0x7FFF7FFF;
        uvtarg = (uv & 0xFE00FE00) + (((uint32_t)(trc1 & 0x7F) + 1) << 9);
        uvtarg |= 0xFFFF0000;
        if (trc1 & 0x80) goto trcolu_i;
        trcolu_l:
        while ((uv & 0xFFFF) < (uvtarg & 0xFFFF) && (uv & 0xFFFF0000) < (uvtarg & 0xFFFF0000) && xy < xytarg) {
          scr[xy] = (scr[xy] & 0xFF00) | remap(texcol_1[(uv & 0x7E00) >> 9], z);
          //scr[xy] = (scr[xy] & 0xFF00) | PURPLE;
          uv = (uv + duv) & 0xFFFFFFFF;
          xy += SCR_W >> 1;
        }
      break;
      case 'R':
        trc2 = trcol_2[(uv & 0x7E000000) >> (9+16)];
        uv &= 0x7FFF7FFF;
        uvtarg = (uv & 0xFE00FE00) + (((uint32_t)(trc2 & 0x7F) + 1) << (9+16));
        uvtarg |= 0x0000FFFF;
        if (trc2 & 0x80) goto trcolu_i;
        trcolu_r:
        while ((uv & 0xFFFF) < (uvtarg & 0xFFFF) && (uv & 0xFFFF0000) < (uvtarg & 0xFFFF0000) && xy < xytarg) {
          scr[xy] = (scr[xy] & 0x00FF) | ((uint16_t)(remap(texcol_2[(uv & 0x7E000000) >> (9+16)], z2)) << 8);
          //scr[xy] = (scr[xy] & 0x00FF) | ((uint16_t)(YELLOW) << 8);
          uv = (uv + duv) & 0xFFFFFFFF;
          xy += SCR_W >> 1;
        }
      break;
      case 'D':
        trc1 = trcol_1[(uv & 0x7E00) >> 9];
        trc2 = trcol_2[(uv & 0x7E000000) >> (9+16)];
        uv &= 0x7FFF7FFF;
        
        uvtarg = (uv & 0xFE00FE00) + (((trc1 & 0x7F) + 1) << (9)) + (((trc2 & 0x7F) + 1) << (9+16));
        
        if (trc1 & 0x80) {
          if (trc2 & 0x80) goto trcolu_i;
          else goto trcolu_r;
        } else if (trc2 & 0x80) goto trcolu_l;

        while ((uv & 0xFFFF) < (uvtarg & 0xFFFF) && (uv >> 16) < (uvtarg >> 16) && xy < xytarg) {
          scr[xy] = remap(texcol_1[(uv & 0x7E00) >> 9], z) | (remap(texcol_2[(uv & 0x7E000000) >> (9 + 16)], z2) << 8);

          xy += SCR_W >> 1;
          uv = (uv + duv) & 0xFFFFFFFF;
        }
      break;
      case 'I':
        uvtarg = 0;
        trcolu_i:
        while ((uv & 0xFFFF) < (uvtarg & 0xFFFF) && (uv >> 16) < (uvtarg >> 16) && xy < xytarg) {
          uv = (uv + duv) & 0xFFFFFFFF;
          xy += SCR_W >> 1;
        }
      break;
    }
  }
}

__attribute__((section(".iwram"), long_call, target("arm")))
uint32_t render_draw(const struct lvl_tile *lvltiles, volatile uint16_t *scr, uint32_t px, uint32_t py, uint32_t pz, uint32_t persp, uint32_t pd, uint32_t polylist_n, uint32_t lim)
{
/*
  uint32_t poly_i;
  uint16_t poly_id, polydat;
  uint32_t tex_dir, wx, wz, wy, vert, idx, texid, count, i, pos;
  uint16_t xy, compressed_uv, pack, xytarg;
  uint32_t uv, duv, uv2, duv2;
  uint32_t jump_to;
  uint8_t texcol[128];
  uint8_t trcol[128];
  uint8_t z, z2, c, j, texel, texel2, trxel, trxel2;
  const uint8_t *tex;
*/
  for (poly_i = 0; poly_i < polylist_n; poly_i++) {
    poly_id = curr_poly_list[poly_i];
    polydat = polyinfo[polyinfo[persp] + poly_id];

    tex_dir = ((polydat & 0xE000) >> 13);
    wx = ((polydat & 0x1E00) >> 9);
    wz = ((polydat & 0x01E0) >> 5);
    wy = ((polydat & 0x001E) >> 1);
    vert = ((polydat & 0x0001) >> 0);
    
    if (tex_dir < 4) {
      tex_dir = (tex_dir + pd) & 3;
    }
    
    idx = ((pz - wz) & 0xF) << 8;
    switch (pd) {
      case DIR_N:
      default:
        idx |= ((px + wx) & 0xF);
        idx |= ((py - wy) & 0xF) << 4;
        break;
      case DIR_S:
        idx |= ((px - wx) & 0xF);
        idx |= ((py + wy) & 0xF) << 4;
        break;
      case DIR_E:
        idx |= ((px + wy) & 0xF);
        idx |= ((py + wx) & 0xF) << 4;
        break;
      case DIR_W:
        idx |= ((px - wy) & 0xF);
        idx |= ((py - wx) & 0xF) << 4;
        break;
    }
    texid = lvltiles[idx].texs[tex_dir];

    pos = newrcd_lookup[persp];
    pos += newrcd[pos + poly_id] * 4;
    
    //scr[((uint32_t)texid) % 19200] = 10000;
    //scr[((uint32_t)poly_id) % 19200] = 10000;
    //continue;

    if (!vert) { // rows
      if (tex_dir == DIR_D)
        tex = &textures_flats_bin[TEX_W * TEX_H * ((texid - 1) * 4 + pd)];
      else
        tex = &textures_flats_bin[TEX_W * TEX_H * ((texid - 1) * 4 + 3 - pd)];
      count = newrcd[pos++]; // lp_count
      for (i = 0; i < count; i++) {
        xy = newrcd[pos++];
        compressed_uv = newrcd[pos++];
        pack = newrcd[pos++];
        z = pack & 0xFF;
        scr[xy] = (scr[xy] & 0xFF00) | remap(tex[compressed_uv], z);
      }

      count = newrcd[pos++]; // rp_count
      for (i = 0; i < count; i++) {
        xy = newrcd[pos++];
        compressed_uv = newrcd[pos++];
        pack = newrcd[pos++];
        z = pack & 0xFF;
        scr[xy] = (scr[xy] & 0x00FF) | (remap(tex[compressed_uv], z) << 8);
      }

      count = newrcd[pos++]; // dp_count
      for (i = 0; i < count; i++) {
        xy = newrcd[pos++];
        pack = newrcd[pos++];
        z = pack & 0xFF;
        c = (pack & 0xFF00) >> 8;
        uv = newrcd[pos++];
        uv |= (newrcd[pos++]) << 16;
        duv = newrcd[pos++];
        duv |= (newrcd[pos++]) << 16;
        
        for (j = 0; j < c; j++) {
          texel = tex[((uv & 0x7E000000) >> 25) * TEX_W + ((uv & 0x7E00) >> 9)];
          uv = (uv + duv) & 0x7FFF7FFF;
          texel2 = tex[((uv & 0x7E000000) >> 25) * TEX_W + ((uv & 0x7E00) >> 9)];
          uv = (uv + duv) & 0x7FFF7FFF;
          scr[xy] = remap(texel, z) | (remap(texel2, z) << 8);
          xy++;
        }
      }

    } else { // cols
      if (texid < 128) {
        wall_solid:
        tex = &textures_walls_bin[TEX_W * TEX_H * (texid - 1)];
        count = newrcd[pos++]; // lp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z = pack & 0xFF;
          c = (pack & 0xFF00) >> 8;
          uv = newrcd[pos++];
          uv |= (newrcd[pos++]) << 16;
          duv = newrcd[pos++];
          fast_copy_texcol(texcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W]);
          for (j = 0; j < c; j++) {
            scr[xy] = (scr[xy] & 0xFF00) | remap(texcol[(uv & 0x7E00) >> 9], z);
            uv += duv;
            xy += SCR_W >> 1;
          }
        }

        count = newrcd[pos++]; // rp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z = pack & 0xFF;
          c = (pack & 0xFF00) >> 8;
          uv = newrcd[pos++];
          uv |= (newrcd[pos++]) << 16;
          duv = newrcd[pos++];
          
          fast_copy_texcol(texcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W]);
          for (j = 0; j < c; j++) {
            scr[xy] = (scr[xy] & 0x00FF) | ((uint16_t)(remap(texcol[(uv & 0x7E00) >> 9], z)) << 8);
            uv += duv;
            xy += SCR_W >> 1;
          }
        }

        count = newrcd[pos++]; // dp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z = pack & 0xFF;
          z2 = (pack & 0xFF00) >> 8;
          uv = newrcd[pos++];
          uv |= (newrcd[pos++]) << 16;
          uv2 = newrcd[pos++];
          uv2 |= (newrcd[pos++]) << 16;
          
          duv = newrcd[pos++];
          duv2 = newrcd[pos++];
          
          c = newrcd[pos++];
          fast_copy_texcols(texcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W], &tex[((uv2 & 0x7E000000) >> 25) * TEX_W]);
          const uint8_t *texcol_1 = &texcol[0];
          const uint8_t *texcol_2 = &texcol[64];
          
          for (j = 0; j < c; j++) {
            texel = texcol_1[(uv & 0x7E00) >> 9];
            texel2 = texcol_2[(uv2 & 0x7E00) >> 9];
            
            scr[xy] = remap(texel, z) | (remap(texel2, z2) << 8);
            uv = (uv + duv) & 0x7FFF7FFF;
            uv2 = (uv2 + duv2) & 0x7FFF7FFF;
            xy += SCR_W >> 1;
          }
        }
      } else { // Transparent wall textures
        if (texid == 128) continue;
        tex = &textures_twalls_bin[TEX_SIZE * ((texid - 129) << 1)];
        if (tex[TEX_SIZE] == 127) goto wall_solid;
        count = newrcd[pos++]; // lp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z = pack & 0xFF;
          xytarg = ((pack & 0xFF00) >> 8) * (SCR_W >> 1) + xy;
          uv = newrcd[pos++];
          uv |= (newrcd[pos++]) << 16;
          duv = newrcd[pos++];
          jump_to = 0;
          fast_copy_texcol(texcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W]);
          fast_copy_texcol(trcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W + TEX_SIZE]);
          uv = (uv & 0x7FFF);
          trcolu('L', scr);
        }

        count = newrcd[pos++]; // rp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z2 = pack & 0xFF;
          xytarg = ((pack & 0xFF00) >> 8) * (SCR_W >> 1) + xy;
          uv2 = newrcd[pos++];
          uv2 |= (newrcd[pos++]) << 16;
          duv2 = newrcd[pos++];
          jump_to = 0;
          fast_copy_texcol(&texcol[64], &tex[((uv2 & 0x7E000000) >> 25) * TEX_W]);
          fast_copy_texcol(&trcol[64], &tex[((uv2 & 0x7E000000) >> 25) * TEX_W + TEX_SIZE]);
          uv = ((uv2 & 0x7FFF) << 16);
          duv = (duv2 << 16);
          trcolu('R', scr);
        }

        count = newrcd[pos++]; // dp_count
        for (i = 0; i < count; i++) {
          xy = newrcd[pos++];
          pack = newrcd[pos++];
          z = pack & 0xFF;
          z2 = (pack & 0xFF00) >> 8;
          uv = newrcd[pos++];
          uv |= (newrcd[pos++]) << 16;
          uv2 = newrcd[pos++];
          uv2 |= (newrcd[pos++]) << 16;
          
          duv = newrcd[pos++];
          duv2 = newrcd[pos++];
          
          xytarg = newrcd[pos++] * (SCR_W >> 1) + xy;
          
          fast_copy_texcols(texcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W], &tex[((uv2 & 0x7E000000) >> 25) * TEX_W]);
          fast_copy_texcols(trcol, &tex[((uv & 0x7E000000) >> 25) * TEX_W + TEX_SIZE], &tex[((uv2 & 0x7E000000) >> 25) * TEX_W + TEX_SIZE]);
          uv = (uv & 0x7FFF) | ((uv2 & 0x7FFF) << 16);
          duv = duv | (duv2 << 16);
          trcolu('D', scr);
        }
      }
    }
  }
  return 0;
}
