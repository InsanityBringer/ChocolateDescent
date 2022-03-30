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

#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"

void gr_upixel(int x, int y)
{
	if (grd_curcanv->cv_bitmap.bm_type != BM_RGB15)
		DATA[ROWSIZE * y + x] = (unsigned char)COLOR;
	else
		*((uint16_t*)&DATA[ROWSIZE * y + x * 2]) = gr_highcolor_clut[COLOR];
}

void gr_pixel(int x, int y)
{
	if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;
	if (grd_curcanv->cv_bitmap.bm_type != BM_RGB15)
		DATA[ROWSIZE * y + x] = (unsigned char)COLOR;
	else
		*((uint16_t*)&DATA[ROWSIZE * y + x * 2]) = gr_highcolor_clut[COLOR];
}

void gr_bm_upixel(grs_bitmap* bm, int x, int y, unsigned char color)
{
	if (bm->bm_type == BM_RGB15)
		*((uint16_t*)&bm->bm_data[bm->bm_rowsize * y + x * 2]) = gr_highcolor_clut[color];
	else
		bm->bm_data[bm->bm_rowsize * y + x] = color;
}

void gr_bm_pixel(grs_bitmap* bm, int x, int y, unsigned char color)
{
	if ((x < 0) || (y < 0) || (x >= bm->bm_w) || (y >= bm->bm_h)) return;
	if (bm->bm_type == BM_RGB15)
		*((uint16_t*)&bm->bm_data[bm->bm_rowsize * y + x * 2]) = gr_highcolor_clut[color];
	else
		bm->bm_data[bm->bm_rowsize * y + x] = color;
}
