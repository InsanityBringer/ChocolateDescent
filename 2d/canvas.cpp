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
#include "mem/mem.h"
#include "misc/error.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "platform/platform.h"

grs_canvas* grd_curcanv;    //active canvas
grs_screen* grd_curscreen;  //active screen

unsigned int gr_var_color;
unsigned int gr_var_bwidth;
unsigned char* gr_var_bitmap;

grs_canvas* gr_create_canvas(int w, int h)
{
	unsigned char* data;
	grs_canvas* newvar;

	newvar = (grs_canvas*)malloc(sizeof(grs_canvas));
	data = (unsigned char*)malloc(w * h * sizeof(unsigned char));

	newvar->cv_bitmap.bm_x = 0;
	newvar->cv_bitmap.bm_y = 0;
	newvar->cv_bitmap.bm_w = w;
	newvar->cv_bitmap.bm_h = h;
	newvar->cv_bitmap.bm_flags = 0;
	newvar->cv_bitmap.bm_type = BM_LINEAR;
	newvar->cv_bitmap.bm_rowsize = w;
	newvar->cv_bitmap.bm_data = data;

	newvar->cv_color = 0;
	newvar->cv_drawmode = 0;
	newvar->cv_font = NULL;
	newvar->cv_font_fg_color = 0;
	newvar->cv_font_bg_color = 0;
	return newvar;
}

grs_canvas* gr_create_canvas_highcolor(int w, int h)
{
	unsigned char* data;
	grs_canvas* newvar;

	newvar = (grs_canvas*)malloc(sizeof(grs_canvas));
	data = (unsigned char*)malloc((w * 2) * h * sizeof(unsigned char));

	newvar->cv_bitmap.bm_x = 0;
	newvar->cv_bitmap.bm_y = 0;
	newvar->cv_bitmap.bm_w = w;
	newvar->cv_bitmap.bm_h = h;
	newvar->cv_bitmap.bm_flags = 0;
	newvar->cv_bitmap.bm_type = BM_RGB15;
	newvar->cv_bitmap.bm_rowsize = w * 2;
	newvar->cv_bitmap.bm_data = data;

	newvar->cv_color = 0;
	newvar->cv_drawmode = 0;
	newvar->cv_font = NULL;
	newvar->cv_font_fg_color = 0;
	newvar->cv_font_bg_color = 0;
	return newvar;
}

grs_canvas* gr_create_sub_canvas(grs_canvas* canv, int x, int y, int w, int h)
{
	grs_canvas* newvar;
	int bpp = 1;

	if (canv->cv_bitmap.bm_type == BM_RGB15)
		bpp = 2;

	newvar = (grs_canvas*)malloc(sizeof(grs_canvas));

	newvar->cv_bitmap.bm_x = x + canv->cv_bitmap.bm_x;
	newvar->cv_bitmap.bm_y = y + canv->cv_bitmap.bm_y;
	newvar->cv_bitmap.bm_w = w;
	newvar->cv_bitmap.bm_h = h;
	newvar->cv_bitmap.bm_flags = 0;
	newvar->cv_bitmap.bm_type = canv->cv_bitmap.bm_type;
	newvar->cv_bitmap.bm_rowsize = canv->cv_bitmap.bm_rowsize;

	newvar->cv_bitmap.bm_data = canv->cv_bitmap.bm_data;
	newvar->cv_bitmap.bm_data += y * canv->cv_bitmap.bm_rowsize;
	newvar->cv_bitmap.bm_data += (x * bpp);

	newvar->cv_color = canv->cv_color;
	newvar->cv_drawmode = canv->cv_drawmode;
	newvar->cv_font = canv->cv_font;
	newvar->cv_font_fg_color = canv->cv_font_fg_color;
	newvar->cv_font_bg_color = canv->cv_font_bg_color;
	return newvar;
}

void gr_init_canvas(grs_canvas* canv, unsigned char* pixdata, int pixtype, int w, int h)
{
	canv->cv_color = 0;
	canv->cv_drawmode = 0;
	canv->cv_font = NULL;
	canv->cv_font_fg_color = 0;
	canv->cv_font_bg_color = 0;

	canv->cv_bitmap.bm_x = 0;
	canv->cv_bitmap.bm_y = 0;
	if (pixtype == BM_MODEX)
		canv->cv_bitmap.bm_rowsize = w / 4;
	else if (pixtype == BM_RGB15)
		canv->cv_bitmap.bm_rowsize = w * 2;
	else
		canv->cv_bitmap.bm_rowsize = w;
	canv->cv_bitmap.bm_w = w;
	canv->cv_bitmap.bm_h = h;
	canv->cv_bitmap.bm_flags = 0;
	canv->cv_bitmap.bm_type = pixtype;
	canv->cv_bitmap.bm_data = pixdata;

}

void gr_init_sub_canvas(grs_canvas* newc, grs_canvas* src, int x, int y, int w, int h)
{
	newc->cv_color = src->cv_color;
	newc->cv_drawmode = src->cv_drawmode;
	newc->cv_font = src->cv_font;
	newc->cv_font_fg_color = src->cv_font_fg_color;
	newc->cv_font_bg_color = src->cv_font_bg_color;

	newc->cv_bitmap.bm_x = src->cv_bitmap.bm_x + x;
	newc->cv_bitmap.bm_y = src->cv_bitmap.bm_y + y;
	newc->cv_bitmap.bm_w = w;
	newc->cv_bitmap.bm_h = h;
	newc->cv_bitmap.bm_flags = 0;
	newc->cv_bitmap.bm_type = src->cv_bitmap.bm_type;
	newc->cv_bitmap.bm_rowsize = src->cv_bitmap.bm_rowsize;


	newc->cv_bitmap.bm_data = src->cv_bitmap.bm_data;
	newc->cv_bitmap.bm_data += y * src->cv_bitmap.bm_rowsize;
	newc->cv_bitmap.bm_data += x;
}

void gr_free_canvas(grs_canvas* canv)
{
	free(canv->cv_bitmap.bm_data);
	free(canv);
}

void gr_free_sub_canvas(grs_canvas* canv)
{
	free(canv);
}

int gr_wait_for_retrace = 1;

void gr_show_canvas(grs_canvas* canv)
{
	//[ISB] I dunno...
	plat_blit_canvas(canv);
}

void gr_set_current_canvas(grs_canvas* canv)
{
	if (canv == NULL)
		grd_curcanv = &(grd_curscreen->sc_canvas);
	else
		grd_curcanv = canv;

	if ((grd_curcanv->cv_color >= 0) && (grd_curcanv->cv_color <= 255)) 
	{
		gr_var_color = grd_curcanv->cv_color;
	}
	else
		gr_var_color = 0;
	gr_var_bitmap = grd_curcanv->cv_bitmap.bm_data;
	gr_var_bwidth = grd_curcanv->cv_bitmap.bm_rowsize;

}

void gr_clear_canvas(int color)
{
	gr_setcolor(color);
	gr_rect(0, 0, WIDTH - 1, HEIGHT - 1);
}

void gr_setcolor(int color)
{
	grd_curcanv->cv_color = color;

	gr_var_color = color;
}
