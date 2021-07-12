#pragma once

extern volatile uint16_t *VIDEO_BUFFER;

void flip_buffer();
void first_buffer();
void paleffect(int pal, int code);
