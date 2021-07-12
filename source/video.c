#include <stdint.h>
#include <tonc_memmap.h>
#include <tonc_memdef.h>
#include "video.h"
#include "macros.h"
#include "pdata.h"

volatile uint16_t *VIDEO_BUFFER = (volatile uint16_t *)MEM_VRAM_BACK;

void flip_buffer() {
  REG_DISPCNT = REG_DISPCNT ^ DCNT_PAGE;
  if (REG_DISPCNT & DCNT_PAGE) {
    VIDEO_BUFFER = (volatile uint16_t *)MEM_VRAM_FRONT;
  } else {
    VIDEO_BUFFER = (volatile uint16_t *)MEM_VRAM_BACK;
  }
}

void first_buffer() {
  REG_DISPCNT = REG_DISPCNT & ~DCNT_PAGE;
  VIDEO_BUFFER = (volatile uint16_t *)MEM_VRAM_BACK;
}

void paleffect(int pal, int code)
{
	volatile uint16_t *p = OBJ_PAL(pal);

  switch (code) {
    case STATUS_POISON:
      for (int i = 0; i < 16; i++) {
        *p = *p & (31 << 5);
        p++;
      }
    break;
    case STATUS_BADPOISON:
      for (int i = 0; i < 16; i++) {
        *p = (*p & (31 << 5)) | (3 << 5);
        p++;
      }
    break;
    case STATUS_PARA:
      for (int i = 0; i < 16; i++) {
        *p = *p & ((31 << 5) | 31);
        p++;
      }
    break;
    case STATUS_WEAK:
      for (int i = 0; i < 16; i++) {
        *p = *p & (31 << 10);
        p++;
      }
    break;
    case STATUS_BARRIER:
      for (int i = 0; i < 16; i++) {
        *p = *p | (31 << 10);
        p++;
      }
    break;
    case STATUS_REGEN:
      for (int i = 0; i < 16; i++) {
        *p = *p | (31 << 5);
        p++;
      }
    break;
    case STATUS_FAST:
      for (int i = 0; i < 16; i++) {
        *p = *p | 31;
        p++;
      }
    break;
    case STATUS_INVISO:
      for (int i = 0; i < 16; i++) {
        *p = *p & 0x56B5;
        p++;
      }
    break;
    case PALEFFECT_ENEMYFLASH:
      for (int i = 0; i < 16; i++) {
        if ((*p & 0x739C) == 0) {
          *p = 0x7FFF;
        } else {
          *p = 0x0000;
        }
        p++;
      }
    break;
    case PALEFFECT_DAMAGE:
      for (int i = 0; i < 16; i++) {
        *p = *p & 31;
        p++;
      }
    break;
  }
}
