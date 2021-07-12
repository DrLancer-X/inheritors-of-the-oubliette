#pragma once
#include <stdint.h>

// fast_set requires 4-byte aligned and multiple of 40 bytes data
void fast_set(volatile void *dest, uint32_t v, uint32_t len);
// fast_copy requires 4-byte aligned and multiple of 32 bytes data
void fast_copy(volatile void *dest, const void *src, uint32_t len);
// This copies one column, 64 bytes long, from src into dest
void fast_copy_texcol(void *dest, const void *src);
// This copies two columns, each 64 bytes long, from src1 and src2 into dest
void fast_copy_texcols(void *dest, const void *src1, const void *src2);
