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
#include <stddef.h>

#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "2d/rle.h"
#include "platform/mono.h"

#include "misc/error.h"

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
uint8_t* gr_bitblt_fade_table = NULL;

//extern void gr_vesa_bitmap(grs_bitmap* source, grs_bitmap* dest, int x, int y); [ISB] seems unused too

// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd
void gr_linear_movsd(void* src, void* dest, unsigned short num_pixels)
{
	int i;
	uint8_t* ldest = (uint8_t*)dest;
	uint8_t* lsrc = (uint8_t*)src;
	for (i = 0; i < num_pixels; i++)
		*ldest++ = *lsrc++;
}

void gr_linear_rep_movsd_faded(void* src, void* dest, unsigned short num_pixels, uint8_t fade_value)
{
	int i;
	uint8_t* ldest = (uint8_t*)dest;
	uint8_t* lsrc = (uint8_t*)src;
	for (i = 0; i < num_pixels; i++)
	{
		*ldest++ = gr_fade_table[*lsrc++ + (256 * fade_value)];
	}
}

void gr_linear_movsdm(void* src, void* dest, unsigned short num_pixels)
{
	int i;
	uint8_t* ldest = (uint8_t*)dest;
	uint8_t* lsrc = (uint8_t*)src;
	for (i = 0; i < num_pixels; i++)
	{
		if (*lsrc != TRANSPARENCY_COLOR)
			*ldest++ = *lsrc++;
		else
		{
			ldest++; lsrc++;
		}
	}
}

void gr_linear_rep_movsdm_faded(void* src, void* dest, unsigned short num_pixels, uint8_t fade_value)
{
	int i;
	uint8_t* ldest = (uint8_t*)dest;
	uint8_t* lsrc = (uint8_t*)src;
	for (i = 0; i < num_pixels; i++)
	{
		if (*lsrc != TRANSPARENCY_COLOR)
		{
			*ldest++ = gr_fade_table[*lsrc + (256 * fade_value)];
			lsrc++;
		}
		else
		{
			ldest++; lsrc++;
		}
	}
}


void gr_ubitmap00(int x, int y, grs_bitmap * bm)
{
	register int y1;
	int dest_rowsize;

	unsigned char* dest;
	unsigned char* src;

	dest_rowsize = grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[dest_rowsize * y + x]);

	src = bm->bm_data;

	if (gr_bitblt_fade_table == NULL)
	{
		for (y1 = 0; y1 < bm->bm_h; y1++)
		{
			gr_linear_movsd(src, dest, bm->bm_w);
			src += bm->bm_rowsize;
			dest += (int)(dest_rowsize);
		}
	}
	
	else 
	{
		for (y1 = 0; y1 < bm->bm_h; y1++) 
		{
			gr_linear_rep_movsdm_faded(src, dest, bm->bm_w, gr_bitblt_fade_table[y1 + y]);
			src += bm->bm_rowsize;
			dest += (int)(dest_rowsize);
		}
	}
}

void gr_ubitmap00m(int x, int y, grs_bitmap* bm)
{
	register int y1;
	int dest_rowsize;

	unsigned char* dest;
	unsigned char* src;

	dest_rowsize = grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[dest_rowsize * y + x]);

	src = bm->bm_data;

	if (gr_bitblt_fade_table == NULL) 
	{
		for (y1 = 0; y1 < bm->bm_h; y1++) 
		{
			gr_linear_movsdm(src, dest, bm->bm_w);
			src += bm->bm_rowsize;
			dest += (int)(dest_rowsize);
		}
	}
	/*
	else {
		for (y1 = 0; y1 < bm->bm_h; y1++) {
			gr_linear_rep_movsdm_faded(src, dest, bm->bm_w, gr_bitblt_fade_table[y1 + y]);
			src += bm->bm_rowsize;
			dest += (int)(dest_rowsize);
		}
	}*/ //[ISB] need to fix fade table;
}

void gr_ubitmapGENERIC(int x, int y, grs_bitmap* bm)
{
	register int x1, y1;

	for (y1 = 0; y1 < bm->bm_h; y1++)
	{
		for (x1 = 0; x1 < bm->bm_w; x1++) 
		{
			gr_setcolor(gr_gpixel(bm, x1, y1));
			gr_upixel(x + x1, y + y1);
		}
	}
}

void gr_ubitmapGENERICm(int x, int y, grs_bitmap* bm)
{
	register int x1, y1;
	uint8_t c;

	for (y1 = 0; y1 < bm->bm_h; y1++) {
		for (x1 = 0; x1 < bm->bm_w; x1++) {
			c = gr_gpixel(bm, x1, y1);
			if (c != 255) {
				gr_setcolor(c);
				gr_upixel(x + x1, y + y1);
			}
		}
	}
}

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest);
void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest);

void gr_ubitmap(int x, int y, grs_bitmap* bm)
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	//[ISB] TODO: VERY SLOW BLITTING AHOY
	if (bm->bm_flags & BM_FLAG_RLE)
		gr_bm_ubitblt00_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
	else
		gr_ubitmap00(x, y, bm);
		//gr_ubitmapGENERIC(x, y, bm);
}

void gr_ubitmapm(int x, int y, grs_bitmap* bm)
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	if (bm->bm_flags & BM_FLAG_RLE)
		gr_bm_ubitblt00m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
	else
	{
		//gr_ubitmapGENERICm(x, y, bm);
		gr_ubitmap00m(x, y, bm);
	}
}

// From linear to SVGA
void gr_bm_ubitblt02(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	Error("gr_bm_ubitblt02: STUB\n");
}

// From SVGA to linear
void gr_bm_ubitblt20(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	Error("gr_bm_ubitblt20: STUB\n");
}

//@extern int Interlacing_on;

// From Linear to Linear
void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	unsigned char* dbits;
	unsigned char* sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;

	int i;

	sbits = src->bm_data + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++) {
		//if (gr_bitblt_double)
		//	gr_linear_rep_movsd_2x(sbits, dbits, w);
		//else
			gr_linear_movsd(sbits, dbits, w);
		sbits += src->bm_rowsize;
		dbits += dstep;
	}
}
// From Linear to Linear Masked
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	unsigned char* dbits;
	unsigned char* sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;

	int i;

	sbits = src->bm_data + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (gr_bitblt_fade_table == NULL) 
	{
		for (i = 0; i < h; i++) 
		{
			gr_linear_movsdm(sbits, dbits, w);
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
	else 
	{
		for (i = 0; i < h; i++) 
		{
			gr_linear_rep_movsdm_faded(sbits, dbits, w, gr_bitblt_fade_table[dy + i]);
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
}


//extern void gr_lbitblt(grs_bitmap* source, grs_bitmap* dest, int height, int width); [ISB] not needed?

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	int dx1 = dx, dx2 = dx + dest->bm_w - 1;
	int dy1 = dy, dy2 = dy + dest->bm_h - 1;

	int sx1 = sx, sx2 = sx + src->bm_w - 1;
	int sy1 = sy, sy2 = sy + src->bm_h - 1;

	if ((dx1 >= dest->bm_w) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h) || (dy2 < 0)) return;
	if (dx1 < 0) { sx1 += -dx1; dx1 = 0; }
	if (dy1 < 0) { sy1 += -dy1; dy1 = 0; }
	if (dx2 >= dest->bm_w) { dx2 = dest->bm_w - 1; }
	if (dy2 >= dest->bm_h) { dy2 = dest->bm_h - 1; }

	if ((sx1 >= src->bm_w) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h) || (sy2 < 0)) return;
	if (sx1 < 0) { dx1 += -sx1; sx1 = 0; }
	if (sy1 < 0) { dy1 += -sy1; sy1 = 0; }
	if (sx2 >= src->bm_w) { sx2 = src->bm_w - 1; }
	if (sy2 >= src->bm_h) { sy2 = src->bm_h - 1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if (dx2 - dx1 + 1 < w)
		w = dx2 - dx1 + 1;
	if (dy2 - dy1 + 1 < h)
		h = dy2 - dy1 + 1;
	if (sx2 - sx1 + 1 < w)
		w = sx2 - sx1 + 1;
	if (sy2 - sy1 + 1 < h)
		h = sy2 - sy1 + 1;

	gr_bm_ubitblt(w, h, dx1, dy1, sx1, sy1, src, dest);
}



void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	//[ISB] TODO: HORRIBLY SLOW BLITTING AHOY
	if (src->bm_flags & BM_FLAG_RLE)
	{
		gr_bm_ubitblt00_rle(w, h, dx, dy, sx, sy, src, dest);
	}
	else
	{
		gr_bm_ubitblt00(w, h, dx, dy, sx, sy, src, dest);
	}
}

// Clipped bitmap ... 

void gr_bitmap(int x, int y, grs_bitmap* bm)
{
	int dx1 = x, dx2 = x + bm->bm_w - 1;
	int dy1 = y, dy2 = y + bm->bm_h - 1;
	int sx = 0, sy = 0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if (dx1 < 0) { sx = -dx1; dx1 = 0; }
	if (dy1 < 0) { sy = -dy1; dy1 = 0; }
	if (dx2 >= grd_curcanv->cv_bitmap.bm_w) { dx2 = grd_curcanv->cv_bitmap.bm_w - 1; }
	if (dy2 >= grd_curcanv->cv_bitmap.bm_h) { dy2 = grd_curcanv->cv_bitmap.bm_h - 1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitblt(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap);

}

void gr_bitmapm(int x, int y, grs_bitmap* bm)
{
	int dx1 = x, dx2 = x + bm->bm_w - 1;
	int dy1 = y, dy2 = y + bm->bm_h - 1;
	int sx = 0, sy = 0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if (dx1 < 0) { sx = -dx1; dx1 = 0; }
	if (dy1 < 0) { sy = -dy1; dy1 = 0; }
	if (dx2 >= grd_curcanv->cv_bitmap.bm_w) { dx2 = grd_curcanv->cv_bitmap.bm_w - 1; }
	if (dy2 >= grd_curcanv->cv_bitmap.bm_h) { dy2 = grd_curcanv->cv_bitmap.bm_h - 1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	//if ((bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_LINEAR)) //[ISB] really need to clean this up and ensure all bitmaps are always linear
	{
		if (bm->bm_flags & BM_FLAG_RLE)
			gr_bm_ubitblt00m_rle(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap);
		else
			gr_bm_ubitblt00m(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap);
			//gr_bm_ubitbltm(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap); //[ISB] this is slow
		return;
	}
}

void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	register int x1, y1;
	uint8_t c;

	for (y1 = 0; y1 < h; y1++) {
		for (x1 = 0; x1 < w; x1++) {
			if ((c = gr_gpixel(src, sx + x1, sy + y1)) != 255)
				gr_bm_pixel(dest, dx + x1, dy + y1, c);
		}
	}

}

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	unsigned char* dbits;
	unsigned char* sbits;

	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + src->bm_h * data_offset];

	if (src->bm_flags & BM_FLAG_RLE_BIG)
	{
		for (i = 0; i < sy; i++)
			sbits += (int)(src->bm_data[4 + i * data_offset] + (src->bm_data[4 + (i * data_offset) + 1] << 8));
	}
	else
	{
		for (i = 0; i < sy; i++)
			sbits += (int)src->bm_data[4 + i];
	}

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++) 
	{
		gr_rle_expand_scanline(dbits, sbits, sx, sx + w - 1);
		if (src->bm_flags & BM_FLAG_RLE_BIG) //[ISB] this code gives me a headache
			sbits += (int)(src->bm_data[4 + ((i + sy) * data_offset)] + (src->bm_data[4 + (((i + sy) * data_offset) + 1)] << 8));
		else
			sbits += (int)src->bm_data[4 + i + sy];

		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	unsigned char* dbits;
	unsigned char* sbits;

	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + src->bm_h * data_offset];

	if (src->bm_flags & BM_FLAG_RLE_BIG)
	{
		for (i = 0; i < sy; i++)
			sbits += (int)(src->bm_data[4 + i * data_offset] + (src->bm_data[4 + (i * data_offset) + 1] << 8));
	}
	else
	{
		for (i = 0; i < sy; i++)
			sbits += (int)src->bm_data[4 + i];
	}

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++)
	{
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx + w - 1);
		if (src->bm_flags & BM_FLAG_RLE_BIG) //[ISB] this code gives me a headache
			sbits += (int)(src->bm_data[4 + ((i + sy) * data_offset)] + (src->bm_data[4 + (((i + sy) * data_offset) + 1)] << 8));
		else
			sbits += (int)src->bm_data[4 + i + sy];
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

// in rle.c
extern void gr_rle_expand_scanline_generic(grs_bitmap* dest, int dx, int dy, uint8_t* src, int x1, int x2);

void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest)
{
	int i;
	register int y1;
	unsigned char* sbits;

	//mprintf( 0, "SVGA RLE!\n" );

	sbits = &src->bm_data[4 + src->bm_h];
	for (i = 0; i < sy; i++)
		sbits += (int)src->bm_data[4 + i];

	for (y1 = 0; y1 < h; y1++) {
		gr_rle_expand_scanline_generic(dest, dx, dy + y1, sbits, sx, sx + w - 1);
		sbits += (int)src->bm_data[4 + y1 + sy];
	}

}
