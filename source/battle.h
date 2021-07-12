#pragma once
#include "btl_types.h"

void battle_engine(uint8_t *enemies, int ambush);
void random_encounter();
void force_random_encounter(int idx);
void get_bounds(int cur, int *x1, int *y1, int *x2, int *y2);

int get_str(int i);
int get_agi(int i);
int get_def(int i);
int get_mag(int i);
int get_acc(struct actor *a);

extern int BATTLE_MOD;
extern int BATTLE_OUTCOME;
extern int BATTLE_CANNOT_RUN;
extern uint8_t BATTLE_SPOILS[];
