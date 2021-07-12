#pragma once

void show_message(int portrait, const char *text);
void open_status();
uint8_t status_config();
int status_load();
void hide_display();
void draw_textbar(const char *text);
void draw_iconmenu_plronly(uint8_t psel, uint8_t spronly, int offy);
void draw_iconmenu(uint8_t psel, const uint8_t *icons, uint8_t ilen, uint8_t isel, uint8_t spronly, int offy);
