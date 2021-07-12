#pragma once

// Simple text functions, because stdio is too heavy for GBA (uses up too much iwram)
void txt_erase(char *buf);
void txt_set(char *buf, const char *txt);
int txt_len(char *buf);
void txt_append_str(char *buf, const char *append);
void txt_append_num(char *buf, int num);
