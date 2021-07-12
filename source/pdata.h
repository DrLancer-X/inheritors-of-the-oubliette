#pragma once
#include "renderer.h"

#define STONE_STAT_CALC(x) ((x) >> 1)

#define EQUIP_WEAPON 0
#define EQUIP_BODY 1
#define EQUIP_HEAD 2
#define EQUIP_ACC 3
#define EQUIP_MAX 4

#define STATUS_POISON 0
#define STATUS_BADPOISON 1
#define STATUS_PARA 2
#define STATUS_WEAK 3
#define STATUS_BARRIER 4
#define STATUS_REGEN 5
#define STATUS_FAST 6
#define STATUS_INVISO 7
#define STATUS_MAX 8

#define PALEFFECT_ENEMYFLASH 8
#define PALEFFECT_DAMAGE 9

/*0 STATUS_POISON - Lose health every turn
1 STATUS_BADPOISON - Lose more health every turn
2 STATUS_PARA - Paralysis (40% of moves fail)
3 STATUS_WEAK - Takes extra damage

4 STATUS_BARRIER - Takes less damage
5 STATUS_REGEN - Regains health per turn
6 STATUS_FAST - Extra turns
7 STATUS_INVISO - Harder to hit*/

#define SLOTS_MAX 4
#define PLOT_MAX 8

#define INV_MAX 32
#define PLAYERS_MAX 2

#define ICONS_MAX 5

#define CMEM_SCUR 0
#define CMEM_SPLR 1
#define CMEM_SEQUIPA 2
#define CMEM_SEQUIPB 4
// 5-11 are used
#define CMEM_SSTONEA 12
#define CMEM_SSTONEB 14
// 15-21 are used
#define CMEM_STECHSA 22
// 23 is used
#define CMEM_SAVESLOT 24
#define CMEM_CONFIG 25
#define CMEM_BCUR 26
// 27 is used
#define CMEM_BTARG 28
// 29-31 used
#define CMEM_BTEC 32
// 33 used
#define CMEM_STECP 34
// 35 used
#define CMEM_MAX 64

// All data about a player that needs to be persisted
struct pd_player {
  uint8_t pid;
  
  int16_t hp;
  int16_t maxhp;
  int16_t mp;
  int16_t maxmp;
  int16_t str;
  int16_t def;
  int16_t agi;
  int16_t mag;
  
  uint8_t equip[EQUIP_MAX];
  uint8_t status[STATUS_MAX];
  uint8_t slots[SLOTS_MAX];
};

struct pd_gamestate {
  uint8_t x, y, z, d, map;
  uint8_t climbing;
  struct pd_player pl[PLAYERS_MAX];
  uint8_t equip_qty[INV_MAX];
  uint8_t gem_qty[INV_MAX];
  uint8_t plot[PLOT_MAX];
  uint32_t ticks;
  uint8_t cmem[CMEM_MAX];
};

#define PLOT_BASE 0 // basic progression
#define PLOT_ELVL 6 // max encountered enemy
#define PLOT_ENC 7 // random encounters

extern uint8_t CMEM_ON;
#define CMEM gs.cmem

//---end persistable data--

extern struct pd_gamestate gs;
extern int16_t CURRENT_MOD;
extern uint32_t TICKER;
extern uint16_t INKEY, INKEY_PRESSED;
extern uint16_t KB_TURNLEFT, KB_TURNRIGHT, KB_STRAFELEFT, KB_STRAFERIGHT;
extern uint8_t RELOAD_PALETTE;
