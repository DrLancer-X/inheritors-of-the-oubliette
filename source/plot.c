#include <stdlib.h>
#include <maxmod.h>
#include "fast_set.h"
#include "pdata.h"
#include "gameplay.h"
#include "soundbank.h"
#include "renderer.h"
#include "video.h"
#include "battle.h"
#include "ui.h"
#include "macros.h"
#include "btl_tachiegfx_bin.h"
#include "btl_tachiepal_bin.h"
#include "btl_objmap_bin.h"

#include <maxmod.h>
#include <tonc_bios.h>
#include <tonc_oam.h>
#include <tonc_memdef.h>
#include <tonc_memmap.h>

#define PLOT_HANDLE(v_, x_, y_, z_, f_) \
do { \
if (((v_) == gs.plot[PLOT_BASE]) && ((x_) == gs.x) && ((y_) == gs.y) && ((z_) == gs.z)) { \
  f_(); \
  MESSAGE_OFFSET = 0; \
  gs.plot[PLOT_BASE] = v_+1; \
} \
} while(0);

#define Q_HAWK 0
#define Q_STAR 1
#define D_THUNDER 2
#define D_SKY 3
#define D_PATRIARCH 4
#define D_RAVEN 5
#define R_EMPEROR 6

static void draw_sp(int s)
{
	MESSAGE_OFFSET = -32;
	fast_copy(OBJ_PAL(15), btl_tachiepal_bin + (s << 7) + (COLSPACE << 5), 32);
	for (uint32_t y = 0; y < 8; y++) {
		fast_copy(OBJ_CHR(24, 0 + y), btl_tachiegfx_bin + (y << 8) + (s << 11), 256);
	}
	OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}
static void draw_sp_fadein(int s)
{
	draw_sp(s);
	REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
	REG_BLDALPHA = BLDA_BUILD(0, 16);
	OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
	for (int i = 0; i <= 16; i++) {
    REG_BLDALPHA = BLDA_BUILD(i, 16-i);
    VBlankIntrWait();
	}
	OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE, ATTR1_SIZE_64x64, 0);
}
static void hide_sp()
{
	obj_hide(&oam_mem[127]);
	fast_copy(OBJ_CHR(0, 0), btl_objmap_bin, 8192);
}
static void dissolve_sp(int fboss)
{
	REG_BLDCNT = BLD_BOT(BLD_BG2 | BLD_BACKDROP) | BLD_STD;
	REG_BLDALPHA = BLDA_BUILD(16, 0);
	OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
	
	for (int i = 0; i < 19; i++) {
		int v = i;
		if (i >= 12) OBJSET(127, 24, 0, 87 - (i-12)/4, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
		REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), min(v, 16));
		VBlankIntrWait();
		
		if (fboss) {
			OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
			v++;
			REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), min(v + 1, 16));
			VBlankIntrWait();
		}
		
		v++;
		REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), v);
		VBlankIntrWait();
		if (i >= 12) OBJSET(127, 24, 0, 89 + (i-12)/4, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
		v--;
		REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), min(v, 16));
		VBlankIntrWait();
		
		if (fboss) {
			OBJSET(127, 24, 0, 88, 80, 15, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_64x64, 0);
			v--;
			REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), min(v + 1, 16));
			VBlankIntrWait();
		}
		
		v++;
		REG_BLDALPHA = BLDA_BUILD(min(20 - v, 16), min(v, 16));
		VBlankIntrWait();
	}
	hide_sp();
}

static void wait()
{
	for (int i = 0; i < 50; i++) {
		VBlankIntrWait();
	}
}


void plot1() {
	show_message(Q_HAWK, "\"The portal worked. Our Qilin clan may yet realise some good fortune from this after all.\"");
	show_message(Q_STAR, "\"So this is the ancient training ground... I can already sense the presence of spirit entities.\"");
	show_message(Q_HAWK, "\"Be careful, clan sister Star. While the spirit crystals have the potential to bring us great power, the spirit entities are not to be underestimated.\"");
	show_message(Q_STAR, "\"I know that, clan brother Hawk. With these godrelics the clan patriarch provided us, we should surely be fine.\"");
	show_message(Q_HAWK, "\"These may be almighty relics back in our realm, but this is an ancient macrocosm. That is why the rewards will be so great. Because the risk is even higher.\"");
	show_message(Q_STAR, "\"Look, we should just hurry along and see if we can find something good to take back to the clan. The portal only opens once every 10,000 years after all. I don't want to miss anything.\"");
}

void plot2() {
	CURRENT_MOD = MOD_TEMP32;
	force_camera_facing();
	draw_sp_fadein(1);
	wait();
	show_message(D_THUNDER, "\"Halt! You have trespassed upon the sacred training grounds of the great Dragonfly clan! Hand over any treasures you have found and leave at once!\"");
	show_message(Q_STAR, "\"Who the hell are you? I've never heard of this Dragonfly clan before. Our Qilin clan has been using these grounds to train our chosen sons and daughters for hundreds of thousands of years!\"");
	show_message(D_THUNDER, "\"That you do not show proper respect to the great Dragonfly Thunderbolt is an insult so great that it can only be washed away with the blood of your entire clan!\"");
	show_message(Q_HAWK, "\"Listen, I don't know who you are and I don't care. I'm only going to warn you once. Turn around and leave now or you will regret it.\"");
	show_message(D_THUNDER, "\"Apparently rudeness and overconfidence go hand in hand. Very well, but it is you who will soon regret your rash choice of actions!\"");
	hide_sp();
	force_fixed_encounter(21);
	draw_sp(1);
	if (after_boss()) return;
	CURRENT_MOD = MOD_TEMP32;
	wait();
	show_message(D_THUNDER, "\"They are too powerful. I can't go on. Forgive me, Patriarch.\"");
	show_message(Q_HAWK, "\"I told you not to mess with us.\"");
	wait();
	dissolve_sp(0);
	gs.plot[PLOT_ELVL] = 8;
}
void plot3() {
	CURRENT_MOD = MOD_TEMP32;
	force_camera_facing();
	draw_sp_fadein(2);
	wait();
	show_message(D_SKY, "\"I finally managed to track you down! You will pay for what you did to the young scion of the Dragonfly clan, Dragonfly Thunderbolt!\"");
	show_message(Q_HAWK, "\"There's more of you Dragonfly people around? All right, I'll bite. What's your name?\"");
	show_message(D_SKY, "\"You would do well to learn the name of Dragonfly Sky, the Holy Daughter of the Dragonfly clan!\"");
	show_message(Q_STAR, "\"I'm not sure we really need to learn it, though.\"");
	
	hide_sp();
	force_fixed_encounter(22);
	draw_sp(2);
	if (after_boss()) return;
	CURRENT_MOD = MOD_TEMP32;
	wait();
	show_message(D_SKY, "\"How is this possible?\"");
	show_message(Q_HAWK, "\"All we wanted you to do was leave us alone... what was your name again?\"");
	show_message(D_SKY, "\"You... you... augh!\"");
	wait();
	dissolve_sp(0);
	gs.plot[PLOT_ELVL] = 12;
}
void plot4() {
	CURRENT_MOD = MOD_TEMP32;
	force_camera_facing();
	draw_sp_fadein(3);
	wait();
	show_message(D_PATRIARCH, "\"How dare you spill the blood of the younger generation of the Dragonfly clan! This insult you have dealt us is close to unendurable. Now only I, Dragonfly Patriarch, can address this insult to my clan!\"");
	show_message(Q_STAR, "\"Listen, we have no great emnity with you. Your clan simply needs to do a better job keepings its Chosen in check. We are willing to walk away.\"");
	show_message(D_PATRIARCH, "\"The time for words has long passed.\"");
	show_message(Q_HAWK, "\"He's in no mood to negotiate. Star, get ready!\"");
	hide_sp();
	force_fixed_encounter(23);
	draw_sp(3);
	if (after_boss()) return;
	CURRENT_MOD = MOD_TEMP32;
	wait();
	show_message(D_PATRIARCH, "\"It's not over. You will not get away with this.\"");
	wait();
	dissolve_sp(0);
	gs.plot[PLOT_ELVL] = 16;
}
void plot5() {
	CURRENT_MOD = MOD_TEMP32;
	force_camera_facing();
	draw_sp_fadein(4);
	wait();
	show_message(D_RAVEN, "\"I see some intrusive insects have emerged.\"");
	show_message(Q_HAWK, "\"Oh great. Don't tell me, you're the Dragonfly Matriarch and you're really angry with us for demolishing your clan.\"");
	show_message(D_RAVEN, "\"I don't have one whit of care for your clan squabbles, but I will take possession of those spirit crystals and spirit items you've been hoarding.\"");
	show_message(Q_STAR, "\"We aren't going to let you do that!\"");
	hide_sp();
	force_fixed_encounter(24);
	draw_sp(4);
	if (after_boss()) return;
	CURRENT_MOD = MOD_TEMP32;
	wait();
	show_message(D_RAVEN, "\"Thunder... Sky... I'm sorry...\"");
	wait();
	dissolve_sp(0);
	gs.plot[PLOT_ELVL] = 20;
}
void plot6() {
	CURRENT_MOD = MOD_TEMP32;
	force_camera_facing();
	draw_sp_fadein(5);
	wait();
	show_message(R_EMPEROR, "\"I see others have come for what is rightfully mine, but only I, Righteous Emperor, have the strength to take it!\"");
	show_message(Q_HAWK, "\"Try telling that to the Dragonfly clan.\"");
	show_message(R_EMPEROR, "\"Laugh it up while you still can. Your adventure and your lives end here.\"");
	show_message(Q_STAR, "\"Hawk, watch out!\"");
	hide_sp();
	force_fixed_encounter(25);
	draw_sp(5);
	if (after_boss()) return;
	CURRENT_MOD = MOD_TEMP32;
	wait();
	show_message(R_EMPEROR, "\"What... how?\"");
	show_message(Q_HAWK, "\"We learned how to fight here. Our ancestors stretching back to time memorial trained on these grounds.\"");
	show_message(Q_STAR, "\"As far as we are concerned, this place is ours. Who are you, and why did you come here?\"");
	show_message(R_EMPEROR, "\"Ugh...\"");
	wait();
	dissolve_sp(1);
	wait();
	MESSAGE_OFFSET = 0;
	CURRENT_MOD = MOD_MAHOPE;
	show_message(Q_HAWK, "\"He's dead.\"");
	show_message(Q_STAR, "\"I'm glad that's over. Clan brother Hawk, grab everything that you can and let's go.\"");
	show_message(Q_HAWK, "\"The Qilin will never be vanquished.\"");
	show_message(Q_STAR, "\"Never.\"");
}

int plot_handle()
{
	BATTLE_CANNOT_RUN = 1;
	PLOT_HANDLE(0,   8, 9, 1, plot1);   //  2/280
	PLOT_HANDLE(5,   7, 12, 3, plot2);  // 20/280
	PLOT_HANDLE(8,   2, 9, 6, plot3);   // 73/280
	PLOT_HANDLE(9,   5, 8, 15, plot4);  // 154/280
	PLOT_HANDLE(10,   14, 3, 12, plot5); // 219/280
	PLOT_HANDLE(11,   12, 2, 0, plot6);  // 
	PLOT_HANDLE(11,   12, 3, 0, plot6);  //
	BATTLE_CANNOT_RUN = 0;
	if (BATTLE_OUTCOME == -1) return 1;
	if (gs.plot[PLOT_BASE] == 12) {
		REG_BLDCNT = BLD_TOP(BLD_ALL | BLD_BACKDROP) | BLD_BLACK;
		WON_GAME = 1;
		for (int i = 0; i <= 16; i++) {
			REG_BLDY = i;
			VBlankIntrWait();
			VBlankIntrWait();
			VBlankIntrWait();
			VBlankIntrWait();
		}
		return 1;
	}
	return 0;
}
