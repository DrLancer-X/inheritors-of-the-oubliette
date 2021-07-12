#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tonc_memdef.h>
#include <tonc_memmap.h>
#include "pdata.h"
#include "saves.h"
#include "textfn.h"
#include "renderer.h"

const size_t save_size = sizeof(struct saveinfo) + sizeof(struct pd_gamestate);
volatile uint8_t *sram = (volatile uint8_t *)MEM_SRAM;

void read_bytes(void *out, volatile void *in, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((uint8_t *)out)[i] = ((volatile uint8_t *)in)[i];
	}
}
void write_bytes(const void *from, volatile void *to, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((volatile uint8_t *)to)[i] = ((const uint8_t *)from)[i];
	}
}

int check_magic()
{
	if (sram[0] == 'I' && sram[1] == 'o' && sram[2] == 't' && sram[3] == 'O') {
		// Now check that the struct size is the same.
		uint16_t size_check;
		read_bytes(&size_check, &sram[4], sizeof(size_check));
		if (size_check == save_size) {
			return 1;
		}
	}
	return 0;
}

void write_magic()
{
	// First check magic
	if (!check_magic()) {
		// The check failed. This means that things are in an indeterminate state
		// and we cannot make assumptions about the contents of SRAM
		
		// Erase all save slots (just erasing the 'filled' byte is enough)
		for (int i = 0; i < 5; i++) {
			size_t save_pos = i * save_size + sizeof(struct header_info);
			sram[save_pos] = 0;
		}
		// Now write the magic
		sram[0] = 'I';
		sram[1] = 'o';
		sram[2] = 't';
		sram[3] = 'O';
		// Write size
		uint16_t size_check = save_size;
		write_bytes(&size_check, &sram[4], sizeof(size_check));
	}
	
	// Before we update the config info, temp disable the magic.
	// This is in case we lose power mid-write
	
	sram[0] = 'W';
	// Write config
	struct header_info header;
	memset(&header, 0, sizeof(header));
	header.colspace = COLSPACE;
	header.curmem = CMEM_ON;
	header.lrstrafe = (KB_STRAFELEFT == KEY_L) ? 1 : 0;
	// Don't write over magic
	write_bytes(((const void *)&header) + 6, &sram[6], sizeof(header) - 6);
	
	// Fix the magic again
	sram[0] = 'I';
}

struct saveinfo *retrieve_saves()
{
	// We assume we have 32kb of sram
	
	struct saveinfo *saves = malloc(sizeof(struct saveinfo) * 5);
	for (int i = 0; i < 5; i++) saves[i].filled = 0;
	
	// First, check if our magic is there. If not, all saves are empty
	if (check_magic()) {
		// Read each save in
		for (int i = 0; i < 5; i++) {
			size_t save_pos = i * save_size + sizeof(struct header_info);
			read_bytes(&saves[i], &sram[save_pos], sizeof(struct saveinfo));
		}
	}
	
	return saves;
}

void write_save(uint8_t slot)
{
	gs.ticks = TICKER;
	
	uint32_t seconds_played;
	if (gs.ticks > 20000000) {
		seconds_played = gs.ticks / 60;
	} else {
		seconds_played = gs.ticks * 100 / 5973;
	}
	
	write_magic();
	
	struct saveinfo *save = malloc(sizeof(struct saveinfo));
	save->filled = 0; // We will write this byte last
	txt_set(save->txt1, "Life: ");
	for (int i = 0; i < 2; i++) {
		if (gs.pl[i].pid != 0xFF) {
			txt_append_num(save->txt1, gs.pl[i].hp);
			txt_append_str(save->txt1, "/");
			txt_append_num(save->txt1, gs.pl[i].maxhp);
			txt_append_str(save->txt1, "  ");
		}
	}
	txt_set(save->txt2, "Qi: ");
	for (int i = 0; i < 2; i++) {
		if (gs.pl[i].pid != 0xFF) {
			txt_append_num(save->txt2, gs.pl[i].mp);
			txt_append_str(save->txt2, "/");
			txt_append_num(save->txt2, gs.pl[i].maxmp);
			txt_append_str(save->txt2, "  ");
		}
	}
	int stones = 0;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < SLOTS_MAX; j++) {
			if (gs.pl[i].slots[j] != 0xFF) stones++;
		}
	}
	for (int i = 0; i < INV_MAX; i++) {
		stones += gs.gem_qty[i];
	}
	txt_set(save->txt3, "Spirit crystals: ");
	txt_append_num(save->txt3, stones);

	txt_set(save->txt4, "Time played: ");
	int mins = seconds_played / 60 % 60;
	int hours = mins / 60;
	seconds_played %= 60;
	if (hours > 0) {
		txt_append_num(save->txt4, hours);
		txt_append_str(save->txt4, ":");
		if (mins < 10) txt_append_str(save->txt4, "0");
	}
	if (mins > 0) {
		txt_append_num(save->txt4, mins);
		txt_append_str(save->txt4, ":");
		if (seconds_played < 10) txt_append_str(save->txt4, "0");
	}
	
	txt_append_num(save->txt4, seconds_played);
	
	txt_append_str(save->txt4, "s");
	
	// Write the save, then pdata
	size_t save_pos = slot * save_size + sizeof(struct header_info);
	write_bytes(save, &sram[save_pos], sizeof(struct saveinfo));
	write_bytes(&gs, &sram[save_pos + sizeof(struct saveinfo)], sizeof(struct pd_gamestate));
	
	// Write the 'filled' byte
	sram[save_pos] = 1;
	
	free(save);
}
void load_save(uint8_t slot)
{
	size_t save_pos = slot * save_size + sizeof(struct header_info);
	read_bytes(&gs, &sram[save_pos + sizeof(struct saveinfo)], sizeof(struct pd_gamestate));
}

void load_config()
{
	if (check_magic()) {
		struct header_info header;
		read_bytes(&header, &sram[0], sizeof(header));
		COLSPACE = header.colspace;
		CMEM_ON = header.curmem;
		if (header.lrstrafe) {
			KB_TURNLEFT = KEY_LEFT;
			KB_TURNRIGHT = KEY_RIGHT;
			KB_STRAFELEFT = KEY_L;
			KB_STRAFERIGHT = KEY_R;
		} else {
			KB_STRAFELEFT = KEY_LEFT;
			KB_STRAFERIGHT = KEY_RIGHT;
			KB_TURNLEFT = KEY_L;
			KB_TURNRIGHT = KEY_R;
		}
		update_colspace();
	}
}
void save_config()
{
	write_magic();
}
