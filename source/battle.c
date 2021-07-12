#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "soundbank.h"
#include "fast_set.h"
#include "renderer.h"
#include "font.h"
#include "ui.h"
#include "textfn.h"
#include "textconsts.h"
#include "gameplay.h"
#include "battlefx.h"
#include "video.h"

#include "btl_objmap_bin.h"
#include "btl_objpal_bin.h"
#include "btl_nums_bin.h"
#include "btl_menuicons_bin.h"

#include "btl_charagfx_bin.h"
#include "btl_charapal_bin.h"
#include "btl_nmegfx_bin.h"
#include "btl_nmepal_bin.h"
#include "btl_nmedata_bin.h"

#include "btl_types.h"

#include "pdata.h"

#include <tonc_oam.h>
#include <tonc_bios.h>
#include <tonc_types.h>
#include <tonc_memmap.h>
#include <tonc_memdef.h>
#include <tonc_math.h>
#include <maxmod.h>

static int SPR_YOFFSET = 0;
//int BATTLE_MOD = MOD_PIP_HITHERE;
int BATTLE_MOD = MOD_MOD3;
int BATTLE_CANNOT_RUN = 0;

int BATTLE_OUTCOME; // 1 - victory. 0 - flee. -1 - game over

uint8_t BATTLE_SPOILS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static int BATTLE_OVER = 0;
//int BATTLE_MOD = MOD_TEMP9A;
static int BACK_SCROLLED_FORWARD = 0;
static uint8_t PASS_RETURN = 0xFF;

#define B_LERP(a, b) \
  (BACK_SCROLLED_FORWARD == 0 ? (a) : ((((a) * (16 - BACK_SCROLLED_FORWARD)) + ((b) * BACK_SCROLLED_FORWARD))/16))

#define MAX_ACTORS 8
static uint8_t turn_order[MAX_ACTORS];
struct actor *actors;

static uint8_t spaces[6];
static uint32_t target_ticker_base;

/*
static uint8_t spr_refx[6];
static uint8_t spr_flip[6];
static uint16_t spr_tick[6];
static uint8_t spr_var[6];
*/
#define OBJSETU(i, cx, cy, x, y, p, a0, a1, a2) OBJSET(i, cx, cy, x, y + SPR_YOFFSET, p, a0, a1, a2)

int get_str(int i)
{
  return actors[i].p->str;
}
int get_agi(int i)
{
  return actors[i].p->agi;
}
int get_def(int i)
{
  return actors[i].p->def;
}
int get_mag(int i)
{
  return actors[i].p->mag;
}

void battle_ui()
{
  // Menu arrows
  OBJSETU(126, 4,  0, 123, 138, 0, ATTR0_TALL, ATTR1_SIZE_8x32, 0); // 123 -> 126 when visible
  OBJSETU(127, 4,  0, 236, 138, 0, ATTR0_TALL, ATTR1_SIZE_8x32 | ATTR1_HFLIP, 0); // 236 -> 232 when visible
  // Player boxes (draw at 3 and 5 instead of 0 and 2 if selected)
  OBJSETU(125, 0,  3, 0,   138, 0, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
  OBJSETU(124, 0,  5, 0,   154, 0, ATTR0_WIDE, ATTR1_SIZE_32x8, 0);
  OBJSETU(123, 0,  3, 32,  138, 0, ATTR0_WIDE, ATTR1_SIZE_32x16 | ATTR1_HFLIP, 0);
  OBJSETU(122, 0,  5, 32,  154, 0, ATTR0_WIDE, ATTR1_SIZE_32x8 | ATTR1_HFLIP, 0);
  OBJSETU(121, 0,  0, 63,  138, 0, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
  OBJSETU(120, 0,  2, 63,  154, 0, ATTR0_WIDE, ATTR1_SIZE_32x8, 0);
  OBJSETU(119, 0,  0, 95,  138, 0, ATTR0_WIDE, ATTR1_SIZE_32x16 | ATTR1_HFLIP, 0);
  OBJSETU(118, 0,  2, 95,  154, 0, ATTR0_WIDE, ATTR1_SIZE_32x8 | ATTR1_HFLIP, 0);
  // Menu boxes (draw at 16,4 and VFLIPped if selected)
  //OBJSETU(117, 16, 1, 130, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
  OBJSETU(117, 16, 4, 130, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32 | ATTR1_VFLIP, 0);
  OBJSETU(116, 16, 1, 151, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
  OBJSETU(115, 16, 1, 172, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
  OBJSETU(114, 16, 1, 193, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
  OBJSETU(113, 16, 1, 214, 138, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
  // HP/MP separators (from 5,2 if selected)
  OBJSETU(112, 5, 2, 39, 141, 1, ATTR0_TALL, ATTR1_SIZE_8x16, 0);
  OBJSETU(111, 5, 0, 102, 141, 1, ATTR0_TALL, ATTR1_SIZE_8x16, 0);
  // HP/MP
  for (uint32_t i = 0; i < 8; i++) {
    // bit 0: curr or max
    // bit 1: chara
    // bit 2: hp or mp
    OBJSETU(110-i, 8 + ((i & 3) << 1), 4 + ((i & 4) >> 2), 23 + (i & 1 ? 21 : 0) + (i & 2 ? 63 : 0), 141 + (i & 4 ? 9 : 0), 1, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
  }
  // Menu icons
  for (uint32_t i = 0; i < 5; i++) {
    OBJSETU(102-i, 6 + (i << 1), 0, 133 + i * 21, 141, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
  }
  // Portraits
  OBJSETU(97, 4, 4, 4, 141, 8, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
  OBJSETU(96, 6, 4, 67, 141, 9, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
  
  // Enemy shadows are 90-95
  // Enemies are 84-89
}

char hex(int val)
{
  if (val < 10) return val + '0';
  return val - 10 + 'A';
}

void battle_engine__()
{
  //CURRENT_MOD = MOD_PIP_HITHERE;
  
  fast_copy(OBJ_PAL(0), btl_objpal_bin + (COLSPACE << 9), 512);
  fast_copy(OBJ_CHR(0, 0), btl_objmap_bin, 8192);

  // Portraits
  fast_copy(OBJ_PAL(8), btl_charapal_bin + 0 + (COLSPACE << 5), 32);
  fast_copy(OBJ_PAL(9), btl_charapal_bin + 128 + (COLSPACE << 5), 32);
  
  fast_copy(OBJ_CHR(4, 4), btl_charagfx_bin + 0, 64);
  fast_copy(OBJ_CHR(4, 5), btl_charagfx_bin + 64, 64);
  fast_copy(OBJ_CHR(6, 4), btl_charagfx_bin + 128, 64);
  fast_copy(OBJ_CHR(6, 5), btl_charagfx_bin + 192, 64);
  
  // 0 - hp, unsel
  // 1000 - mp, unsel
  // 2000 - hp, sel
  // 3000 - mp, sel
  fast_copy(OBJ_CHR(8, 4), btl_nums_bin + (2100 << 6), 64);
  fast_copy(OBJ_CHR(10, 4), btl_nums_bin + (2200 << 6), 64);
  fast_copy(OBJ_CHR(12, 4), btl_nums_bin + (300 << 6), 64);
  fast_copy(OBJ_CHR(14, 4), btl_nums_bin + (400 << 6), 64);
  fast_copy(OBJ_CHR(8, 5), btl_nums_bin + (3500 << 6), 64);
  fast_copy(OBJ_CHR(10, 5), btl_nums_bin + (3600 << 6), 64);
  fast_copy(OBJ_CHR(12, 5), btl_nums_bin + (1700 << 6), 64);
  fast_copy(OBJ_CHR(14, 5), btl_nums_bin + (1800 << 6), 64);
  
  // menu icons (+128 for sel)
  fast_copy(OBJ_CHR(6, 0), btl_menuicons_bin + (0 << 8) + 128, 64);
  fast_copy(OBJ_CHR(6, 1), btl_menuicons_bin + (0 << 8) + 192, 64);
  fast_copy(OBJ_CHR(8, 0), btl_menuicons_bin + (1 << 8) + 0, 64);
  fast_copy(OBJ_CHR(8, 1), btl_menuicons_bin + (1 << 8) + 64, 64);
  fast_copy(OBJ_CHR(10, 0), btl_menuicons_bin + (2 << 8) + 0, 64);
  fast_copy(OBJ_CHR(10, 1), btl_menuicons_bin + (2 << 8) + 64, 64);
  fast_copy(OBJ_CHR(12, 0), btl_menuicons_bin + (3 << 8) + 0, 64);
  fast_copy(OBJ_CHR(12, 1), btl_menuicons_bin + (3 << 8) + 64, 64);
  fast_copy(OBJ_CHR(14, 0), btl_menuicons_bin + (4 << 8) + 0, 64);
  fast_copy(OBJ_CHR(14, 1), btl_menuicons_bin + (4 << 8) + 64, 64);
  
  uint32_t nme_idx = 1;
  // Enemy pal - enemyidx*128 + ...
  fast_copy(OBJ_PAL(10), btl_nmepal_bin + (nme_idx << 7) + (COLSPACE << 5), 32);
  // Enemy gfx (2048 bytes each)
  
  fast_copy(OBJ_CHR(0, 8), btl_nmegfx_bin + 0 + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 9), btl_nmegfx_bin + (1<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 10), btl_nmegfx_bin + (2<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 11), btl_nmegfx_bin + (3<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 12), btl_nmegfx_bin + (4<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 13), btl_nmegfx_bin + (5<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 14), btl_nmegfx_bin + (6<<8) + (nme_idx << 11), 256);
  fast_copy(OBJ_CHR(0, 15), btl_nmegfx_bin + (7<<8) + (nme_idx << 11), 256);
  
  //OBJSETU(0, 0, 8, 88, 38, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
  
  //OBJSETU(127, 0, 0, 0, 0, 0, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
  //OBJSETU(126, 0, 0, 4, 4, 0, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
  //OBJSETU(125, 0, 0, 8, 8, 0, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
  
  //REG_BLDCNT = BLD_TOP(BLD_OBJ) | BLD_BOT(BLD_BG2 | BLD_OBJ) | BLD_STD;
  REG_BLDCNT = BLD_BOT(BLD_BG2) | BLD_STD;
  
  //REG_BLDALPHA = BLD_EVA(8) | BLD_EVB(8);
  
  
  //REG_DISPCNT |= DCNT_OBJ_2D | DCNT_OBJ;
  
  //REG_BG2CNT = BG_PRIO(2);
  
  //for (uint32_t i = 0; i < 32; i++) {
  for (uint32_t i = 0; i < 16; i++) {
    
    
    REG_BLDY = 15-i;
    SPR_YOFFSET = ((15 - i) * 3) >> 1;
    battle_ui();
    /*
    REG_BG2PA = 256 - (i >> 2);
    REG_BG2PB = 0;
    //REG_BG2PC = 256;
    REG_BG2PD = 256 - (i >> 2);
    REG_BG2X = (i << 8 >> 3);
    REG_BG2Y = (i << 8 >> 3);
    */
    REG_BG2PA = 256 - (i);
    REG_BG2PB = 0;
    //REG_BG2PC = 256;
    REG_BG2PD = 256 - (i);
    REG_BG2X = (i << 7);
    REG_BG2Y = (i << 9);
    
    //Shadows
    
    // Floating enemies -10 (front) / -6 (back)
    /*
    OBJSETU(90, 6, 3, 88 + 16, 59 + 64 -10, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(91, 10, 3, 88 - 80 + 16, 59 + 64 -10, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(92, 10, 3, 88 + 80 + 16, 59 + 64 -10, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(93, 10, 3, 88 + 16, 38 + 49 -6, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(1) , 0);
    //OBJSETU(94, 6, 3, 88 - 48 + 16, 38 + 49 -6, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(1) , 0);
    OBJSETU(95, 6, 3, 88 + 48 + 16, 38 + 49 -6, 1, ATTR0_WIDE | ATTR0_AFF, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(1) , 0);
    
    //OBJSET(84, 0, 8, 100, 0, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(2) , 0);
    
    OBJSETU(84, 0, 8, 88, 59 -10, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(85, 0, 8, 88 - 80, 59 -10, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(86, 0, 8, 88 + 80, 59 -10, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    
    OBJSETU(87, 0, 8, 88, 38 -6, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    //OBJSETU(88, 0, 8, 88 - 48, 38 -6, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    OBJSETU(89, 0, 8, 88 + 48, 38 -6, 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    */
    OBJSETU(84, 0, 8, 88, 59 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(85, 0, 8, 88 - 80, 59 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    OBJSETU(86, 0, 8, 88 + 80, 59 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0) , 0);
    
    OBJSETU(87, 0, 8, 88, 38 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    //OBJSETU(88, 0, 8, 88 - 48, 38 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    OBJSETU(89, 0, 8, 88 + 48, 38 , 10, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(1) , 0);
    
    AFFSET(0, -271 + i, 0, 0, 271 - i);
    AFFSET(1, -435 + i, 0, 0, 436 - i);
    //AFFSET(2, 384 - i * 2, 0, 0, 384 - i * 2);
    
    // 1.64x
    
    // Show vcount
    
    
    if (i == 14) {
      fast_copy(OBJ_CHR(14, 4), btl_nums_bin + (REG_VCOUNT << 6), 64);
      const char *string = "The quick brown fox.";

      draw_text(3, 51, string);
      
      fast_copy(OBJ_CHR(12, 4), btl_nums_bin + (REG_VCOUNT << 6), 64);
    }
    if (i == 15) i--;
    VBlankIntrWait();
    VBlankIntrWait();
    VBlankIntrWait();
    VBlankIntrWait();
    //VBlankIntrWait();
    //VBlankIntrWait();
    //VBlankIntrWait();
    //VBlankIntrWait();
  }
  
  /*
  {
      const char *string = "The quick brown fox.";
      draw_text(3, 51, string);
  }
  */
  
  for (;;) {
    
    VBlankIntrWait();
  }
}

void draw_enemies(int offy, uint8_t front_affine, uint8_t blend_all)
{
  // Do portraits too

  for (int i = 0; i < 2; i++) {
    fast_copy(OBJ_PAL(8 + i), btl_charapal_bin + (gs.pl[i].pid << 7) + (COLSPACE << 5), 32);
    int status = 0xFF;
    for (int j = 0; j < STATUS_MAX; j++) {
      if (gs.pl[i].status[j] > 0) status = j;
    }
    if (status != 0xFF) {
      paleffect(8 + i, status);
    }
  }
  
  for (int i = 0; i < 6; i++) {
    struct actor *a = &actors[i + 2];
    if (!a->active) continue;
    
    fast_copy(OBJ_PAL(10 + i), btl_nmepal_bin + (a->e->spriteid << 7) + (COLSPACE << 5), 32);
    if (TICKER % 16 < 8) {
      int status = 0xFF;
      for (int j = 0; j < STATUS_MAX; j++) {
        if (a->p->status[j] > 0) status = j;
      }
      if (status != 0xFF) {
        paleffect(10 + i, status);
      }
    }
    
    //uint8_t e = en[i];
    uint8_t sp = 84 + i;
    uint8_t shsp = 90 + i;
    uint8_t fly = a->e->fly;
    uint8_t back = i >= 3;
    
    int x = 88;
    int y = 59;
    int xdispl;
    if (back) { // Back row
      xdispl = B_LERP(48, 80);
      y -= B_LERP(21, 0);
    } else {
      xdispl = 80;
    }
    uint32_t t = TICKER + a->tick;
    
    // Flip this enemy?
    uint32_t flip_timer = 32 + a->var / 4;
    if ((t % flip_timer) == 0) {
      if (rand() % 2)
        a->flip ^= 1;
    }
    if (a->e->flip == 0) a->flip = 0;
    
    if (i % 3 == 0) x -= xdispl; // left
    if (i % 3 == 2) x += xdispl; // right
    
    uint16_t attr0 = 0;
    uint16_t attr1 = 0;
    if (blend_all || a->p->hp == 0) {
      attr0 |= ATTR0_BLEND;
    }
    if (front_affine || i >= 3) {
      attr0 |= ATTR0_AFF;
    } else {
      if (a->flip) attr1 |= ATTR1_HFLIP;
    }
    if (fly) {
      y -= back ? B_LERP(8, 14) : 14;
      
      // Store x and y before bobbing is introduced
      a->x = x;
      a->y = y;
      
      x += lu_sin(t * (150 + a->var)) / (back ? B_LERP(840, 512) : 512);
      
      int ht = lu_cos(t * (450 - a->var)) / (back ? B_LERP(1260, 768) : 768);
      
      if (spaces[i] == 1) { // Only cast a shadow if there is floor here
        OBJSET(shsp, ht > 0 ? 10 : 6, 3, x + 16, y + (back ? B_LERP(49, 64) : 64) + offy, 1, ATTR0_WIDE | attr0, ATTR1_SIZE_32x8 | ATTR1_AFF_ID(i), 0);
      }
      
      y += ht;
    } else {
      // Store x and y before bobbing is introduced
      a->x = x;
      a->y = y;
      
      // Non-flying enemies move around a little bit
      uint8_t mvdata = (t / 16) ^ a->var;
      if (back && BACK_SCROLLED_FORWARD < 8) {
        if (mvdata & 3) x--;
        if (mvdata & 12) y--;
        if (mvdata & 48) x++;
        if (mvdata & 192) y++;
      } else {
        if ((mvdata & 3) == 3) x--;
        if ((mvdata & 12) == 12) y--;
        if ((mvdata & 48) == 48) x++;
        if ((mvdata & 192) == 192) y++;
      }
    }
    
    
    OBJSET(sp, a->refx, 8, x, y + offy, 10 + i, ATTR0_SQUARE | attr0, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(i) | attr1, 0);
  }
}

void draw_aff(uint32_t transition) {
  if (transition < 15)
    for (int j = 0; j < 3; j++) AFFSET(j, actors[j+2].flip ? (-271 + transition) : (271 - transition), 0, 0, 271 - transition);
  for (int j = 3; j < 6; j++) AFFSET(j, actors[j+2].flip ? (B_LERP(-435, -271) + transition) : (B_LERP(435, 271) - transition), 0, 0, B_LERP(435, 271) - transition);
}

void battle_transition(int ambush)
{
  REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
  
  if (ambush) draw_textbar("Surprise attack!");
  uint8_t menuicons[5];
  for (int i = 0; i < 5; i++) menuicons[i] = i;
  
  for (uint32_t i = 0; i < 16; i++) {
    int offy = ((15 - i) * 3) >> 1;
    draw_iconmenu(255, menuicons, 5, 255, 0, offy);

    REG_BLDALPHA = BLDA_BUILD(i, 16-i);
    REG_BG2PA = 256 - (i);
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 256 - (i);
    REG_BG2X = (i << 7);
    REG_BG2Y = (i << 9);
    
    for (int k = 0; k < 4; k++) {
      draw_enemies(offy, 1, 1);
      draw_aff(i);
      //for (int j = 0; j < 3; j++) AFFSET(j, spr_flip[j] ? (-271 + i) : (271 - i), 0, 0, 271 - i);
      //for (int j = 3; j < 6; j++) AFFSET(j, spr_flip[j] ? (-435 + i) : (435 - i), 0, 0, 435 - i);
      
      VBlankIntrWait();
    }
  }
}

void battle_transitionout()
{
  uint8_t menuicons[5];
  for (int i = 0; i < 5; i++) menuicons[i] = i;
  
  for (int i = 15; i >= 0; i--) {
    int offy = ((15 - i) * 3) >> 1;
    draw_iconmenu(255, menuicons, 5, 255, 0, offy);

    REG_BLDALPHA = BLDA_BUILD(i, 16-i);
    REG_BG2PA = 256 - (i);
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 256 - (i);
    REG_BG2X = (i << 7);
    REG_BG2Y = (i << 9);
    
    // In case there are any left
    for (int k = 0; k < 4; k++) {
      mmSetModuleVolume(256 - (((15-i) * 4 + k) * 4));
      draw_enemies(offy, 1, 1);
      draw_aff(i);
      //for (int j = 0; j < 3; j++) AFFSET(j, spr_flip[j] ? (-271 + i) : (271 - i), 0, 0, 271 - i);
      //for (int j = 3; j < 6; j++) AFFSET(j, spr_flip[j] ? (-435 + i) : (435 - i), 0, 0, 435 - i);
      
      VBlankIntrWait();
    }
  }
  CURRENT_MOD = -1;
  mmSetModuleVolume(256);
  oam_init(obj_mem, 256);
}


void allocate_enemy_sprites(uint8_t *en)
{
  uint8_t refs[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t refsx[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t free_x = 0;
  
  for (int i = 0; i < 6; i++) {
    struct actor *a = &actors[i + 2];
    a->active = 0;
    a->id = i;
    a->side = 1;
    if (en[i] != 0xFF) {
      uint32_t e = en[i];
      a->active = 1;
      a->p = malloc(sizeof(struct pd_player));
      a->e = &NMEDATA[e];
      a->p->maxhp = a->e->maxhp;
      a->p->maxmp = a->e->maxmp;
      a->p->str = a->e->str;
      a->p->def = a->e->def;
      a->p->agi = a->e->agi;
      a->p->mag = a->e->mag;
      a->p->hp = a->p->maxhp;
      a->p->mp = a->p->maxmp;
      for (int j = 0; j < EQUIP_MAX; j++) a->p->equip[j] = 0xFF;
      for (int j = 0; j < STATUS_MAX; j++) a->p->status[j] = 0;
      for (int j = 0; j < SLOTS_MAX; j++) a->p->slots[j] = 0xFF;
      //fast_copy(OBJ_PAL(10 + i), btl_nmepal_bin + (e << 7) + (COLSPACE << 5), 32);
      a->tick = rand();
      a->var = rand();
      a->flip = rand() % 2;
      
      for (int j = 0; j < 4; j++) {
        if (e == refs[j]) {
          a->refx = refsx[j];
          break;
        }
        if (refs[j] == 0xFF) {
          refs[j] = e;
          refsx[j] = free_x;
          a->refx = free_x;
          
          for (uint32_t y = 0; y < 8; y++) {
            fast_copy(OBJ_CHR(free_x, 8 + y), btl_nmegfx_bin + (y << 8) + (a->e->spriteid << 11), 256);
          }
          
          free_x += 8;
          break;
        }
      }
    }
  }
}

void fill_turn_order(int ambush)
{
  uint8_t removed[MAX_ACTORS];
  memset(removed, 0, MAX_ACTORS);
  int turn_order_n = 0;
  for (int s = 0; s < 2; s++) {
    int side = ambush ? 1-s : s;
    
    for (;;) {
      int best_agi = -1;
      int best_agi_i = -1;
      for (int i = 0; i < MAX_ACTORS; i++) {
        if (removed[i]) continue;
        if (!actors[i].active) continue;
        if (actors[i].p->hp == 0) continue;
        if (actors[i].side != side) continue;
        int agi = get_agi(i);
        if (agi > best_agi) {
          best_agi = agi;
          best_agi_i = i;
        }
      }
      if (best_agi_i == -1) break;
      turn_order[turn_order_n++] = best_agi_i;
      removed[best_agi_i] = 1;
    }
  }
  while (turn_order_n < MAX_ACTORS) {
    turn_order[turn_order_n++] = 0xFF;
  }
}

void battle_wait()
{
  draw_enemies(0, 0, 0);
  draw_aff(15);
  VBlankIntrWait();
}

int cur_sel(struct actor *a, uint8_t *menuicons, uint8_t menulen, int curmem, const char * const *cur_msgs)
{
  uint8_t cur = clamp(CMEM_ON ? CMEM[curmem] : 0, 0, menulen - 1);
  
  for (;;) {
    hide_display();
    draw_iconmenu(a->id, menuicons, menulen, cur, 0, 0);
    
    if (cur_msgs != NULL) {
      draw_textbar(cur_msgs[cur]);
    } else {
      // Must be techniques
      char txt[128];
      txt_set(txt, NMEDATA[menuicons[cur]-40].sname);
      txt_append_str(txt, " (Qi: ");
      txt_append_num(txt, NMEDATA[menuicons[cur]-40].spdata[SP_COST]);
      txt_append_str(txt, ")");
      draw_textbar(txt);
    }
		for (;;) {
      battle_wait();
			if (INKEY_PRESSED & KEY_ACCEPT) {
				return cur;
			}
			else if (INKEY_PRESSED & KEY_B) {
				return -1;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? menulen - 1 : 1)) % menulen;
				mmEffect(SFX_CURSOR);
				CMEM[curmem] = cur;
				break;
			}
			
		}
  }
}

int can_target(int cur, int targdead, int ranged)
{
  if (cur >= 5 && !ranged) return false;
  if (actors[cur].active) {
    if (actors[cur].p->hp > 0 || targdead) {
      return true;
    }
  }
  return false;
}

void get_bounds(int cur, int *x1, int *y1, int *x2, int *y2)
{
  *x1 = actors[cur].x;
  *y1 = actors[cur].y;
  int w, h;
  if (cur < 2) { // Players
    w = 56;
    h = 14;
  } else if (cur < 5) { // Front row enemies
    w = 64;
    h = 64;
  } else { // Back row enemies
    *x1 += 8;
    *y1 += 8;
    w = 48;
    h = 48;
  }
  *x2 = *x1 + w;
  *y2 = *y1 + h;
}

void draw_indiv_target(int cur, uint8_t *sp)
{
  int x1, y1, x2, y2;
  get_bounds(cur, &x1, &y1, &x2, &y2);
  OBJSET(*sp, 16, 0, x1 - 4, y1 - 4, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
  OBJSET(*sp + 1, 16, 0, x2 - 4, y1 - 4, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8 | ATTR1_HFLIP, 0);
  OBJSET(*sp + 2, 16, 0, x1 - 4, y2 - 4, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8 | ATTR1_VFLIP, 0);
  OBJSET(*sp + 3, 16, 0, x2 - 4, y2 - 4, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8 | ATTR1_HFLIP | ATTR1_VFLIP, 0);
  *sp += 4;
}

void hide_targets()
{
  for (int i = 32; i < 64; i++) {
    obj_hide(&oam_mem[i]);
  }
}

void draw_target(int cur, int all)
{
  uint8_t sp = 32;
  uint32_t t = TICKER - target_ticker_base;
  
  if (t % 24 < 12) {
    if (!all) {
      draw_indiv_target(cur, &sp);
    } else {
      for (uint8_t i = 0; i < MAX_ACTORS; i++) {
        if (actors[i].active && actors[i].side == actors[cur].side) {
          draw_indiv_target(i, &sp);
        }
      }
    }
  } else {
    hide_targets();
  }
}


int cursor_correct_left(int cur, int targdead, int ranged)
{
  int attempts = 4;
  do { 
    cur--;
    if (cur == -1) cur = 1;
    else if (cur == 1) cur = 4;
    else if (cur == 4) cur = 7;
    attempts--;
    if (attempts == 0) return -1;
  } while (!can_target(cur, targdead, ranged));
  return cur;
}
int cursor_correct_right(int cur, int targdead, int ranged)
{
  int attempts = 4;
  do {
    cur++;
    if (cur == 2) cur = 0;
    else if (cur == 5) cur = 2;
    else if (cur == 8) cur = 5;
    attempts--;
    if (attempts == 0) return -1;
  } while (!can_target(cur, targdead, ranged));
  return cur;
}

int targ_sel(struct actor *a, int side, int all, int targdead, int ranged)
{
  int curmem = CMEM_BTARG + a->id * 2 + side;
  int cur = clamp(CMEM_ON ? CMEM[curmem] : 0, 0, MAX_ACTORS - 1);
  target_ticker_base = TICKER;
  for (;;) {
    if (can_target(cur, targdead, ranged) && actors[cur].side == side) {
      break;
    }
    cur = (cur + 1) % MAX_ACTORS;
  }
  
  for (;;) {
    draw_target(cur, all);
    battle_wait();
    
    if (INKEY_PRESSED & KEY_ACCEPT) {
      hide_targets();
      return cur;
    }
    else if (INKEY_PRESSED & KEY_B) {
      hide_targets();
      return -1;
    }
    else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) {
      if (INKEY_PRESSED & KEY_LEFT) {
        cur = cursor_correct_left(cur, targdead, ranged);
      }
      else if (INKEY_PRESSED & KEY_RIGHT) {
        cur = cursor_correct_right(cur, targdead, ranged);
      }
      else if (INKEY_PRESSED & KEY_UP) {
        for (;;) {
          if (cur >= 5) cur = 0;
          else if (cur >= 2) cur += 3;
          else cur = 3;
          if (can_target(cur, targdead, ranged)) break;
          int ncur = cursor_correct_left(cur, targdead, ranged);
          if (ncur > 0) {
            cur = ncur;
            break;
          }
        }
      }
      else if (INKEY_PRESSED & KEY_DOWN) {
        for (;;) {
          if (cur >= 5) cur -= 3;
          else if (cur >= 2) cur = 0;
          else cur = 6;
          if (can_target(cur, targdead, ranged)) break;
          int ncur = cursor_correct_left(cur, targdead, ranged);
          if (ncur > 0) {
            cur = ncur;
            break;
          }
        }
      }
      mmEffect(SFX_CURSOR);
      CMEM[curmem] = cur;
    }
  }
  
  
  return 0;
}

void death_drop(struct actor *a)
{
  uint8_t spoil = 0xFF;
  if (a->e->spriteid >= 20) {
    // Rivals always drop their equipment
    spoil = a->e->spriteid + 80;
  } else {
    int roll = rand() % 100 + 1;
    if (roll <= a->e->gem_accum) {
      // Won a soulstone
      spoil = a->e->spriteid + 40;
      // Do another roll, see if we get equipment
      roll = rand() % 100 + 1;
      if (roll <= 25) {
        // Upgrade it to equipment
        spoil = a->e->spriteid + 80;
      }
    }
  }
  
  for (int i = 0; i < 6; i++) {
    if (BATTLE_SPOILS[i] == 0xFF) {
      BATTLE_SPOILS[i] = spoil;
      break;
    }
  }
}

static const uint8_t MENUICONS[] = {0,1,2,3,4};
int aftermath()
{
  draw_iconmenu(255, MENUICONS, 5, 255, 0, 0);
  // Aftermath
  int newly_killed = 0;
  for (int i = 2; i < MAX_ACTORS; i++) {
    struct actor *a = &actors[i];
    if (a->active && a->side == 1 && a->p->hp == 0) {
      newly_killed++;
    }
  }
  if (newly_killed > 0) {
    mmEffect(SFX_MELEEHIT);
    for (int i = 0; i < 8; i++) {
      REG_BLDALPHA = BLDA_BUILD(16, i);
      battle_wait();
    }
    for (int i = 0; i < 8; i++) {
      REG_BLDALPHA = BLDA_BUILD(16, 8-i);
      battle_wait();
    }
    for (int i = 0; i < 16; i++) {
      REG_BLDALPHA = BLDA_BUILD(15-i, i);
      battle_wait();
    }
    for (int i = 2; i < MAX_ACTORS; i++) {
      struct actor *a = &actors[i];
      if (a->active && a->side == 1 && a->p->hp == 0) {
        death_drop(a);
        a->active = 0;
        obj_hide(&oam_mem[84 + i - 2]);
        obj_hide(&oam_mem[90 + i - 2]);
      }
    }
    REG_BLDALPHA = 16;
    battle_wait();
  }
  // Is there a victor?
  int pl_alive = 0, en_alive = 0;
  for (int i = 0; i < MAX_ACTORS; i++) {
    struct actor *a = &actors[i];
    if (a->active && a->p->hp > 0) {
      if (a->side == 0) pl_alive++;
      else if (a->side == 1) en_alive++;
    }
  }
  if (pl_alive == 0) { // Game over
    BATTLE_OVER = 1;
    BATTLE_OUTCOME = -1;
    return 1;
  }
  if (en_alive == 0) { // Victory
    BATTLE_OVER = 1;
    BATTLE_OUTCOME = 1;
    return 1;
  }
  
  // Do enemies in the back need to come forward?
  int any_in_front = 0;
  for (int i = 2; i <= 4; i++) {
    struct actor *a = &actors[i];
    if (a->active && a->p->hp > 0) {
      any_in_front = 1;
      break;
    }
  }
  if (!any_in_front) {
    for (int i = 0; i < 64; i++) {
      BACK_SCROLLED_FORWARD = i / 4;
      battle_wait();
    }
    for (int i = 2; i <= 4; i++) {
      free(actors[i].p);
      actors[i].p = NULL;
      actors[i] = actors[i + 3];
      actors[i].id = i - 2;
      actors[i + 3].active = 0;
      obj_hide(&oam_mem[84 + i + 3 - 2]);
      obj_hide(&oam_mem[90 + i + 3 - 2]);
    }
    BACK_SCROLLED_FORWARD = 0;
    battle_wait();
  }
  return 0;
}

void use_tech(struct actor *user, struct actor *targ, uint8_t tech, int all, int targdead)
{
  uint8_t targets[MAX_ACTORS];
  uint8_t targets_done[MAX_ACTORS];
  for (int i = 0; i < MAX_ACTORS; i++) {
    targets[i] = 0xFF;
    targets_done[i] = 0;
  }
  int targets_n = 0;
  
  if (!all) {
    targets[targets_n++] = targ->id + (targ->side * 2);
  } else {
    for (int i = 0; i < MAX_ACTORS; i++) {
      if (actors[i].active && actors[i].side == targ->side) {
        if (targdead || actors[i].p->hp > 0) {
          targets[targets_n++] = i;
        }
      }
    }
  }
  // Shuffle

  for (int i = 1; i < targets_n; i++) {
    int j = rand() % (i + 1);
    uint8_t t = targets[i];
    targets[i] = targets[j];
    targets[j] = t;
  }
  
  // Give each animation 10 frames by itself before moving to the next target
  int anim_timer = 0;
  int fin = 0;

  while (fin < targets_n) {
    uint8_t sp = 83;
    draw_enemies(0, 0, 0);
    draw_aff(15);
    for (int i = 0; i < targets_n; i++) {
      int t = anim_timer - i * 10;
      if (t >= 0 && !targets_done[i]) {
        int r = use_tech_indiv(anim_timer, t, user, &actors[targets[i]], tech, &sp);
        if (r == 1) {
          targets_done[i] = 1;
          fin++;
        }
      }
    }
    anim_timer++;

    VBlankIntrWait();
    
    for (uint8_t i = sp; i < 84; i++) {
      obj_hide(&oam_mem[i]);
    }
  }
  
  // Aftermath processing
  if (aftermath()) return;
}

void msg_pause()
{
  for (int i = 0; i < 60; i++) {
    battle_wait();
    if (INKEY_PRESSED & KEY_ANY) break;
  }
}

void get_tech_info(int tech, int *tech_cost, int *tech_side, int *tech_targdead, int *tech_all)
{
  *tech_cost = NMEDATA[tech].spdata[SP_COST];
  *tech_side = 1; // Default, presume enemies
  *tech_targdead = 0;
  if (NMEDATA[tech].spdata[SP_INFLICT] >= 4 && NMEDATA[tech].spdata[SP_INFLICT] <= 7) *tech_side = 0; // Beneficial status
  if (NMEDATA[tech].spdata[SP_CURE] > 0) *tech_side = 0; // Curing
  if (NMEDATA[tech].spdata[SP_REVIVE] > 0) {
    *tech_side = 0; // Reviving
    *tech_targdead = 1;
  }
  if (NMEDATA[tech].spdata[SP_HEAL] > 0) *tech_side = 0; // Healing
  *tech_all = (NMEDATA[tech].spdata[SP_ALL] > 0) ? 1 : 0;
}

void battle_exec_enemy(struct actor *a)
{
  // If we have the MP to use our skill, there is a 50% chance of using it
  int use_skill = rand() % 2;
  int tech = a->e->spriteid;
  int tech_cost, tech_side, tech_targdead, tech_all;
  get_tech_info(tech, &tech_cost, &tech_side, &tech_targdead, &tech_all);
  if (tech_cost > a->p->mp) use_skill = 0;
  
  if (use_skill == 0) {
    tech = 0xFF;
    tech_cost = 0;
    tech_side = 1;
    tech_targdead = 0;
    tech_all = 0;
    if (a->id >= 3) return; // Back row
  }
  
  // Invert side
  tech_side = tech_side ^ 1;
  
  a->p->mp -= tech_cost;
  // Find a target for it
  for (;;) {
    int t = rand() % MAX_ACTORS;
    if (!actors[t].active) continue;
    if (actors[t].p->hp == 0 && !tech_targdead) continue;
    if (actors[t].side != tech_side) continue;
    
    if (tech != 0xFF) {
      draw_textbar(NMEDATA[tech].sname);
    }
    
    for (int i = 0; i < 32; i++) {
      draw_enemies(0, 0, 0);
      if (i % 16 < 8) {
        paleffect(10 + a->id, PALEFFECT_ENEMYFLASH);
      }
      draw_aff(15);
      VBlankIntrWait();
    }
    
    hide_display();
    
    use_tech(a, &actors[t], tech, tech_all, tech_targdead); // a, targ, tech, all, targdead
    break;
  }
}

void battle_exec_player(struct actor *a)
{
  uint8_t menuicons[5];
  for (int i = 0; i < 5; i++) menuicons[i] = i;
  a->defending = 0;
  for (;;) {
    int c = cur_sel(a, menuicons, 5, CMEM_BCUR + a->id, BATTLE_MENU);
    if (c == -1) continue;
    switch (c) {
      case 0: // Attack
        int t = targ_sel(a, 1,        0, 0, 0); // a, side, all, targdead, ranged
        if (t == -1) break;
        use_tech(a, &actors[t], 0xFF, 0, 0); // a, targ, tech, all, targdead
        return;
      case 1: // Spirit technique
        uint8_t techicons[5];
        techicons[0] = a->id == 0 ? 66 : a->id + 59;
        uint8_t techicons_n = 1;
        for (int i = 0; i < 4; i++) {
          uint8_t tid = a->p->slots[i];
          if (tid != 0xFF) {
            techicons[techicons_n] = tid + 40;
            techicons_n++;
          }
        }
        
        int tc = cur_sel(a, techicons, techicons_n, CMEM_BTEC + a->id, NULL);
        if (tc == -1) break;
        int tech = techicons[tc] - 40;
        int tech_cost, tech_side, tech_targdead, tech_all;
        get_tech_info(tech, &tech_cost, &tech_side, &tech_targdead, &tech_all);
        
        if (tech_cost > a->p->mp) {
          draw_textbar("Insufficient Qi");
          mmEffect(SFX_NOPE);
          msg_pause();
          break;
        }
        
        int tg = targ_sel(a, tech_side, tech_all, tech_targdead, 1); // a, side, all, targdead, ranged
        if (tg == -1) break;
        a->p->mp -= tech_cost;
        use_tech(a, &actors[tg], tech, tech_all, tech_targdead); // a, targ, tech, all, targdead
        
        return;
      case 2: // Defend
        a->defending = 1;
        draw_textbar("Defending");
        msg_pause();
        return;
      case 3: // Pass
        PASS_RETURN = a->id;
        draw_textbar("Passing");
        msg_pause();
        return;
      case 4: // Escape
        if (BATTLE_CANNOT_RUN) {
          draw_textbar("There is no escape");
          mmEffect(SFX_NOPE);
          msg_pause();
          break;
        }
        // Flat 50% chance of escape
        int roll = rand() % 2;
        if (roll == 1) {
          BATTLE_OVER = 1;
          BATTLE_OUTCOME = 0;
          mmEffect(SFX_RUN);
          draw_textbar("Escaped!");
        } else {
          mmEffect(SFX_SELECTNONE);
          draw_textbar("Failed to escape");
          msg_pause();
        }
        return;
    }
  }
}

int get_acc(struct actor *a)
{
  if (a->p->equip[EQUIP_ACC] != 0xFF) {
    return NMEDATA[a->p->equip[EQUIP_ACC]].eqatt[EQATT_DMG];
  }
  return 0xFF;
}

void battle_exec(int ambush)
{
  uint8_t pr = 0xFF;
  hide_display();
  
  for (;;) {
    fill_turn_order(ambush);
    
    for (int turn = 0; turn < MAX_ACTORS || pr != 0xFF; turn++) {
      
      uint8_t aid = 0xFF;
      if (pr != 0xFF) {
        aid = pr;
        turn--;
      } else {
        aid = turn_order[turn];
      }
      if (aid == 0xFF) continue;

      struct actor *a = &actors[aid];
      
      // Might need to be skipped
      if (!a->active) continue;
      if (a->p->hp == 0) continue;
      
      // Handle poison here
      if (a->p->status[STATUS_POISON] > 0) {
        a->p->hp = max(a->p->hp - 10, 0);
      }
      if (a->p->status[STATUS_BADPOISON] > 0) {
        a->p->hp = max(a->p->hp - 50, 0);
      }
      if (a->p->status[STATUS_REGEN] > 0) {
        a->p->hp = min(a->p->hp + 25, a->p->maxhp);
      }
      if (get_acc(a) == ACC_MPREGEN) {
        a->p->mp = min(a->p->mp + 10, a->p->maxmp);
      }
      if (get_acc(a) == ACC_MPMAX) {
        a->p->mp = a->p->maxmp;
      }
      // Tick down all statuses
      for (int i = 0; i < STATUS_MAX; i++) {
        a->p->status[i] = max(a->p->status[i] - 1, 0);
      }
      
      if (aftermath()) return;
      if (a->p->hp == 0) continue;
      
      if (a->p->status[STATUS_PARA] > 0) {
        int roll = rand() % 10;
        if (roll <= 3) {
          // Paralysed and cannot move
          if (a->side == 0) {
            draw_textbar("Paralysis");
            msg_pause();
          }
          continue;
        }
      }
      
      pr = PASS_RETURN;
      PASS_RETURN = 0xFF;
      
      int trns = 1;
      if (get_acc(a) == ACC_SPEEDBOOTS) trns++;
      if (a->p->status[STATUS_FAST]) trns++;


      for (int tr = 0; tr < trns; tr++) {
        if (a->side == 0) { // Player
          battle_exec_player(a);
        } else {
          battle_exec_enemy(a);
        }
        if (BATTLE_OVER) return;
      }
      
      battle_wait();
    }
  }
}

void allocate_players()
{
  for (int i = 0; i < 2; i++) {
    actors[i].active = gs.pl[i].pid != 0xFF;
    actors[i].id = i;
    actors[i].side = 0;
    actors[i].p = &gs.pl[i];
    actors[i].e = NULL;
    actors[i].x = i * 63 + 4;
    actors[i].y = 142;
  }
}

void battle_engine(uint8_t *enemies, int ambush)
{
  for (int i = 0; i < 6; i++) BATTLE_SPOILS[i] = 0xFF;
  
  actors = malloc(sizeof(actors[0]) * MAX_ACTORS);
  memset(actors, 0, sizeof(actors[0]) * MAX_ACTORS);

  allocate_players();
  allocate_enemy_sprites(enemies);
  battle_transition(ambush);
   
  battle_exec(ambush);
  // Remove all status effects
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < STATUS_MAX; j++) {
      gs.pl[i].status[j] = 0;
    }
  }
  BATTLE_OVER = 0;
  if (BATTLE_OUTCOME != 1) { // Only victors gain the spoils of war
    for (int i = 0; i < 6; i++) BATTLE_SPOILS[i] = 0xFF;
  }
  for (int i = 2; i < MAX_ACTORS; i++) {
    if (actors[i].p) free(actors[i].p);
  }
  free(actors);
  
  battle_transitionout();
}

int check_space(uint8_t dir, int fliers)
{
  // Which spaces can house enemies?
  // 0 - no. 1 - yes. 2 - flying only
  
  //  3 4 5
  // 0  1  2

  for (int i = 0; i < 6; i++) spaces[i] = 0;
  
  if (getTexDirFrom(gs.x, gs.y, gs.z, dir) % 128) {
    return 0;
  }
  int x = gs.x + getDirX(dir);
  int y = gs.y + getDirY(dir);
  int z = gs.z + getDirZ(dir);
  for (int i = 0; i < 6; i++) spaces[i] = 1;
  uint8_t left = (dir - 1) & 3;
  uint8_t right = (dir + 1) & 3;
  
  if (getTexDirFrom(x, y, z, dir) % 128) {
    spaces[4] = 0;
  }
  if (getTexDirFrom(x, y, z, left) % 128) {
    spaces[0] = 0;
    spaces[3] = 0;
  }
  if (getTexDirFrom(x, y, z, right) % 128) {
    spaces[2] = 0;
    spaces[5] = 0;
  }
  if (getTexDirFrom(x + getDirX(left), y + getDirY(left), z + getDirZ(left), dir) % 128) {
    spaces[3] = 0;
  }
  if (getTexDirFrom(x + getDirX(right), y + getDirY(right), z + getDirZ(right), dir) % 128) {
    spaces[5] = 0;
  }
  
  // See which ones only support flying enemies
  for (int i = 0; i < 6; i++) {
    if (spaces[i] == 0) continue;
    int tx = x;
    int ty = y;
    int tz = z;
    if (i >= 3) { // back row
      tx += getDirX(dir);
      ty += getDirY(dir);
      tz += getDirZ(dir);
    }
    if (i % 3 == 0) { // left
      tx += getDirX(left);
      ty += getDirY(left);
      tz += getDirZ(left);
    }
    if (i % 3 == 2) { // right
      tx += getDirX(right);
      ty += getDirY(right);
      tz += getDirZ(right);
    }
    
    if (getTexDirFrom(tx, ty, tz, DIR_D) == 0) {
      spaces[i] = 2;
      // If a space only supports flying enemies, we do not want the space behind it supporting
      // a regular enemy. Make it flying-only
      if (i < 3) {
        if (spaces[i + 3] == 1) spaces[i + 3] = 2;
      }
    }
  }
  
  int c = 0;
  for (int i = 0; i < 6; i++) {
    if (spaces[i] == 1 || (fliers && spaces[i] == 2))
      c++;
  }
  
  //char txt[128];
  //txt_set(txt, ">");
  //for (int i = 0; i < 6; i++) {
  //  txt_append_str(txt, " ");
  //  txt_append_num(txt, spaces[i]);
  //}
  //txt_append_str(txt, ": ");
  //txt_append_num(txt, c);
  
  //show_message(-1, txt);
  return c;
}

void random_encounter_dir(uint8_t *elist, int dir)
{
  CURRENT_MOD = BATTLE_MOD;
  
  if (dir == -1) {
    turnLeft();
  } else if (dir == 1) {
    turnRight();
  } else if (dir == 2) {
    turnRight();
    turnRight();
  }
  check_space(gs.d, 1);
  
  uint8_t enemies[6];
  for (int i = 0; i < 6; i++) enemies[i] = 0xFF;
  
  uint8_t goodslots[6];
  
  // If there is only 1 enemy and the middle space is compatible, skip the rest of this
  uint8_t skip = 0;
  if (elist[1] == 0xFF) {
    uint8_t e = elist[0];
    if (spaces[1] == 1 || (spaces[1] == 2 && NMEDATA[e].fly)) {
      enemies[1] = e;
      skip = 1;
    }
  }
  
  if (!skip) {
    for (int i = 0; i < 6; i++) {
      if (elist[i] == 0xFF) continue;
      uint8_t e = elist[i];
      // Find a compatible slot for this enemy
      int goodslots_n = 0;
      int spcs = 3;
      // If there is an enemy in the first row, back row enemies are eligible
      if (enemies[0] != 0xFF || enemies[1] != 0xFF || enemies[2] != 0xFF) spcs = 6;
      for (int j = 0; j < spcs; j++) {
        if (enemies[j] == 0xFF && (spaces[j] == 1 || (spaces[j] == 2 && NMEDATA[e].fly))) {
          goodslots[goodslots_n++] = j;
        }
      }
      if (goodslots_n == 0) continue;
      
      uint8_t s = goodslots[rand() % goodslots_n];
      enemies[s] = e;
    }
  }
  
  /*
  char txt[192];
  txt_set(txt, "Encounter ");
  txt_append_num(txt, lu_sin(8192));
  show_message(-1, txt);
  */
  /*
  char txt[192];
  txt_set(txt, "Encounter ");
  for (int i = 0; i < 6; i++) {
    txt_append_num(txt, enemies[i]);
    if (enemies[i] != 0xFF) {
    

    txt_append_num(txt, NMEDATA[enemies[i]].fly);
    txt_append_str(txt, ")");
    }
    txt_append_str(txt, ":");

    txt_append_num(txt, spaces[i]);
    txt_append_str(txt, " ");
  }
  show_message(-1, txt);
  */

  battle_engine(enemies, dir != 0);
}

void get_encounter_list(uint8_t *elist)
{
  for (;;) {
    uint32_t mask = 0;
    
    for (int i = 0; i < 6; i++)
      elist[i] = 0xFF;
    int ecount = rand() % 5 + 1;
    for (int i = 0; i < ecount; i++)
      elist[i] = max(rand() % gs.plot[PLOT_ELVL], rand() % gs.plot[PLOT_ELVL]);
    for (int i = 0; i < 6; i++)
      if (elist[i] != 0xFF)
        mask |= 1 << elist[i];
    
    if (__builtin_popcount(mask) <= 4) break;
  }
}

void random_encounter()
{
  uint8_t elist[6];
  get_encounter_list(elist);
  int fliers = 0;
  for (int i = 0; i < 6; i++) {
    uint8_t e = elist[i];
    if (e != 0xFF) {
      if (NMEDATA[e].fly) fliers++;
    }
  }
  
  int front_c = check_space(gs.d, fliers > 0);
  int left_c = check_space((gs.d - 1) & 3, fliers > 0);
  int right_c = check_space((gs.d + 1) & 3, fliers > 0);
  
  int ambush_chance = 5;
  if (front_c > max(left_c, right_c)) ambush_chance += 2;
  if (front_c == 0) ambush_chance -= 3;
  int roll = rand();
  int ambush = (roll % ambush_chance) == 0;
  
  if (!ambush) {
    if (front_c > 0) random_encounter_dir(elist, 0);
  } else { // ambush
    roll /= ambush_chance;    
    if (roll % 2) {
      if (left_c > 0 && left_c >= right_c) random_encounter_dir(elist, -1);
      else if (right_c > 0 && right_c >= left_c) random_encounter_dir(elist, 1);
    } else {
      if (right_c > 0 && right_c >= left_c) random_encounter_dir(elist, 1);
      else if (left_c > 0 && left_c >= right_c) random_encounter_dir(elist, -1);
    }
  }
}

void force_random_encounter(int idx)
{
  uint8_t elist[6];
  for (int i = 0; i < 6; i++) {
    elist[i] = 0xFF;
  }
  elist[0] = idx;
  
  check_space(gs.d, 0);
  if (spaces[1] == 1) {
    random_encounter_dir(elist, 0);
    return;
  }
  
  check_space((gs.d - 1) & 3, 0);
  if (spaces[1] == 1) {
    random_encounter_dir(elist, -1);
    return;
  }
  check_space((gs.d + 1) & 3, 0);
  if (spaces[1] == 1) {
    random_encounter_dir(elist, 1);
    return;
  }
  check_space((gs.d + 2) & 3, 0);
  if (spaces[1] == 1) {
    random_encounter_dir(elist, 2);
    return;
  }
}
