#pragma once

uint16_t text_width(const char *text);
void draw_text(uint8_t x, uint8_t y, const char *text);
extern const uint8_t *GLYPH_W;
