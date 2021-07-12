#include <stdint.h>
#include <stdlib.h>

#include <tonc_memmap.h>
#include <tonc_memdef.h>
#include <tonc_irq.h>
#include <tonc_bios.h>
#include <tonc_oam.h>
#include <maxmod.h>

#include "soundbank_bin.h"
#include "btl_objpal_bin.h"
#include "btl_objmap_bin.h"

#include "renderer.h"
#include "pdata.h"
#include "gameplay.h"
#include "macros.h"
#include "fast_set.h"
#include "saves.h"
#include "title.h"
#include "video.h"

struct pd_gamestate gs;

int16_t CURRENT_MOD = -1;
int16_t curr_playing_mod = -1;
uint8_t RELOAD_PALETTE = 0;
uint32_t TICKER = 0;
uint16_t INKEY = 0, INKEY_PRESSED = 0;

uint8_t CMEM_ON = 1;
uint16_t KB_TURNLEFT = KEY_LEFT;
uint16_t KB_TURNRIGHT = KEY_RIGHT;
uint16_t KB_STRAFELEFT = KEY_L;
uint16_t KB_STRAFERIGHT = KEY_R;

uint16_t inkey_last = 0;

__attribute__((section(".iwram"), long_call, target("arm")))
void irqvector()
{
  mmFrame();
  inkey_last = INKEY;
  INKEY = ~REG_KEYINPUT;
  INKEY_PRESSED = INKEY & ~inkey_last;
  if (CURRENT_MOD != curr_playing_mod) {
    curr_playing_mod = CURRENT_MOD;
    if (mmActive()) mmStop();
    if (CURRENT_MOD != -1)
      mmStart(CURRENT_MOD, MM_PLAY_LOOP);
  }
  TICKER++;
}

void initCnt()
{
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ_2D | DCNT_OBJ;
  REG_WAITCNT = WS_PREFETCH | WS_ROM2_N8 | WS_ROM0_S1 | WS_ROM0_N3 | WS_SRAM_8; // 0x4317, suitable for actual hw
  
  oam_init(obj_mem, 128);
}

void initIrq()
{
  IRQ_INIT();
  irq_add(II_VBLANK, mmVBlank);
  irq_enable(II_VBLANK);
}

static void *maxmod_buffer = NULL;
static uint8_t maxmod_mix_memory[MM_MIXLEN_31KHZ];

void initMaxmod()
{
  if (maxmod_buffer != NULL)
    free(maxmod_buffer);

  uint8_t channels = 8;
  uint16_t mix_len = MM_MIXLEN_31KHZ;
  mm_gba_system sys;

  maxmod_buffer = malloc(mix_len + (channels * (MM_SIZEOF_MODCH + MM_SIZEOF_ACTCH + MM_SIZEOF_MIXCH)));

  sys.mixing_mode = MM_MIX_31KHZ;
  sys.mod_channel_count = channels;
  sys.mix_channel_count = channels;
  sys.wave_memory = (mm_addr) (maxmod_buffer);
  sys.module_channels = (mm_addr) (maxmod_buffer + mix_len);
  sys.active_channels = (mm_addr) (maxmod_buffer + mix_len + (channels * (MM_SIZEOF_MODCH)));
  sys.mixing_channels = (mm_addr) (maxmod_buffer + mix_len + (channels * (MM_SIZEOF_MODCH + MM_SIZEOF_ACTCH)));
  sys.mixing_memory = maxmod_mix_memory;
  sys.soundbank = (mm_addr)soundbank_bin;

  mmInit(&sys);
  mmSetVBlankHandler(irqvector);
  mmSetModuleVolume(256);
}

void initPdata()
{
  for (int i = 0; i < PLAYERS_MAX; i++) {
    for (int j = 0; j < EQUIP_MAX; j++)
      gs.pl[i].equip[j] = 255;
    for (int j = 0; j < STATUS_MAX; j++)
      gs.pl[i].status[j] = 0;
    for (int j = 0; j < SLOTS_MAX; j++)
      gs.pl[i].slots[j] = 255;
  }
  gs.x = 9;
  gs.y = 9;
  gs.z = 1;
  gs.d = DIR_W;
  gs.map = 0;
  gs.climbing = 0;
  
  gs.pl[0].pid = 0;
  gs.pl[0].maxhp = 100;
  gs.pl[0].maxmp = 20;
  gs.pl[0].str = 30;
  gs.pl[0].def = 15;
  gs.pl[0].agi = 20;
  gs.pl[0].mag = 10;
  gs.pl[0].equip[EQUIP_WEAPON] = 26;
  
  gs.pl[1].pid = 1;
  gs.pl[1].maxhp = 80;
  gs.pl[1].maxmp = 60;
  gs.pl[1].str = 15;
  gs.pl[1].def = 10;
  gs.pl[1].agi = 15;
  gs.pl[1].mag = 30;
  gs.pl[1].equip[EQUIP_WEAPON] = 20;
  
  for (int i = 0; i < PLAYERS_MAX; i++) {
    gs.pl[i].hp = gs.pl[i].maxhp;
    gs.pl[i].mp = gs.pl[i].maxmp;
  }
  
  for (int i = 0; i < PLOT_MAX; i++) {
    gs.plot[i] = 0;
  }
  gs.plot[PLOT_ELVL] = 4;
  gs.plot[PLOT_ENC] = 8 + rand() % 10;
  /*
  gs.plot[PLOT_BASE] = 5;
  gs.x = 12;
  gs.y = 2;
  gs.z = 1;
  */
  /*
  gs.equip_qty[0] = 1;
  gs.equip_qty[1] = 1;
  gs.equip_qty[2] = 1;
  gs.equip_qty[3] = 1;
  gs.equip_qty[4] = 1;
  gs.equip_qty[5] = 1;
  gs.gem_qty[2] = 1;
  gs.gem_qty[3] = 1;
  */
}

void initVmem()
{
  fast_copy(OBJ_PAL(0), btl_objpal_bin + (COLSPACE << 9), 512);
  fast_copy(OBJ_CHR(0, 0), btl_objmap_bin, 8192);
}

int main()
{
  int skip_titlescreen = 0;
	REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_WHITE;
	REG_BLDY = 16;
	
  initCnt();
  initIrq();
  initMaxmod();
  load_config();
  if (!skip_titlescreen) logo_screen();

  for (;;) {
    first_buffer();
    initPdata();
    initVmem();
    if (!skip_titlescreen) title_screen();
    initVmem();
    loadMap();
    gameplay();
  }
}
