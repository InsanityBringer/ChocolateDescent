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

#include <stdlib.h>

#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

#include <stdio.h>
#include <string.h>

#include "misc/types.h"
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "2d/palette.h"
#include "platform/platform.h"

uint8_t* gr_video_memory = (unsigned char*)NULL;

char gr_pal_default[768];

int gr_installed = 0;

int gr_show_screen_info = 0;

uint8_t is_graphics_mode()
{
	return 1;
}

void gr_close()
{
	if (gr_installed == 1)
	{
		gr_installed = 0;
		mem_free(grd_curscreen);
		//[ISB] Oops, gr_close is an atexit, but plat_close_window was expected to be called on the SDL code before. Keep call for Windows code atm.
		plat_close_window();
	}
}

int gr_vesa_setmode(int mode)
{
	Error("gr_vesa_setmode: STUB\n");
	return 0;
}

#define NUMSCREENS 2 

int gr_set_mode(int mode)
{
	//int retcode;
	unsigned int w, h, t, data, r;

	//JOHNgr_disable_default_palette_loading();

	switch (mode)
	{
	case SM_ORIGINAL:
		return 0;
	case 0:
		//if (!isvga()) return 1;
		//gr_set_misc_mode(0x13);
		w = 320; r = 320; h = 200; t = BM_LINEAR; data = 0xB0000;
		break;
	case SM_320x240U:
		w = 320; r = 320; h = 240; t = BM_SVGA;
		break;
	case SM_640x400V:
		//retcode = gr_vesa_setmode(0x100);
		//if (retcode != 0) return retcode;
		w = 640; r = 640; h = 400; t = BM_SVGA; data = 0;
		break;
	case SM_640x480V:
		//retcode = gr_vesa_setmode(0x101);
		//if (retcode != 0) return retcode;
		w = 640; r = 640; h = 480; t = BM_SVGA; data = 0;
		break;
	case SM_800x600V:
		//retcode = gr_vesa_setmode(0x103);
		//if (retcode != 0) return retcode;
		w = 800; r = 800; h = 600; t = BM_SVGA; data = 0;
		break;
	case SM_1024x768V:
		//retcode = gr_vesa_setmode(0x105);
		//if (retcode != 0) return retcode;
		w = 1024; r = 1024; h = 768; t = BM_SVGA; data = 0;
		break;
	case SM_1280x1024V:
		w = 1280; r = 1280; h = 1024; t = BM_SVGA; data = 0;
		break;
	case SM_640x480V15:
		//retcode = gr_vesa_setmode(0x110);
		//if (retcode != 0) return retcode;
		w = 640; r = 640 * 2; h = 480; t = BM_SVGA15; data = 0;
		break;
	case SM_800x600V15:
		//retcode = gr_vesa_setmode(0x113);
		//if (retcode != 0) return retcode;
		w = 800; r = 800 * 2; h = 600; t = BM_SVGA15; data = 0;
		break;
	case SM_320x400U:
		w = 320; r = 320; h = 400; t = BM_SVGA;
		break;
	case 19:
		w = 320; r = 320; h = 100; t = BM_SVGA; data = 0;
		break;
	case 20:
		//retcode = gr_vesa_setmode(0x102);
		//gr_enable_default_palette_loading();
		//if (retcode != 0) return retcode;
		//gr_16_to_256();
		//gr_set_linear();
		//gr_set_cellheight( 1 );
		//gr_vesa_setlogical(400);
		w = 400; r = 400; h = 600; t = BM_SVGA; data = 0;
		break;
	case 21:
		//if (!isvga()) return 1;
		//gr_set_misc_mode(0xd);
		//gr_16_to_256();
		//gr_set_linear();
		//gr_set_cellheight(3);
		w = 160; r = 160; h = 100; t = BM_LINEAR; data = 0xA0000;
		break;
	case 22:			// 3dmax 320x400
		//if (!isvga()) return 1;
		//gr_set_3dbios_mode(0x31);
		//w = 320; r = 320/4; h = 400; t=BM_MODEX; data = 0;
		w = 320; r = 320; h = 400; t = BM_SVGA; data = 0;
		break;
	default:
		Error("gr_set_mode: stub mode selected\n");
		//if (!isvga()) return 1;
		//w = gr_modex_setmode(mode);
		//gr_enable_default_palette_loading();
		//h = w & 0xffff; w = w >> 16; r = w / 4; t = BM_MODEX; data = 0;
		return 1;
	}
	//[ISB] Dropping the linearization of all modes, to allow the paged video modes to work. 
	gr_palette_clear();

	plat_set_gr_mode(mode);

	memset(grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w * 3, grd_curscreen->sc_h * 4);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = r;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = t;

	//[ISB] Point the bitmap data at the big video memory buffer. This formerly reallocated on mode change, but this caused too many problems. 
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = gr_video_memory;
	memset(grd_curscreen->sc_canvas.cv_bitmap.bm_data, 0, r * h * NUMSCREENS * sizeof(unsigned char));

#ifndef BUILD_DESCENT2
	gr_set_current_canvas(NULL);
#else
	gr_set_current_canvas(&grd_curscreen->sc_canvas);
#endif

	//gr_enable_default_palette_loading();

	return 0;
}

int gr_init(int mode)
{
	int org_gamma;
	int retcode;

	// Only do this function once!
	if (gr_installed == 1)
		return 1;

	retcode = plat_create_window();
	if (retcode)
	{
		Error("gr_init: Error initalizing graphics library.");
	}

	//[ISB] THIS IS A GODDAMNED HACK
	//[ISB] okay so many problems with offset screens actually are pretty rational: The offscreen buffers point into video memory. Video memory keeps on jittering and being reallocated. So uh, lets just allocate once
	MALLOC(gr_video_memory, uint8_t, 1280 * 1024 * 2);

	// Save the current palette, and fade it out to black.
	/*gr_palette_read((uint8_t*)gr_pal_default);
	gr_palette_faded_out = 0;
	org_gamma = gr_palette_get_gamma();
	gr_palette_set_gamma(0);
	gr_palette_fade_out((uint8_t*)gr_pal_default, 32, 0);
	gr_palette_clear();
	gr_palette_set_gamma(org_gamma);
	gr_sync_display();
	gr_sync_display();*/

	MALLOC(grd_curscreen,grs_screen,1);
	memset(grd_curscreen, 0, sizeof(grs_screen));

	// Set the mode.
	if (retcode = gr_set_mode(mode))
	{
		return retcode;
	}
	//JOHNgr_disable_default_palette_loading();

	// Set all the screen, canvas, and bitmap variables that
	// aren't set by the gr_set_mode call:
	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas(&grd_curscreen->sc_canvas);

	// Set flags indicating that this is installed.
	gr_installed = 1;
	atexit(gr_close);
	return 0;
}

//  0=Mode set OK
//  1=No VGA adapter installed
//  2=Program doesn't support this VESA granularity
//  3=Monitor doesn't support that VESA mode.:
//  4=Video card doesn't support that VESA mode.
//  5=No VESA driver found.
//  6=Bad Status after VESA call/
//  7=Not enough DOS memory to call VESA functions.
//  8=Error using DPMI.
//  9=Error setting logical line width.
// 10=Error allocating selector for A0000h
// 11=Not a valid mode support by gr.lib

int gr_check_mode(int mode)
{
	return plat_check_gr_mode(mode);
}

//[ISB] come to think of it is there any point of having these stub fuctions 
//that do nothing but call I_ functions
void gr_sync_display()
{
	plat_wait_for_vbl();
}