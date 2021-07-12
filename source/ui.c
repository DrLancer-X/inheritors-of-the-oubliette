#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tonc_memmap.h>
#include <tonc_memdef.h>
#include <tonc_bios.h>
#include <tonc_oam.h>
#include <maxmod.h>
#include "soundbank.h"
#include "macros.h"
#include "font.h"
#include "fast_set.h"
#include "ui.h"
#include "pdata.h"
#include "saves.h"
#include "textfn.h"
#include "btl_types.h"
#include "textconsts.h"
#include "battle.h"
#include "video.h"

#include "btl_objmap_bin.h"
#include "btl_uimap_bin.h"
#include "btl_charapal_bin.h"
#include "btl_charagfx_bin.h"
#include "btl_menuicons_bin.h"
#include "btl_nums_bin.h"
#include "btl_nmedata_bin.h"

void wrap_text(int pos_x, int pos_y, const int *linelim, int linespacing, const char *text)
{
	char linbuf[100];
	int line = 0;
	int linelen = 0;
	int lim = linelim[0];
	int draw_y = pos_y;
	const char *linebegin = text;
	const char *p = linebegin;
	const char *lastspace = NULL;
	
	while (*p != '\0') {
		if (*p == ' ') {
			 lastspace = p;
		}
		linelen += GLYPH_W[*p - ' '];
		if (linelen > lim) {
			memcpy(linbuf, linebegin, lastspace - linebegin);
			linbuf[lastspace - linebegin] = '\0';
			draw_text(pos_x - lim, draw_y, linbuf);
			draw_y += linespacing;
			line++;
			lim = linelim[line];
			linebegin = lastspace + 1;
			p = linebegin;
			lastspace = NULL;
			linelen = 0;
			continue;
		}
		p++;
	}
	memcpy(linbuf, linebegin, p - linebegin);
	linbuf[p - linebegin] = '\0';
	draw_text(pos_x - lim, draw_y, linbuf);
}
void show_message(int portrait, const char *text)
{
  //fast_copy(OBJ_PAL(0), btl_objpal_bin + (COLSPACE << 9), 512);
  
  const int linelim_dlg[] = {168, 168, 184, 184, 184, 184};
  const int linelim_txt[] = {184, 184, 184, 184, 184, 184};
  const int *linelim;
  int linespacing;
  if (portrait == -1) { // Text box
		linelim = linelim_txt;
		linespacing = 8;
		fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (1 << 13), 8192);
  } else { // Dialogue box
		linelim = linelim_dlg;
		linespacing = 8;
		fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (0 << 13), 8192);
  }
  
  wrap_text(188, 68, linelim, linespacing, text);
  
  if (portrait >= 0) {
    // Portrait
    fast_copy(OBJ_PAL(8), btl_charapal_bin + (portrait<<7) + (COLSPACE << 5), 32);
    fast_copy(OBJ_CHR(4, 4), btl_charagfx_bin + (portrait<<7) + 0, 64);
    fast_copy(OBJ_CHR(4, 5), btl_charagfx_bin + (portrait<<7) + 64, 64);
  }
  
  // Now wait for vblank
  VBlankIntrWait();
  
  if (portrait >= 0)
		OBJSET(12, 4, 4, 24+1, 48+1, 8, ATTR0_SQUARE | ATTR0_BLEND | ATTR0_AFF, ATTR1_SIZE_16x16 | ATTR1_AFF_ID(8), 0);
  
  // Message box
  OBJSET(15, 0, 8, 24, 48, 1, ATTR0_SQUARE | ATTR0_BLEND | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(8), 0);
  OBJSET(14, 8, 8, 24+64, 48, 1, ATTR0_SQUARE | ATTR0_BLEND | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(8), 0);
  OBJSET(13, 16, 8, 24+128, 48, 1, ATTR0_SQUARE | ATTR0_BLEND | ATTR0_AFF, ATTR1_SIZE_64x64 | ATTR1_AFF_ID(8), 0);
  
  // Quick fading anim
  REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
  for (int i = 1; i <= 20; i++) {
    uint32_t scale = 576 - (i << 4);
    uint32_t offset = 64 - (64 * 256 / scale);
    // 48x13
    obj_set_pos(&oam_mem[15], 24 + offset, 48);
    obj_set_pos(&oam_mem[13], 24+128 - offset, 48);
    
    obj_set_pos(&oam_mem[12], 24+1 + offset * 7 / 5, 48+1 + offset * 3 / 8);
    AFFSET(8, scale, 0, 0, scale);
    REG_BLDALPHA = BLD_EVA(min(i, 16)) | BLD_EVB(max(16-i, 0));
    //REG_BLDALPHA = BLD_EVA(16) | BLD_EVB(0);
    
    VBlankIntrWait();
  }
  int spoils_x = 112 + 12;
  for (int i = 0; i < 6; i++) {
		if (BATTLE_SPOILS[i] != 0xFF) {
			spoils_x -= 12;
		}
  }
  for (int i = 0; i < 6; i++) {
		if (BATTLE_SPOILS[i] != 0xFF) {
			VBlankIntrWait();
			VBlankIntrWait();
			uint8_t ico = BATTLE_SPOILS[i];
			int sx = (i % 3) * 2 + 24;
			int sy = (i / 3) * 2 + 8;
			fast_copy(OBJ_CHR(sx, sy), btl_menuicons_bin + (ico << 8) + 0, 64);
			fast_copy(OBJ_CHR(sx, sy+1), btl_menuicons_bin + (ico << 8) + 64, 64);
			OBJSET(11 - i, sx, sy, spoils_x, 80, 1, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_16x16, 0);
			spoils_x += 24;
			VBlankIntrWait();
			if (INKEY_PRESSED & (KEY_A | KEY_B)) break;
			VBlankIntrWait();
			if (INKEY_PRESSED & (KEY_A | KEY_B)) break;
		}
  }
  
  for (;;) {
    if (INKEY_PRESSED & (KEY_A | KEY_B)) break;
    VBlankIntrWait();
  }
  
  // Fade out
  
  for (int i = 1; i <= 16; i++) {
    REG_BLDALPHA = BLD_EVB(i) | BLD_EVA(16-i);
    VBlankIntrWait();
  }
  
  // Disable sprites
  oam_init(&oam_mem[6], 10);
}

void draw_textbar(const char *text)
{
	hide_display();
	fast_copy(OBJ_CHR(0, 6), btl_objmap_bin + (6 * 1024), 2048);
	uint16_t w = text_width(text);
	draw_text((128 - w) / 2, 51, text);
	VBlankIntrWait();
	for (int i = 0; i < 4; i++) {
		OBJSET(15 - i, 4 * i, 6, 56 + i * 32, 2, 1, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
	}
}

void draw_iconmenu_plronly(uint8_t psel, uint8_t spronly, int offy)
{
	// Player info
  for (uint32_t i = 0; i < 2; i++) {
		OBJSET(125 - i * 4, 0, (i == psel) * 3 + 0, i * 63 + 0,  138 + offy, 0, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
		OBJSET(124 - i * 4, 0, (i == psel) * 3 + 2, i * 63 + 0,  154 + offy, 0, ATTR0_WIDE, ATTR1_SIZE_32x8, 0);
		OBJSET(123 - i * 4, 0, (i == psel) * 3 + 0, i * 63 + 32, 138 + offy, 0, ATTR0_WIDE, ATTR1_SIZE_32x16 | ATTR1_HFLIP, 0);
		OBJSET(122 - i * 4, 0, (i == psel) * 3 + 2, i * 63 + 32, 154 + offy, 0, ATTR0_WIDE, ATTR1_SIZE_32x8 | ATTR1_HFLIP, 0);
		if (gs.pl[i].pid != 0xFF) {
			uint8_t p = gs.pl[i].pid;
			// Portrait
			if (!spronly) {
				fast_copy(OBJ_PAL(8 + i), btl_charapal_bin + (p << 7) + (COLSPACE << 5), 32);
				int status = 0xFF;
				for (int j = 0; j < STATUS_MAX; j++) {
					if (gs.pl[i].status[j] > 0) status = j;
				}
				if (status != 0xFF) {
					paleffect(8 + i, status);
				}
				fast_copy(OBJ_CHR(4 + i * 2, 4), btl_charagfx_bin + (p << 7) + 0, 64);
				fast_copy(OBJ_CHR(4 + i * 2, 5), btl_charagfx_bin + (p << 7) + 64, 64);
			}
			OBJSET(97-i, 4+i*2, 4, i * 63 + 4, 141 + offy, 8 + i, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
			
			int v, maxv, pad, low;
			// HP
			v = gs.pl[i].hp;
			maxv = gs.pl[i].maxhp;
			pad = (v < 100 ? 5 : 0) + (v < 10 ? 5 : 0);
			low = v < (maxv >> 2) ? 1 : 0;
			if (!spronly) fast_copy(OBJ_CHR(8 + i * 4, 4), btl_nums_bin + ((v + (psel == i ? 2000 : 0)) << 6), 64);
			if (!spronly) fast_copy(OBJ_CHR(10 + i * 4, 4), btl_nums_bin + ((maxv + (psel == i ? 2000 : 0)) << 6), 64);
			
			OBJSET(110 - i * 4, 8 + i * 4, 4, 23 + i * 63 + pad, 141 + offy, 1 + low, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
			OBJSET(109 - i * 4, 10 + i * 4, 4, 44 + i * 63, 141 + offy, 1 + low, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
			// MP
			v = gs.pl[i].mp;
			maxv = gs.pl[i].maxmp;
			pad = (v < 100 ? 5 : 0) + (v < 10 ? 5 : 0);
			low = v < (maxv >> 2) ? 1 : 0;
			if (!spronly) fast_copy(OBJ_CHR(8 + i * 4, 5), btl_nums_bin + ((v + (psel == i ? 3000 : 1000)) << 6), 64);
			if (!spronly) fast_copy(OBJ_CHR(10 + i * 4, 5), btl_nums_bin + ((maxv + (psel == i ? 3000 : 1000)) << 6), 64);
			OBJSET(108 - i * 4, 8 + i * 4, 5, 23 + i * 63 + pad, 150 + offy, 1 + low, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
			OBJSET(107 - i * 4, 10 + i * 4, 5, 44 + i * 63, 150 + offy, 1 + low, ATTR0_WIDE, ATTR1_SIZE_16x8, 0);
			
			// Sep
			OBJSET(112 - i, 5, (i == psel) * 2, i * 63 + 39, 141 + offy, 1, ATTR0_TALL, ATTR1_SIZE_8x16, 0);
		}
  }
}
void draw_iconmenu(uint8_t psel, const uint8_t *icons, uint8_t ilen, uint8_t isel, uint8_t spronly, int offy) {
	draw_iconmenu_plronly(psel, spronly, offy);
  int ioff = isel == 0xFF ? 0 : isel - 2;
  ioff = max(min(ioff, ilen - 5), 0);
  // Left menu arrow
  OBJSET(126, 4,  0, ioff > 0 ? 126 : 123, 138 + offy, 0, ATTR0_TALL, ATTR1_SIZE_8x32, 0);
  // Right menu arrow
  OBJSET(127, 4,  0, ioff < ilen - 5 ? 232 : 236, 138 + offy, 0, ATTR0_TALL, ATTR1_SIZE_8x32 | ATTR1_HFLIP, 0);
  
  // Icons
  for (uint8_t x = 0; x < 5; x++) {
		uint8_t i = x + ioff;
		uint8_t ico = i < ilen ? icons[i] : 39;
		if (!spronly) fast_copy(OBJ_CHR(x * 2 + 6, 0), btl_menuicons_bin + (ico << 8) + (i == isel ? 128 : 0), 64);
		if (!spronly) fast_copy(OBJ_CHR(x * 2 + 6, 1), btl_menuicons_bin + (ico << 8) + (i == isel ? 192 : 64), 64);
		if (i != isel) {
			OBJSET(117-x, 16, 1, 130 + 21 * x, 138 + offy, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
		} else {
			OBJSET(117-x, 16, 4, 130 + 21 * x, 138 + offy, 0, ATTR0_SQUARE, ATTR1_SIZE_32x32 | ATTR1_VFLIP, 0);
		}
		OBJSET(102-x, 6 + (x << 1), 0, 133 + x * 21, 141 + offy, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
  }
}
const char *get_name(int pid)
{
	if (pid == 0) return NMEDATA[26].mname;
	else return NMEDATA[19+pid].mname;
}
uint8_t status_selplayer(int cm)
{
	uint8_t plrcount = gs.pl[1].pid != 0xFF ? 2 : 1;
	uint8_t cur = clamp(CMEM_ON ? CMEM[cm] : 0, 0, plrcount - 1);
	
	draw_iconmenu_plronly(cur, 0, 0);
	draw_textbar(get_name(gs.pl[cur].pid));
	for (;;) {
		VBlankIntrWait();
		if (INKEY_PRESSED & KEY_START) {
			hide_display();
			return 0xFE;
		}
		else if (INKEY_PRESSED & KEY_B) {
			mmEffect(SFX_CANCEL);
			hide_display();
			return 0xFF;
		}
		else if (INKEY_PRESSED & KEY_A) {
			mmEffect(SFX_SELECT);
			hide_display();
			return cur;
		}
		else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
			cur = min(cur ^ 1, plrcount - 1);
			mmEffect(SFX_CURSOR);
			CMEM[cm] = cur;
			draw_iconmenu_plronly(cur, 0, 0);
			draw_textbar(get_name(gs.pl[cur].pid));
		}
	}
	
	return 0;
}


void hide_display()
{
	for (int i = 12; i < 32; i++)
		obj_hide(&obj_mem[i]);
}

void show_equipment(uint8_t plr, uint8_t eq, uint8_t ico, uint8_t id)
{
	hide_display();
	fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (4 << 13), 8192);
	
	fast_copy(OBJ_CHR(24, 8), btl_menuicons_bin + (ico << 8) + 0, 64);
	fast_copy(OBJ_CHR(24, 9), btl_menuicons_bin + (ico << 8) + 64, 64);
	
	if (id != 0xFF) {
		draw_text(28, 68 + 4, NMEDATA[id].ename);
		char txt[48];
		switch (eq) {
			case 0:
				txt_set(txt, "Damage: ");
				txt_append_num(txt, NMEDATA[id].eqatt[EQATT_DMG]);
				txt_append_str(txt, " (Acc: ");
				txt_append_num(txt, NMEDATA[id].eqatt[EQATT_PCT]);
				txt_append_str(txt, "%)");
				draw_text(28, 68+16 + 2, txt);
				
				if (NMEDATA[id].eqatt[EQATT_STR] != 0xFF) {
					txt_set(txt, "Damage type: ");
					txt_append_str(txt, ELEMENTS[NMEDATA[id].eqatt[EQATT_STR]]);
					draw_text(28, 68+32, txt);
				}
				if (NMEDATA[id].eqatt[EQATT_WEAK] != 0xFF) {
					txt_set(txt, "Inflicts: ");
					txt_append_str(txt, STATUSES[NMEDATA[id].eqatt[EQATT_WEAK]]);
					draw_text(28, 68+48 - 2, txt);
				}
				break;
			case 1:
			case 2:
				txt_set(txt, "Defense: ");
				txt_append_num(txt, NMEDATA[id].eqatt[EQATT_DMG]);
				txt_append_str(txt, " (Block chance: ");
				txt_append_num(txt, NMEDATA[id].eqatt[EQATT_PCT]);
				txt_append_str(txt, "%)");
				draw_text(28, 68+16 + 2, txt);
				if (NMEDATA[id].eqatt[EQATT_STR] != 0xFF) {
					txt_set(txt, "Protects against: ");
					txt_append_str(txt, ELEMENTS[NMEDATA[id].eqatt[EQATT_STR]]);
					draw_text(28, 68+32, txt);
				}
				if (NMEDATA[id].eqatt[EQATT_WEAK] != 0xFF) {
					txt_set(txt, "Vulnerable to: ");
					txt_append_str(txt, ELEMENTS[NMEDATA[id].eqatt[EQATT_WEAK]]);
					draw_text(28, 68+48 - 2, txt);
				}
				break;
			case 3:
				const int linelim[] = {157, 157, 157};
				wrap_text(185, 68+16+2, linelim, 16, ACC_DESC[NMEDATA[id].eqatt[EQATT_DMG]]);
				break;
		}
		
		//draw_text(33, 68+16, "Line 2");
		//draw_text(33, 68+32, "Line 3");
		//draw_text(33, 68+48, "Line 4");
	} else {
		const char *str;
		switch (eq) {
			case 0: default: str = "No weapon"; break;
			case 1: str = "No armour"; break;
			case 2: str = "No helmet"; break;
			case 3: str = "No accessory"; break;
		}
		draw_text(48, 64+28, str);
	}
	
	VBlankIntrWait();
	OBJSET(12, 24, 8, 24+3, 37+24, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);

	// Message box
	OBJSET(15, 0, 8, 24, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(14, 8, 8, 24+64, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(13, 16, 8, 24+128, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}

void stat_text(char *txt, const char *str, int baseval, int addval)
{
	txt_set(txt, str);
	txt_append_num(txt, baseval + addval);
	
	if (addval != 0) {
		txt_append_str(txt, " (+");
		txt_append_num(txt, addval);
		txt_append_str(txt, ")");
	}
}

void show_stone(uint8_t plr, uint8_t eq, uint8_t ico, uint8_t id, uint8_t remfirst)
{
	hide_display();
	fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (4 << 13), 8192);
	
	fast_copy(OBJ_CHR(24, 8), btl_menuicons_bin + (ico << 8) + 0, 64);
	fast_copy(OBJ_CHR(24, 9), btl_menuicons_bin + (ico << 8) + 64, 64);
	
	int base_maxhp = gs.pl[plr].maxhp;
	int base_maxmp = gs.pl[plr].maxmp;
	int base_str = gs.pl[plr].str;
	int base_def = gs.pl[plr].def;
	int base_agi = gs.pl[plr].agi;
	int base_mag = gs.pl[plr].mag;
	
	if (remfirst) {
		// Remove current soulstone from stats
		uint8_t tid = gs.pl[plr].slots[eq];
		if (tid != 0xFF) {
			base_maxhp -= STONE_STAT_CALC(NMEDATA[tid].maxhp);
			base_maxmp -= STONE_STAT_CALC(NMEDATA[tid].maxmp);
			base_str -= STONE_STAT_CALC(NMEDATA[tid].str);
			base_def -= STONE_STAT_CALC(NMEDATA[tid].def);
			base_agi -= STONE_STAT_CALC(NMEDATA[tid].agi);
			base_mag -= STONE_STAT_CALC(NMEDATA[tid].mag);
		}
	}
	int add_maxhp = 0;
	int add_maxmp = 0;
	int add_str = 0;
	int add_def = 0;
	int add_agi = 0;
	int add_mag = 0;
	
	if (id != 0xFF) {
		draw_text(28, 68 + 4, NMEDATA[id].mname);
		add_maxhp = STONE_STAT_CALC(NMEDATA[id].maxhp);
		add_maxmp = STONE_STAT_CALC(NMEDATA[id].maxmp);
		add_str = STONE_STAT_CALC(NMEDATA[id].str);
		add_def = STONE_STAT_CALC(NMEDATA[id].def);
		add_agi = STONE_STAT_CALC(NMEDATA[id].agi);
		add_mag = STONE_STAT_CALC(NMEDATA[id].mag);
	} else {
		draw_text(48, 64+4, "No spirit crystal");
	}
	char txt[48];
	stat_text(txt, "Life: ", base_maxhp, add_maxhp);
	draw_text(28, 68 + 16 + 2, txt);
	stat_text(txt, "Qi: ", base_maxmp, add_maxmp);
	draw_text(28 + 80, 68 + 16 + 2, txt);
	stat_text(txt, "Str: ", base_str, add_str);
	draw_text(28, 68 + 32, txt);
	stat_text(txt, "Def: ", base_def, add_def);
	draw_text(28 + 80, 68 + 32, txt);
	stat_text(txt, "Agi: ", base_agi, add_agi);
	draw_text(28, 68 + 48 - 2, txt);
	stat_text(txt, "Mag: ", base_mag, add_mag);
	draw_text(28 + 80, 68 + 48 - 2, txt);

	VBlankIntrWait();
	OBJSET(12, 24, 8, 24+3, 37+24, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);

	// Message box
	OBJSET(15, 0, 8, 24, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(14, 8, 8, 24+64, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(13, 16, 8, 24+128, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}
void show_tech(uint8_t plr, uint8_t ico, uint8_t id)
{
	hide_display();
	fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (4 << 13), 8192);
	
	fast_copy(OBJ_CHR(24, 8), btl_menuicons_bin + (ico << 8) + 0, 64);
	fast_copy(OBJ_CHR(24, 9), btl_menuicons_bin + (ico << 8) + 64, 64);
	
	char txt[48];
	txt_set(txt, NMEDATA[id].sname);
	txt_append_str(txt, " (Qi cost: ");
	txt_append_num(txt, NMEDATA[id].spdata[SP_COST]);
	txt_append_str(txt, ")");
	draw_text(28, 68 + 4, txt);
	
	const int linelim[] = {157, 157, 157};
	wrap_text(185, 68+16+2, linelim, 16, NMEDATA[id].sdesc);

	VBlankIntrWait();
	OBJSET(12, 24, 8, 24+3, 37+24, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);

	// Message box
	OBJSET(15, 0, 8, 24, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(14, 8, 8, 24+64, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(13, 16, 8, 24+128, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}
void show_save(struct saveinfo *saves, uint8_t ico, uint8_t cur)
{
	hide_display();
	fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (4 << 13), 8192);
	
	fast_copy(OBJ_CHR(24, 8), btl_menuicons_bin + (ico << 8) + 0, 64);
	fast_copy(OBJ_CHR(24, 9), btl_menuicons_bin + (ico << 8) + 64, 64);
	
	if (saves[cur].filled == 0x01) {
		draw_text(28, 68 + 4, saves[cur].txt1);
		draw_text(28, 68 + 16 + 2, saves[cur].txt2);
		draw_text(28, 68 + 32, saves[cur].txt3);
		draw_text(28, 68 + 48 - 2, saves[cur].txt4);
	} else {
		draw_text(48, 64+28, "Unoccupied save slot");
	}

	VBlankIntrWait();
	OBJSET(12, 24, 8, 24+3, 37+24, 0, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);

	// Message box
	OBJSET(15, 0, 8, 24, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(14, 8, 8, 24+64, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	OBJSET(13, 16, 8, 24+128, 37, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}

uint8_t status_equip_sel(uint8_t plr, uint8_t eq)
{
	// Fill equipment list
	uint8_t icons[INV_MAX];
	uint8_t icons_id[INV_MAX];
	uint8_t menulen = 0;
	for (int i = 0; i < INV_MAX; i++) {
		if (i >= NMEDATA_COUNT) break;
		if (NMEDATA[i].equiptype == eq) {
			uint8_t qty = gs.equip_qty[i];
			if (gs.pl[plr].equip[eq] == i) qty++;
			if (qty > 0) {
				icons[menulen] = 80 + i;
				icons_id[menulen] = i;
				menulen++;
			}
		}
	}
	icons[menulen] = 20 + eq;
	icons_id[menulen] = 0xFF;
	menulen++;
	
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SEQUIPB + plr*4 + eq] : 0, 0, menulen - 1);
	for (;;) {
		show_equipment(plr, eq, icons[cur], icons_id[cur]);
		
		draw_iconmenu(plr, icons, menulen, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				
				// Unequip existing equipment
				if (gs.pl[plr].equip[eq] != 0xFF) {
					gs.equip_qty[gs.pl[plr].equip[eq]]++;
				}
				gs.pl[plr].equip[eq] = icons_id[cur];
				if (gs.pl[plr].equip[eq] != 0xFF) {
					mmEffect(SFX_DECIDE);
					gs.equip_qty[gs.pl[plr].equip[eq]]--;
				} else {
					mmEffect(SFX_SELECTNONE);
				}
				return 0;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? menulen - 1 : 1)) % menulen;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SEQUIPB + plr*4 + eq] = cur;
				break;
				draw_iconmenu(plr, icons, menulen, cur, 0, 0);
			}
			VBlankIntrWait();
		}
	}
	
	return 0;
}
uint8_t status_stones_sel(uint8_t plr, uint8_t eq)
{
	// Fill stones list
	uint8_t icons[INV_MAX];
	uint8_t icons_id[INV_MAX];
	uint8_t menulen = 0;
	for (int i = 0; i < INV_MAX; i++) {
		if (i >= NMEDATA_COUNT) break;
		uint8_t qty = gs.gem_qty[i];
		if (gs.pl[plr].slots[eq] == i) qty++;
		if (qty > 0) {
			icons[menulen] = 40 + i;
			icons_id[menulen] = i;
			menulen++;
		}
	}
	icons[menulen] = 24;
	icons_id[menulen] = 0xFF;
	menulen++;
	
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SSTONEB + plr*4 + eq] : 0, 0, menulen - 1);
	for (;;) {
		show_stone(plr, eq, icons[cur], icons_id[cur], 1);
		
		draw_iconmenu(plr, icons, menulen, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				
				// Unequip existing stone
				uint8_t tid = gs.pl[plr].slots[eq];
				if (tid != 0xFF) {
					gs.gem_qty[tid]++;
					// Remove stats
					gs.pl[plr].maxhp -= STONE_STAT_CALC(NMEDATA[tid].maxhp);
					gs.pl[plr].maxmp -= STONE_STAT_CALC(NMEDATA[tid].maxmp);
					gs.pl[plr].str -= STONE_STAT_CALC(NMEDATA[tid].str);
					gs.pl[plr].def -= STONE_STAT_CALC(NMEDATA[tid].def);
					gs.pl[plr].agi -= STONE_STAT_CALC(NMEDATA[tid].agi);
					gs.pl[plr].mag -= STONE_STAT_CALC(NMEDATA[tid].mag);
					
					gs.pl[plr].hp = clamp(gs.pl[plr].hp, 0, gs.pl[plr].maxhp);
					gs.pl[plr].mp = clamp(gs.pl[plr].mp, 0, gs.pl[plr].maxmp);
				}
				tid = icons_id[cur];
				gs.pl[plr].slots[eq] = tid;
				if (tid != 0xFF) {
					gs.gem_qty[tid]--;
					int full_hp = gs.pl[plr].hp == gs.pl[plr].maxhp;
					int full_mp = gs.pl[plr].mp == gs.pl[plr].maxmp;
					// Add stats
					gs.pl[plr].maxhp += STONE_STAT_CALC(NMEDATA[tid].maxhp);
					gs.pl[plr].maxmp += STONE_STAT_CALC(NMEDATA[tid].maxmp);
					gs.pl[plr].str += STONE_STAT_CALC(NMEDATA[tid].str);
					gs.pl[plr].def += STONE_STAT_CALC(NMEDATA[tid].def);
					gs.pl[plr].agi += STONE_STAT_CALC(NMEDATA[tid].agi);
					gs.pl[plr].mag += STONE_STAT_CALC(NMEDATA[tid].mag);
					if (full_hp) gs.pl[plr].hp = gs.pl[plr].maxhp;
					if (full_mp) gs.pl[plr].mp = gs.pl[plr].maxmp;
					
					mmEffect(SFX_DECIDE);
				} else {
					mmEffect(SFX_SELECTNONE);
				}
				return 0;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? menulen - 1 : 1)) % menulen;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SSTONEB + plr*4 + eq] = cur;
				break;
				draw_iconmenu(plr, icons, menulen, cur, 0, 0);
			}
			VBlankIntrWait();
		}
	}
	
	return 0;
}
uint8_t status_techs_sel(uint8_t plr, uint8_t tid)
{
	// Only revive and heal make sense out of battle
	if (NMEDATA[tid].spdata[SP_REVIVE] || NMEDATA[tid].spdata[SP_HEAL]) {
		int mpcost = NMEDATA[tid].spdata[SP_COST];
		if (gs.pl[plr].mp < mpcost) {
			mmEffect(SFX_NOPE);
			draw_textbar("Insufficient Qi");
			return 0;
		}
		int all = NMEDATA[tid].spdata[SP_ALL];
		int from = 0;
		int to = 1;
		if (!all) {
			uint8_t t = status_selplayer(CMEM_STECP);
			if (t == 0xFE) return 1;
			if (t == 0xFF) return 0;
			from = t;
			to = t;
		}
		gs.pl[plr].mp -= mpcost;
		mmEffect(SFX_HEAL);
		for (int i = from; i <= to; i++) {
			int hpheal = NMEDATA[tid].spdata[SP_DAMAGE];
			hpheal = hpheal * (100 + get_mag(plr)) / 100;
			gs.pl[i].hp = min(gs.pl[i].hp + hpheal, gs.pl[i].maxhp);
		}
	}
	return 0;
}
uint8_t status_equip() {
	uint8_t plr = status_selplayer(CMEM_SPLR);
	if (plr == 0xFE) return 1;
	if (plr == 0xFF) return 2;
	
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SEQUIPA + plr] : 0, 0, 3);
	
	for (;;) {
		uint8_t icons[4];
		
		for (int i = 0; i < 4; i++) {
			icons[i] = 20 + i;
			if (gs.pl[plr].equip[i] != 0xFF) {
				icons[i] = gs.pl[plr].equip[i] + 80;
			}
		}
		
		show_equipment(plr, cur, icons[cur], gs.pl[plr].equip[cur]);
		
		draw_iconmenu(plr, icons, 4, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				mmEffect(SFX_SELECT);
				if (status_equip_sel(plr, cur) == 1) {
					return 1;
				}
				break;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? 3 : 1)) % 4;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SEQUIPA + plr] = cur;
				break;
			}
			VBlankIntrWait();
		}
	}
}
uint8_t status_stones() {
	uint8_t plr = status_selplayer(CMEM_SPLR);
	if (plr == 0xFE) return 1;
	if (plr == 0xFF) return 2;
	
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SSTONEA + plr] : 0, 0, 3);
	
	for (;;) {
		uint8_t icons[4];
		for (int i = 0; i < 4; i++) {
			icons[i] = 24;
			if (gs.pl[plr].slots[i] != 0xFF) {
				icons[i] = gs.pl[plr].slots[i] + 40;
			}
		}
		
		show_stone(plr, cur, icons[cur], gs.pl[plr].slots[cur], 1);
		
		draw_iconmenu(plr, icons, 4, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				mmEffect(SFX_SELECT);
				if (status_stones_sel(plr, cur) == 1) {
					return 1;
				}
				break;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? 3 : 1)) % 4;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SSTONEA + plr] = cur;
				break;
			}
			VBlankIntrWait();
		}
	}
}
uint8_t status_techs() {
	uint8_t plr = status_selplayer(CMEM_SPLR);
	if (plr == 0xFE) return 1;
	if (plr == 0xFF) return 2;
	
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_STECHSA + plr] : 0, 0, 3);
	
	for (;;) {
		uint8_t icons[5];
		uint8_t icons_id[5];
		int menulen = 0;
		uint8_t pid = gs.pl[plr].pid;
		icons[menulen] = pid == 0 ? 66 : pid + 59;
		icons_id[menulen] = pid == 0 ? 26 : pid + 19;
		menulen++;

		for (int i = 0; i < 4; i++) {
			uint8_t tid = gs.pl[plr].slots[i];
			if (tid != 0xFF) {
				icons[menulen] = tid + 40;
				icons_id[menulen] = tid;
				menulen++;
			}
		}
		
		show_tech(plr, icons[cur], icons_id[cur]);
		
		draw_iconmenu(plr, icons, menulen, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				mmEffect(SFX_SELECT);
				if (status_techs_sel(plr, icons_id[cur]) == 1) {
					return 1;
				}
				break;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? menulen - 1 : 1)) % menulen;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SSTONEA + plr] = cur;
				break;
			}
			VBlankIntrWait();
		}
	}
}
void save_sel(uint8_t cur)
{
	write_save(cur);
}

uint8_t status_save() {
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SAVESLOT] : 0, 0, 4);
	
	struct saveinfo *saves = NULL;
	
	for (;;) {
		uint8_t icons[5];
		if (saves == NULL)
			saves = retrieve_saves();
		
		for (int i = 0; i < 5; i++) {
			icons[i] = saves[i].filled == 0x01 ? 15 + i : 10 + i;
		}
		
		show_save(saves, icons[cur], cur);
		draw_iconmenu(255, icons, 5, cur, 0, 0);
		for (;;) {
			if (INKEY_PRESSED & KEY_START) {
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				mmEffect(SFX_SELECT);
				save_sel(cur);
				free(saves);
				saves = NULL;
				break;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				cur = (cur + ((INKEY_PRESSED & KEY_LEFT) ? 4 : 1)) % 5;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_SAVESLOT] = cur;
				break;
			}
			VBlankIntrWait();
		}
	}	
	if (saves != NULL) free(saves);

	return 0;
}
void draw8x2(int sp1, int sp2, int x, int y, int dx, int dy)
{
	OBJSET(sp1, x, y, dx, dy, 1, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
	OBJSET(sp2, x+4, y, dx+32, dy, 1, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
}

uint8_t status_config()
{
	fast_copy(OBJ_CHR(0, 8), btl_uimap_bin + (3 << 13), 8192);
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_CONFIG] : 0, 0, 2);
	uint8_t saved_colspace = COLSPACE;
	uint8_t cfg_cursormem = CMEM_ON ? 0 : 1;
	uint8_t cfg_lrturn = (KB_STRAFELEFT == KEY_L) ? 0 : 1;
	
	for (;;) {
		int x, y, dx, dy;
		x = 0; y = 8; dx = 16; dy = 48;
		if (cur == 0) x += 8;
		draw8x2(31, 30, x, y, dx, dy);
		x = 0; y = 10; dx = 16; dy = 72;
		if (cur == 1) x += 8;
		draw8x2(29, 28, x, y, dx, dy);
		x = 0; y = 12; dx = 16; dy = 96;
		if (cur == 2) x += 8;
		draw8x2(27, 26, x, y, dx, dy);
		
		// Display modes
		for (int i = 0; i < 4; i++) {
			x = 16 + i * 2; y = 8; dx = 154 + 18 * i; dy = 48;
			if (COLSPACE == i) x += 8;
			OBJSET(25 - i, x, y, dx, dy, 1, ATTR0_SQUARE, ATTR1_SIZE_16x16, 0);
		}
		// Cursor mem
		for (int i = 0; i < 2; i++) {
			x = 16 + i * 4; y = 10; dx = 158 + 34 * i; dy = 72;
			if (cfg_cursormem == i) x += 8;
			OBJSET(21 - i, x, y, dx, dy, 1, ATTR0_WIDE, ATTR1_SIZE_32x16, 0);
		}
		// Controls
		for (int i = 0; i < 2; i++) {
			x = i * 8; y = 14; dx = 94 + 66 * i; dy = 96;
			if (cfg_lrturn == i) x += 16;
			draw8x2(19 - i, 17 - i, x, y, dx, dy);
		}
		// Help
		draw8x2(15, 14, 16, 12, 2, 120);
		draw8x2(13, 12, 24, 12, 66, 120);
		
		for (;;) {
			VBlankIntrWait();
			if (INKEY_PRESSED & KEY_START) {
				COLSPACE = saved_colspace;
				update_colspace();
				return 1;
			}
			else if (INKEY_PRESSED & KEY_B) {
				mmEffect(SFX_CANCEL);
				COLSPACE = saved_colspace;
				update_colspace();
				return 0;
			}
			else if (INKEY_PRESSED & KEY_A) {
				mmEffect(SFX_DECIDE);
				CMEM_ON = cfg_cursormem == 0 ? 1 : 0;
				if (cfg_lrturn == 1) {
					KB_STRAFELEFT = KEY_LEFT;
					KB_STRAFERIGHT = KEY_RIGHT;
					KB_TURNLEFT = KEY_L;
					KB_TURNRIGHT = KEY_R;
				} else {
					KB_TURNLEFT = KEY_LEFT;
					KB_TURNRIGHT = KEY_RIGHT;
					KB_STRAFELEFT = KEY_L;
					KB_STRAFERIGHT = KEY_R;
				}
				save_config();
				return 0;
			}
			else if (INKEY_PRESSED & (KEY_LEFT | KEY_RIGHT)) {
				int cfgmax = (cur == 0) ? 4 : 2;
				int dir = (INKEY_PRESSED & KEY_LEFT) ? cfgmax - 1 : 1;
				switch (cur) {
					case 0:
						COLSPACE = (COLSPACE + dir) % cfgmax;
						update_colspace();
						break;
					case 1:
						cfg_cursormem = (cfg_cursormem + dir) % cfgmax;
						break;
					case 2:
						cfg_lrturn = (cfg_lrturn + dir) % cfgmax;
						break;
				}
				mmEffect(SFX_SELECT);
				break;
			}
			else if (INKEY_PRESSED & (KEY_UP | KEY_DOWN)) {
				cur = (cur + ((INKEY_PRESSED & KEY_UP) ? 2 : 1)) % 3;
				mmEffect(SFX_CURSOR);
				CMEM[CMEM_CONFIG] = cur;
				break;
			}
			
		}
	}
	
	return 0;
}

uint8_t status_submenu(uint8_t *icons, int menulen, uint8_t cur)
{
	uint8_t r = 0;
	//draw_iconmenu(255, icons, menulen, 255, 0, 0);
	switch (cur) {
		case 0: r = status_equip(); break;
		case 1: r = status_stones(); break;
		case 2: r = status_techs(); break;
		case 3: r = status_save(); break;
		case 4: r = status_config(); break;
	}
	if (r == 2) r = 0;
	return r;
}

void iconmenu_scroll(int off) {
	for (int i = 96; i < 128; i++) {
		int y = BFN_GET(obj_mem[i].attr0, ATTR0_Y);
		BFN_SET(obj_mem[i].attr0, y + off, ATTR0_Y);
	}
}

void status_root_msgbar(uint8_t cur)
{
	const char *msgbar = NULL;
	switch (cur) {
		case 0: msgbar = "Equip weapons / armour"; break;
		case 1: msgbar = "Equip spirit crystals"; break;
		case 2: msgbar = "Use spirit techniques"; break;
		case 3: msgbar = "Save your progress"; break;
		case 4: msgbar = "Configuration"; break;
	}
	if (msgbar) draw_textbar(msgbar);
}

void open_status() {
	mmEffect(SFX_STATUSOPEN);
	uint8_t icons[5] = {5, 6, 7, 8, 9};
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SCUR] : 0, 0, 4);
	
	draw_iconmenu(255, icons, 5, cur, 0, 21);
	// Scroll open
	for (int i = 0; i < 21; i++) {
		VBlankIntrWait();
		//draw_iconmenu(255, icons, 5, cur, 1, 20 - i);
		iconmenu_scroll(-1);
	}
	status_root_msgbar(cur);
	for (;;) {
		if (INKEY_PRESSED & (KEY_START | KEY_B)) break;
		else if (INKEY_PRESSED & KEY_LEFT) {
			cur = (cur + 4) % 5;
			mmEffect(SFX_CURSOR);
			draw_iconmenu(255, icons, 5, cur, 0, 0);
			status_root_msgbar(cur);
		}
		else if (INKEY_PRESSED & KEY_RIGHT) {
			cur = (cur + 1) % 5;
			mmEffect(SFX_CURSOR);
			draw_iconmenu(255, icons, 5, cur, 0, 0);
			status_root_msgbar(cur);
		}
		else if (INKEY_PRESSED & KEY_A) {
			mmEffect(SFX_SELECT);
			hide_display();
			uint8_t r = status_submenu(icons, 5, cur);
			hide_display();
			if (r == 1) break;
			draw_iconmenu(255, icons, 5, cur, 0, 0);
			status_root_msgbar(cur);
		}
		CMEM[CMEM_SCUR] = cur;
		VBlankIntrWait();
	}
	hide_display();
	// Scroll shut
	mmEffect(SFX_STATUSCLOSE);
	for (int i = 0; i < 21; i++) {
		VBlankIntrWait();
		iconmenu_scroll(1);
		//draw_iconmenu(255, icons, 5, cur, 1, i + 1);
	}
	oam_init((OBJ_ATTR *)&OBJMEM[96], 32);
}

int status_load()
{
	fast_copy(OBJ_CHR(0, 0), btl_uimap_bin + (5 << 13), 16384);
	struct saveinfo *saves = retrieve_saves();
	uint8_t cur = clamp(CMEM_ON ? CMEM[CMEM_SAVESLOT] : 0, 0, 4);
	
	for (;;) {
		//hide_display();
		fast_copy(OBJ_CHR(0, 0), btl_uimap_bin + (5 << 13), 8192);

		if (saves[cur].filled == 0x01) {
			draw_text(28, 4 + 4, saves[cur].txt1);
			draw_text(28, 4 + 16 + 2, saves[cur].txt2);
			draw_text(28, 4 + 32, saves[cur].txt3);
			draw_text(28, 4 + 48 - 2, saves[cur].txt4);
		} else {
			draw_text(48, 0+28, "Unoccupied save slot");
		}
	
		VBlankIntrWait();
		for (int i = 0; i < 5; i++) {
			int x, y, dx, dy;
			x = i * 4;
			y = 8;
			if (saves[i].filled == 0x01) y += 4;
			dx = 40 * i + 24;
			dy = 96;
			OBJSET(31-i, x, y, dx, dy, cur == i ? 3 : 1, ATTR0_SQUARE, ATTR1_SIZE_32x32, 0);
		}
		
		OBJSET(15, 0, 0, 24, 28, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
		OBJSET(14, 8, 0, 24+64, 28, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
		OBJSET(13, 16, 0, 24+128, 28, 1, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
		
		
		for (;;) {
			VBlankIntrWait();
			if (INKEY_PRESSED & KEY_B) {
				free(saves);
				return 0;
			}
			else if (INKEY_PRESSED & KEY_LEFT) {
				cur = (cur + 4) % 5;
				mmEffect(SFX_CURSOR);
				break;
			}
			else if (INKEY_PRESSED & KEY_RIGHT) {
				cur = (cur + 1) % 5;
				mmEffect(SFX_CURSOR);
				break;
			}
			else if (INKEY_PRESSED & KEY_A) {
				if (saves[cur].filled == 0x01) {
					mmEffect(SFX_DECIDE);
					free(saves);
					load_save(cur);
					return 1;
				} else {
					mmEffect(SFX_NOPE);
				}
			}
		}	
	}
}
