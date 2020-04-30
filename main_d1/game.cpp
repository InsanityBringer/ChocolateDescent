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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>
#include "misc/rand.h"
#include "inferno.h"
#include "game.h"
#include "platform/key.h"
#include "object.h"
#include "physics.h"
#include "misc/error.h"
#include "platform/joy.h"
#include "platform/mono.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "platform/timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap/texmap.h"
#include "3d/3d.h"
#include "effects.h"
#include "2d/effect2d.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "2d/ibitblt.h"
#include "mem/mem.h"
#include "2d/palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "controls.h"
#include "newmenu.h"
#include "network.h"
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "platform/mouse.h"
#ifdef ARCADE
#include "coindev.h"
#endif
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "2d/pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "2d/gr.h"
#include "2d/i_gr.h"

//#define TEST_TIMER	1		//if this is set, do checking on timer

#define	SHOW_EXIT_PATH	1
#define FINAL_CHEATS 1

#ifdef ARCADE
#include "arcade.h"
#else
#define Arcade_mode 0
#endif

#ifdef EDITOR
#include "editor\editor.h"
#endif

//#define _MARK_ON 1
//#include <wsample.h>            //should come after inferno.h to get mark setting
//Above file is missing in the release version of the source. -KRB

//[ISB] holy fucking shit was watcom c not case sensitive or something
int	speedtest_on = 0;

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
int	Speedtest_start_time;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	Speedtest_count = 0;				//	number of times to do the debug test.
#endif

static fix last_timer_value = 0;

#if defined(TIMER_TEST) && !defined(NDEBUG)
fix _timer_value, actual_last_timer_value, _last_frametime;
int stop_count, start_count;
int time_stopped, time_started;
#endif

uint8_t* Game_cockpit_copy_code = NULL;

uint8_t new_cheats[] = { KEY_B ^ 0xaa, KEY_B ^ 0xaa, KEY_B ^ 0xaa, KEY_F ^ 0xaa, KEY_A ^ 0xaa,
							KEY_U ^ 0xaa, KEY_I ^ 0xaa, KEY_R ^ 0xaa, KEY_L ^ 0xaa, KEY_H ^ 0xaa,
							KEY_G ^ 0xaa, KEY_G ^ 0xaa, KEY_U ^ 0xaa, KEY_A ^ 0xaa, KEY_I ^ 0xaa,
							KEY_G ^ 0xaa, KEY_R ^ 0xaa, KEY_I ^ 0xaa, KEY_S ^ 0xaa, KEY_M ^ 0xaa,
							KEY_I ^ 0xaa, KEY_E ^ 0xaa, KEY_N ^ 0xaa, KEY_H ^ 0xaa, KEY_S ^ 0xaa,
							KEY_N ^ 0xaa, KEY_D ^ 0xaa, KEY_X ^ 0xaa, KEY_X ^ 0xaa, KEY_A ^ 0xaa };

uint8_t			VR_use_paging = 0;
uint8_t			VR_current_page = 0;
uint8_t			VR_switch_eyes = 0;
fix			VR_eye_width = F1_0;
int			VR_eye_offset = 0;
int			VR_eye_offset_changed = 0;
int			VR_screen_mode = 0;
int			VR_render_width = 0;
int			VR_render_height = 0;
int			VR_render_mode = VR_NONE;
int			VR_compatible_menus = 0;
int			VR_low_res = 3;				// Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity = 1;		// 0 - 2
grs_canvas* VR_offscreen_buffer = NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.
grs_canvas	VR_screen_pages[2];					//  Two pages of VRAM if paging is available
grs_canvas	VR_editor_canvas;						//  The canvas that the editor writes to.

int Debug_pause = 0;				//John's debugging pause system

int Cockpit_mode = CM_FULL_COCKPIT;		//set game.h for values
int old_cockpit_mode = -1;
int force_cockpit_redraw = 0;

int framerate_on = 0;

int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.
int	Dummy_var;
int* Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char faded_in;
#endif

#ifndef NDEBUG                          //these only exist if debugging

int Game_double_buffer = 1;     //double buffer by default
fix fixed_frametime = 0;          //if non-zero, set frametime to this

#endif

int Game_suspended = 0;           //if non-zero, nothing moves but player

int Game_vfx_flag = 0;
int Game_victor_flag = 0;
int Game_vio_flag = 0;
int Game_3dmax_flag = 0;


#define VR_SEPARATION F1_0*7/10
#define VR_PIXEL_SHIFT -6
#define VR_DEFAULT_OFFSET_RATIO F1_0
#define VR_MIN_SEPARATION F1_0/20
#define VR_MAX_SEPARATION F1_0*2
#define VR_DEFAULT_OFFSET_RATIO F1_0
#define VR_MIN_OFFSET_RATIO F1_0/5
#define VR_MAX_OFFSET_RATIO F1_0*2

// basically, if pixel-shift equals -6, this adjustment factor
//	  equals 7/6 (fixed-point);
#define VR_ADJUST_OFFSET_RATIO (F1_0*(VR_PIXEL_SHIFT-1)/VR_PIXEL_SHIFT)

int simuleyes_offset_ratio = VR_DEFAULT_OFFSET_RATIO;
int simuleyes_adjusted_offset_ratio;
int simuleyes_right_eye_view;
int Game_simuleyes_flag = 0;

int svr_black = 0x00;
int svr_white = 0xFF;

void vr_reset_display();

fix 	RealFrameTime;
fix	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
fix	Fusion_next_sound_time = 0;

int Debug_spew = 1;
int Game_turbo_mode = 0;

int Game_mode = GM_GAME_OVER;

int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;

grs_bitmap background_bitmap;

int Game_aborted;

extern void newdemo_strip_frames(char*, int);

#define BACKGROUND_NAME "statback.pcx"

//-----------------------------------------------------------------------------
// [ISB] timing nonsense
//-----------------------------------------------------------------------------

//[ISB] FPS limit for the current session, defaults to 30 FPS
int FPSLimit = 30;
//[ISB] Start time for the polled FPS loop
uint64_t startTime = 0;

//	==============================================================================================

#ifndef NDEBUG
void speedtest_init(void)
{
	Speedtest_start_time = timer_get_fixed_seconds();
	speedtest_on = 1;
	Speedtest_segnum = 0;
	Speedtest_sidenum = 0;
	Speedtest_frame_start = FrameCount;

	mprintf((0, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", Highest_segment_index + 1));
}

void speedtest_frame(void)
{
	vms_vector	view_dir, center_point;

	Speedtest_sidenum = Speedtest_segnum % MAX_SIDES_PER_SEGMENT;

	compute_segment_center(&Viewer->pos, &Segments[Speedtest_segnum]);
	Viewer->pos.x += 0x10;		Viewer->pos.y -= 0x10;		Viewer->pos.z += 0x17;

	obj_relink(Viewer - Objects, Speedtest_segnum);
	compute_center_point_on_side(&center_point, &Segments[Speedtest_segnum], Speedtest_sidenum);
	vm_vec_normalized_dir_quick(&view_dir, &center_point, &Viewer->pos);
	vm_vector_2_matrix(&Viewer->orient, &view_dir, NULL, NULL);

	if (((FrameCount - Speedtest_frame_start) % 10) == 0)
		mprintf((0, "."));

	Speedtest_segnum++;

	if (Speedtest_segnum > Highest_segment_index) 
	{
		mprintf((0, "\nSpeedtest done:  %i frames, %7.3f seconds, %7.3f frames/second.\n",
			FrameCount - Speedtest_frame_start,
			f2fl(timer_get_fixed_seconds() - Speedtest_start_time),
			(float)(FrameCount - Speedtest_frame_start) / f2fl(timer_get_fixed_seconds() - Speedtest_start_time)));
		Speedtest_count--;
		if (Speedtest_count == 0)
			speedtest_on = 0;
		else
			speedtest_init();
	}

}

#endif

//this is called once per game
void init_game()
{
	uint8_t pal[256 * 3];
	int pcx_error;

	atexit(close_game);             //for cleanup

	init_objects();

	hostage_init();

	init_special_effects();

	init_ai_system();

	init_gauge_canvases();

	init_exploding_walls();

	background_bitmap.bm_data = NULL;
	pcx_error = pcx_read_bitmap(BACKGROUND_NAME, &background_bitmap, BM_LINEAR, pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s", BACKGROUND_NAME, pcx_errormsg(pcx_error));
	gr_remap_bitmap_good(&background_bitmap, pal, -1, -1);

	Clear_window = 2;		//	do portal only window clear.
}


void reset_palette_add()
{
	PaletteRedAdd = 0;
	PaletteGreenAdd = 0;
	PaletteBlueAdd = 0;
	//gr_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );
}


void game_show_warning(const char* s)
{

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		stop_time();

	nm_messagebox(TXT_WARNING, 1, TXT_OK, s);

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		start_time();
}


//these should be in gr.h 
#define cv_w  cv_bitmap.bm_w
#define cv_h  cv_bitmap.bm_h

#define LETTERBOX_HEIGHT 140

int Game_window_w = 0;
int Game_window_h = 0;
int max_window_h = 0;

int last_drawn_cockpit[2] = { -1, -1 };

extern int Rear_view;

// This actually renders the new cockpit onto the screen.
void update_cockpits(int force_redraw)
{
	int x, y, w, h;

	if (Cockpit_mode != last_drawn_cockpit[VR_current_page] || force_redraw)
		last_drawn_cockpit[VR_current_page] = Cockpit_mode;
	else
		return;

	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE)	return;

	switch (Cockpit_mode) 
	{
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:
		gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
		PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode]);
		gr_ubitmapm(0, 0, &GameBitmaps[cockpit_bitmap[Cockpit_mode].index]);
		break;
	case CM_FULL_SCREEN:
		break;
	case CM_STATUS_BAR:
		gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
		PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode]);
		gr_ubitmapm(0, max_window_h, &GameBitmaps[cockpit_bitmap[Cockpit_mode].index]);
		w = Game_window_w;
		h = Game_window_h;
		x = (VR_render_width - w) / 2;
		y = (max_window_h - h) / 2;
		fill_background(x, y, w, h, x, y);
		break;
	case CM_LETTERBOX:
		gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
		gr_clear_canvas(BM_XRGB(0, 0, 0));
		break;
	}

	gr_set_current_canvas(&VR_screen_pages[VR_current_page]);

	if (Cockpit_mode == CM_FULL_COCKPIT || Cockpit_mode == CM_STATUS_BAR)
		init_gauges();

}

grs_canvas* cockpitCanvas = NULL;

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	int minx, maxx, miny, maxy;

	//Initialize the on-screen canvases

	if (VR_render_mode != VR_NONE)
		Cockpit_mode = CM_FULL_SCREEN;

	if (VR_screen_mode != SM_320x200C) {		// This really should be 'if VR_screen_mode isn't linear'
		// We can only use cockpit or rearview mode in mode 13h
		//if ( (Cockpit_mode==CM_FULL_COCKPIT) || (Cockpit_mode==CM_REAR_VIEW) )	{
		//	if (Game_window_h > max_window_h)			//too big for scalable
		//		Cockpit_mode = CM_FULL_SCREEN;
		//	else
		//		Cockpit_mode = CM_STATUS_BAR;
		//}
		//if ( (grd_curscreen->sc_w > 320) || (VR_use_paging) )
		//		Cockpit_mode = CM_FULL_SCREEN;
		Cockpit_mode = CM_FULL_SCREEN;
	}

	if (Screen_mode == SCREEN_EDITOR)
		Cockpit_mode = CM_FULL_SCREEN;

	gr_set_curfont(GAME_FONT);

	if (Game_cockpit_copy_code)
		free(Game_cockpit_copy_code);
	Game_cockpit_copy_code = NULL;

	switch (Cockpit_mode) 
	{
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW: 
	{
		grs_bitmap* bm = &GameBitmaps[cockpit_bitmap[Cockpit_mode].index];

		PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode]);
		gr_set_current_canvas(VR_offscreen_buffer);
		gr_bitmap(0, 0, bm);
		bm = &VR_offscreen_buffer->cv_bitmap;
		bm->bm_flags = BM_FLAG_TRANSPARENT;
		gr_ibitblt_find_hole_size(bm, &minx, &miny, &maxx, &maxy);
		if (Cockpit_mode == CM_REAR_VIEW)
			printf("%d %d %d %d\n", minx, miny, maxx, maxy);
		Game_cockpit_copy_code = gr_ibitblt_create_mask(bm, minx, miny, maxx - minx + 1, maxy - miny + 1, VR_offscreen_buffer->cv_bitmap.bm_rowsize);
		bm->bm_flags = 0;		// Clear all flags for offscreen canvas
		//game_init_render_sub_buffers(0, 0, maxx - minx + 1, maxy - miny + 1);
		game_init_render_sub_buffers(minx, 0, maxx - minx + 1, maxy - miny + 1); //[ISB] honestly not sure why this works with y set to 0.....
		break;
	}
	case CM_FULL_SCREEN:
		game_init_render_sub_buffers(0, 0, VR_render_width, VR_render_height);
		break;

	case CM_STATUS_BAR: 
	{
		int x, y;

		if (Game_window_h > max_window_h) 
		{
			Game_window_w = VR_render_width;
			Game_window_h = max_window_h;
		}

		x = (VR_render_width - Game_window_w) / 2;
		y = (max_window_h - Game_window_h) / 2;

		game_init_render_sub_buffers(x, y, Game_window_w, Game_window_h);
		break;
	}
	case CM_LETTERBOX: 
	{
		int x, y, w, h;

		x = 0; w = VR_render_width;
		h = LETTERBOX_HEIGHT;
		y = (VR_render_height - h) / 2;

		game_init_render_sub_buffers(x, y, w, h);
		break;
	}
	}
	gr_set_current_canvas(NULL);
}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(int mode)
{
	if (mode != Cockpit_mode) {		//new mode
		Cockpit_mode = mode;
		init_cockpit();
	}
}

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw = 1;
	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;
}

void HUD_clear_messages();

void toggle_cockpit()
{
	int new_mode = 0;

	switch (Cockpit_mode) 
	{

	case CM_FULL_COCKPIT:
		if (Game_window_h > max_window_h)			//too big for scalable
			new_mode = CM_FULL_SCREEN;
		else
			new_mode = CM_STATUS_BAR;
		break;

	case CM_STATUS_BAR:
	case CM_FULL_SCREEN:
		if (Rear_view)
			return;
		new_mode = CM_FULL_COCKPIT;
		break;

	case CM_REAR_VIEW:
	case CM_LETTERBOX:
		return;			//do nothing
		break;

	}

	select_cockpit(new_mode);
	HUD_clear_messages();
}

#define WINDOW_W_DELTA 24	//20
#define WINDOW_H_DELTA 12	//10

#define WINDOW_MIN_W		160

void grow_window()
{
	if (Cockpit_mode == CM_FULL_COCKPIT) {
		Game_window_h = max_window_h;
		Game_window_w = VR_render_width;
		toggle_cockpit();
		grow_window();
		HUD_init_message("Press F3 to return to Cockpit mode");
		return;
	}

	if (Cockpit_mode != CM_STATUS_BAR)
		return;

	if (Game_window_h >= max_window_h) {
		Game_window_w = VR_render_width;
		Game_window_h = VR_render_height;
		select_cockpit(CM_FULL_SCREEN);
	}
	else {
		int x, y;

		Game_window_w += WINDOW_W_DELTA;
		Game_window_h += WINDOW_H_DELTA;

		if (Game_window_h > max_window_h)
			Game_window_h = max_window_h;

		if (Game_window_w > VR_render_width)
			Game_window_w = VR_render_width;

		if (Game_window_h > VR_render_height)
			Game_window_h = VR_render_height;

		x = (VR_render_width - Game_window_w) / 2;
		y = (max_window_h - Game_window_h) / 2;

		game_init_render_sub_buffers(x, y, Game_window_w, Game_window_h);
	}

	HUD_clear_messages();	//	@mk, 11/11/94
}

void copy_background_rect(int left, int top, int right, int bot)
{
	grs_bitmap* bm = &background_bitmap;
	int x, y;
	int tile_left, tile_right, tile_top, tile_bot;
	int ofs_x, ofs_y;
	int dest_x, dest_y;

	tile_left = left / bm->bm_w;
	tile_right = right / bm->bm_w;
	tile_top = top / bm->bm_h;
	tile_bot = bot / bm->bm_h;

	ofs_y = top % bm->bm_h;
	dest_y = top;

	for (y = tile_top; y <= tile_bot; y++) {
		int w, h;

		ofs_x = left % bm->bm_w;
		dest_x = left;

		//h = (bot < dest_y+bm->bm_h)?(bot-dest_y+1):(bm->bm_h-ofs_y);
		h = std::min(bot - dest_y + 1, bm->bm_h - ofs_y);

		for (x = tile_left; x <= tile_right; x++) {

			//w = (right < dest_x+bm->bm_w)?(right-dest_x+1):(bm->bm_w-ofs_x);
			w = std::min(right - dest_x + 1, bm->bm_w - ofs_x);

			gr_bm_ubitblt(w, h, dest_x, dest_y, ofs_x, ofs_y,
				&background_bitmap, &grd_curcanv->cv_bitmap);

			ofs_x = 0;
			dest_x += w;
		}

		ofs_y = 0;
		dest_y += h;
	}


}

void fill_background(int x, int y, int w, int h, int dx, int dy)
{
	gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
	copy_background_rect(x - dx, y - dy, x - 1, y + h + dy - 1);
	copy_background_rect(x + w, y - dy, x + w + dx - 1, y + h + dy - 1);
	copy_background_rect(x, y - dy, x + w - 1, y - 1);
	copy_background_rect(x, y + h, x + w - 1, y + h + dy - 1);
}

void shrink_window()
{
	if (Cockpit_mode == CM_FULL_COCKPIT) {
		Game_window_h = max_window_h;
		Game_window_w = VR_render_width;
		toggle_cockpit();
		//		shrink_window();
		//		shrink_window();
		HUD_init_message("Press F3 to return to Cockpit mode");
		return;
	}

	if (Cockpit_mode == CM_FULL_SCREEN) {
		Game_window_h = max_window_h;
		select_cockpit(CM_STATUS_BAR);
		return;
	}

	if (Cockpit_mode != CM_STATUS_BAR)
		return;

	if (Game_window_w > WINDOW_MIN_W) {
		int x, y;

		Game_window_w -= WINDOW_W_DELTA;
		Game_window_h -= WINDOW_H_DELTA;

		if (Game_window_w < WINDOW_MIN_W)
			Game_window_w = WINDOW_MIN_W;

		x = (VR_render_width - Game_window_w) / 2;
		y = (max_window_h - Game_window_h) / 2;

		fill_background(x, y, Game_window_w, Game_window_h, WINDOW_W_DELTA / 2, WINDOW_H_DELTA / 2);

		game_init_render_sub_buffers(x, y, Game_window_w, Game_window_h);
	}

	HUD_clear_messages();

}

void game_init_render_sub_buffers(int x, int y, int w, int h)
{
	gr_init_sub_canvas(&VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h);
	gr_init_sub_canvas(&VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h);
}


// Sets up the canvases we will be rendering to
void game_init_render_buffers(int screen_mode, int render_w, int render_h, int use_paging, int render_method, int compatible_menus)
{
	if (!VR_offscreen_buffer) 
	{
		VR_use_paging = use_paging;
		VR_switch_eyes = 0;

		VR_eye_width = VR_SEPARATION;    // 11/9/95
		VR_eye_offset = VR_PIXEL_SHIFT;		   // 11/9/95
		simuleyes_adjusted_offset_ratio = fixdiv(VR_DEFAULT_OFFSET_RATIO *
			VR_PIXEL_SHIFT, VR_SEPARATION); // 11/10/95

		VR_screen_mode = screen_mode;
		VR_render_mode = render_method;
		VR_render_width = render_w;
		VR_render_height = render_h;
		VR_compatible_menus = compatible_menus;

		Game_window_w = render_w;
		Game_window_h = render_h;

		/*if ((VR_render_mode == VR_AREA_DET) || (VR_render_mode == VR_INTERLACED)) 
		{
			if (render_h * 2 < 200)
				VR_offscreen_buffer = gr_create_canvas(render_w, 200);
			else
				VR_offscreen_buffer = gr_create_canvas(render_w, render_h * 2);
			gr_init_sub_canvas(&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
			gr_init_sub_canvas(&VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h);
		}
		else*/
		{
			if (render_h < 200)
				VR_offscreen_buffer = gr_create_canvas(render_w, 200);
			else
				VR_offscreen_buffer = gr_create_canvas(render_w, render_h);
			gr_init_sub_canvas(&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
			gr_init_sub_canvas(&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h);
		}
		game_init_render_sub_buffers(0, 0, render_w, render_h);
	}
}

void game_3dmax_off()
{
	Error("game_3dmax_off: STUB\n");
}

void game_3dmax_on()
{
	Error("game_3dmax_on: STUB\n");
}


//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int set_screen_mode(int sm)
{
	if ((sm == SCREEN_MENU) && (Screen_mode == SCREEN_GAME) && VR_compatible_menus) 
	{
		sm = SCREEN_GAME;
	}

#ifdef EDITOR
	if ((sm == SCREEN_MENU) && (Screen_mode == SCREEN_EDITOR)) {
		gr_set_current_canvas(Canv_editor);
		return 1;
	}
#endif

	if (Screen_mode == sm)
	{
		gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
		//		if ( VR_use_paging )	gr_show_canvas( &VR_screen_pages[VR_current_page] );
		return 1;
	}

	Screen_mode = sm;

#ifdef EDITOR
	Canv_editor = NULL;
#endif

	switch (Screen_mode) 
	{
	case SCREEN_MENU:
		I_SetRelative(0); //[ISB] doesn't work rip
		if (grd_curscreen->sc_mode != SM_320x200C) 
		{
			if (gr_set_mode(SM_320x200C)) Error("Cannot set screen mode for game!");
			gr_palette_load(gr_palette);
		}

		gr_init_sub_canvas(&VR_screen_pages[0], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h);
		gr_init_sub_canvas(&VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h);
		break;
	case SCREEN_GAME:
		I_SetRelative(1);
		if (grd_curscreen->sc_mode != VR_screen_mode)
			if (gr_set_mode(VR_screen_mode)) 
			{
				if (VR_screen_mode != SM_640x480V)
					Error("Cannot set screen mode for game!");
				else
					Error("Cannot set screen mode for game!\nMake sure that you have a VESA driver loaded\nthat can display 640x480 in 256 colors.\n");
			}

		/*if (Game_3dmax_flag)
			game_3dmax_on();

		if (Game_vfx_flag)
		{
			vfx_init_graphics();
			Beam_brightness = 0x38000;
		}*/

		if (VR_render_mode == VR_NONE)
		{
			if (max_window_h == 0) 
			{
				max_window_h = grd_curscreen->sc_h - GameBitmaps[cockpit_bitmap[CM_STATUS_BAR].index].bm_h;
				Game_window_h = max_window_h;
			}
			else 
			{
				max_window_h = grd_curscreen->sc_h - GameBitmaps[cockpit_bitmap[CM_STATUS_BAR].index].bm_h;
			}
		}
		else 
		{
			Cockpit_mode = CM_FULL_SCREEN;
		}
		gr_init_sub_canvas(&VR_screen_pages[0], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h);
		if (VR_use_paging)
			gr_init_sub_canvas(&VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, grd_curscreen->sc_h, grd_curscreen->sc_w, grd_curscreen->sc_h);
		else
			gr_init_sub_canvas(&VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h);
		break;
#ifdef EDITOR
	case SCREEN_EDITOR:
		if (grd_curscreen->sc_mode != SM_800x600V) {
			int gr_error;
			if ((gr_error = gr_set_mode(SM_800x600V)) != 0) { //force into game scrren
				Warning("Cannot init editor screen (error=%d)", gr_error);
				return 0;
			}
		}
		gr_palette_load(gr_palette);

		gr_init_sub_canvas(&VR_editor_canvas, &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h);
		Canv_editor = &VR_editor_canvas;
		gr_init_sub_canvas(&VR_screen_pages[0], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		gr_init_sub_canvas(&VR_screen_pages[1], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		gr_set_current_canvas(Canv_editor);
		init_editor_screen();   //setup other editor stuff
		break;
#endif
	default:
		Error("Invalid screen mode %d", sm);
	}

	VR_current_page = 0;

	init_cockpit();

	gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
	if (VR_use_paging)	gr_show_canvas(&VR_screen_pages[VR_current_page]);

	return 1;
}

#ifndef RELEASE
fix frame_time_list[8] = { 0,0,0,0,0,0,0,0 };
fix frame_time_total = 0;
int frame_time_cntr = 0;

void ftoa(char* string, fix f)
{
	int decimal, fractional;

	decimal = f2i(f);
	fractional = ((f & 0xffff) * 100) / 65536;
	if (fractional < 0)
		fractional *= -1;
	if (fractional > 99) fractional = 99;
	sprintf(string, "%d.%02d", decimal, fractional);
}

void show_framerate()
{
	char temp[50];
	fix rate;

	frame_time_total += RealFrameTime - frame_time_list[frame_time_cntr];
	frame_time_list[frame_time_cntr] = RealFrameTime;
	frame_time_cntr = (frame_time_cntr + 1) % 8;

	rate = fixdiv(f1_0 * 8, frame_time_total);

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(gr_getcolor(0, 31, 0), -1);

	ftoa(temp, rate);	// Convert fixed to string
	gr_printf(grd_curcanv->cv_w - 50, grd_curcanv->cv_h - 20, "FPS: %s ", temp);
}
#endif

static int timer_paused = 0;

void stop_time()
{
	if (timer_paused == 0) {
		fix time;
		time = timer_get_fixed_seconds();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
#if defined(TIMER_TEST) && !defined(NDEBUG)
			Int3();		//get Matt!!!!
#endif
			last_timer_value = 0;
		}
#if defined(TIMER_TEST) && !defined(NDEBUG)
		time_stopped = time;
#endif
	}
	timer_paused++;

#if defined(TIMER_TEST) && !defined(NDEBUG)
	stop_count++;
#endif
}

void start_time()
{
	timer_paused--;
	Assert(timer_paused >= 0);
	if (timer_paused == 0) {
		fix time;
		time = timer_get_fixed_seconds();
#if defined(TIMER_TEST) && !defined(NDEBUG)
		if (last_timer_value < 0)
			Int3();		//get Matt!!!!
	}
#endif
	last_timer_value = time - last_timer_value;
#if defined(TIMER_TEST) && !defined(NDEBUG)
	time_started = time;
#endif
}

#if defined(TIMER_TEST) && !defined(NDEBUG)
start_count++;
#endif
}

void game_flush_inputs()
{
	int dx, dy;
	key_flush();
	joy_flush();
	mouse_flush();
	mouse_get_delta(&dx, &dy);	// Read mouse
	memset(&Controls, 0, sizeof(control_info));
}

void reset_time()
{
	last_timer_value = timer_get_fixed_seconds();
}

void calc_frame_time()
{
	fix timer_value, last_frametime = FrameTime;

#if defined(TIMER_TEST) && !defined(NDEBUG)
	_last_frametime = last_frametime;
#endif

	timer_value = timer_get_fixed_seconds();
	FrameTime = timer_value - last_timer_value;

#if defined(TIMER_TEST) && !defined(NDEBUG)
	_timer_value = timer_value;
#endif

#ifndef NDEBUG
	if (!(((FrameTime > 0) && (FrameTime <= F1_0)) || (Function_mode == FMODE_EDITOR) || (Newdemo_state == ND_STATE_PLAYBACK))) {
		mprintf((1, "Bad FrameTime - value = %x\n", FrameTime));
		if (FrameTime == 0)
			Int3();	//	Call Mike or Matt or John!  Your interrupts are probably trashed!
//		if ( !dpmi_virtual_memory )
//			Int3();		//Get MATT if hit this!
	}
#endif

#if defined(TIMER_TEST) && !defined(NDEBUG)
	actual_last_timer_value = last_timer_value;
#endif

	if (Game_turbo_mode)
		FrameTime *= 2;

	// Limit frametime to be between 5 and 150 fps.
	RealFrameTime = FrameTime;
	if (FrameTime < F1_0 / 150) FrameTime = F1_0 / 150;
	if (FrameTime > F1_0 / 5) FrameTime = F1_0 / 5;

	last_timer_value = timer_value;

	if (FrameTime < 0)						//if bogus frametime...
		FrameTime = last_frametime;		//...then use time from last frame

#ifndef NDEBUG
	if (fixed_frametime) FrameTime = fixed_frametime;
#endif

#ifndef NDEBUG
	// Pause here!!!
	if (Debug_pause)
	{
		int c;
		c = 0;
		while (c == 0)
		{
			I_DoEvents();
			c = key_peekkey();
		}

		if (c == KEY_P) 
		{
			Debug_pause = 0;
			c = key_inkey();
		}
		last_timer_value = timer_get_fixed_seconds();
	}
#endif

#if Arcade_mode
	FrameTime /= 2;
#endif

#if defined(TIMER_TEST) && !defined(NDEBUG)
	stop_count = start_count = 0;
#endif

	//	Set value to determine whether homing missile can see target.
	//	The lower frametime is, the more likely that it can see its target.
	if (FrameTime <= F1_0 / 16)
		Min_trackable_dot = 3 * (F1_0 - MIN_TRACKABLE_DOT) / 4 + MIN_TRACKABLE_DOT;
	else if (FrameTime < F1_0 / 4)
		Min_trackable_dot = fixmul(F1_0 - MIN_TRACKABLE_DOT, F1_0 - 4 * FrameTime) + MIN_TRACKABLE_DOT;
	else
		Min_trackable_dot = MIN_TRACKABLE_DOT;

}

//--unused-- int Auto_flythrough=0;  //if set, start flythough automatically

void move_player_2_segment(segment* seg, int side)
{
	vms_vector vp;

	compute_segment_center(&ConsoleObject->pos, seg);
	compute_center_point_on_side(&vp, seg, side);
	vm_vec_sub2(&vp, &ConsoleObject->pos);
	vm_vector_2_matrix(&ConsoleObject->orient, &vp, NULL, NULL);

	obj_relink(ConsoleObject - Objects, SEG_PTR_2_NUM(seg));

}

fix Show_view_text_timer = -1;

#ifndef NDEBUG

void draw_window_label()
{
	if (Show_view_text_timer > 0)
	{
		const char* viewer_name, * control_name;

		Show_view_text_timer -= FrameTime;
		gr_set_curfont(GAME_FONT);

		switch (Viewer->type)
		{
		case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
		case OBJ_ROBOT:		viewer_name = "Robot"; break;
		case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
		case OBJ_PLAYER:		viewer_name = "Player"; break;
		case OBJ_WEAPON:		viewer_name = "Weapon"; break;
		case OBJ_CAMERA:		viewer_name = "Camera"; break;
		case OBJ_POWERUP:		viewer_name = "Powerup"; break;
		case OBJ_DEBRIS:		viewer_name = "Debris"; break;
		case OBJ_CNTRLCEN:	viewer_name = "Control Center"; break;
		default:					viewer_name = "Unknown"; break;
		}

		switch (Viewer->control_type) {
		case CT_NONE:			control_name = "Stopped"; break;
		case CT_AI:				control_name = "AI"; break;
		case CT_FLYING:		control_name = "Flying"; break;
		case CT_SLEW:			control_name = "Slew"; break;
		case CT_FLYTHROUGH:	control_name = "Flythrough"; break;
		case CT_MORPH:			control_name = "Morphing"; break;
		default:					control_name = "Unknown"; break;
		}

		gr_set_fontcolor(gr_getcolor(31, 0, 0), -1);
		gr_printf(0x8000, 45, "%s View - %s", viewer_name, control_name);

	}
}
#endif


void render_countdown_gauge()
{
	if (!Endlevel_sequence && Fuelcen_control_center_destroyed && (Fuelcen_seconds_left > -1) && (Fuelcen_seconds_left < 127)) 
	{
		int	y;
		gr_set_curfont(Gamefonts[4]);    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0, 63, 0), -1);
		y = 20;
		if (!((Cockpit_mode == CM_STATUS_BAR) && (Game_window_w >= 19)))
			y += 5;

		gr_printf(0x8000, y, "T-%d s", Fuelcen_seconds_left);
	}
}

#ifdef NETWORK
void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN + 25];

	if ((Game_mode & GM_MULTI) && (multi_sending_message)) {
		gr_set_curfont(GAME_FONT);    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0, 63, 0), -1);
		sprintf(temp_string, "%s: %s_", TXT_MESSAGE, Network_message);
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h / 2 - 16, temp_string);

	}

	if ((Game_mode & GM_MULTI) && (multi_defining_message)) {
		gr_set_curfont(GAME_FONT);    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0, 63, 0), -1);
		sprintf(temp_string, "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message);
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h / 2 - 16, temp_string);
	}
}
#endif

// Returns the length of the first 'n' characters of a string.
int string_width(char* s, int n)
{
	int w, h, aw;
	char p;
	p = s[n];
	s[n] = 0;
	gr_get_string_size(s, &w, &h, &aw);
	s[n] = p;
	return w;
}

// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void draw_centered_text(int y, char* s)
{
	int i, l;
	char p;


	l = strlen(s);

	if (string_width(s, l) < grd_curcanv->cv_bitmap.bm_w) {
		gr_string(0x8000, y, s);
		return;
	}

	for (i = 0; i < l; i++) {
		if (string_width(s, i) > (grd_curcanv->cv_bitmap.bm_w - 16)) {
			p = s[i];
			s[i] = 0;
			gr_string(0x8000, y, s);
			s[i] = p;
			gr_string(0x8000, y + grd_curcanv->cv_font->ft_h + 1, &s[i]);
			return;
		}
	}
}

extern fix Cruise_speed;

void game_draw_hud_stuff()
{

#ifndef NDEBUG
	if (Debug_pause) {
		gr_set_curfont(HELP_FONT);
		gr_set_fontcolor(gr_getcolor(31, 31, 31), -1); // gr_getcolor(31,0,0));
		gr_ustring(0x8000, 85 / 2, "Debug Pause - Press P to exit");
	}
#endif

#ifdef CROSSHAIR
	if (Viewer->type == OBJ_PLAYER)
		laser_do_crosshair(Viewer);
#endif

#ifndef NDEBUG
	draw_window_label();
#endif

#ifdef NETWORK
	game_draw_multi_message();
#endif

	if ((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) {
		char message[128];
		int h, w, aw;

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_vcr_state != ND_STATE_PRINTSCREEN) {
				sprintf(message, "%s (%d%%%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			}
			else {
				sprintf(message, "");
			}
		}
		else
			sprintf(message, TXT_DEMO_RECORDING);

		gr_set_curfont(GAME_FONT);    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(27, 0, 0), -1);

		gr_get_string_size(message, &w, &h, &aw);
		if (Cockpit_mode == CM_FULL_COCKPIT)
			h += 15;
		else if (Cockpit_mode == CM_LETTERBOX)
			h += 7;

		if (Cockpit_mode != CM_REAR_VIEW)
			gr_printf((grd_curcanv->cv_bitmap.bm_w - w) / 2, grd_curcanv->cv_bitmap.bm_h - h - 2, message);
	}

	render_countdown_gauge();

	if (Player_num > -1 && Viewer->type == OBJ_PLAYER && Viewer->id == Player_num) {
		int	x = 3;
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(gr_getcolor(0, 31, 0), -1);
		if (Cruise_speed > 0)
			if (Cockpit_mode == CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= 64;
				else
					y -= 24;
			}
			else if (Cockpit_mode == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= 48;
				else
					y -= 24;
			}
			else {
				y = 12;
				x = 20;
			}

		gr_printf(x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed));

	}

#ifdef ARCADE
	arcade_frame_info();
#endif

#ifndef RELEASE
	if (framerate_on)
		show_framerate();
#endif

#ifndef SHAREWARE
	if ((Newdemo_state == ND_STATE_PLAYBACK))
		Game_mode = Newdemo_game_mode;
#endif

	draw_hud();

#ifndef SHAREWARE
	if ((Newdemo_state == ND_STATE_PLAYBACK))
		Game_mode = GM_NORMAL;
#endif

	if (Player_is_dead)
		player_dead_message();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_wait_for_retrace;
extern int gr_bitblt_double;

//render a frame for the game in stereo
void game_render_frame_stereo_vfx()
{
	Error("game_render_frame_stereo_vfx: STUB\n");
}



//render a frame for the game in stereo
void game_render_frame_stereo_interlaced()
{
	Error("game_render_frame_stereo_interlaced: STUB\n");
}



//render a frame for the game
void game_render_frame_mono(void)
{
	grs_canvas Screen_3d_window;

	gr_init_sub_canvas(&Screen_3d_window, &VR_screen_pages[0],
		VR_render_sub_buffer[0].cv_bitmap.bm_x, VR_render_sub_buffer[0].
		cv_bitmap.bm_y, VR_render_sub_buffer[0].cv_bitmap.bm_w,
		VR_render_sub_buffer[0].cv_bitmap.bm_h);

	if (Game_double_buffer)
		gr_set_current_canvas(&VR_render_sub_buffer[0]);
	else 
	{
		gr_set_current_canvas(&Screen_3d_window);
	}

	render_frame(0);

	game_draw_hud_stuff();

	if (Game_double_buffer) //copy to visible screen
	{
		if (Game_cockpit_copy_code == NULL) 
		{
			if (VR_use_paging) {
				VR_current_page = !VR_current_page;
				gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
				gr_bm_ubitblt(VR_render_sub_buffer[0].cv_w,
					VR_render_sub_buffer[0].cv_h, VR_render_sub_buffer[0].
					cv_bitmap.bm_x, VR_render_sub_buffer[0].cv_bitmap.bm_y,
					0, 0, &VR_render_sub_buffer[0].cv_bitmap,
					&VR_screen_pages[VR_current_page].cv_bitmap);
				gr_wait_for_retrace = 0;
				gr_show_canvas(&VR_screen_pages[VR_current_page]);
				gr_wait_for_retrace = 1;
			}
			else 
			{
				gr_bm_ubitblt(VR_render_sub_buffer[0].cv_w, VR_render_sub_buffer[0].cv_h, VR_render_sub_buffer[0].cv_bitmap.bm_x, VR_render_sub_buffer[0].cv_bitmap.bm_y, 0, 0, &VR_render_sub_buffer[0].cv_bitmap, &VR_screen_pages[0].cv_bitmap);
			}
		}
		else 
		{
			gr_ibitblt(&VR_render_buffer[0].cv_bitmap, &VR_screen_pages[0].cv_bitmap, Game_cockpit_copy_code);
		}
	}

	if (Cockpit_mode == CM_FULL_COCKPIT || Cockpit_mode == CM_STATUS_BAR) 
	{

#ifndef SHAREWARE
		if ((Newdemo_state == ND_STATE_PLAYBACK))
			Game_mode = Newdemo_game_mode;
#endif

		render_gauges();

#ifndef SHAREWARE
		if ((Newdemo_state == ND_STATE_PLAYBACK))
			Game_mode = GM_NORMAL;
#endif
	}

}

void game_render_frame()
{
	set_screen_mode(SCREEN_GAME);

	update_cockpits(0);

	play_homing_warning();

	/*if (VR_render_mode == VR_INTERLACED)
		game_render_frame_stereo_interlaced();
	else if (VR_render_mode == VR_AREA_DET)
		game_render_frame_stereo_vfx();
	else if (VR_render_mode == VR_NONE)*/
		game_render_frame_mono();

	// Make sure palette is faded in
	stop_time();
	gr_palette_fade_in(gr_palette, 32, 0);
	start_time();

}

#ifdef EDITOR
void do_photos();
void level_with_floor();
#endif

void save_screen_shot(int automap_flag)
{
	fix t1;
	char message[100];
	grs_canvas* screen_canv = &grd_curscreen->sc_canvas;
	grs_font* save_font;
	static int savenum = 0;
	grs_canvas* temp_canv, * save_canv;
	char savename[13];
	uint8_t pal[768];
	int w, h, aw, x, y;

	// Can't do screen shots in VR modes.
	if (VR_render_mode != VR_NONE)
		return;

	stop_time();

	save_canv = grd_curcanv;
	temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w, screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(0, 0, &screen_canv->cv_bitmap);

	if (savenum > 99) savenum = 0;
	sprintf(savename, "screen%02d.pcx", savenum++);
	sprintf(message, "%s '%s'", TXT_DUMPING_SCREEN, savename);

	gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
	save_font = grd_curcanv->cv_font;
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(gr_find_closest_color_current(0, 31, 0), -1);
	gr_get_string_size(message, &w, &h, &aw);
	x = (VR_screen_pages[VR_current_page].cv_w - w) / 2;
	y = (VR_screen_pages[VR_current_page].cv_h - h) / 2;

	if (automap_flag) 
	{
		modex_print_message(32, y, message);
	}
	else 
	{
		gr_setcolor(gr_find_closest_color_current(0, 0, 0));
		gr_rect(x - 2, y - 2, x + w + 2, y + h + 2);
		gr_printf(x, y, message);
		gr_set_curfont(save_font);
	}
	t1 = timer_get_fixed_seconds() + F1_0;

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename, &temp_canv->cv_bitmap, pal);

	while (timer_get_fixed_seconds() < t1);		// Wait so that messag stays up at least 1 second.

	gr_set_current_canvas(screen_canv);

	if (!automap_flag)
		gr_ubitmap(0, 0, &temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);

	gr_set_current_canvas(save_canv);
	key_flush();
	start_time();
}

//initialize flying
void fly_init(object* obj)
{
	obj->control_type = CT_FLYING;
	obj->movement_type = MT_PHYSICS;

	vm_vec_zero(&obj->mtype.phys_info.velocity);
	vm_vec_zero(&obj->mtype.phys_info.thrust);
	vm_vec_zero(&obj->mtype.phys_info.rotvel);
	vm_vec_zero(&obj->mtype.phys_info.rotthrust);
}

//void morph_test(), morph_step();

int sound_nums[] = { 10,11,20,21,30,31,32,33,40,41,50,51,60,61,62,70,80,81,82,83,90,91 };

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))

int test_sound_num = 0;

void play_test_sound()
{
	digi_play_sample(sound_nums[test_sound_num], F1_0);
}

//	------------------------------------------------------------------------------------
void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num = 0;

}

#ifdef EDITOR
void test_anim_states();
#endif

#include "fvi.h"

//put up the help message
void do_show_help()
{
	show_help();
}

//--unused-- int save_newdemo_state;

extern int been_in_editor;

#ifdef RESTORE_AFTERBURNER
//	------------------------------------------------------------------------------------
//	Return 1 if toggled, else return 0.
//	Might not be able to toggle if off and not allowed to turn on.
int toggle_afterburner_status(void)
{
	player	*pp = &Players[Player_num];

	if (!(pp->flags & PLAYER_FLAGS_AFTERBURNER)) 
	{
		//	Was off, try to turn on.
		if ((pp->afterburner_time > AFTERBURNER_MAX_TIME/4) && (pp->energy > 0)) 
		{
			pp->flags |= PLAYER_FLAGS_AFTERBURNER;
			say_afterburner_status();
		} 
		else 
		{
			mprintf((0, "Cannot turn on afterburner due to energy or recharge.\n"));
			return 0;
		}
		return 1;
	} else 
	{
		pp->flags ^= PLAYER_FLAGS_AFTERBURNER;
		say_afterburner_status();
		return 1;
	}
}
#endif

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;
	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			// mprintf(0, "Cloak time left: %7.3f\n", f2fl(CLOAK_TIME_MAX - (GameTime - Players[Player_num].cloak_time)));
			if (GameTime - Players[i].cloak_time > CLOAK_TIME_MAX) {
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample(SOUND_CLOAK_OFF, F1_0);
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					multi_send_decloak(); // For demo recording
#endif
//					mprintf((0, " --- You have been DE-CLOAKED! ---\n"));
				}
			}
		}
}

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (GameTime - Players[Player_num].invulnerable_time > INVULNERABLE_TIME_MAX) {
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
#ifdef NETWORK
			maybe_drop_net_powerup(POW_INVULNERABILITY);
#endif
			digi_play_sample(SOUND_INVULNERABILITY_OFF, F1_0);
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
#endif
			mprintf((0, " --- You have been DE-INVULNERABLEIZED! ---\n"));
		}
	}
}

#ifdef RESTORE_AFTERBURNER
//	------------------------------------------------------------------------------------
void do_afterburner_stuff(void)
{
	player	*pp = &Players[Player_num];

//	mprintf(0, "[Afterburner] Time: %7.3f, status = %i\n", f2fl(pp->afterburner_time), pp->flags & PLAYER_FLAGS_AFTERBURNER);

	if (pp->flags & PLAYER_FLAGS_AFTERBURNER)
	{
		if (pp->afterburner_time > 0)
		{
			pp->afterburner_time -= FrameTime;
			pp->energy -= FrameTime/2;
			if (pp->afterburner_time <= 0)
			{
				pp->afterburner_time = 0;
				pp->flags &= ~PLAYER_FLAGS_AFTERBURNER;
				say_afterburner_status();
			}
			if (pp->energy <= 0) 
			{
				pp->energy = 0;
				pp->flags &= ~PLAYER_FLAGS_AFTERBURNER;
				say_afterburner_status();
			}
		}
	} else 
	{
		//	Since afterburner is probably almost always max, do the check, we save time.
		if (pp->afterburner_time < AFTERBURNER_MAX_TIME) 
		{
			pp->afterburner_time += FrameTime/2;
			if (pp->afterburner_time > AFTERBURNER_MAX_TIME)
				pp->afterburner_time = AFTERBURNER_MAX_TIME;
		}
	}
}
#endif

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE	16		//	gots to be a power of 2, else change the code in diminish_palette_towards_normal

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;

	//	Diminish at DIMINISH_RATE units/second.
	//	For frame rates > DIMINISH_RATE Hz, use randomness to achieve this.
	if (FrameTime < F1_0 / DIMINISH_RATE) {
		if (P_Rand() < FrameTime * DIMINISH_RATE / 2)	//	Note: P_Rand() is in 0..32767, and 8 Hz means decrement every frame
			dec_amount = 1;
	}
	else {
		dec_amount = f2i(FrameTime * DIMINISH_RATE);		// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;						// make sure we decrement by something
	}

	if (PaletteRedAdd > 0) { PaletteRedAdd -= dec_amount; if (PaletteRedAdd < 0) PaletteRedAdd = 0; }
	if (PaletteRedAdd < 0) { PaletteRedAdd += dec_amount; if (PaletteRedAdd > 0) PaletteRedAdd = 0; }

	if (PaletteGreenAdd > 0) { PaletteGreenAdd -= dec_amount; if (PaletteGreenAdd < 0) PaletteGreenAdd = 0; }
	if (PaletteGreenAdd < 0) { PaletteGreenAdd += dec_amount; if (PaletteGreenAdd > 0) PaletteGreenAdd = 0; }

	if (PaletteBlueAdd > 0) { PaletteBlueAdd -= dec_amount; if (PaletteBlueAdd < 0) PaletteBlueAdd = 0; }
	if (PaletteBlueAdd < 0) { PaletteBlueAdd += dec_amount; if (PaletteBlueAdd > 0) PaletteBlueAdd = 0; }

	if ((Newdemo_state == ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd))
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	gr_palette_step_up(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	//mprintf(0, "%2i %2i %2i\n", PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);
}

int	Redsave, Bluesave, Greensave;

void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

void palette_restore(void)
{
	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);
}

extern void dead_player_frame(void);

#ifndef RELEASE
void do_cheat_menu()
{
	int mmn;
	newmenu_item mm[16];
	char score_text[21];

	sprintf(score_text, "%d", Players[Player_num].score);

	mm[0].type = NM_TYPE_CHECK; mm[0].value = Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE; mm[0].text = (char*)"Invulnerability";
	mm[1].type = NM_TYPE_CHECK; mm[1].value = Players[Player_num].flags & PLAYER_FLAGS_IMMATERIAL; mm[1].text = (char*)"Immaterial";
	mm[2].type = NM_TYPE_CHECK; mm[2].value = 0; mm[2].text = (char*)"All keys";
	mm[3].type = NM_TYPE_NUMBER; mm[3].value = f2i(Players[Player_num].energy); mm[3].text = (char*)"% Energy"; mm[3].min_value = 0; mm[3].max_value = 200;
	mm[4].type = NM_TYPE_NUMBER; mm[4].value = f2i(Players[Player_num].shields); mm[4].text = (char*)"% Shields"; mm[4].min_value = 0; mm[4].max_value = 200;
	mm[5].type = NM_TYPE_TEXT; mm[5].text = (char*)"Score:";
	mm[6].type = NM_TYPE_INPUT; mm[6].text_len = 10; mm[6].text = score_text;
	mm[7].type = NM_TYPE_RADIO; mm[7].value = (Players[Player_num].laser_level == 0); mm[7].group = 0; mm[7].text = (char*)"Laser level 1";
	mm[8].type = NM_TYPE_RADIO; mm[8].value = (Players[Player_num].laser_level == 1); mm[8].group = 0; mm[8].text = (char*)"Laser level 2";
	mm[9].type = NM_TYPE_RADIO; mm[9].value = (Players[Player_num].laser_level == 2); mm[9].group = 0; mm[9].text = (char*)"Laser level 3";
	mm[10].type = NM_TYPE_RADIO; mm[10].value = (Players[Player_num].laser_level == 3); mm[10].group = 0; mm[10].text = (char*)"Laser level 4";
	mm[11].type = NM_TYPE_NUMBER; mm[11].value = Players[Player_num].secondary_ammo[CONCUSSION_INDEX]; mm[11].text = (char*)"Missiles"; mm[11].min_value = 0; mm[11].max_value = 200;

	mmn = newmenu_do("Wimp Menu", NULL, 12, mm, NULL);

	if (mmn > -1) {
		if (mm[0].value) 
		{
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime + i2f(1000);
		}
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if (mm[1].value)
			Players[Player_num].flags |= PLAYER_FLAGS_IMMATERIAL;
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_IMMATERIAL;
		if (mm[2].value) Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		Players[Player_num].energy = i2f(mm[3].value);
		Players[Player_num].shields = i2f(mm[4].value);
		Players[Player_num].score = atoi(mm[6].text);
		if (mm[7].value) Players[Player_num].laser_level = 0;
		if (mm[8].value) Players[Player_num].laser_level = 1;
		if (mm[9].value) Players[Player_num].laser_level = 2;
		if (mm[10].value) Players[Player_num].laser_level = 3;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[11].value;
		init_gauges();
	}
}
#endif

//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_is_dead)
	{
		Global_missile_firing_count = 0;
		return 0;
	}

	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (Next_laser_fire_time > GameTime)
		if (Next_laser_fire_time < GameTime + 2 * F1_0)
			return 0;

	return 1;
}

fix	Next_flare_fire_time = 0;

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime)
		if (Next_flare_fire_time < GameTime + F1_0)	//	In case time is bogus, never wait > 1 second.
			return 0;

	Next_flare_fire_time = GameTime + F1_0 / 4;

	return 1;
}

int allowed_to_fire_missile(void)
{
	// mprintf(0, "Next fire = %7.3f, Cur time = %7.3f\n", f2fl(Next_missile_fire_time), f2fl(GameTime));
		//	Make sure enough time has elapsed to fire missile, but if it looks like it will
		//	be a long while before missile can be fired, then there must be some mistake!
	if (Next_missile_fire_time > GameTime)
		if (Next_missile_fire_time < GameTime + 5 * F1_0)
			return 0;

	return 1;
}

typedef struct bkg 
{
	short x, y, w, h;			// The location of the menu.
	grs_bitmap* bmp;			// The background under the menu.
} bkg;

bkg bg = { 0,0,0,0,NULL };

//show a message in a nice little box
void show_boxed_message(char* msg)
{
	int w, h, aw;
	int x, y;

	gr_set_current_canvas(&VR_screen_pages[VR_current_page]);
	gr_set_curfont(HELP_FONT);

	gr_get_string_size(msg, &w, &h, &aw);

	x = (grd_curscreen->sc_w - w) / 2;
	y = (grd_curscreen->sc_h - h) / 2;

	if (bg.bmp) {
		gr_free_bitmap(bg.bmp);
		bg.bmp = NULL;
	}

	// Save the background of the display
	bg.x = x; bg.y = y; bg.w = w; bg.h = h;

	bg.bmp = gr_create_bitmap(w + 30, h + 30);
	gr_bm_ubitblt(w + 30, h + 30, 0, 0, x - 15, y - 15, &(grd_curscreen->sc_canvas.cv_bitmap), bg.bmp);

	nm_draw_background(x - 15, y - 15, x + w + 15 - 1, y + h + 15 - 1);

	gr_set_fontcolor(gr_getcolor(31, 31, 31), -1);
	gr_ustring(0x8000, y, msg);

}

void clear_boxed_message()
{
	if (bg.bmp) 
	{
		gr_bitmap(bg.x - 15, bg.y - 15, bg.bmp);
		gr_free_bitmap(bg.bmp);
		bg.bmp = NULL;
	}
}

extern int Death_sequence_aborted;

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int do_game_pause(int allow_menu)
{
	int paused;
	int key;

	if (Game_mode & GM_MULTI)
	{
		HUD_init_message(TXT_CANT_PAUSE);
		return(KEY_PAUSE);
	}

	digi_pause_all();
	stop_time();

	palette_save();
	reset_palette_add();

	game_flush_inputs();

	paused = 1;

	set_screen_mode(SCREEN_MENU);
	gr_palette_load(gr_palette);

	show_boxed_message(TXT_PAUSE);
	//I_DrawCurrentCanvas(0);

	while (paused) 
	{
		I_MarkStart();
		I_DrawCurrentCanvas(0);
		I_DoEvents();
		key = key_getch();

		switch (key)
		{
		case 0:
			break;

		case KEY_ESC:
			if (allow_menu)
				Function_mode = FMODE_MENU;
			clear_boxed_message();
			paused = 0;
			break;

		case KEY_F1:
			clear_boxed_message();
			do_show_help();
			show_boxed_message(TXT_PAUSE);
			break;

		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;

#ifndef RELEASE
		case KEY_BACKSP: Int3(); break;
#endif

		default:
			clear_boxed_message();
			paused = 0;
			break;
		}

		I_MarkEnd(1000000 / FPSLimit);
	}

	game_flush_inputs();

	palette_restore();

	start_time();
	digi_resume_all();

	return key;
}


void show_help()
{
	newmenu_item m[14];

	//if (VR_render_mode != VR_NONE) //[ISB] I'm confused
	{
		m[0].type = NM_TYPE_TEXT; m[0].text = TXT_HELP_ESC;
		m[1].type = NM_TYPE_TEXT; m[1].text = TXT_HELP_ALT_F2;
		m[2].type = NM_TYPE_TEXT; m[2].text = TXT_HELP_ALT_F3;
		m[3].type = NM_TYPE_TEXT; m[3].text = TXT_HELP_F2;
		m[4].type = NM_TYPE_TEXT; m[4].text = TXT_HELP_F4;
		m[5].type = NM_TYPE_TEXT; m[5].text = TXT_HELP_F5;
		m[6].type = NM_TYPE_TEXT; m[6].text = TXT_HELP_PAUSE;
		m[7].type = NM_TYPE_TEXT; m[7].text = TXT_HELP_1TO5;
		m[8].type = NM_TYPE_TEXT; m[8].text = TXT_HELP_6TO10;
		m[9].type = NM_TYPE_TEXT; m[9].text = (char*)"";
		m[10].type = NM_TYPE_TEXT; m[10].text = TXT_HELP_TO_VIEW;
		newmenu_do(NULL, TXT_KEYS, 11, m, NULL);
	}
}


#ifdef ARCADE
void arcade_frame_info()
{
	if (!Arcade_mode) return;

	if (Newdemo_state == ND_STATE_PLAYBACK) {
		gr_set_curfont(GAME_FONT);    //game_font );
		gr_set_fontcolor(gr_getcolor(0, 31, 0), -1);
		gr_printf(0x8000, 5, "Insert Coins to Play");
		return;
	}

	if (Arcade_timer > 0) {
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(gr_getcolor(0, 31, 0), -1);
		gr_printf(0x8000, 5, "%d seconds left", f2i(Arcade_timer));
	}
	else {
		gr_set_curfont(Gamefonts[GFONT_BIG_1]);    //GAME_FONT );
		gr_printf(0x8000, 40, "Game Over");

		gr_set_curfont(Gamefonts[GFONT_MEDIUM_2]);    //GAME_FONT );
		gr_printf(0x8000, 60, "Insert Coins to Continue");
		gr_printf(0x8000, 75, "%d", f2i(Arcade_timer) + 10);
	}
}
#endif

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

	static int leave_mode;
	static fix entry_time;

	if (Controls.rear_view_down_count) //key/button has gone down
	{

		if (Rear_view) 
		{
			Rear_view = 0;
			if (Cockpit_mode == CM_REAR_VIEW) 
			{
				select_cockpit(old_cockpit_mode);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else 
		{
			Rear_view = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = timer_get_fixed_seconds();
			if (Cockpit_mode == CM_FULL_COCKPIT) 
			{
				old_cockpit_mode = Cockpit_mode;
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else if (Controls.rear_view_down_state) 
	{
		if (leave_mode == 0 && (timer_get_fixed_seconds() - entry_time) > LEAVE_TIME)
			leave_mode = 1;
	}
	else 
	{
		if (leave_mode == 1 && Rear_view) 
		{
			Rear_view = 0;
			if (Cockpit_mode == CM_REAR_VIEW) 
			{
				select_cockpit(old_cockpit_mode);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
	}
}

void reset_rear_view(void)
{
	if (Rear_view) 
	{
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_restore_rearview();
	}

	Rear_view = 0;

	if (Cockpit_mode == CM_REAR_VIEW)
		select_cockpit(old_cockpit_mode);
}

#ifdef ARCADE
int keys_override;
#endif

int Automap_flag;
int Config_menu_flag;

jmp_buf LeaveGame;

#ifndef FINAL_CHEATS
uint8_t cheat_enable_keys[] = { KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_H,KEY_E,KEY_Y };
#endif

int8_t	Enable_john_cheat_1, Enable_john_cheat_2, Enable_john_cheat_3, Enable_john_cheat_4;

int cheat_enable_index;
#define CHEAT_ENABLE_LENGTH (sizeof(cheat_enable_keys) / sizeof(*cheat_enable_keys))

#ifdef FINAL_CHEATS
uint8_t cheat_enable_keys[] = { KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_H,KEY_E,KEY_Y };

uint8_t cheat_wowie[] = { KEY_S,KEY_C,KEY_O,KEY_U,KEY_R,KEY_G,KEY_E };
uint8_t cheat_allkeys[] = { KEY_M,KEY_I,KEY_T,KEY_Z,KEY_I };
uint8_t cheat_invuln[] = { KEY_R,KEY_A,KEY_C,KEY_E,KEY_R,KEY_X };
uint8_t cheat_cloak[] = { KEY_G,KEY_U,KEY_I,KEY_L,KEY_E };
uint8_t cheat_shield[] = { KEY_T,KEY_W,KEY_I,KEY_L,KEY_I,KEY_G,KEY_H,KEY_T };
uint8_t cheat_warp[] = { KEY_F,KEY_A,KEY_R,KEY_M,KEY_E,KEY_R,KEY_J,KEY_O,KEY_E };
uint8_t cheat_astral[] = { KEY_A,KEY_S,KEY_T,KEY_R,KEY_A,KEY_L };

#define NUM_NEW_CHEATS 5

#define CHEAT_WOWIE_LENGTH (sizeof(cheat_wowie) / sizeof(*cheat_wowie))
#define CHEAT_ALLKEYS_LENGTH (sizeof(cheat_allkeys) / sizeof(*cheat_allkeys))
#define CHEAT_INVULN_LENGTH (sizeof(cheat_invuln) / sizeof(*cheat_invuln))
#define CHEAT_CLOAK_LENGTH (sizeof(cheat_cloak) / sizeof(*cheat_cloak))
#define CHEAT_SHIELD_LENGTH (sizeof(cheat_shield) / sizeof(*cheat_shield))
#define CHEAT_WARP_LENGTH (sizeof(cheat_warp) / sizeof(*cheat_warp))
#define CHEAT_ASTRAL_LENGTH (sizeof(cheat_astral) / sizeof(*cheat_astral))

#define CHEAT_TURBOMODE_OFS	0
#define CHEAT_WOWIE2_OFS		1
#define CHEAT_NEWLIFE_OFS		2
#define CHEAT_EXITPATH_OFS		3
#define CHEAT_ROBOTPAUSE_OFS	4

#define CHEAT_TURBOMODE_LENGTH	6
#define CHEAT_WOWIE2_LENGTH		6
#define CHEAT_NEWLIFE_LENGTH		5
#define CHEAT_EXITPATH_LENGTH		5
#define CHEAT_ROBOTPAUSE_LENGTH	6

int cheat_wowie_index;
int cheat_allkeys_index;
int cheat_invuln_index;
int cheat_cloak_index;
int cheat_shield_index;
int cheat_warp_index;
int cheat_astral_index;
int cheat_turbomode_index;
int cheat_wowie2_index;
int cheat_newlife_index;
int cheat_exitpath_index;
int cheat_robotpause_index;

#endif

int Cheats_enabled = 0;

extern int Laser_rapid_fire, Ugly_robot_cheat;
extern void do_lunacy_on(), do_lunacy_off();

extern int Physics_cheat_flag;

void game_disable_cheats()
{
	Game_turbo_mode = 0;
	Cheats_enabled = 0;
	do_lunacy_off();
	Laser_rapid_fire = 0;
	Ugly_robot_cheat = 0;
	Physics_cheat_flag = 0;
}

//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game()
{
	do_lunacy_on();		//	Copy values for insane into copy buffer in ai.c
	do_lunacy_off();		//	Restore true insane mode.

	Game_aborted = 0;
	last_drawn_cockpit[0] = -1;				// Force cockpit to redraw next time a frame renders.
	last_drawn_cockpit[1] = -1;				// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	cheat_enable_index = 0;

#ifdef FINAL_CHEATS
	cheat_wowie_index = cheat_allkeys_index = cheat_invuln_index = cheat_cloak_index = cheat_shield_index = cheat_warp_index = cheat_astral_index = 0;
	cheat_turbomode_index = cheat_wowie2_index = 0;
#endif

	set_screen_mode(SCREEN_GAME);
	reset_palette_add();

#ifdef ARCADE
	keys_override = FindArg("-keys");
#endif

	set_warn_func(game_show_warning);

	init_cockpit();
	init_gauges();
	//digi_init_sounds();

	//keyd_repeat = 0;                // Don't allow repeat in game
	keyd_repeat = 1;                // Do allow repeat in game

	//_MARK_("start of game");Commented out -KRB

#ifdef EDITOR
	if (Segments[ConsoleObject->segnum].segnum == -1)      //segment no longer exists
		obj_relink(ConsoleObject - Objects, SEG_PTR_2_NUM(Cursegp));

	if (!check_obj_seg(ConsoleObject))
		move_player_2_segment(Cursegp, Curside);
#endif

	Viewer = ConsoleObject;
	fly_init(ConsoleObject);

	Game_suspended = 0;

#ifdef ARCADE
	if (Arcade_mode) {
		NewGame(1);
		newdemo_start_playback(NULL);
	}
#endif

	reset_time();
	FrameTime = 0;			//make first frame zero

#ifdef EDITOR
	if (Current_level_num == 0) {			//not a real level
		init_player_stats_game();
		init_ai_objects();
	}
#endif

	fix_object_segs();

	game_flush_inputs();

	if (setjmp(LeaveGame) == 0) 
	{
		if (VR_screen_mode != SCREEN_MENU)
			vr_reset_display();

		while (1) 
		{
			// GAME LOOP!
			Automap_flag = 0;
			Config_menu_flag = 0;

			startTime = I_GetUS();

			Assert(ConsoleObject == &Objects[Players[Player_num].objnum]);

			GameLoop(1, 1);		// Do game loop with rendering and reading controls.

			if (Config_menu_flag) 
			{
				if (!(Game_mode & GM_MULTI)) palette_save();
				do_options_menu();
				if (!(Game_mode & GM_MULTI)) palette_restore();
			}

			if (Automap_flag) 
			{
				int save_w = Game_window_w, save_h = Game_window_h;
				if (Game_3dmax_flag)
					game_3dmax_off();
				do_automap(0);
				Screen_mode = -1; set_screen_mode(SCREEN_GAME);
				Game_window_w = save_w; Game_window_h = save_h;
				init_cockpit();
				last_drawn_cockpit[0] = -1;
				last_drawn_cockpit[1] = -1;
				if (Game_3dmax_flag)
					game_3dmax_on();

				if (VR_screen_mode != SCREEN_MENU)
					vr_reset_display();
			}

			if ((Function_mode != FMODE_GAME) && Auto_demo && (Newdemo_state != ND_STATE_NORMAL))
			{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				choice = nm_messagebox(NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
				Function_mode = fmode;
				if (choice == 0) {
					Auto_demo = 0;
					newdemo_stop_playback();
					Function_mode = FMODE_MENU;
				}
				else 
				{
					Function_mode = FMODE_GAME;
				}
			}

			if ((Function_mode != FMODE_GAME) && (Newdemo_state != ND_STATE_PLAYBACK) && (Function_mode != FMODE_EDITOR)) 
			{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				choice = nm_messagebox(NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				Function_mode = fmode;
				if (choice != 0)
					Function_mode = FMODE_GAME;
			}

			if (Function_mode != FMODE_GAME)
				longjmp(LeaveGame, 0);

			//[ISB] assumption is that anything calling without renderflag (basically network mode) will already be updating. 
			I_DrawCurrentCanvas(0);
			I_DoEvents();
			//waiting loop for polled fps mode
			uint64_t numUS = 1000000 / FPSLimit;
			//[ISB] Combine a sleep with the polling loop to try to spare CPU cycles
			uint64_t diff = (startTime + numUS) - I_GetUS();
			if (diff > 2000) //[ISB] Sleep only if there's sufficient time to do so, since the scheduler isn't precise enough
				I_DelayUS(diff - 2000);
			while (I_GetUS() < startTime + numUS);
		}
	}

	digi_stop_all();

	if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
		newdemo_stop_recording();

#ifdef NETWORK
	multi_leave_game();
#endif

	if (Newdemo_state == ND_STATE_PLAYBACK)
		newdemo_stop_playback();

	if (Function_mode != FMODE_EDITOR)
		gr_palette_fade_out(gr_palette, 32, 0);			// Fade out before going to menu

	//_MARK_("end of game");//Commented out -KRB

	clear_warn_func(game_show_warning);     //don't use this func anymore

	game_disable_cheats();
}

extern void john_cheat_func_1(int);
extern void john_cheat_func_2(int);
extern void john_cheat_func_3(int);
extern void john_cheat_func_4(int);

//called at the end of the program
void close_game()
{
	if (VR_offscreen_buffer) 
	{
		gr_free_canvas(VR_offscreen_buffer);
		VR_offscreen_buffer = NULL;
	}

	close_gauge_canvases();

	restore_effect_bitmap_icons();

	if (Game_cockpit_copy_code) 
	{
		free(Game_cockpit_copy_code);
		Game_cockpit_copy_code = NULL;
	}

	if (background_bitmap.bm_data)
		free(background_bitmap.bm_data);

	clear_warn_func(game_show_warning);     //don't use this func anymore
}

grs_canvas* get_current_game_screen()
{
	return &VR_screen_pages[VR_current_page];
}

uint8_t exploding_flag = 0;

#ifdef EDITOR
extern void dump_used_textures_all();
#endif
//extern kconfig_center_headset();

#ifdef SHOW_EXIT_PATH
int create_special_path(void);
#endif

void ReadControls()
{
	int key;
	fix key_time;
	static fix newdemo_single_frame_time;

#ifdef ARCADE
	if (Arcade_mode) {
		int coins;
		if (Newdemo_state != ND_STATE_PLAYBACK) {
			Arcade_timer -= 2 * FrameTime;
			if (Arcade_timer < (-F1_0 * 10)) {
				newdemo_toggle_playback();
			}
		}
		coins = coindev_count(0);
		if (coins > 0) {
			if (Newdemo_state == ND_STATE_PLAYBACK) {
				newdemo_toggle_playback();
				Arcade_timer = F1_0 * ARCADE_FIRST_SECONDS;		// Two minutes to play...
				if (coins > 1)
					Arcade_timer += F1_0 * ARCADE_CONTINUE_SECONDS * (coins - 1);		// Two minutes to play...
				NewGame(1);
			}
			else {
				if (Arcade_timer < 0)
					Arcade_timer = 0;
				Arcade_timer += F1_0 * ARCADE_CONTINUE_SECONDS * coins;		// Two minutes to play...
			}
		}
	}
#endif

	Player_fired_laser_this_frame = -1;

#ifndef NDEBUG
	if (speedtest_on)
		speedtest_frame();
#endif

	if (!Endlevel_sequence && !Player_is_dead) {

#ifdef ARCADE
		if (Arcade_mode) {
			if (Arcade_timer > 0)
				if (Newdemo_state == ND_STATE_PLAYBACK)
					memset(&Controls, 0, sizeof(control_info));
				else
					controls_read_all();		//NOTE LINK TO ABOVE!!!
		}
		else
#endif
			if ((Newdemo_state == ND_STATE_PLAYBACK)
#ifdef NETWORK
				|| multi_sending_message || multi_defining_message
#endif
				) 	// WATCH OUT!!! WEIRD CODE ABOVE!!!
				memset(&Controls, 0, sizeof(control_info));
			else
				controls_read_all();		//NOTE LINK TO ABOVE!!!

		check_rear_view();

		//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if (Controls.automap_down_count && !((Game_mode & GM_MULTI) && Fuelcen_control_center_destroyed && (Fuelcen_seconds_left < 10)))
			Automap_flag = 1;

		if (Controls.fire_flare_down_count)
			if (allowed_to_fire_flare())
				Flare_create(ConsoleObject);

		if (allowed_to_fire_missile())
			Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[Secondary_weapon]].fire_count * (Controls.fire_secondary_state || Controls.fire_secondary_down_count);

		if (Global_missile_firing_count) 
		{
			do_missile_firing();
			Global_missile_firing_count--;
		}

		if (Global_missile_firing_count < 0)
			Global_missile_firing_count = 0;

		//	Drop proximity bombs.
		if (Controls.drop_bomb_down_count) 
		{
			int	ssw_save = Secondary_weapon;
			Secondary_weapon = PROXIMITY_INDEX;
			while (Controls.drop_bomb_down_count--)
				do_missile_firing();
			Secondary_weapon = ssw_save;
		}
	}

	if (Player_exploded) 
	{
		if (exploding_flag == 0) 
		{
			exploding_flag = 1;	 		// When player starts exploding, clear all input devices...
			game_flush_inputs();
		}
		else 
		{
			int i;
			if (key_down_count(KEY_BACKSP))
				Int3();
			if (key_down_count(KEY_PRINT_SCREEN))
				save_screen_shot(0);
			for (i = 0; i < 4; i++)
				if (joy_get_button_down_cnt(i) > 0) Death_sequence_aborted = 1;
			for (i = 0; i < 3; i++)
				if (mouse_button_down_count(i) > 0) Death_sequence_aborted = 1;
			for (i = 0; i < 256; i++)
				if (key_down_count(i) > 0) Death_sequence_aborted = 1;
			if (Death_sequence_aborted)
				game_flush_inputs();
		}
	}
	else 
	{
		exploding_flag = 0;
	}

	if (Newdemo_state == ND_STATE_PLAYBACK) 
	{
		if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT])
			Newdemo_vcr_state = ND_STATE_FASTFORWARD;
		else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT])
			Newdemo_vcr_state = ND_STATE_REWINDING;
		else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((timer_get_fixed_seconds() - newdemo_single_frame_time) >= F1_0))
			Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
		else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((timer_get_fixed_seconds() - newdemo_single_frame_time) >= F1_0))
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
		else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
			Newdemo_vcr_state = ND_STATE_PLAYBACK;
	}

	while ((key = key_inkey_time(&key_time)) != 0) 
	{
		john_cheat_func_1(key);

#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (multi_sending_message || multi_defining_message)) {
			multi_message_input_sub(key);
			key = 0;		// Wipe out key!
		}
#endif

		if (!(Game_mode & GM_MULTI) && key == cheat_enable_keys[cheat_enable_index]) 
		{
			if (++cheat_enable_index == CHEAT_ENABLE_LENGTH) 
			{
				HUD_init_message(TXT_CHEATS_ENABLED);
				digi_play_sample(SOUND_CHEATER, F1_0);
				Cheats_enabled = 1;
				Players[Player_num].score = 0;
			}
		}
		else
			cheat_enable_index = 0;

		john_cheat_func_2(key);

#ifdef FINAL_CHEATS
		if (Cheats_enabled) 
		{
			if (!(Game_mode & GM_MULTI) && key == cheat_wowie[cheat_wowie_index]) 
			{
				if (++cheat_wowie_index == CHEAT_WOWIE_LENGTH)
				{
					int i;

					HUD_init_message(TXT_WOWIE_ZOWIE);
					digi_play_sample(SOUND_CHEATER, F1_0);

					Players[Player_num].primary_weapon_flags |= 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
					Players[Player_num].secondary_weapon_flags |= 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);

					for (i = 0; i < 3; i++)
						Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

					for (i = 0; i < 3; i++)
						Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

					if (Newdemo_state == ND_STATE_RECORDING)
						newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

					Players[Player_num].energy = MAX_ENERGY;
					Players[Player_num].laser_level = MAX_LASER_LEVEL;
					Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
					update_laser_weapon_info();

					cheat_wowie_index = 0;
				}
			}
			else
				cheat_wowie_index = 0;

			if (!(Game_mode & GM_MULTI) && key == (0xaa ^ new_cheats[cheat_wowie2_index * NUM_NEW_CHEATS + CHEAT_WOWIE2_OFS]))
			{
				if (++cheat_wowie2_index == CHEAT_WOWIE2_LENGTH)
				{
					int i;

					HUD_init_message("SUPER %s", TXT_WOWIE_ZOWIE);
					digi_play_sample(SOUND_CHEATER, F1_0);

					Players[Player_num].primary_weapon_flags = 0xff;
					Players[Player_num].secondary_weapon_flags = 0xff;

					for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
						Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

					for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
						Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

					if (Newdemo_state == ND_STATE_RECORDING)
						newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

					Players[Player_num].energy = MAX_ENERGY;
					Players[Player_num].laser_level = MAX_LASER_LEVEL;
					Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
					update_laser_weapon_info();

					cheat_wowie2_index = 0;
				}
			}
			else
				cheat_wowie2_index = 0;

			if (!(Game_mode & GM_MULTI) && key == cheat_allkeys[cheat_allkeys_index]) 
			{
				if (++cheat_allkeys_index == CHEAT_ALLKEYS_LENGTH) 
				{
					HUD_init_message(TXT_ALL_KEYS);
					digi_play_sample(SOUND_CHEATER, F1_0);
					Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;

					cheat_allkeys_index = 0;
				}
			}
			else
				cheat_allkeys_index = 0;


			if (!(Game_mode & GM_MULTI) && key == cheat_invuln[cheat_invuln_index]) 
			{
				if (++cheat_invuln_index == CHEAT_INVULN_LENGTH)
				{
					Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
					HUD_init_message("%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) ? TXT_ON : TXT_OFF);
					digi_play_sample(SOUND_CHEATER, F1_0);
					Players[Player_num].invulnerable_time = GameTime + i2f(1000);

					cheat_invuln_index = 0;
				}
			}
			else
				cheat_invuln_index = 0;

			if (!(Game_mode & GM_MULTI) && key == cheat_cloak[cheat_cloak_index]) 
			{
				if (++cheat_cloak_index == CHEAT_CLOAK_LENGTH) 
				{
					Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
					HUD_init_message("%s %s!", TXT_CLOAK, (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) ? TXT_ON : TXT_OFF);
					digi_play_sample(SOUND_CHEATER, F1_0);
					if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) 
					{
						ai_do_cloak_stuff();
						Players[Player_num].cloak_time = GameTime;
					}

					cheat_cloak_index = 0;
				}
			}
			else
				cheat_cloak_index = 0;

			if (!(Game_mode & GM_MULTI) && key == cheat_shield[cheat_shield_index]) 
			{
				if (++cheat_shield_index == CHEAT_SHIELD_LENGTH) 
				{
					HUD_init_message(TXT_FULL_SHIELDS);
					digi_play_sample(SOUND_CHEATER, F1_0);
					Players[Player_num].shields = MAX_SHIELDS;

					cheat_shield_index = 0;
				}
			}
			else
				cheat_shield_index = 0;

			if (!(Game_mode & GM_MULTI) && key == cheat_warp[cheat_warp_index]) 
			{
				if (++cheat_warp_index == CHEAT_WARP_LENGTH) 
				{
					newmenu_item m;
					char text[10] = "";
					int new_level_num;
					int item;
					digi_play_sample(SOUND_CHEATER, F1_0);
					m.type = NM_TYPE_INPUT; m.text_len = 10; m.text = text;
					item = newmenu_do(NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL);
					if (item != -1) {
						new_level_num = atoi(m.text);
						if (new_level_num != 0 && new_level_num >= 0 && new_level_num <= Last_level)
							StartNewLevel(new_level_num);
					}

					cheat_warp_index = 0;
				}
			}
			else
				cheat_warp_index = 0;

			if (!(Game_mode & GM_MULTI) && key == cheat_astral[cheat_astral_index]) 
			{
				if (++cheat_astral_index == CHEAT_ASTRAL_LENGTH) 
				{
					digi_play_sample(SOUND_CHEATER, F1_0);
					if (Physics_cheat_flag == 0xBADA55) 
					{
						Physics_cheat_flag = 0;
					}
					else 
					{
						Physics_cheat_flag = 0xBADA55;
					}
					HUD_init_message("%s %s!", "Ghosty mode", Physics_cheat_flag == 0xBADA55 ? TXT_ON : TXT_OFF);
					cheat_astral_index = 0;
				}
			}
			else
				cheat_astral_index = 0;

			if (!(Game_mode & GM_MULTI) && key == (0xaa ^ new_cheats[cheat_turbomode_index * NUM_NEW_CHEATS + CHEAT_TURBOMODE_OFS])) 
			{
				if (++cheat_turbomode_index == CHEAT_TURBOMODE_LENGTH) 
				{
					Game_turbo_mode ^= 1;
					HUD_init_message("%s %s!", "Turbo mode", Game_turbo_mode ? TXT_ON : TXT_OFF);
					digi_play_sample(SOUND_CHEATER, F1_0);
				}
			}
			else
				cheat_turbomode_index = 0;

			if (!(Game_mode & GM_MULTI) && key == (0xaa ^ new_cheats[cheat_newlife_index * NUM_NEW_CHEATS + CHEAT_NEWLIFE_OFS])) 
			{
				if (++cheat_newlife_index == CHEAT_NEWLIFE_LENGTH) 
				{
					if (Players[Player_num].lives < 50)
					{
						Players[Player_num].lives++;
						HUD_init_message("Extra life!");
						digi_play_sample(SOUND_CHEATER, F1_0);
					}

					cheat_newlife_index = 0;
				}
			}
			else
				cheat_newlife_index = 0;

			if (!(Game_mode & GM_MULTI) && key == (0xaa ^ new_cheats[cheat_exitpath_index * NUM_NEW_CHEATS + CHEAT_EXITPATH_OFS])) 
			{
				if (++cheat_exitpath_index == CHEAT_EXITPATH_LENGTH)
				{
#ifdef SHOW_EXIT_PATH
					if (create_special_path()) 
					{
						HUD_init_message("Exit path illuminated!");
						digi_play_sample(SOUND_CHEATER, F1_0);
					}
#endif
					cheat_exitpath_index = 0;
				}
			}
			else
				cheat_exitpath_index = 0;


			if (!(Game_mode & GM_MULTI) && key == (0xaa ^ new_cheats[cheat_robotpause_index * NUM_NEW_CHEATS + CHEAT_ROBOTPAUSE_OFS])) {
				if (++cheat_robotpause_index == CHEAT_ROBOTPAUSE_LENGTH) {
					Robot_firing_enabled = !Robot_firing_enabled;
					HUD_init_message("%s %s!", "Robot firing", Robot_firing_enabled ? TXT_ON : TXT_OFF);
					digi_play_sample(SOUND_CHEATER, F1_0);

					cheat_robotpause_index = 0;
				}

			}
			else
				cheat_robotpause_index = 0;


		}
#endif

		john_cheat_func_3(key);

#ifndef RELEASE
#ifdef NETWORK
		if ((key & KEY_DEBUGGED) && (Game_mode & GM_MULTI)) {
			Network_message_reciever = 100;		// Send to everyone...
			sprintf(Network_message, "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
#endif
#endif

		if (Endlevel_sequence) {

			if (key == KEY_PRINT_SCREEN)
				save_screen_shot(0);

			if (key == KEY_PAUSE)
				key = do_game_pause(0);		//so esc from pause will end level

			if (key == KEY_ESC) {
				stop_endlevel_sequence();
				last_drawn_cockpit[0] = -1;
				last_drawn_cockpit[1] = -1;
				return;
			}

			if (key == KEY_BACKSP)
				Int3();

			break;		//don't process any other keys
		}

		john_cheat_func_4(key);

		if (Player_is_dead) {

			if (key == KEY_PRINT_SCREEN)
				save_screen_shot(0);

			if (key == KEY_PAUSE) {
				key = do_game_pause(0);		//so esc from pause will end level
				Death_sequence_aborted = 0;		// Clear because code above sets this for any key.
			}

			if (key == KEY_ESC) {
				if (ConsoleObject->flags & OF_EXPLODING)
					Death_sequence_aborted = 1;
			}

			if (key == KEY_BACKSP) {
				Death_sequence_aborted = 0;		// Clear because code above sets this for any key.
				Int3();
			}

			break;		//don't process any other keys
		}

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			switch (key) {

			case KEY_DEBUGGED + KEY_I:
				Newdemo_do_interpolate = !Newdemo_do_interpolate;
				if (Newdemo_do_interpolate)
					mprintf((0, "demo playback interpolation now on\n"));
				else
					mprintf((0, "demo playback interpolation now off\n"));
				break;
#ifndef NDEBUG
			case KEY_DEBUGGED + KEY_K: {
				int how_many, c;
				char filename[13], num[16];
				newmenu_item m[6];

				filename[0] = '\0';
				m[0].type = NM_TYPE_TEXT; m[0].text = (char*)"output file name";
				m[1].type = NM_TYPE_INPUT; m[1].text_len = 8; m[1].text = filename;
				c = newmenu_do(NULL, NULL, 2, m, NULL);
				if (c == -2)
					break;
				strcat(filename, ".dem");
				num[0] = '\0';
				m[0].type = NM_TYPE_TEXT; m[0].text = (char*)"strip how many bytes";
				m[1].type = NM_TYPE_INPUT; m[1].text_len = 16; m[1].text = num;
				c = newmenu_do(NULL, NULL, 2, m, NULL);
				if (c == -2)
					break;
				how_many = atoi(num);
				if (how_many <= 0)
					break;
				newdemo_strip_frames(filename, how_many);
			}
									   break;
#endif

			case KEY_F3:				toggle_cockpit();			break;
			case KEY_SHIFTED + KEY_MINUS:
			case KEY_MINUS:			shrink_window();			break;
			case KEY_SHIFTED + KEY_EQUAL:
			case KEY_EQUAL:			grow_window();				break;
			case KEY_F2:				Config_menu_flag = 1;	break;
			case KEY_F7:
#ifdef NETWORK
				Show_kill_list = (Show_kill_list + 1) % ((Newdemo_game_mode & GM_TEAM) ? 3 : 2);
#endif
				break;
			case KEY_BACKSP:
				Int3();
				break;
			case KEY_ESC:
				Function_mode = FMODE_MENU;
				break;
			case KEY_UP:
				Newdemo_vcr_state = ND_STATE_PLAYBACK;
				break;
			case KEY_DOWN:
				Newdemo_vcr_state = ND_STATE_PAUSED;
				break;
			case KEY_LEFT:
				newdemo_single_frame_time = timer_get_fixed_seconds();
				Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
				break;
			case KEY_RIGHT:
				newdemo_single_frame_time = timer_get_fixed_seconds();
				Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
				break;
			case KEY_CTRLED + KEY_RIGHT:
				newdemo_goto_end();
				break;
			case KEY_CTRLED + KEY_LEFT:
				newdemo_goto_beginning();
				break;
			case KEY_PAUSE:
				do_game_pause(0);
				break;
			case KEY_PRINT_SCREEN: {
				int old_state;

				old_state = Newdemo_vcr_state;
				Newdemo_vcr_state = ND_STATE_PRINTSCREEN;
				game_render_frame_mono();
				save_screen_shot(0);
				Newdemo_vcr_state = old_state;
				break;
			}
			}
			break;
		}


#ifndef FINAL_CHEATS
		//Here are the "legal" cheats
		if (Cheats_enabled && !(Game_mode & GM_MULTI))
			switch (key) {
			case KEY_ALTED + KEY_1: {
				int i;

				HUD_init_message(TXT_WOWIE_ZOWIE);

#ifndef SHAREWARE
				Players[Player_num].primary_weapon_flags = 0xff;
				Players[Player_num].secondary_weapon_flags = 0xff;
#else
				Players[Player_num].primary_weapon_flags = 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
				Players[Player_num].secondary_weapon_flags = 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);
#endif

				for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
					Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

				for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
					Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

				Players[Player_num].energy = MAX_ENERGY;
				Players[Player_num].laser_level = MAX_LASER_LEVEL;
				Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
				update_laser_weapon_info();

				break;
			}

			case KEY_ALTED + KEY_2:
				HUD_init_message(TXT_ALL_KEYS);
				Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
				break;

			case KEY_ALTED + KEY_3:
				Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
				HUD_init_message("%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) ? TXT_ON : TXT_OFF);
				Players[Player_num].invulnerable_time = GameTime + i2f(1000);
				break;

			case KEY_ALTED + KEY_4:
				Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
				HUD_init_message("%s %s!", TXT_CLOAK, (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) ? TXT_ON : TXT_OFF);
				if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
					ai_do_cloak_stuff();
					Players[Player_num].cloak_time = GameTime;
				}
				break;

			case KEY_ALTED + KEY_5:
				HUD_init_message(TXT_FULL_SHIELDS);
				Players[Player_num].shields = MAX_SHIELDS;
				break;

			case KEY_ALTED + KEY_6: {
				newmenu_item m;
				char text[10] = "";
				int new_level_num;
				int item;
				m.type = NM_TYPE_INPUT; m.text_len = 10; m.text = text;
				item = newmenu_do(NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL);
				if (item != -1) {
					new_level_num = atoi(m.text);
					if (new_level_num != 0 && new_level_num >= 0 && new_level_num <= LAST_LEVEL)
						StartNewLevel(new_level_num);
				}
				break;

			}
			}
#endif
#ifdef RESTORE_AFTERBURNER
		//Players[Player_num].flags &= ~PLAYER_FLAGS_AFTERBURNER;	//	Turn off, keypress might turn it on.
#endif

		switch (key) 
		{

			//	================================================================================================
			//FIRST ARE ALL THE REAL GAME KEYS.  PUT TEST AND DEBUG KEYS LATER.
#ifdef RESTORE_AFTERBURNER
			case KEY_SHIFTED+KEY_A:	toggle_afterburner_status();	break;
#endif
		case KEY_ESC:
			Game_aborted = 1;
			Function_mode = FMODE_MENU;
			break;
		case KEY_F1: 				do_show_help();			break;
		case KEY_F2:				Config_menu_flag = 1;	break;
		case KEY_F3:				toggle_cockpit();			break;
		//case KEY_F4:				palette_save(); joydefs_calibrate(); palette_restore(); break;
		case KEY_F5:
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_stop_recording();
			else if (Newdemo_state == ND_STATE_NORMAL)
				newdemo_start_recording();
			break;
		case KEY_F6:
#ifdef NETWORK
			Show_reticle_name = (Show_reticle_name + 1) % 2;
#endif
			break;
		case KEY_F7:
#ifdef NETWORK
			Show_kill_list = (Show_kill_list + 1) % ((Game_mode & GM_TEAM) ? 3 : 2);
#endif
			break;
		case KEY_F8:
#ifdef NETWORK
			multi_send_message_start();
#endif
			break;
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
#ifdef NETWORK
			multi_send_macro(key);
#endif
			break;		// send taunt macros

		case KEY_ALTED + KEY_F9:
#ifdef NETWORK
			multi_send_audio_taunt(0);
#endif
			break;
		case KEY_ALTED + KEY_F10:
#ifdef NETWORK
			multi_send_audio_taunt(1);
#endif
			break;
		case KEY_ALTED + KEY_F11:
#ifdef NETWORK
			multi_send_audio_taunt(2);
#endif
			break;
		case KEY_ALTED + KEY_F12:
#ifdef NETWORK
			multi_send_audio_taunt(3);
#endif
			break;

		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
#ifdef NETWORK
			multi_define_macro(key);
#endif
			break;		// redefine taunt macros

		case KEY_PAUSE:			do_game_pause(1); 		break;
		case KEY_PRINT_SCREEN: 	save_screen_shot(0);		break;

		case KEY_SHIFTED + KEY_MINUS:
		case KEY_MINUS:			shrink_window();			break;
		case KEY_SHIFTED + KEY_EQUAL:
		case KEY_EQUAL:			grow_window();				break;

			//	Select primary or secondary weapon.
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
			do_weapon_select(key - KEY_1, 0);
			break;

		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_0:
			do_weapon_select(key - KEY_6, 1);
			break;

		case KEY_SHIFTED + KEY_ESC: //quick exit
#ifdef EDITOR
			if (!SafetyCheck()) break;
			close_editor_screen();
#endif

			Game_aborted = 1;
			Function_mode = FMODE_EXIT;
			break;

#ifdef SHAREWARE
		case KEY_ALTED + KEY_F2:
		case KEY_ALTED + KEY_F3:
			HUD_init_message(TXT_ONLY_REGISTERED);
			digi_play_sample(SOUND_BAD_SELECTION, F1_0);
			break;
#else
		case KEY_ALTED + KEY_F2:	state_save_all(0);		break;	// 0 means not between levels.
		case KEY_ALTED + KEY_F3:	state_restore_all(1);		break;
#endif
		//[ISB] kill VR stuff

			//use function keys for window sizing

			//	================================================================================================
			//ALL KEYS BELOW HERE GO AWAY IN RELEASE VERSION

#ifndef RELEASE

		case KEY_DEBUGGED + KEY_0:	show_weapon_status();	break;

#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED + KEY_1:	create_special_path();	break;
#endif

		case KEY_DEBUGGED + KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;

		case KEY_BACKSP:
		case KEY_CTRLED + KEY_BACKSP:
		case KEY_ALTED + KEY_BACKSP:
		case KEY_SHIFTED + KEY_BACKSP:
		case KEY_SHIFTED + KEY_ALTED + KEY_BACKSP:
		case KEY_CTRLED + KEY_ALTED + KEY_BACKSP:
		case KEY_SHIFTED + KEY_CTRLED + KEY_BACKSP:
		case KEY_SHIFTED + KEY_CTRLED + KEY_ALTED + KEY_BACKSP:
			Int3(); break;

		case KEY_DEBUGGED + KEY_S:			digi_reset(); break;

		case KEY_DEBUGGED + KEY_P:
			if (Game_suspended & SUSP_ROBOTS)
				Game_suspended &= ~SUSP_ROBOTS;         //robots move
			else
				Game_suspended |= SUSP_ROBOTS;          //robots don't move
			break;

		case KEY_DEBUGGED + KEY_K:	Players[Player_num].shields = 1;	break;						//	a virtual kill
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_K:	Players[Player_num].shields = -1;	break;	//	an actual kill
		case KEY_DEBUGGED + KEY_X: Players[Player_num].lives++; break; // Extra life cheat key.
		case KEY_DEBUGGED + KEY_H:
			//					if (!(Game_mode & GM_MULTI) )	{
			Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
#endif
				ai_do_cloak_stuff();
				Players[Player_num].cloak_time = GameTime;
				mprintf((0, "You are cloaked!\n"));
			}
			else
				mprintf((0, "You are DE-cloaked!\n"));
			//					}
			break;


		case KEY_DEBUGGED + KEY_R:
			Robot_firing_enabled = !Robot_firing_enabled;
			break;

#ifdef EDITOR		//editor-specific functions

		case KEY_E + KEY_DEBUGGED:
#ifdef NETWORK
			network_leave_game();
#endif
			Function_mode = FMODE_EDITOR;
			break;

		case KEY_C + KEY_SHIFTED + KEY_DEBUGGED:
			if (!(Game_mode & GM_MULTI))
				move_player_2_segment(Cursegp, Curside);
			break;   //move eye to curseg


		case KEY_DEBUGGED + KEY_W:	draw_world_from_game();	break;

#endif	//#ifdef EDITOR

		case KEY_DEBUGGED + KEY_LAPOSTRO: Show_view_text_timer = 0x30000; object_goto_next_viewer(); break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_LAPOSTRO: Viewer = ConsoleObject; break;

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_O: toggle_outline_mode(); break;
#endif
		case KEY_DEBUGGED + KEY_T:
			*Toggle_var = !*Toggle_var;
			mprintf((0, "Variable at %08x set to %i\n", Toggle_var, *Toggle_var));
			break;
		case KEY_DEBUGGED + KEY_L:
			if (++Lighting_on >= 2) Lighting_on = 0; break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_L:
			Beam_brightness = 0x38000 - Beam_brightness; break;
		case KEY_PAD5: slew_stop(); break;

		case KEY_DEBUGGED + KEY_F11: play_test_sound(); break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_F11: advance_sound(); play_test_sound(); break;

		case KEY_DEBUGGED + KEY_M:
			Debug_spew = !Debug_spew;
			if (Debug_spew)
			{
				mopen(0, 8, 1, 78, 16, "Debug Spew");
				HUD_init_message("Debug Spew: ON");
			}
			else 
			{
				mclose(0);
				HUD_init_message("Debug Spew: OFF");
			}
			break;

		case KEY_DEBUGGED + KEY_C:

			do_cheat_menu();
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
			do_megawow_powerup(10);
			break;
		case KEY_DEBUGGED + KEY_A: 
		{
			do_megawow_powerup(200);
			break;
		}

		case KEY_DEBUGGED + KEY_F:	framerate_on = !framerate_on; break;

		case KEY_DEBUGGED + KEY_SPACEBAR:              //KEY_F7:                       // Toggle physics flying
			slew_stop();
			game_flush_inputs();
			if (ConsoleObject->control_type != CT_FLYING)
			{
				fly_init(ConsoleObject);
				Game_suspended &= ~SUSP_ROBOTS;         //robots move
			}
			else 
			{
				slew_init(ConsoleObject);                                              //start player slewing
				Game_suspended |= SUSP_ROBOTS;          //robots don't move
			}
			break;

		case KEY_DEBUGGED + KEY_COMMA: Render_zoom = fixmul(Render_zoom, 62259); break;
		case KEY_DEBUGGED + KEY_PERIOD: Render_zoom = fixmul(Render_zoom, 68985); break;

		case KEY_DEBUGGED + KEY_P + KEY_SHIFTED: Debug_pause = 1; break;

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_F8: speedtest_init(); Speedtest_count = 1;	break;
		case KEY_DEBUGGED + KEY_F9: speedtest_init(); Speedtest_count = 10;	break;

		case KEY_DEBUGGED + KEY_D:
			if ((Game_double_buffer = !Game_double_buffer) != 0)
				init_cockpit();
			break;
#endif

#ifdef EDITOR
		case KEY_DEBUGGED + KEY_Q:
			stop_time();
			dump_used_textures_all();
			start_time();
			break;
#endif

#endif		//#ifndef RELEASE

		default:        break;

		}       //switch (key)
	}
}


void vr_reset_display()
{
}


#ifndef	NDEBUG
int	Debug_slowdown = 0;
#endif

#ifdef EDITOR
extern void player_follow_path(object* objp);
extern void check_create_player_path(void);

#endif

extern	int	Do_appearance_effect;

void GameLoop(int RenderFlag, int ReadControlsFlag)
{
	static int desc_dead_countdown = 100;   /*  used if player shouldn't be playing */

	//I_DrawCurrentCanvas(0);
	//I_DoEvents();
	//[ISB] Okay I really don't want to track all the changes and mini loops and shit
	//so the game loop will ensure the mouse is always in relative mode
	I_SetRelative(1);
#ifndef	NDEBUG
	//	Used to slow down frame rate for testing things.
//	RenderFlag = 1; // DEBUG
	if (Debug_slowdown)
	{
		int	h, i, j = 0;

		for (h = 0; h < Debug_slowdown; h++)
			for (i = 0; i < 1000; i++)
				j += i;
	}
#endif

	if (desc_id_exit_num) // are we supposed to be checking
	{
		if (!(--desc_dead_countdown))  // if so, at zero, then pull the plug
			Error("Loading overlay -- error number: %d\n", (int)desc_id_exit_num);
	}

#ifndef RELEASE
	if (FindArg("-invulnerability"))
		Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
#endif

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
#ifdef RESTORE_AFTERBURNER
	do_afterburner_stuff();
#endif
	do_cloak_stuff();
	do_invulnerable_stuff();
	remove_obsolete_stuck_objects();
#ifdef EDITOR
	check_create_player_path();
	player_follow_path(ConsoleObject);
#endif
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		multi_do_frame();
#endif

	if (RenderFlag)
	{
		if (force_cockpit_redraw) //screen need redrawing?
		{
			init_cockpit();
			force_cockpit_redraw = 0;
		}
		game_render_frame();
	}

	//		mprintf(0,"Velocity %2.2f\n", f2fl(vm_vec_mag(&ConsoleObject->phys_info.velocity)));

#if 0
	mem_fill();
	mem_check();
#endif

	calc_frame_time();

	dead_player_frame();
	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_controlcen_dead_frame();

	if (ReadControlsFlag)
		ReadControls();
	else
		memset(&Controls, 0, sizeof(Controls));

	GameTime += FrameTime;

	digi_sync_sounds();

	if (Endlevel_sequence)
	{
		do_endlevel_frame();
		powerup_grab_cheat_all();
		do_special_effects();
		return;					//skip everything else
	}

#ifdef ARCADE
	if (Arcade_mode && (Arcade_timer < 0) && (Newdemo_state != ND_STATE_PLAYBACK)) {
		memset(&Controls, 0, sizeof(Controls));
		continue;
	}
#endif

	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_exploding_wall_frame();
	if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED))
	{
		do_special_effects();
		wall_frame_process();
		triggers_frame_process();
	}


	if (Fuelcen_control_center_destroyed)
	{
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_control_center_destroyed();
		flash_frame();
	}

	if (Newdemo_state == ND_STATE_PLAYBACK)
	{
		newdemo_playback_one_frame();
		if (Newdemo_state != ND_STATE_PLAYBACK)
		{
			longjmp(LeaveGame, 0);		// Go back to menu
		}
	}
	else
	{ // Note the link to above!

		Players[Player_num].homing_object_dist = -1;		//	Assume not being tracked.  Laser_do_weapon_sequence modifies this.

		object_move_all();
		powerup_grab_cheat_all();

		if (Endlevel_sequence)	//might have been started during move
			return;

		fuelcen_update_all();

		do_ai_frame_all();

		if (allowed_to_fire_laser())
		{
			Global_laser_firing_count += Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count * (Controls.fire_primary_state || Controls.fire_primary_down_count);
			if ((Primary_weapon == FUSION_INDEX) && (Global_laser_firing_count))
			{
				if ((Players[Player_num].energy < F1_0 * 2) && (Auto_fire_fusion_cannon_time == 0))
				{
					Global_laser_firing_count = 0;
				}
				else
				{
					if (Fusion_charge == 0)
						Players[Player_num].energy -= F1_0 * 2;

					Fusion_charge += FrameTime;
					Players[Player_num].energy -= FrameTime;

					if (Players[Player_num].energy <= 0) {
						Players[Player_num].energy = 0;
						Auto_fire_fusion_cannon_time = GameTime - 1;				//	Fire now!
					}
					else
						Auto_fire_fusion_cannon_time = GameTime + FrameTime / 2 + 1;		//	Fire the fusion cannon at this time in the future.

					if (Fusion_charge < F1_0 * 2)
						PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
					else
						PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

					if (Fusion_next_sound_time < GameTime)
					{
						if (Fusion_charge > F1_0 * 2)
						{
							digi_play_sample(11, F1_0);
							apply_damage_to_player(ConsoleObject, ConsoleObject, P_Rand() * 4);
						}
						else
						{
							create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
							digi_play_sample(SOUND_FUSION_WARMUP, F1_0);
#ifdef NETWORK
							if (Game_mode & GM_MULTI)
								multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
#endif
						}
						Fusion_next_sound_time = GameTime + F1_0 / 8 + P_Rand() / 4;
					}
				}
			}
		}

		if (Auto_fire_fusion_cannon_time) {
			if (Primary_weapon != FUSION_INDEX)
				Auto_fire_fusion_cannon_time = 0;
			else if (GameTime + FrameTime / 2 >= Auto_fire_fusion_cannon_time)
			{
				Auto_fire_fusion_cannon_time = 0;
				Global_laser_firing_count = 1;
			}
			else
			{
				vms_vector	rand_vec;
				fix			bump_amount;

				Global_laser_firing_count = 0;

				ConsoleObject->mtype.phys_info.rotvel.x += (P_Rand() - 16384) / 8;
				ConsoleObject->mtype.phys_info.rotvel.z += (P_Rand() - 16384) / 8;
				make_random_vector(&rand_vec);

				bump_amount = F1_0 * 4;

				if (Fusion_charge > F1_0 * 2)
					bump_amount = Fusion_charge * 4;

				bump_one_object(ConsoleObject, &rand_vec, bump_amount);
			}
		}

		if (Global_laser_firing_count)
		{
			//	Don't cap here, gets capped in Laser_create_new and is based on whether in multiplayer mode, MK, 3/27/95
			// if (Fusion_charge > F1_0*2)
			// 	Fusion_charge = F1_0*2;
			Global_laser_firing_count -= do_laser_firing_player();	//do_laser_firing(Players[Player_num].objnum, Primary_weapon);
		}

		if (Global_laser_firing_count < 0)
			Global_laser_firing_count = 0;
	}

	if (Do_appearance_effect)
	{
		create_player_appearance_effect(ConsoleObject);
		Do_appearance_effect = 0;
	}
}

//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void powerup_grab_cheat(object* player, int objnum)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert(Objects[objnum].type == OBJ_POWERUP);

	powerup_size = Objects[objnum].size;
	player_size = player->size;

	dist = vm_vec_dist_quick(&Objects[objnum].pos, &player->pos);

	if ((dist < 2 * (powerup_size + player_size)) && !(Objects[objnum].flags & OF_SHOULD_BE_DEAD))
	{
		vms_vector	collision_point;

		vm_vec_avg(&collision_point, &Objects[objnum].pos, &player->pos);
		collide_player_and_powerup(player, &Objects[objnum], &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void powerup_grab_cheat_all(void)
{
	segment* segp;
	int		objnum;

	segp = &Segments[ConsoleObject->segnum];
	objnum = segp->objects;

	while (objnum != -1) 
	{
		if (Objects[objnum].type == OBJ_POWERUP)
			powerup_grab_cheat(ConsoleObject, objnum);
		objnum = Objects[objnum].next;
	}

}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
int mark_player_path_to_segment(int segnum)
{
	int		i;
	object* objp = ConsoleObject;
	short		player_path_length = 0;
	int		player_hide_index = -1;

	if (Last_level_path_created == Current_level_num) 
	{
		return 0;
	}

	Last_level_path_created = Current_level_num;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) 
	{
		mprintf((0, "Unable to form path of length %i for myself\n", 100));
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH * 2 > MAX_POINT_SEGS)
	{
		mprintf((1, "Can't create path.  Not enough point_segs.\n"));
		ai_reset_all_paths();
		return 0;
	}

	for (i = 1; i < player_path_length; i++) 
	{
		int			segnum, objnum;
		vms_vector	seg_center;
		object* obj;

		segnum = Point_segs[player_hide_index + i].segnum;
		mprintf((0, "%3i ", segnum));
		seg_center = Point_segs[player_hide_index + i].point;

		objnum = obj_create(OBJ_POWERUP, POW_ENERGY, segnum, &seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (objnum == -1)
		{
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &Objects[objnum];
		obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0 * 100 + P_Rand() * 4;
	}

	mprintf((0, "\n"));
	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	int	i, j;

	//	---------- Find exit doors ----------
	for (i = 0; i <= Highest_segment_index; i++)
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].children[j] == -2) 
			{
				mprintf((0, "Exit at segment %i\n", i));
				return mark_player_path_to_segment(i);
			}

	return 0;
}

#endif


#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used objects.
void show_free_objects(void)
{
	if (!(FrameCount & 8)) 
	{
		int	i;
		int	count = 0;

		mprintf((0, "Highest_object_index = %3i, MAX_OBJECTS = %3i, now used = ", Highest_object_index, MAX_OBJECTS));

		for (i = 0; i <= Highest_object_index; i++)
			if (Objects[i].type != OBJ_NONE)
				count++;

		mprintf((0, "%3i", count));

		if (count > Max_obj_count_mike) 
		{
			Max_obj_count_mike = count;
			mprintf((0, " ***"));
		}

		mprintf((0, "\n"));
	}

}
#endif