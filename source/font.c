#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tonc_memmap.h>
#include <tonc_memdef.h>
#include <tonc_bios.h>
#include <tonc_oam.h>
#include "btl_font_bin.h"
#include "macros.h"
#include "pdata.h"
#include "font.h"
#include "fast_set.h"
#include "renderer.h"

const uint8_t *GLYPH_W = btl_font_bin + 0;
const uint16_t *GLYPH_POS = (const uint16_t *)(btl_font_bin + 96);
#define ROW_W ((btl_font_bin_size - 288) / 8)
#define ROW_POS(i) (btl_font_bin + 288 + ROW_W*(i))
const uint8_t * const GLYPH_ROWS[] = {ROW_POS(0), ROW_POS(1), ROW_POS(2), ROW_POS(3), ROW_POS(4), ROW_POS(5), ROW_POS(6), ROW_POS(7)};
#define WAIT_HBLANK() do {} while (!(REG_DISPSTAT & DSTAT_IN_HBL))

uint16_t text_width(const char *text)
{
  const char *p = text;
  uint16_t w = 0;
  while (*p != '\0') {
    uint8_t g = *p - ' ';
    w += GLYPH_W[g];
    p++;
  }
  return w;
}

//uint8_t dbuffer[1024];

void draw_text(uint8_t draw_x, uint8_t draw_y, const char *text)
{
  //uint8_t prow = 255;
  for (int y = 0; y < 8; y++) {
    const char *p = text;
    uint8_t shift = (draw_x & 3) << 2;
    volatile uint16_t *o = OBJ_CHR(draw_x >> 3, (draw_y + y) >> 3) + (((draw_y + y) & 7) << 1) + ((draw_x & 7) >> 2);
    //volatile uint16_t *o = (uint16_t *)(dbuffer + ((uint16_t)draw_x >> 3 << 5)) + (((draw_y + y) & 7) << 1) + ((draw_x & 7) >> 2);
    uint16_t odata = *o;
    
    while (*p != '\0') {
      uint8_t g = *p - ' ';
      const uint8_t *gp = GLYPH_ROWS[y] + GLYPH_POS[g];
      for (uint8_t x = 0; x < GLYPH_W[g]; x++) {
        if (*gp != 0) {
          odata = (odata & ~(0x000F << shift)) | ((uint16_t)*gp << shift);
        }
        shift = (shift + 4) & 15;
        if (shift == 0) {
          *o = odata;
          if ((uint32_t)o & 2) {
            o += 15;
          } else {
            o++;
          }
          odata = *o;
        }
        gp++;
      }
      p++;
    }
    *o = odata;
  }
  //WAIT_HBLANK();
  //if (prow != 255) fast_copy(OBJ_CHR(0, prow), dbuffer, 1024);
}
