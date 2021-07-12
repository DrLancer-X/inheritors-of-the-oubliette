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
#include "title.h"
#include "video.h"

#include "btl_uimap_bin.h"
#include "btl_charapal_bin.h"
#include "btl_charagfx_bin.h"
#include "btl_menuicons_bin.h"
#include "btl_nums_bin.h"
#include "btl_nmedata_bin.h"

#include "btl_imsoftgfx_bin.h"
#include "btl_imsoftpal_bin.h"
#include "btl_skygfx_bin.h"
#include "btl_skypal_bin.h"

#include "btl_titlegfx_bin.h"
#include "btl_titlepal_bin.h"
#include "btl_titleobjgfx_bin.h"
#include "btl_titleobjpal_bin.h"
#include "btl_titlemenugfx_bin.h"
#include "btl_titlemenupal_bin.h"

void title_video_init()
{
	REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ_1D | DCNT_OBJ;
	
}

void title_video_restore()
{
	REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ_2D | DCNT_OBJ;
}

void logo_anim()
{
	title_video_init();
	mmEffect(SFX_STATION02);
	
	load_palette(btl_skypal_bin, NULL);
	//fast_copy((volatile uint16_t *)MEM_PAL_BG, btl_skypal_bin + (COLSPACE << 9), 512);
	fast_copy(OBJ_PAL(15), btl_imsoftpal_bin + (COLSPACE << 5), 32);
	fast_copy(OBJ_CHR(0, 0), btl_imsoftgfx_bin, btl_imsoftgfx_bin_size);
	uint8_t sp;
	
	size_t sky_pos = (1024 - 160) * 240;
	/*
	for (uint32_t y = 0; y < 9; y++) {
		for (uint32_t x = 0; x < 8; x++) {
			OBJSET(sp, sp * 4, 0, x * 16 + 56, y * 16 + 8, 15, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_16x16 | ATTR1_AFF_ID(x + y), 0);
			obj_hide(&obj_mem[sp]);
			sp++;
		}
	}
	*/
	
	VBlankIntrWait();
	
	int fr = 0;
	
	for (;;) {
		if (INKEY_PRESSED & KEY_ANY) break;
		if (fr <= 256)
			REG_BLDY = (256 - fr) >> 4;
		if (fr > 64 && fr <= 308) {
			sp = 0;
			for (uint32_t y = 0; y < 9; y++) {
				for (uint32_t x = 0; x < 8; x++) {
					uint32_t i = x + y;
					int v = i * 12 + 128 - fr;
					if (v < 64) {
						OBJSET(sp, sp * 4, 0, x * 16 + 56, y * 16 + 8, 15, ATTR0_SQUARE | ATTR0_AFF, ATTR1_SIZE_16x16 | ATTR1_AFF_ID(x + y), 0);
					}
					sp++;
				}
			}
			for (int i = 0; i <= 15; i++) {
				int v = i * 12 + 128 - fr;
				if (v >= 0 && v < 64) {
					int scale = (v + 64) * 4;
					//AFFSET(i, scale, 0, 0, scale);
					obj_aff_rotscale(&obj_aff_mem[i], scale, scale, v * 1024);
				}
			}
		}
		if (fr == 308) {
			REG_BLDCNT = BLD_TOP(BLD_OBJ) | BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
		}
		if (fr >= 308 && fr <= 372) {
			int fade = clamp((fr - 308) / 4, 0, 16);
			REG_BLDALPHA = BLDA_BUILD(16-fade, fade);
		}
		if (fr == 373) {
			for (sp = 0; sp < 72; sp++) {
				obj_hide(&obj_mem[sp]);
			}
			REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
		}
		if (fr >= 373) {
			REG_BLDY = clamp((fr - 373) / 4, 0, 16);
		}
		fast_copy((volatile uint16_t *)MEM_VRAM_FRONT, &btl_skygfx_bin[sky_pos], 38400);
		VBlankIntrWait();
		if (sky_pos == 0) break;
		sky_pos -= 240;
		fr++;
		if (fr >= 500) break;
	}
}

void draw_menu(int cur, uint16_t attr0)
{
	uint8_t sp = 116;
	for (int i = 0; i < 4; i++) {
		int y = 102 + i * 14;
		if (i == 3) y = 149;
		int x = 0;
		if (cur == i) x += 16;
		OBJSET(sp, x, 2 * i, 82, y, 14, ATTR0_WIDE | attr0, ATTR1_SIZE_32x16, 0);
		sp++;
		OBJSET(sp, x+4, 2 * i, 82 + 32, y, 14, ATTR0_WIDE | attr0, ATTR1_SIZE_32x16, 0);
		sp++;
		OBJSET(sp, x+8, 2 * i, 82 + 64, y, 14, ATTR0_WIDE | attr0, ATTR1_SIZE_32x16, 0);
		sp++;
	}
}
void screenanim()
{
	if (TICKER % 16 == 0) {
		int frame = (TICKER / 16) % 2;
		load_palette(btl_titlepal_bin + 4096 + (frame ? 2048 : 0), NULL);
		//fast_copy((volatile uint16_t *)MEM_PAL_BG, btl_titlepal_bin + 4096 + (frame ? 2048 : 0) + (COLSPACE << 9), 512);
		fast_copy((volatile uint16_t *)MEM_VRAM_FRONT, btl_titlegfx_bin + (240*240) + (240*200) + (frame ? 240*160 : 0), 38400);
	}
}
void fadeout(int ani)
{
	int bl = (ani % 128) - 64;
	if (bl < 0) bl *= -1;
	bl = (bl / 8) + 7;
	
	while (bl >= 0) {
		REG_BLDALPHA = BLDA_BUILD(bl, 16 - bl);
		for (int j = 0; j < 8; j++) {
			screenanim();
			VBlankIntrWait();
			if (INKEY_PRESSED & KEY_ANY) break;
		}
		bl--;
		if (INKEY_PRESSED & KEY_ANY) break;
	}
	oam_init(obj_mem, 4);
	REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
	for (int i = 0; i < 16; i++) {
		REG_BLDY = i;
		for (int j = 0; j < 8; j++) {
			if (INKEY_PRESSED & KEY_ANY) break;
			mmSetModuleVolume(256 - ((i * 8 + j) * 2));
			screenanim();
			VBlankIntrWait();
		}
		if (INKEY_PRESSED & KEY_ANY) break;
	}
	REG_BLDY = 16;
	CURRENT_MOD = -1;
	VBlankIntrWait();
	mmSetModuleVolume(256);
}
void title_screen_play()
{
	CURRENT_MOD = MOD_AIRSTRIKE;

	fast_copy(OBJ_PAL(15), btl_titleobjpal_bin + (COLSPACE << 5), 32);
	fast_copy(OBJ_PAL(14), btl_titlemenupal_bin + (COLSPACE << 5), 32);
	fast_copy(OBJ_CHR(0, 0), btl_titlemenugfx_bin, btl_titlemenugfx_bin_size);
	fast_copy(OBJ_CHR(0, 8), btl_titleobjgfx_bin, btl_titleobjgfx_bin_size);
	
	for (int t = 0; t < 2; t++) {
		
		
		size_t sky_pos = 0;
		int sky_y = 0;
		const uint8_t *titledata;
		const uint8_t *paldata;
		int sky_lim;
		if (t == 0) {
			sky_lim = 80;
			titledata = btl_titlegfx_bin;
			paldata = btl_titlepal_bin;
		} else {
			sky_lim = 40;
			titledata = btl_titlegfx_bin + (240 * 240);
			paldata = btl_titlepal_bin + 2048;
		}
		load_palette(paldata, NULL);
		//fast_copy((volatile uint16_t *)MEM_PAL_BG, paldata + (COLSPACE << 9), 512);
		int i;
		int x = t == 0 ? 2 : 4;
		for (i = 0; i <= 16; i++) {
			if (INKEY_PRESSED & KEY_ANY) break;
			REG_BLDY = 16 - i;
			fast_copy((volatile uint16_t *)MEM_VRAM_FRONT, &titledata[sky_pos], 38400);
			if (i % x == x - 1) {
				sky_pos += 240;
				sky_y++;
			}
		}
		while (sky_y <= sky_lim) {
			if (INKEY_PRESSED & KEY_ANY) break;
			fast_copy((volatile uint16_t *)MEM_VRAM_FRONT, &titledata[sky_pos], 38400);
			if (i % x == x - 1) {
				sky_pos += 240;
				sky_y++;
			}
			i++;
			int v = sky_lim - sky_y;
			if (v < 16) REG_BLDY = 16 - v;
			VBlankIntrWait();
		}
		if (INKEY_PRESSED & KEY_ANY) break;
	}
	load_palette(btl_titlepal_bin + 4096, NULL);
	//fast_copy((volatile uint16_t *)MEM_PAL_BG, btl_titlepal_bin + 4096 + (COLSPACE << 9), 512);
	fast_copy((volatile uint16_t *)MEM_VRAM_FRONT, btl_titlegfx_bin + (240*240) + (240*200), 38400);
	for (int i = 0; i <= 16; i++) {
		REG_BLDY = 16 - i;
		screenanim();
		VBlankIntrWait();
	}
	
	REG_BLDCNT = BLD_TOP(BLD_OBJ) | BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
	
	for (int i = 0; i < 4; i++) {
		OBJSET(i, i * 8, 8, i * 64 + 6, 38, 15, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
	}
	for (int i = 0; i <= 16; i++) {
		REG_BLDALPHA = BLDA_BUILD(i, 16 - i);
		for (int j = 0; j < 8; j++) {
			if (INKEY_PRESSED & KEY_ANY) goto skip;
			screenanim();
			VBlankIntrWait();
		}
		if (INKEY_PRESSED & KEY_ANY) break;
	}
	
	REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
	
	for (int i = 0; i <= 16; i++) {
		REG_BLDALPHA = BLDA_BUILD(i, 16 - i);
		for (int j = 0; j < 8; j++) {
			screenanim();
			draw_menu(-1, ATTR0_BLEND);
			VBlankIntrWait();
		}
	}
	skip:
	REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
	for (int i = 0; i < 4; i++) {
		OBJSET(i, i * 8, 8, i * 64 + 6, 38, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
	}
	int cur = 0;
	int ani = 0;
	for (;;) {
		for (;;) {
			int bl = (ani % 128) - 64;
			if (bl < 0) bl *= -1;
			bl = (bl / 8) + 7;
			REG_BLDALPHA = BLDA_BUILD(bl, 16 - bl);
			ani++;
			screenanim();
			draw_menu(cur, 0);
			VBlankIntrWait();
			if (INKEY_PRESSED & KEY_UP) {
				cur = (cur + 2) % 3;
				mmEffect(SFX_CURSOR);
			} else if (INKEY_PRESSED & KEY_DOWN) {
				cur = (cur + 1) % 3;
				mmEffect(SFX_CURSOR);
			} else if (INKEY_PRESSED & KEY_ACCEPT) {
				mmEffect(SFX_SELECT);
				break;
			}
		}
		draw_menu(-1, 0);
		switch (cur) {
			case 0:
				fadeout(ani);
				return;
			case 1:
				oam_init(obj_mem, 4);
				oam_init(&obj_mem[116], 12);
				
				int r = status_load();
				
				oam_init(obj_mem, 128);
				fast_copy(OBJ_CHR(0, 0), btl_titlemenugfx_bin, btl_titlemenugfx_bin_size);
				fast_copy(OBJ_CHR(0, 8), btl_titleobjgfx_bin, btl_titleobjgfx_bin_size);
				for (int i = 0; i < 4; i++) {
					OBJSET(i, i * 8, 8, i * 64 + 6, 38, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
				}
				
				if (r == 1) {
					fadeout(ani);
					return;
				}
				
				break;
			case 2:
				oam_init(obj_mem, 4);
				oam_init(&obj_mem[116], 12);
				
				status_config();
				
				oam_init(obj_mem, 128);
				fast_copy(OBJ_CHR(0, 8), btl_titleobjgfx_bin, btl_titleobjgfx_bin_size);
				for (int i = 0; i < 4; i++) {
					OBJSET(i, i * 8, 8, i * 64 + 6, 38, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
				}
				
				break;
		}
		
	}
	
	int b = 0;
	for (;;) {
		if (INKEY_PRESSED & KEY_ANY) break;
		int bl = (b % 128) - 64;
		if (bl < 0) bl *= -1;
		bl = bl / 8 + 8;
		REG_BLDALPHA = BLDA_BUILD(bl, 16 - bl);
		VBlankIntrWait();
		b++;
		
		
	}
}

void logo_screen()
{
	logo_anim();
	mmEffectCancelAll();
	title_video_restore();
	oam_init(obj_mem, 72);
	VBlankIntrWait();
}

void title_screen()
{
	REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
	REG_BLDY = 16;
	title_screen_play();
	title_video_restore();
	oam_init(obj_mem, 4);
	oam_init(&obj_mem[116], 16);
	
	VBlankIntrWait();
}
