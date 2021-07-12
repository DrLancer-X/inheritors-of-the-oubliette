#pragma once
#include <stdint.h>

struct header_info {
	uint8_t magic[6];
	uint8_t colspace;
	uint8_t curmem;
	uint8_t lrstrafe;
};

struct saveinfo {
	uint8_t filled;
	char txt1[48];
	char txt2[48];
	char txt3[48];
	char txt4[48];
};

struct saveinfo *retrieve_saves();
void write_save(uint8_t slot);
void load_save(uint8_t slot);

void load_config();
void save_config();
