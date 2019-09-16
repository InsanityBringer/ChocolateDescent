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
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "misc/types.h"
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "2d/palette.h"
#include "2d/i_gr.h"

unsigned char* gr_video_memory = (unsigned char*)0xA0000;

char gr_pal_default[768];

int gr_installed = 0;

#if 0
uint8_t* pVideoMode = (volatile uint8_t*)0x449;
uint16_t* pNumColumns = (volatile uint16_t*)0x44a;
uint8_t* pNumRows = (volatile uint8_t*)0x484;
uint16_t* pCharHeight = (volatile uint16_t*)0x485;
uint16_t* pCursorPos = (volatile uint16_t*)0x450;
uint16_t* pCursorType = (volatile uint16_t*)0x460;
uint16_t* pTextMemory = (volatile uint16_t*)0xb8000;
#endif

typedef struct screen_save {
	uint8_t 	video_mode;
	uint8_t 	is_graphics;
	uint16_t	char_height;
	uint8_t		width;
	uint8_t		height;
	uint8_t		cursor_x, cursor_y;
	uint8_t		cursor_sline, cursor_eline;
	uint16_t* video_memory;
} screen_save;

screen_save gr_saved_screen;

int gr_show_screen_info = 0;

void gr_set_cellheight(uint8_t height)
{
#if 0
	uint8_t temp;

	outp(0x3d4, 9);
	temp = inp(0x3d5);
	temp &= 0xE0;
	temp |= height;
	outp(0x3d5, temp);
#endif
}

void gr_set_linear()
{
#if 0
	outpw(0x3c4, 0xE04);		  // enable chain4 mode
	outpw(0x3d4, 0x4014);		  // turn on dword mode
	outpw(0x3d4, 0xa317);		  // turn off byte mode
#endif
}

void gr_16_to_256()
{
#if 0
	outpw(0x3ce, 0x4005);	 	// set Shift reg to 1

	inp(0x3da);					// dummy input

	outp(0x3c0, 0x30);
	outp(0x3c0, 0x61);		   // turns on PCS & PCC

	inp(0x3da);					// dummy input

	outp(0x3c0, 0x33);
	outp(0x3c0, 0);
#endif
}

void gr_turn_screen_off()
{
#if 0
	uint8_t temp;
	temp = inp(0x3da);
	outp(0x3c0, 0);
#endif
}

void gr_turn_screen_on()
{
#if 0
	uint8_t temp;
	temp = inp(0x3da);
	outp(0x3c0, 0x20);
#endif
}

void gr_set_misc_mode(uint32_t mode)
{
#if 0
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = mode;
	int386(0x10, &regs, &regs);
#endif
}

void gr_set_3dbios_mode(uint32_t mode)
{
#if 0
	union REGS regs;
	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x4fd0;
	regs.w.bx = 0x3d00 | (mode & 0xff);
	int386(0x10, &regs, &regs);
#endif
}



void gr_set_text_25()
{
#if 0
	union REGS regs;

	regs.w.ax = 3;
	int386(0x10, &regs, &regs);
#endif
}

void gr_set_text_43()
{
#if 0
	union REGS regs;

	regs.w.ax = 0x1201;
	regs.w.bx = 0x30;
	int386(0x10, &regs, &regs);

	regs.w.ax = 3;
	int386(0x10, &regs, &regs);

	regs.w.ax = 0x1112;
	regs.w.bx = 0x0;
	int386(0x10, &regs, &regs);
#endif
}

void gr_set_text_50()
{
#if 0
	union REGS regs;

	regs.w.ax = 0x1202;
	regs.w.bx = 0x30;
	int386(0x10, &regs, &regs);

	regs.w.ax = 3;
	int386(0x10, &regs, &regs);

	regs.w.ax = 0x1112;
	regs.w.bx = 0x0;
	int386(0x10, &regs, &regs);
#endif
}

uint8_t is_graphics_mode()
{
	return 1;
#if 0
	int8_t tmp;
	tmp = inp(0x3DA);		// Reset flip-flip
	outp(0x3C0, 0x30);		// Select attr register 10
	tmp = inp(0x3C1);	// Get graphics/text bit
	return tmp & 1;
#endif
}

void gr_setcursor(uint8_t x, uint8_t y, uint8_t sline, uint8_t eline)
{
#if 0
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0200;
	regs.w.bx = 0;
	regs.h.dh = y;
	regs.h.dl = x;
	int386(0x10, &regs, &regs);

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0100;
	regs.h.ch = sline & 0xf;
	regs.h.cl = eline & 0xf;
	int386(0x10, &regs, &regs);
#endif
}

void gr_getcursor(uint8_t* x, uint8_t* y, uint8_t* sline, uint8_t* eline)
{
#if 0
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0300;
	regs.w.bx = 0;
	int386(0x10, &regs, &regs);
	*y = regs.h.dh;
	*x = regs.h.dl;
	*sline = regs.h.ch;
	*eline = regs.h.cl;
#endif
}


int gr_save_mode()
{
	Warning("gr_save_mode: stub\n");
#if 0
	int i;

	gr_saved_screen.is_graphics = is_graphics_mode();
	gr_saved_screen.video_mode = *pVideoMode;

	if (!gr_saved_screen.is_graphics) {
		gr_saved_screen.width = *pNumColumns;
		gr_saved_screen.height = *pNumRows + 1;
		gr_saved_screen.char_height = *pCharHeight;
		gr_getcursor(&gr_saved_screen.cursor_x, &gr_saved_screen.cursor_y, &gr_saved_screen.cursor_sline, &gr_saved_screen.cursor_eline);
		//MALLOC(gr_saved_screen.video_memory,uint16_t, gr_saved_screen.width*gr_saved_screen.height );//Hack by Krb
		gr_saved_screen.video_memory = (uint16_t*)malloc((gr_saved_screen.width * gr_saved_screen.height) * sizeof(uint16_t));
		for (i = 0; i < gr_saved_screen.width * gr_saved_screen.height; i++)
			gr_saved_screen.video_memory[i] = pTextMemory[i];
	}

	if (gr_show_screen_info) {
		printf("Current video mode 0x%x:\n", gr_saved_screen.video_mode);
		if (gr_saved_screen.is_graphics)
			printf("Graphics mode\n");
		else {
			printf("Text mode\n");
			printf("( %d columns by %d rows)\n", gr_saved_screen.width, gr_saved_screen.height);
			printf("Char height is %d pixel rows\n", gr_saved_screen.char_height);
			printf("Cursor of type 0x%x,0x%x is at (%d, %d)\n", gr_saved_screen.cursor_sline, gr_saved_screen.cursor_eline, gr_saved_screen.cursor_x, gr_saved_screen.cursor_y);
		}
	}

#endif
	return 0;
}

int isvga()
{
	return 0;
#if 0
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x1a00;
	int386(0x10, &regs, &regs);

	if (regs.h.al == 0x1a)
		return 1;

	return 0;
#endif
}

void gr_restore_mode()
{
	Warning("gr_restore_mode: stub\n");
#if 0
	int i;

	//gr_set_text_25(); 

	gr_palette_fade_out(gr_palette, 32, 0);
	gr_palette_set_gamma(0);

	if (gr_saved_screen.video_mode == 3) {
		switch (gr_saved_screen.height) {
		case 43:	gr_set_text_43(); break;
		case 50:	gr_set_text_50(); break;
		default:	gr_set_text_25(); break;
		}
	}
	else {
		gr_set_misc_mode(gr_saved_screen.video_mode);
	}

	if (gr_saved_screen.is_graphics == 0) {
		gr_sync_display();
		gr_sync_display();
		gr_palette_read(gr_pal_default);
		gr_palette_clear();

		for (i = 0; i < gr_saved_screen.width * gr_saved_screen.height; i++)
			pTextMemory[i] = gr_saved_screen.video_memory[i];
		gr_setcursor(gr_saved_screen.cursor_x, gr_saved_screen.cursor_y, gr_saved_screen.cursor_sline, gr_saved_screen.cursor_eline);
		gr_palette_faded_out = 1;
		gr_palette_fade_in(gr_pal_default, 32, 0);
	}
#endif
}

void gr_close()
{
	if (gr_installed == 1)
	{
		gr_installed = 0;
		gr_restore_mode();
		free(grd_curscreen);
		if (gr_saved_screen.video_memory) {
			free(gr_saved_screen.video_memory);
			gr_saved_screen.video_memory = NULL;
		}
		//[ISB]: System specific 
		I_ShutdownGraphics();
	}
}

int gr_vesa_setmode(int mode)
{
	Error("gr_vesa_setmode: STUB\n");
	return 0;
}

#define NUMSCREENS 4 

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
		w = 320; r = 320; h = 200; t = BM_LINEAR; data = 0xA0000;
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
	//[ISB] Linearize all modes
	//t = BM_LINEAR;
	gr_palette_clear();

	I_SetMode(mode);

	//[ISB] Since the bitmap doesn't exist in video memory anymore, we must manage it manually
	if (grd_curscreen->sc_canvas.cv_bitmap.bm_data != NULL)
		free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
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
	//grd_curscreen->sc_canvas.cv_bitmap.bm_data = (unsigned char*)data;
	//[ISB] Allocate memory. Set up space for multiple screens for double buffering
	MALLOC(grd_curscreen->sc_canvas.cv_bitmap.bm_data, unsigned char, r * h * NUMSCREENS);
	memset(grd_curscreen->sc_canvas.cv_bitmap.bm_data, 0, r * h * NUMSCREENS * sizeof(unsigned char));
	//[ISB] Set the screen buffer
	I_SetScreenCanvas(&grd_curscreen->sc_canvas);

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

	retcode = I_InitWindow();
	if (retcode)
	{
		Error("gr_init: Error initalizing graphics library.");
	}

	// Save the current text screen mode
	if (gr_save_mode() == 1)
		return 1;

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

	MALLOC( grd_curscreen,grs_screen,1 );//Hack by KRB
	//grd_curscreen = (grs_screen*)malloc(1 * sizeof(grs_screen)); //[ISB] unhack? 
	memset(grd_curscreen, 0, sizeof(grs_screen));

	// Set the mode.
	if (retcode = gr_set_mode(mode))
	{
		gr_restore_mode();
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

	//if (!dpmi_allocate_selector(&gr_fade_table, 256 * GR_FADE_LEVELS, &gr_fade_table_selector))
	//	Error("Error allocating fade table selector!");

	//if (!dpmi_allocate_selector(&gr_palette, 256 * 3, &gr_palette_selector))
	//	Error("Error allocating palette selector!");

	//	if (!dpmi_allocate_selector( &gr_inverse_table, 32*32*32, &gr_inverse_table_selector ))
	//		Error( "Error allocating inverse table selector!" );


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
	return I_CheckMode(mode);
}

//[ISB] come to think of it is there any point of having these stub fuctions 
//that do nothing but call I_ functions
void gr_sync_display()
{
	I_WaitVBL();
}