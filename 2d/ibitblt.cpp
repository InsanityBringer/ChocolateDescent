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

#include "misc/types.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "2d/ibitblt.h"
#include "misc/error.h"
#include "mem/mem.h"

#define FIND_START		1
#define FIND_STOP		2

#define MAX_WIDTH				640
#define MAX_SCANLINES			480
#define MAX_HOLES				100

static short start_points[MAX_SCANLINES][MAX_HOLES];
static short hole_length[MAX_SCANLINES][MAX_HOLES];

void gr_ibitblt(grs_bitmap* src_bmp, grs_bitmap* dest_bmp, uint8_t *mask)
{
	int x, y, sw, sh, srowsize, drowsize, dstart, sy;// , dy;
	uint8_t* src, * dest;

	// variable setup

	sw = src_bmp->bm_w;
	sh = src_bmp->bm_h;
	srowsize = src_bmp->bm_rowsize;
	drowsize = dest_bmp->bm_rowsize;
	src = src_bmp->bm_data;
	dest = dest_bmp->bm_data;

	sy = 0;
	while (start_points[sy][0] == -1)
	{
		sy++;
		dest += drowsize;
	}

	{
		Assert(sw <= MAX_WIDTH);
		Assert(sh <= MAX_SCANLINES);
		for (y = sy; y < sy + sh; y++) 
		{
			for (x = 0; x < MAX_HOLES; x++) 
			{
				if (start_points[y][x] == -1)
					break;
				dstart = start_points[y][x];
				gr_linear_movsd(&(src[dstart]), &(dest[dstart]), hole_length[y][x]);
			}
			dest += drowsize;
			src += srowsize;
		}
	}
}

uint8_t* gr_ibitblt_create_mask(grs_bitmap* mask_bmp, int sx, int sy, int sw, int sh, int srowsize)
{
	int x, y;// , dest_offset;
	uint8_t /*pixel,*/ mode;
	int count = 0;

	Assert((!(mask_bmp->bm_flags & BM_FLAG_RLE)));

	for (y = 0; y < MAX_SCANLINES; y++) 
	{
		for (x = 0; x < MAX_HOLES; x++) 
		{
			start_points[y][x] = -1;
			hole_length[y][x] = -1;
		}
	}

	for (y = sy; y < sy + sh; y++) 
	{
		count = 0;
		mode = FIND_START;
		for (x = sx; x < sx + sw; x++) 
		{
			if ((mode == FIND_START) && (mask_bmp->bm_data[mask_bmp->bm_rowsize * y + x] == TRANSPARENCY_COLOR)) 
			{
				start_points[y][count] = x;
				mode = FIND_STOP;
			}
			else if ((mode == FIND_STOP) && (mask_bmp->bm_data[mask_bmp->bm_rowsize * y + x] != TRANSPARENCY_COLOR)) 
			{
				hole_length[y][count] = x - start_points[y][count];
				count++;
				mode = FIND_START;
			}
		}
		if (mode == FIND_STOP) {
			hole_length[y][count] = x - start_points[y][count];
			count++;
		}
		Assert(count <= MAX_HOLES);
	}

	int* mask;
	MALLOC(mask, int, 1);
	*mask = 65536;
	return (unsigned char*)mask; //[ISB] hack
}

uint8_t* gr_ibitblt_create_mask_svga(grs_bitmap* mask_bmp, int sx, int sy, int sw, int sh, int srowsize)
{
	return gr_ibitblt_create_mask(mask_bmp, sx, sy, sw, sh, srowsize);
}

void gr_ibitblt_find_hole_size(grs_bitmap* mask_bmp, int* minx, int* miny, int* maxx, int* maxy)
{
	uint8_t c;
	int x, y, count = 0;

	Assert((!(mask_bmp->bm_flags & BM_FLAG_RLE)));
	Assert(mask_bmp->bm_flags & BM_FLAG_TRANSPARENT);

	*minx = mask_bmp->bm_w - 1;
	*maxx = 0;
	*miny = mask_bmp->bm_h - 1;
	*maxy = 0;

	//if (scanline == NULL)
	//	scanline = (double*)malloc(sizeof(double) * (MAX_WIDTH / sizeof(double)));

	for (y = 0; y < mask_bmp->bm_h; y++) {
		for (x = 0; x < mask_bmp->bm_w; x++) {
			c = mask_bmp->bm_data[mask_bmp->bm_rowsize * y + x];
			if (c == TRANSPARENCY_COLOR) {				// don't look for transparancy color here.
				count++;
				if (x < *minx)* minx = x;
				if (y < *miny)* miny = y;
				if (x > * maxx)* maxx = x;
				if (y > * maxy)* maxy = y;
			}
		}
	}
	Assert(count);
}
