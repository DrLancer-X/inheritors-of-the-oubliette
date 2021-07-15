#include <stdlib.h>
#include <maxmod.h>
#include "fast_set.h"
#include "pdata.h"
#include "gameplay.h"
#include "maps.h"
#include "soundbank.h"
#include "renderer.h"
#include "video.h"
#include "battle.h"
#include "ui.h"
#include "macros.h"
#include "plot.h"

#include <maxmod.h>
#include <tonc_bios.h>
#include <tonc_oam.h>
#include <tonc_memdef.h>
#include <tonc_irq.h>
#include <tonc_memmap.h>

#define MOVE_WAIT 6

static int skip_encounters = 0;
static int show_loc = 0;

struct lvl_tile *lvl_bin = NULL;
const void *lvl_vis_bin;
const uint8_t *lvl_backdrop;
static int GAMEPLAY_MUS = -1;

void loadMap() {
  struct mapinfo m = MAPS[gs.map];
  
  if (lvl_bin == NULL) lvl_bin = malloc(sizeof(struct lvl_tile) * 4096);
  fast_copy(lvl_bin, m.map, sizeof(struct lvl_tile) * 4096);

  lvl_vis_bin = m.vis;
  lvl_backdrop = m.backdrop;
  load_palette(m.palette, m.colmap);
  init_textures(m.flats, m.walls, m.twalls);
}

void drawBackdrop(int persp) {
  if (((uint16_t *)lvl_backdrop)[0] == 240) {
    fast_copy(VIDEO_BUFFER, lvl_backdrop, 38400);
  } else {
      volatile uint16_t *scrptr_p = VIDEO_BUFFER;
      const uint8_t *backdrop_p = &lvl_backdrop[4 + ((gs.d * 3) + (persp == PERSP_ROT1 ? 1 : 0) + (persp == PERSP_ROT2 ? 2 : 0)) * 80];
      
      for (int y = 0; y < 160; y++) {
        fast_copy(scrptr_p, backdrop_p, 224); // 16 bytes left
        ((volatile uint32_t *)scrptr_p)[56] = ((const uint32_t *)backdrop_p)[56];
        ((volatile uint32_t *)scrptr_p)[57] = ((const uint32_t *)backdrop_p)[57];
        ((volatile uint32_t *)scrptr_p)[58] = ((const uint32_t *)backdrop_p)[58];
        ((volatile uint32_t *)scrptr_p)[59] = ((const uint32_t *)backdrop_p)[59];
        scrptr_p += 120;
        backdrop_p += 1200;
      }
  }
}

void drawInstant(int persp) {
  drawBackdrop(persp);
    
  uint32_t polylist_n = render_begin(lvl_bin, lvl_vis_bin, gs.x, gs.y, gs.z, persp, gs.d);
  render_draw(lvl_bin, VIDEO_BUFFER, gs.x, gs.y, gs.z, persp, gs.d, polylist_n, 9999);
  VBlankIntrWait();
  flip_buffer();
}

void drawMovingFrame(int persp) {
  int waitfor = TICKER + MOVE_WAIT;
  drawInstant(persp);
  while (TICKER < waitfor) {
    VBlankIntrWait();
  }
}

int getDirX(uint8_t dir) {
  switch (dir) {
    case DIR_W: return -1; break;
    case DIR_E: return 1; break;
  }
  return 0;
}
int getDirY(uint8_t dir) {
  switch (dir) {
    case DIR_N: return -1; break;
    case DIR_S: return 1; break;
  }
  return 0;
}
int getDirZ(uint8_t dir) {
  switch (dir) {
    case DIR_U: return 1; break;
    case DIR_D: return -1; break;
  }
  return 0;
}
int isLadder(uint8_t tex) {
  if (gs.map == 0)
    if (tex == 14 || tex == 23 || tex == 24 || tex == 26 || tex == 30 || tex == 34 || tex == 35) return 1;
  if (gs.map == 1)
    if (tex == 5 || tex == 7 || tex == 11 || tex == 14) return 1;
  return 0;
}
int isRestoreTile(uint8_t tex) {
  if (gs.map == 0)
    if (tex == 7 || tex == 8 || tex == 10 || tex == 17 || tex == 37) return 1;
  if (gs.map == 1)
    if (tex == 10) return 1;
  return 0;
}
int isOutsideDoor(uint8_t tex) {
  if (gs.map == 0)
    if (tex == 25 || tex == 37 || tex == 38) return 1;
  if (gs.map == 1)
    if (tex == 8 || tex == 16 || tex == 17 || tex == 18) return 1;
  return 0;
}
int isDoor(uint8_t tex) {
  if (gs.map == 0)
    if (tex >= 133 && tex <= 140) return 1;
  return 0;
}
int isSwitch(uint8_t tex) {
  if (gs.map == 0)
    if (tex == 5 || tex == 6 || tex == 8 || tex == 9 || tex == 11 || tex == 12) return 1;
  return 0;
}
int isPlaque(uint8_t tex) {
  if (gs.map == 0)
    if (tex == 15 || tex == 16 || tex == 18 || tex == 36) return 1;
  return 0;
}
uint8_t toggleSwitch(uint8_t tex) {
  int switch_base;
  if (tex == 5 || tex == 6) switch_base = 5;
  if (tex == 8 || tex == 9) switch_base = 8;
  if (tex == 11 || tex == 12) switch_base = 11;
  
  int swplot = -1;
  
  if (gs.plot[PLOT_BASE] >= 1 && gs.plot[PLOT_BASE] <= 3 && gs.x == 6 && gs.y == 13 && gs.z == 0) swplot = 1;
  if (gs.plot[PLOT_BASE] >= 3 && gs.plot[PLOT_BASE] <= 5 && gs.x == 6 && gs.y == 7 && gs.z == 3) swplot = 3;
  if (gs.plot[PLOT_BASE] >= 6 && gs.plot[PLOT_BASE] <= 8 && gs.x == 9 && gs.y == 6 && gs.z == 3) swplot = 6;
  
  int offset = tex - switch_base;
  
  if (swplot != -1) {
    if (offset == 1) {
      // Enable switch
      gs.plot[PLOT_BASE] = swplot + 2;
    } else {
      // Disable switch
      gs.plot[PLOT_BASE] = swplot + 1;
    }
  }
  
  return switch_base + (offset ^ 1);
  
  
  switch (tex) {
    case 5: return 6;
    case 6: return 5;
    case 8: return 9;
    case 9: return 8;
    case 11: return 12;
    case 12: return 11;
  }
  return 0xFF;
}

void moveDir(uint8_t dir) {
  gs.x += getDirX(dir);
  gs.y += getDirY(dir);
  gs.z += getDirZ(dir);
}
uint8_t getTexDirFrom(uint16_t x, uint16_t y, uint16_t z, uint8_t dir) {
  return lvl_bin[(z << 8) | (y << 4) | (x)].texs[dir];
}
uint8_t getTexDir(uint8_t dir) {
  return getTexDirFrom(gs.x, gs.y, gs.z, dir);
}
int isBlocked(uint8_t dir) {
  if (lvl_bin[(gs.z << 8) | (gs.y << 4) | (gs.x)].flags & (128 >> dir)) {
    return 1;
  }
  return 0;
}

int phaseForward() {
  gs.climbing = 0;
  drawMovingFrame(PERSP_STEP1);
  drawMovingFrame(PERSP_STEP2);
  moveDir(gs.d);
  drawMovingFrame(PERSP_FWD);
  return 1;
}

int moveForward() {
  uint8_t dir = gs.d;
  if (!isBlocked(dir)) {
    gs.climbing = 0;
    drawMovingFrame(PERSP_STEP1);
    drawMovingFrame(PERSP_STEP2);
    moveDir(dir);
    drawMovingFrame(PERSP_FWD);
    return 1;
  } else {
    if (isLadder(getTexDir(dir))) {
      if (!isBlocked(DIR_U)) {
        gs.climbing = 1;
        mmEffect(SFX_CLIMB);
        drawMovingFrame(PERSP_UP1);
        drawMovingFrame(PERSP_UP2);
        moveDir(DIR_U);
        drawMovingFrame(PERSP_FWD);
        return 1;
      }
    }
  }
  return 0;
}

int moveBackward() {
  // Special check in case we are on a ladder
  if (gs.climbing) {
    if (isLadder(getTexDir(gs.d)) || isLadder(getTexDirFrom(gs.x, gs.y, gs.z-1, gs.d))) {
      if (!isBlocked(DIR_D)) {
        moveDir(DIR_D);
        mmEffect(SFX_CLIMB);
        drawMovingFrame(PERSP_UP2);
        drawMovingFrame(PERSP_UP1);
        drawMovingFrame(PERSP_FWD);
        return 1;
      }
    }
  }

  uint8_t dir = (gs.d + 2) & 3;
  if (!isBlocked(dir)) {
    gs.climbing = 0;
    moveDir(dir);
    drawMovingFrame(PERSP_STEP2);
    drawMovingFrame(PERSP_STEP1);
    drawMovingFrame(PERSP_FWD);
    return 1;
  }
  return 0;
}

int strafeRight() {
  uint8_t dir = (gs.d + 1) & 3;
  if (!isBlocked(dir)) {
    gs.climbing = 0;
    drawMovingFrame(PERSP_STRAFE1);
    drawMovingFrame(PERSP_STRAFE2);
    moveDir(dir);
    drawMovingFrame(PERSP_FWD);
    return 1;
  }
  return 0;
}

int strafeLeft() {
  uint8_t dir = (gs.d - 1) & 3;
  if (!isBlocked(dir)) {
    gs.climbing = 0;
    moveDir(dir);
    drawMovingFrame(PERSP_STRAFE2);
    drawMovingFrame(PERSP_STRAFE1);
    drawMovingFrame(PERSP_FWD);
    return 1;
  }
  return 0;
}

int turnRight() {
  drawMovingFrame(PERSP_ROT1);
  drawMovingFrame(PERSP_ROT2);
  gs.d = (gs.d + 1) & 3;
  drawMovingFrame(PERSP_FWD);
  
  return 0;
}

int turnLeft() {
  gs.d = (gs.d - 1) & 3;
  drawMovingFrame(PERSP_ROT2);
  drawMovingFrame(PERSP_ROT1);
  drawMovingFrame(PERSP_FWD);
  
  return 0;
}

void fall() {
  moveDir(DIR_D);
  drawMovingFrame(PERSP_UP2);
  drawMovingFrame(PERSP_UP1);
  drawMovingFrame(PERSP_FWD);
  if (isBlocked(DIR_D)) {
    mmEffect(SFX_FALL);
  } else if (isLadder(getTexDir(gs.d))) {
    mmEffect(SFX_CATCHLADDER);
    gs.climbing = 1;
  }
}

void game_over()
{
  CURRENT_MOD = MOD_TEMP9A;
  show_message(-1, "Your inheritor's quest has come to an end.");
  REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
  for (int i = 0; i <= 16; i++) {
    REG_BLDY = i;
    VBlankIntrWait();
    VBlankIntrWait();
    VBlankIntrWait();
    VBlankIntrWait();
  }
  CURRENT_MOD = -1;
}

void victory()
{
  mmEffect(SFX_VICTORY);
  const char *msg = "You were victorious in combat!";
  if (BATTLE_SPOILS[0] != 0xFF) {
    msg = "You were victorious in combat! You have obtained the following:";
  }
  
  show_message(-1, msg);
  int got_eq = 0;
  int got_gem = 0;
  for (int i = 0; i < 6; i++) {
    uint8_t ico = BATTLE_SPOILS[i];
    if (ico != 0xFF) {
      if (ico >= 80) { // equipment
        gs.equip_qty[ico - 80]++;
        got_eq = 1;
      } else if (ico >= 40) { // soulstone
        gs.gem_qty[ico - 40]++;
        got_gem = 1;
      }
    }
    BATTLE_SPOILS[i] = 0xFF;
  }
  
  if (got_gem && !(gs.plot[PLOT_TUT] & TUTORIAL_FIRSTGEM)) {
    gs.plot[PLOT_TUT] |= TUTORIAL_FIRSTGEM;
    
    show_message(1, "Is this...?");
    show_message(0, "Correct, this is what we came here for. The inert form of these spirit creatures; spirit crystals.");
    show_message(1, "Fantastic; we can use these to gain some of the powers of these spirit creatures. They will also be invaluable treasures to bring back to the clan.");
    show_message(-1, "(Press START to open up the menu and select the second option to equip your party with spirit crystals. Spirit crystals improve your stats and give you access to more techniques.)");
  }
  if (got_eq && !(gs.plot[PLOT_TUT] & TUTORIAL_FIRSTEQUIP)) {
    
    gs.plot[PLOT_TUT] |= TUTORIAL_FIRSTEQUIP;
    show_message(1, "That creature seems to have dropped something...");
    show_message(0, "Yes, this is a fantastic find. The latent Qi in this place has coalesced into godrelics. Powerful equipment that will help us survive in here.");
    show_message(1, "Indeed, this is a fortunate discovery. We should equip it at once.");
    show_message(-1, "(Press START to open up the menu and select the first option to equip your party with godrelics. Godrelics can improve your combat potential, reduce the damage you take and more.)");
  }
  
  mmEffectCancelAll();
}

int openDoor(uint8_t dir)
{
  // Is this door locked?
  int plotdoor = -1;
  if ((gs.x == 6) && (gs.y == 10) && (gs.z == 2)) plotdoor = 1;
  if ((gs.x == 5) && (gs.y == 11) && (gs.z == 3)) plotdoor = 3;
  if ((gs.x == 8) && (gs.y == 8) && (gs.z == 5)) plotdoor = 6;
  if (gs.plot[PLOT_BASE] == plotdoor || gs.plot[PLOT_BASE] == plotdoor + 1) {
    mmEffect(SFX_LOCKED);
    if (gs.plot[PLOT_BASE] == 1 || gs.plot[PLOT_BASE] == 3 || gs.plot[PLOT_BASE] == 6) {
      for (int i = 0; i < 16; i++) {
        VBlankIntrWait();
      }
    }
    switch (gs.plot[PLOT_BASE]) {
      case 1:
        show_message(0, "A locked door...");
        show_message(1, "We should look around and see if there is something that unlocks this. The antecedents were very fond of their elaborate mechanical contraptions and devices.");
        gs.plot[PLOT_BASE]++;
      break;
      case 3:
        show_message(0, "Looks like another locked door. We should search for another switch or button, see if something opens this.");
        gs.plot[PLOT_BASE]++;
      break;
      case 6:
        show_message(1, "Locked again...");
        show_message(0, "This is starting to get old. Did you see anything on the wall that we might have missed on the way here?");
        show_message(1, "I don't think so, but we should certainly look around.");
        gs.plot[PLOT_BASE]++;
      break;
      default:
        break;
    }
    
    return 0;
  }
  
  mmEffect(SFX_DOOR);
  int x = gs.x;
  int y = gs.y;
  int z = gs.z;
  lvl_bin[(z << 8) | (y << 4) | (x)].texs[dir] += 8;
  lvl_bin[(z << 8) | (y << 4) | (x)].flags &= ~(128 >> dir);

  drawInstant(PERSP_FWD);
  for (int i = 0; i <= 16; i++) {
    VBlankIntrWait();
  }
  
  moveForward();
  lvl_bin[(z << 8) | (y << 4) | (x)].texs[dir] -= 8;
  lvl_bin[(z << 8) | (y << 4) | (x)].flags |= (128 >> dir);
  
  return 1;
}

int openOutsideDoor(uint8_t dir)
{
  mmEffect(SFX_DOOR);

  drawInstant(PERSP_STEP1);
  for (int i = 0; i <= MOVE_WAIT; i++) VBlankIntrWait();
  REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
  for (int i = 0; i <= 16; i++) {
    REG_BLDY = i;
    VBlankIntrWait();
  }
  gs.map ^= 1;
  loadMap();
  gs.x += getDirX(dir);
  gs.y += getDirY(dir);
  gs.z += getDirZ(dir);
  drawInstant(PERSP_FWD);

  for (int i = 0; i <= 16; i++) {
    REG_BLDY = 16 - i;
    VBlankIntrWait();
  }
  
  return 1;
}

int interactEnv()
{
  uint8_t dir = gs.d;
  if (isBlocked(dir)) {
    uint8_t t = getTexDir(dir);
    if (isLadder(t)) return moveForward();
    if (isDoor(t)) return openDoor(dir);
    if (isOutsideDoor(t)) return openOutsideDoor(dir);
    if (isSwitch(t)) {
      mmEffect(SFX_CLICK);
      lvl_bin[(gs.z << 8) | (gs.y << 4) | (gs.x)].texs[dir] = toggleSwitch(t);
      drawInstant(PERSP_FWD);
    }
    if (isPlaque(t)) {
      show_message(-1, "The mysterious plaque evades your attempts at comprehension.");
    }
  }
  return 0;
}

int after_boss()
{
  if (BATTLE_OUTCOME == -1) {
    game_over();
    return 1;
  }
  if (BATTLE_OUTCOME == 1) {
    victory();
  }
  CURRENT_MOD = GAMEPLAY_MUS;
  return 0;
}

static int fuzz = 0;
__attribute__((section(".iwram"), long_call, target("arm")))
void hblank()
{
  REG_BLDY = max((fuzz - ((REG_VCOUNT - fuzz) & 0x1F)) >> 3, 0);
  REG_BG2X = ((((((REG_VCOUNT + fuzz) & 0xF) - 8)) << 4)) * fuzz;
}

void gameplay() {
  mmSetModuleVolume(0);
  GAMEPLAY_MUS = MOD_TEMP31;
  BATTLE_OUTCOME = 0;
  CURRENT_MOD = -1;
  
  drawInstant(PERSP_FWD);
  if (NEW_GAME) {
    mmEffect(SFX_WARP);
    irq_add(II_HBLANK, hblank);
    irq_enable(II_HBLANK);
    for (int i = 0; i <= 128; i++) {
      
      fuzz = 128 - i;
      REG_BLDY = (128 - i) / 8;
      if (i == 112) CURRENT_MOD = GAMEPLAY_MUS;
      mmSetModuleVolume(max(i * 16 - 1792, 0));
      VBlankIntrWait();
    }
    REG_BG2PA = 256;
    irq_disable(II_HBLANK);
  } else {
    CURRENT_MOD = GAMEPLAY_MUS;
    for (int i = 0; i <= 16; i++) {
      REG_BLDY = (16 - i);
      mmSetModuleVolume(i * 16);
      VBlankIntrWait();
    }
  }
  for (;;) { // Main loop
    if (show_loc) {
      // Show coords
      
      for (int i = 0; i < 7; i++) obj_hide(&oam_mem[i]);
      int val = gs.x;
      if (val / 10 != 0) OBJSET(0, 6 + val / 10, 2, 0, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      OBJSET(1, 6 + val % 10, 2, 6, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      val = gs.y;
      if (val / 10 != 0) OBJSET(2, 6 + val / 10, 2, 20, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      OBJSET(3, 6 + val % 10, 2, 26, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      val = gs.z;
      if (val / 10 != 0) OBJSET(4, 6 + val / 10, 2, 40, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      OBJSET(5, 6 + val % 10, 2, 46, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      val = gs.d;
      OBJSET(6, 6 + val, 2, 60, 0, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
      
      
    }
    // Use up a random number
    rand();
    
    // First, check if we are falling
    if (!gs.climbing && !isBlocked(DIR_D)) {
      fall();
      continue;
    }
    int old_x = gs.x;
    int old_y = gs.y;
    int moved = 0;
    if (INKEY_PRESSED & KEY_A) moved = interactEnv();
    else if (INKEY & KEY_UP) moved = moveForward();
    else if (INKEY & KEY_DOWN) moved = moveBackward();
    else if (INKEY & KB_TURNLEFT) moved = turnLeft();
    else if (INKEY & KB_TURNRIGHT) moved = turnRight();
    else if (INKEY & KB_STRAFELEFT) moved = strafeLeft();
    else if (INKEY & KB_STRAFERIGHT) moved = strafeRight();
    else if (INKEY & KEY_START) open_status();
    else if (INKEY & KEY_B) open_status();
    
    if (moved) {
      if (old_x != gs.x || old_y != gs.y) {
        if (isRestoreTile(getTexDir(DIR_D))) {
          mmEffect(SFX_HEAL);
          for (int i = 0; i < 2; i++) {
            gs.pl[i].hp = gs.pl[i].maxhp;
            gs.pl[i].mp = gs.pl[i].maxmp;
          }
          show_message(-1, "Stepping on the tile has healed and reenergised you.");
        }
        if (plot_handle() == 1) return;
        if (!skip_encounters) {
          gs.plot[PLOT_ENC]--;
          if (gs.plot[PLOT_ENC] <= 0) {
            gs.plot[PLOT_ENC] = 10 + rand() % 30;
            BATTLE_OUTCOME = 0;
            random_encounter();
            if (BATTLE_OUTCOME == -1) {
              game_over();
              return;
            }
            
            if (BATTLE_OUTCOME == 1) {
              victory();
            }
            CURRENT_MOD = GAMEPLAY_MUS;
          }
        }
      }
    }
    
    VBlankIntrWait();
  }
}
