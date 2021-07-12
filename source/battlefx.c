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
#include "battle.h"
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

static int anim_timer = -1;
static int t;
static int x1, y1, x2, y2;
static int id;
static int cx, cy;
static struct actor *user, *targ;
static uint8_t tech;
static uint8_t *sp;
static struct {
	int x, y, digit, life, green;
} *digits = NULL;
static int digits_n = 0;

static int anim_model; // 0 - melee. 1 - offensive tech. 2 - beneficial tech

static int targ_hit = 0;

#define ODRW(sx, sy, dx, dy, pal, attr0, attr1, attr2) \
do { \
	if ((dx) >= 0 && (dy) >= 0 && (dx) < 240 && (dy) < 160 && *sp != 0xFF) { \
	  OBJSET(*sp, (sx), (sy), (dx), (dy), (pal), (attr0), (attr1), (attr2)); \
	  (*sp)--; \
	} \
} while(0);

#define RAND(x, y) (rand() % ((y) - (x) + 1) + x)

static void add_digits(int x, int y, int v)
{
	int green = 0;
	if (v < 0) {
		v = -v;
		green = 1;
	}
	x -= 4;
	if (v >= 10) x += 4;
	if (v >= 100) x += 4;
	if (v >= 1000) x += 4;
	int i = 0;
	for (;;) {
		digits[digits_n].x = x;
		digits[digits_n].y = y - i;
		digits[digits_n].digit = v % 10;
		digits[digits_n].life = 40 - (i * 2);
		digits[digits_n].green = green;
		x -= 8;
		digits_n++;
		i++;
		v /= 10;
		if (v == 0) break;
	}
}
static void add_miss(int x, int y)
{
	digits[digits_n].x = x - 8;
	digits[digits_n].y = y - 4;
	digits[digits_n].digit = -1;
	digits[digits_n].life = 40;
	digits[digits_n].green = 0;
	digits_n++;
}

int get_weapon()
{
	return user->p->equip[EQUIP_WEAPON];
}

void get_base_stats(int *acc, int *dmg, int *elem, int *inflict)
{
	*elem = 0xFF;
	*inflict = 0xFF;
	if (tech != 0xFF) {
		*acc = 100;
		*elem = NMEDATA[tech].spdata[SP_ELEM];
		*dmg = NMEDATA[tech].spdata[SP_DAMAGE];
		*inflict = NMEDATA[tech].spdata[SP_INFLICT];
	} else { // Weapon
		int w = get_weapon();
		if (w == 0xFF) {
			// Bare handed
			*acc = 90;
			*dmg = 5;
			*elem = 0xFF;
			*inflict = 0xFF;
		} else {
			*dmg = NMEDATA[w].eqatt[EQATT_DMG];
			*acc = NMEDATA[w].eqatt[EQATT_PCT];
			*elem = NMEDATA[w].eqatt[EQATT_STR];
			*inflict = NMEDATA[w].eqatt[EQATT_WEAK];
		}
	}
		
}

static void inflict_status(int dmg, int inflict)
{
	if (inflict == 0xFF) return;
	
	if (get_acc(targ) == ACC_POISONRING && inflict <= 1) return;
	if (get_acc(targ) == ACC_AMULET && inflict <= 3) return;
	
	int chance = 50;
	// Positive status effects always work
	if (inflict >= 4 && inflict <= 7) chance = 100;
	// Non-damaging effects always work
	if (dmg == 0) chance = 100;
	
	int roll = (rand() % 100) + 1;
	if (roll <= chance) {
		// 5 rounds of status
		targ->p->status[inflict] = 5;
	}
}
static void non_damaging(int dmg, int inflict) {
	if (tech == 0xFF) return; // should not happen
	
	// Healing-type skills
	
	if (NMEDATA[tech].spdata[SP_CURE]) {
		// Remove negative statuses
		for (int i = 0; i < 4; i++) {
			targ->p->status[i] = 0;
		}
	}
	if (NMEDATA[tech].spdata[SP_CURE]) {
		// Remove negative statuses
		for (int i = 0; i < 4; i++) {
			targ->p->status[i] = 0;
		}
	}
	if (NMEDATA[tech].spdata[SP_REVIVE]) {
		if (targ->p->hp == 0) targ->p->hp = 1;
	}
	if (NMEDATA[tech].spdata[SP_HEAL]) {
		int heal_amt = dmg;
		int skill = get_mag(user->id + user->side * 2);
	
		heal_amt = heal_amt * (100 + skill) / 100;
		
		add_digits(cx, cy, -heal_amt);
		targ->p->hp = min(targ->p->hp + heal_amt, targ->p->maxhp);
		targ_hit = 1;
		mmEffect(SFX_HEAL);
	}
	inflict_status(dmg, inflict);
	
	
}
static void damage()
{
	int acc, dmg, elem, inflict;
	get_base_stats(&acc, &dmg, &elem, &inflict);
	if (dmg == 0 || (tech != 0xFF && NMEDATA[tech].spdata[SP_HEAL])) {
		non_damaging(dmg, inflict);
		return;
	}
	// Damage increased by skill (str or mag)
	int skill;
	if (tech == 0xFF) skill = get_str(user->id + user->side * 2);
	else skill = get_mag(user->id + user->side * 2);
	
	dmg = dmg * (100 + skill) / 100;
	if (elem != 0xFF) {
		// Damage multiplied by weakness, reduced by strength
		if (targ->e) {
			if (targ->e->weak[elem] == 1) dmg = dmg * 25 / 10;
			if (targ->e->weak[elem] == -1) dmg = dmg * 4 / 10;
		}
		for (int i = EQUIP_BODY; i <= EQUIP_HEAD; i++) {
			int eq = targ->p->equip[i]; // Armour and helms
			if (eq != 0xFF) {
				if (NMEDATA[eq].eqatt[EQATT_WEAK] == elem) dmg = dmg * 25 / 10;
				if (NMEDATA[eq].eqatt[EQATT_STR] == elem) dmg = dmg * 4 / 10;
			}
		}
	}
	
	// Accuracy increased by agility
	int inacc = 100 - acc;
	inacc = inacc * 100 / (100 + get_agi(user->id + user->side * 2));
	
	// Damage reduced by defense
	int def = get_def(id);
	
	// Add armour bonuses
	{
		for (int i = EQUIP_BODY; i <= EQUIP_HEAD; i++) {
			int eq = targ->p->equip[i]; // Armour and helms
			if (eq != 0xFF) {
				def += NMEDATA[eq].eqatt[EQATT_DMG];
				inacc += NMEDATA[eq].eqatt[EQATT_PCT];
			}
		}
	}
	
	dmg = dmg * 100 / (100 + def);
	
	if (targ->p->status[STATUS_WEAK] > 0) dmg *= 2;
	if (targ->p->status[STATUS_BARRIER] > 0) dmg /= 2;
	if (targ->defending) dmg /= 2;
	
	// Target agility
	inacc = inacc * (10 + get_agi(targ->id + targ->side * 2)) / 10;
	
	// Inviso
	if (targ->p->status[STATUS_INVISO] > 0) inacc = 100 - ((100 - inacc) / 2);
	
	// Did we hit?
	int roll = rand() % 100;
	if (roll >= inacc) {
		// Hit
		// Do we inflict a status effect?
		inflict_status(dmg, inflict);
		add_digits(cx, cy, dmg);
		targ->p->hp = max(targ->p->hp - dmg, 0);
		targ_hit = 1;
		mmEffect(SFX_MELEEHIT);
	} else {
		add_miss(cx, cy);
	}
}

static void redden_palette()
{
	paleffect(8 + id, PALEFFECT_DAMAGE);
/*
	volatile uint16_t *p = OBJ_PAL(8 + id);
	for (int i = 0; i < 16; i++) {
		*p = *p & 31;
		p++;
	}
*/
	
}

static void target_healed()
{
	if (!targ_hit) return;
	
	paleffect(8 + id, STATUS_REGEN);
	if (id >= 2) {
		uint8_t sp = 84 + id - 2;
		uint8_t shsp = 90 + id - 2;
		int xoff = RAND(-2, 2);
		int yoff = RAND(-2, 2);
		BFN_SET(obj_mem[sp].attr0, BFN_GET(obj_mem[sp].attr0, ATTR0_Y) + yoff, ATTR0_Y);
		BFN_SET(obj_mem[sp].attr1, BFN_GET(obj_mem[sp].attr1, ATTR1_X) + xoff, ATTR1_X);
		BFN_SET(obj_mem[shsp].attr1, BFN_GET(obj_mem[shsp].attr1, ATTR1_X) + xoff, ATTR1_X);
	}
}

static void target_hurt()
{
	if (!targ_hit) return;
	
	redden_palette();
	if (id >= 2) {
		uint8_t sp = 84 + id - 2;
		uint8_t shsp = 90 + id - 2;
		int xoff = RAND(-2, 2);
		int yoff = RAND(-2, 2);
		BFN_SET(obj_mem[sp].attr0, BFN_GET(obj_mem[sp].attr0, ATTR0_Y) + yoff, ATTR0_Y);
		BFN_SET(obj_mem[sp].attr1, BFN_GET(obj_mem[sp].attr1, ATTR1_X) + xoff, ATTR1_X);
		BFN_SET(obj_mem[shsp].attr1, BFN_GET(obj_mem[shsp].attr1, ATTR1_X) + xoff, ATTR1_X);
	}
}

static int plain_atk()
{
	if (t == 0) {
		mmEffect(SFX_SWORD5);
	}
	if (t == 15) {
		damage();
	}
  if (t < 30) {
		if (t >= 15 && t < 30) target_hurt();
    ODRW(RAND(19, 26), 2, cx - 12 - 30 + (t * 2), cy - 12 - 30 + (t * 2), 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 26), 2, cx - 4 - 30 + (t * 2), cy - 4 - 30 + (t * 2), 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 26), 2, cx + 4 - 30 + (t * 2), cy + 4 - 30 + (t * 2), 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    return 0;
  } else {
    return 1;
  }
}

static int tech_atk()
{
	if (t == 0) {
		mmEffect(SFX_TECH);
	}
	if (t == 15) {
		damage();
	}
  if (t < 30) {
		if (t >= 15 && t < 30) target_hurt();
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    return 0;
  } else {
    return 1;
  }
}

static int heal_atk()
{
	if (t == 0) {
		mmEffect(SFX_CURE);
	}
	if (t == 15) {
		damage();
	}
  if (t < 30) {
		if (t >= 15 && t < 30) target_healed();
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    ODRW(RAND(19, 31), 1, cx + RAND(-30,30) - 4, cy + RAND(-30,30) - 4, 4, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
    return 0;
  } else {
    return 1;
  }
}

static int cont_atk()
{
	if (anim_model == 0) return plain_atk();
	else if (anim_model == 1) return tech_atk();
	else if (anim_model == 2) return heal_atk();
	else return plain_atk();
}

static void draw_digits()
{
	for (int i = 0; i < digits_n; i++) {
		if (digits[i].life > 0) {
			if (digits[i].digit == -1) {
				ODRW(14, 3, digits[i].x, digits[i].y + 10 - digits[i].life / 2, 1, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
			} else {
				ODRW(6+digits[i].digit, 2, digits[i].x, digits[i].y + 10 - digits[i].life / 2, digits[i].green ? 2 : 1, ATTR0_SQUARE, ATTR1_SIZE_8x8, 0);
			}
			digits[i].life--;
		}
	}
}

void elem_col()
{
	anim_model = 0;
	if (tech != 0xFF) anim_model = 1;
	
	int colcode = -1;
	int acc, dmg, elem, inflict;
	get_base_stats(&acc, &dmg, &elem, &inflict);
	switch (elem) {
		case 0: colcode = PALEFFECT_DAMAGE; break;
		case 1: colcode = STATUS_WEAK; break;
		case 2: colcode = STATUS_PARA; break;
		case 3: colcode = STATUS_INVISO; break;
	}
	
	int good = 0;
	if (tech != 0xFF) {
		if (NMEDATA[tech].spdata[SP_INFLICT] >= 4 && NMEDATA[tech].spdata[SP_INFLICT] <= 7) good = 1;
		if (NMEDATA[tech].spdata[SP_CURE] > 0) good = 1;
		if (NMEDATA[tech].spdata[SP_HEAL] > 0) good = 1;
		if (NMEDATA[tech].spdata[SP_REVIVE] > 0) good = 1;
	}
	if (good) {
		anim_model = 2;
		colcode = STATUS_POISON;
	}
	fast_copy(OBJ_PAL(4), btl_objpal_bin + (4 << 5) + (COLSPACE << 9), 32);
	paleffect(4, colcode);
}

int use_tech_indiv(int anim_timer_, int t_, struct actor *user_, struct actor *targ_, uint8_t tech_, uint8_t *sp_)
{
	if (!digits) {
		digits = malloc(sizeof(digits[0]) * 64);
		memset(digits, 0, sizeof(digits[0]) * 64);
	}
	int new_tick = (anim_timer != anim_timer_);
	anim_timer = anim_timer_;
	t = t_;
	tech = tech_;
	sp = sp_;
	user = user_;
	targ = targ_;
  id = targ->id + (targ->side * 2);
  get_bounds(id, &x1, &y1, &x2, &y2);
  cx = (x1 + x2) / 2;
  cy = (y1 + y2) / 2;
  
  elem_col();
  
  if (anim_timer == 0) {
		targ_hit = 0;
		digits_n = 0;
	}
	
	if (new_tick) draw_digits();
  
	int r = cont_atk();
	if (r == 1) {
		// Waiting for digits to finish displaying
		for (int i = 0; i < digits_n; i++) {
			if (digits[i].life > 0) return 0;
		}
		return 1;
	}

	return 0;
}
