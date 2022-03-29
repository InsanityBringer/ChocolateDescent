/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "misc/error.h"
#include "menu.h"
#include "platform/mono.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "wall.h"
#include "arcade.h"
#include "screens.h"
#include "stringtable.h"

int hud_first = 0;
int hud_last = 0;

#define HUD_MESSAGE_LENGTH	150
#define HUD_MAX_NUM 4

int 	HUD_nmessages = 0;
fix	HUD_message_timer = 0;		// Time, relative to Players[Player_num].time (int.frac seconds.frac), at which to erase gauge message
char  HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH + 5];

extern void copy_background_rect(int left, int top, int right, int bot);
char Displayed_background_message[HUD_MESSAGE_LENGTH] = "";
int	Last_msg_ycrd = -1;
int	Last_msg_height = 6;
int	HUD_color = -1;

//	-----------------------------------------------------------------------------
void clear_background_messages(void)
{
	if ((Cockpit_mode == CM_STATUS_BAR) && (Last_msg_ycrd != -1) && (VR_render_sub_buffer.cv_bitmap.bm_y >= 6))
	{
		grs_canvas* canv_save = grd_curcanv;
		gr_set_current_canvas(get_current_game_screen());
		copy_background_rect(0, Last_msg_ycrd, grd_curcanv->cv_bitmap.bm_w, Last_msg_ycrd + Last_msg_height - 1);
		gr_set_current_canvas(canv_save);
		Displayed_background_message[0] = 0;
		Last_msg_ycrd = -1;
	}

}

void HUD_clear_messages()
{
	int i;
	HUD_nmessages = 0;
	hud_first = hud_last = 0;
	HUD_message_timer = 0;
	clear_background_messages();
	for (i = 0; i < HUD_MAX_NUM; i++)
		sprintf(HUD_messages[i], "SlagelSlagel!!");
}

//	-----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame()
{
	int i, y, n;
	int h, w, aw;

	if ((HUD_nmessages < 0) || (HUD_nmessages > HUD_MAX_NUM))
		Int3(); // Get Rob!

	if (HUD_nmessages < 1) return;

	HUD_message_timer -= FrameTime;

	if (HUD_message_timer < 0) 
	{
		// Timer expired... get rid of oldest message...
		if (hud_last != hud_first) 
		{
			//&HUD_messages[hud_first][0] is deing deleted...;
			hud_first = (hud_first + 1) % HUD_MAX_NUM;
			HUD_message_timer = F1_0 * 2;
			HUD_nmessages--;
			clear_background_messages();			//	If in status bar mode and no messages, then erase.
		}
	}

	if (HUD_nmessages > 0)
	{
		gr_set_curfont(Gamefonts[GFONT_SMALL]);
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0, 28, 0);

		if ((Cockpit_mode == CM_STATUS_BAR) && (VR_render_sub_buffer.cv_bitmap.bm_y >= 19)) 
		{
			// Only display the most recent message in this mode
			char* message = HUD_messages[(hud_first + HUD_nmessages - 1) % HUD_MAX_NUM];

			if (strcmp(Displayed_background_message, message)) 
			{
				int	ycrd;
				grs_canvas* canv_save = grd_curcanv;

				ycrd = grd_curcanv->cv_bitmap.bm_y - 9;

				if (ycrd < 0)
					ycrd = 0;

				gr_set_current_canvas(get_current_game_screen());

				gr_get_string_size(message, &w, &h, &aw);
				clear_background_messages();
				gr_set_fontcolor(HUD_color, -1);
				gr_printf((grd_curcanv->cv_bitmap.bm_w - w) / 2, ycrd, message);

				strcpy(Displayed_background_message, message);
				gr_set_current_canvas(canv_save);
				Last_msg_ycrd = ycrd;
				Last_msg_height = h;
			}
		}
		else 
		{
			y = 3;

			for (i = 0; i < HUD_nmessages; i++) {
				n = (hud_first + i) % HUD_MAX_NUM;
				if ((n < 0) || (n >= HUD_MAX_NUM))
				{
					Int3(); // Get Rob!!
					return;
				}
				if (!strcmp(HUD_messages[n], "This is a bug."))
					Int3(); // Get Rob!!
				gr_get_string_size(&HUD_messages[n][0], &w, &h, &aw);
				gr_set_fontcolor(HUD_color, -1);
				gr_printf((grd_curcanv->cv_bitmap.bm_w - w) / 2, y, &HUD_messages[n][0]);
				y += h + 1;
			}
		}
	}

	gr_set_curfont(GAME_FONT);
}

void HUD_init_message(const char* format, ...)
{
	va_list args;
	int temp;
	char* message = NULL;
	char* last_message = NULL;

	if ((hud_last < 0) || (hud_last >= HUD_MAX_NUM))
	{
		Int3(); // Get Rob!!
		return; //[ISB] int3 could fall thorugh
	}

	va_start(args, format);
	message = &HUD_messages[hud_last][0];
	vsprintf(message, format, args);
	va_end(args);
	//		mprintf((0, "Hud_message: [%s]\n", message));

	if (HUD_nmessages > 0) 
	{
		if (hud_last == 0)
			last_message = &HUD_messages[HUD_MAX_NUM - 1][0];
		else
			last_message = &HUD_messages[hud_last - 1][0];
	}

	temp = (hud_last + 1) % HUD_MAX_NUM;

	if (temp == hud_first)
	{
		// If too many messages, remove oldest message to make room
		hud_first = (hud_first + 1) % HUD_MAX_NUM;
		HUD_nmessages--;
	}

	if (last_message && (!strcmp(last_message, message))) {
		HUD_message_timer = F1_0 * 3;		// 1 second per 5 characters
		return;	// ignore since it is the same as the last one
	}

	hud_last = temp;
	// Check if memory has been overwritten at this point.
	if (strlen(message) >= HUD_MESSAGE_LENGTH)
		Error("Your message to HUD is too long.  Limit is %i characters.\n", HUD_MESSAGE_LENGTH);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_hud_message(message);
	HUD_message_timer = F1_0 * 3;		// 1 second per 5 characters
	HUD_nmessages++;
}


void player_dead_message(void)
{
	if (!Arcade_mode && Player_exploded) { //(ConsoleObject->flags & OF_EXPLODING)) {
		gr_set_curfont(Gamefonts[GFONT_SMALL]);
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0, 28, 0);
		gr_set_fontcolor(HUD_color, -1);

		gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h - 8, TXT_PRESS_ANY_KEY);
		gr_set_curfont(GAME_FONT);
	}

}

#ifdef RESTORE_AFTERBURNER
void say_afterburner_status(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		HUD_init_message("Afterburner engaged.");
	else
		HUD_init_message("Afterburner disengaged.");
}
#endif
