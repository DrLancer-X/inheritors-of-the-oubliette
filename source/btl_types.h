#pragma once
#include <stdint.h>
#include "btl_extypes.h"
#include "btl_nmedata_bin.h"
#include "pdata.h"

#define NMEDATA ((const struct btl_enemy *)btl_nmedata_bin)
#define NMEDATA_COUNT (btl_nmedata_bin_size / sizeof(struct btl_enemy))

#define EQATT_DMG 0
#define EQATT_PCT 1
#define EQATT_STR 2
#define EQATT_WEAK 3

#define SP_COST 0
#define SP_DAMAGE 1
#define SP_ELEM 2
#define SP_INFLICT 3
#define SP_ALL 4
#define SP_CURE 5
#define SP_REVIVE 6
#define SP_HEAL 7

#define ACC_POISONRING 0
#define ACC_MPREGEN 1
#define ACC_SPEEDBOOTS 2
#define ACC_AMULET 3
#define ACC_MPMAX 4

struct actor {
  uint8_t active; // 0 - no. 1 - yes
  uint8_t id; // 0-1 for player, 0-5 for enemies. 0-5 for enemies must match 2-7 in actor list
  uint8_t side; // 0 - player, 1 - enemies
  uint8_t refx, flip, tick, var;
  struct pd_player *p;
  const struct btl_enemy *e;
  uint8_t x, y;
  uint8_t defending;
};

/*
Weapon (0)
eq_power: Damage
eq_chance: Accuracy
eq_strong: Element
eq_weak: Inflicts

Armour (1)
eq_power: Defense
eq_chance: Dodge
eq_strong: Strong against element
eq_weak: Weak against element

Helmet (2)
eq_power: Defense
eq_chance: Dodge
eq_strong: Strong against element
eq_weak: Weak against element

Accessory (3)
eq_power: Type
eq_chance: 
eq_strong: 
eq_weak:
*/
