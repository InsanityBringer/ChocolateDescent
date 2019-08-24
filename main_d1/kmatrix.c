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
/*
 * $Source: f:/miner/source/main/rcs/kmatrix.c $
 * $Revision: 2.3 $
 * $Author: john $
 * $Date: 1995/05/02 17:01:22 $
 *
 * Kill matrix displayed at end of level.
 *
 */

#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "misc/error.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "bios/mono.h"
#include "bios/key.h"
#include "2d/palette.h"
#include "game.h"
#include "gamefont.h"
#include "mem/mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "gamefont.h"
#include "bios/mouse.h"
#include "bios/joy.h"
#include "bios/timer.h"
#include "text.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "2d/pcx.h"
#include "network.h"

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)

int kmatrix_kills_changed = 0;

void kmatrix_draw_item(int  i, int* sorted)
{
	int j, x, y;

	y = 50 + i * 9;

	// Print player name.

	gr_printf(CENTERING_OFFSET(N_players), y, "%s", Players[sorted[i]].callsign);

	for (j = 0; j < N_players; j++) 
	{

		x = 70 + CENTERING_OFFSET(N_players) + j * 25;

		if (sorted[i] == sorted[j]) 
		{
			if (kill_matrix[sorted[i]][sorted[j]] == 0) {
				gr_set_fontcolor(BM_XRGB(10, 10, 10), -1);
				gr_printf(x, y, "%d", kill_matrix[sorted[i]][sorted[j]]);
			}
			else 
			{
				gr_set_fontcolor(BM_XRGB(25, 25, 25), -1);
				gr_printf(x, y, "-%d", kill_matrix[sorted[i]][sorted[j]]);
			}
		}
		else 
		{
			if (kill_matrix[sorted[i]][sorted[j]] <= 0) 
			{
				gr_set_fontcolor(BM_XRGB(10, 10, 10), -1);
				gr_printf(x, y, "%d", kill_matrix[sorted[i]][sorted[j]]);
			}
			else {
				gr_set_fontcolor(BM_XRGB(25, 25, 25), -1);
				gr_printf(x, y, "%d", kill_matrix[sorted[i]][sorted[j]]);
			}
		}
	}

	x = 70 + CENTERING_OFFSET(N_players) + N_players * 25;
	gr_set_fontcolor(BM_XRGB(25, 25, 25), -1);
	gr_printf(x, y, "%4d", Players[sorted[i]].net_kills_total);
}

void kmatrix_draw_names(int* sorted)
{
	int j, x;

	int color;

	for (j = 0; j < N_players; j++)
	{
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[j]);
		else
			color = sorted[j];

		x = 70 + CENTERING_OFFSET(N_players) + j * 25;
		gr_set_fontcolor(gr_getcolor(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b), -1);
		gr_printf(x, 40, "%c", Players[sorted[j]].callsign[0]);
	}

	x = 70 + CENTERING_OFFSET(N_players) + N_players * 25;
	gr_set_fontcolor(BM_XRGB(31, 31, 31), -1);
	gr_printf(x, 40, TXT_KILLS);

}


void kmatrix_draw_deaths(int* sorted)
{
	int j, x, y;

	y = 55 + N_players * 9;

	//	gr_set_fontcolor(gr_getcolor(player_rgb[j].r,player_rgb[j].g,player_rgb[j].b),-1 );
	gr_set_fontcolor(BM_XRGB(31, 31, 31), -1);

	x = CENTERING_OFFSET(N_players);
	gr_printf(x, y, TXT_DEATHS);

	for (j = 0; j < N_players; j++) 
	{
		x = 70 + CENTERING_OFFSET(N_players) + j * 25;
		gr_printf(x, y, "%d", Players[sorted[j]].net_killed_total);
	}

	y = 55 + 72 + 12;
	x = 35;

	{
		int sw, sh, aw;
		gr_get_string_size(TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
		gr_printf(160 - (sw / 2), y, TXT_PRESS_ANY_KEY2);
	}
}

void kmatrix_redraw()
{
	int i, pcx_error, color;

	int sorted[MAX_NUM_NET_PLAYERS];

	multi_sort_kill_list();

	gr_set_current_canvas(NULL);

	pcx_error = pcx_read_bitmap("STARS.PCX", &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, NULL);
	Assert(pcx_error == PCX_ERROR_NONE);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	gr_string(0x8000, 15, TXT_KILL_MATRIX_TITLE);

	grd_curcanv->cv_font = Gamefonts[GFONT_SMALL];

	multi_get_kill_list(sorted);

	kmatrix_draw_names(sorted);

	for (i = 0; i < N_players; i++) 
	{
		//		mprintf((0, "Sorted kill list pos %d = %d.\n", i+1, sorted[i]));

		if (Game_mode & GM_TEAM)
			color = get_team(sorted[i]);
		else
			color = sorted[i];

		gr_set_fontcolor(gr_getcolor(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b), -1);
		kmatrix_draw_item(i, sorted);
	}

	kmatrix_draw_deaths(sorted);
}

#define MAX_VIEW_TIME	F1_0*60


void kmatrix_view(int network)
{
	int i, k, done;
	fix entry_time = timer_get_approx_seconds();
	int key;

	set_screen_mode(SCREEN_MENU);

	kmatrix_redraw();

	gr_palette_fade_in(gr_palette, 32, 0);
	game_flush_inputs();

	done = 0;

	while (!done) 
	{

		for (i = 0; i < 4; i++)
			if (joy_get_button_down_cnt(i) > 0) done = 1;
		for (i = 0; i < 3; i++)
			if (mouse_button_down_count(i) > 0) done = 1;

		k = key_inkey();
		switch (k)
		{
		case KEY_ENTER:
		case KEY_SPACEBAR:
		case KEY_ESC:
			done = 1;
			break;
		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;
		case KEY_BACKSP:
			Int3();
			break;
		default:
			break;
		}
		if (timer_get_approx_seconds() > entry_time + MAX_VIEW_TIME)
			done = 1;

		if (network && (Game_mode & GM_NETWORK))
		{
			kmatrix_kills_changed = 0;
			network_endlevel_poll2(0, NULL, &key, 0);
			if (kmatrix_kills_changed) 
			{
				kmatrix_redraw();
			}
			if (key < -1)
				done = 1;
		}
	}

	// Restore background and exit
	gr_palette_fade_out(gr_palette, 32, 0);

	game_flush_inputs();
}
#endif
