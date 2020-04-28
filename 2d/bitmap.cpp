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

#include "mem/mem.h"
#include "misc/error.h"

#include "2d/gr.h"
#include "2d/grdef.h"

grs_bitmap* gr_create_bitmap(int w, int h)
{
	grs_bitmap* newbm;

	newbm = (grs_bitmap*)malloc(sizeof(grs_bitmap));
	newbm->bm_x = 0;
	newbm->bm_y = 0;
	newbm->bm_w = w;
	newbm->bm_h = h;
	newbm->bm_type = 0;
	newbm->bm_flags = 0;
	newbm->bm_rowsize = w;
	newbm->bm_selector = 0;

	newbm->bm_data = (unsigned char*)malloc(w * h * sizeof(unsigned char));

	return newbm;
}

grs_bitmap* gr_create_bitmap_raw(int w, int h, unsigned char* raw_data)
{
	grs_bitmap* newbm;

	newbm = (grs_bitmap*)malloc(sizeof(grs_bitmap));
	newbm->bm_x = 0;
	newbm->bm_y = 0;
	newbm->bm_w = w;
	newbm->bm_h = h;
	newbm->bm_flags = 0;
	newbm->bm_type = 0;
	newbm->bm_rowsize = w;
	newbm->bm_data = raw_data;
	newbm->bm_selector = 0;

	return newbm;
}

void gr_init_bitmap(grs_bitmap* bm, int mode, int x, int y, int w, int h, int bytesperline, unsigned char* data)
{
	bm->bm_x = x;
	bm->bm_y = y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = 0;
	bm->bm_type = mode;
	bm->bm_rowsize = bytesperline;
	bm->bm_data = data;
	bm->bm_selector = 0;
}


grs_bitmap* gr_create_sub_bitmap(grs_bitmap* bm, int x, int y, int w, int h)
{
	grs_bitmap* newbm;

	newbm = (grs_bitmap*)malloc(sizeof(grs_bitmap));
	newbm->bm_x = x + bm->bm_x;
	newbm->bm_y = y + bm->bm_y;
	newbm->bm_w = w;
	newbm->bm_h = h;
	newbm->bm_flags = bm->bm_flags;
	newbm->bm_type = bm->bm_type;
	newbm->bm_rowsize = bm->bm_rowsize;
	newbm->bm_data = bm->bm_data + (unsigned int)((y * bm->bm_rowsize) + x);
	newbm->bm_selector = 0;

	return newbm;
}


void gr_free_bitmap(grs_bitmap* bm)
{
	if (bm->bm_data != NULL)
		free(bm->bm_data);
	bm->bm_data = NULL;
	if (bm != NULL)
		free(bm);
}

void gr_free_sub_bitmap(grs_bitmap* bm)
{
	if (bm != NULL)
		free(bm);
}

void build_colormap_good(uint8_t* palette, uint8_t* colormap, int* freq);

//NO_INVERSE_TABLE void build_colormap_asm( uint8_t * palette, uint8_t * cmap, int * count );
//NO_INVERSE_TABLE #pragma aux build_colormap_asm parm [esi] [edi] [edx] modify exact [eax ebx ecx edx esi edi] = \
//NO_INVERSE_TABLE 	"mov  ecx, 256"			\
//NO_INVERSE_TABLE 	"xor	eax,eax"				\
//NO_INVERSE_TABLE "again2x:"						\
//NO_INVERSE_TABLE 	"mov	al,[esi]"			\
//NO_INVERSE_TABLE 	"inc	esi"					\
//NO_INVERSE_TABLE 	"shr	eax, 1"				\
//NO_INVERSE_TABLE 	"shl	eax, 5"				\
//NO_INVERSE_TABLE 	"mov	bl,[esi]"			\
//NO_INVERSE_TABLE 	"inc	esi"					\
//NO_INVERSE_TABLE 	"shr	bl, 1"				\
//NO_INVERSE_TABLE 	"or	al, bl"				\
//NO_INVERSE_TABLE 	"shl	eax, 5"				\
//NO_INVERSE_TABLE 	"mov	bl,[esi]"			\
//NO_INVERSE_TABLE 	"inc	esi"					\
//NO_INVERSE_TABLE 	"shr	bl, 1"				\
//NO_INVERSE_TABLE 	"or 	al, bl"				\
//NO_INVERSE_TABLE 	"mov	al, gr_inverse_table[eax]"			\
//NO_INVERSE_TABLE 	"mov	[edi], al"			\
//NO_INVERSE_TABLE 	"inc	edi"					\
//NO_INVERSE_TABLE 	"xor	eax,eax"				\
//NO_INVERSE_TABLE 	"mov	[edx], eax"			\
//NO_INVERSE_TABLE 	"add	edx, 4"					\
//NO_INVERSE_TABLE 	"dec	ecx"					\
//NO_INVERSE_TABLE 	"jne	again2x"				\

void decode_data_asm(uint8_t* data, int num_pixels, uint8_t* colormap, int* count)
{ //[ISB] From Mac source. 
	int i;

	for (i = 0; i < num_pixels; i++) {
		count[*data]++;
		*data = colormap[*data];
		data++;
	}
}

#if 0
#pragma aux decode_data_asm parm [esi] [ecx] [edi] [ebx] modify exact [esi edi eax ebx ecx] = \
"again_ddn:"							\
	"xor	eax,eax"				\
	"mov	al,[esi]"			\
	"inc	dword ptr [ebx+eax*4]"		\
	"mov	al,[edi+eax]"		\
	"mov	[esi],al"			\
	"inc	esi"					\
	"dec	ecx"					\
	"jne	again_ddn"
#endif

void gr_remap_bitmap(grs_bitmap * bmp, uint8_t * palette, int transparent_color, int super_transparent_color)
{
	uint8_t colormap[256];
	int freq[256];

	// This should be build_colormap_asm, but we're not using invert table, so...
	build_colormap_good(palette, colormap, freq);

	if ((super_transparent_color >= 0) && (super_transparent_color <= 255))
		colormap[super_transparent_color] = 254;

	if ((transparent_color >= 0) && (transparent_color <= 255))
		colormap[transparent_color] = 255;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq);

	if ((transparent_color >= 0) && (transparent_color <= 255) && (freq[transparent_color] > 0))
		bmp->bm_flags |= BM_FLAG_TRANSPARENT;

	if ((super_transparent_color >= 0) && (super_transparent_color <= 255) && (freq[super_transparent_color] > 0))
		bmp->bm_flags |= BM_FLAG_SUPER_TRANSPARENT;
}

void build_colormap_good(uint8_t* palette, uint8_t* colormap, int* freq)
{
	int i, r, g, b;

	for (i = 0; i < 256; i++) {
		r = *palette++;
		g = *palette++;
		b = *palette++;
		*colormap++ = gr_find_closest_color(r, g, b);
		*freq++ = 0;
	}
}


void gr_remap_bitmap_good(grs_bitmap* bmp, uint8_t* palette, int transparent_color, int super_transparent_color)
{
	uint8_t colormap[256];
	int freq[256];

	build_colormap_good(palette, colormap, freq);

	if ((super_transparent_color >= 0) && (super_transparent_color <= 255))
		colormap[super_transparent_color] = 254;

	if ((transparent_color >= 0) && (transparent_color <= 255))
		colormap[transparent_color] = 255;

	//[ISB] From Descent 2, fixes automap bug
	if (bmp->bm_w == bmp->bm_rowsize)
		decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq);
	else 
	{
		int y;
		uint8_t* p = bmp->bm_data;
		for (y = 0; y < bmp->bm_h; y++, p += bmp->bm_rowsize)
			decode_data_asm(p, bmp->bm_w, colormap, freq);
	}
	//decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq);

	if ((transparent_color >= 0) && (transparent_color <= 255) && (freq[transparent_color] > 0))
		bmp->bm_flags |= BM_FLAG_TRANSPARENT;

	if ((super_transparent_color >= 0) && (super_transparent_color <= 255) && (freq[super_transparent_color] > 0))
		bmp->bm_flags |= BM_FLAG_SUPER_TRANSPARENT;
}

void gr_bitmap_check_transparency(grs_bitmap* bmp)
{
	int x, y;
	uint8_t* data;

	data = bmp->bm_data;

	for (y = 0; y < bmp->bm_h; y++) {
		for (x = 0; x < bmp->bm_w; x++) {
			if (*data++ == 255) {
				bmp->bm_flags = BM_FLAG_TRANSPARENT;
				return;
			}
		}
		data += bmp->bm_rowsize - bmp->bm_w;
	}

	bmp->bm_flags = 0;

}