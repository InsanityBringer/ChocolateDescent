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

#pragma once

#include "misc/types.h"
#include "2d/gr.h"

uint8_t* gr_rle_decode(uint8_t* src, uint8_t* dest); 
int gr_rle_encode(int org_size, uint8_t* src, uint8_t* dest);
int gr_rle_getsize(int org_size, uint8_t* src);
uint8_t* gr_rle_find_xth_pixel(uint8_t* src, int x, int* count, uint8_t color);
int gr_bitmap_rle_compress(grs_bitmap* bmp);
void gr_rle_expand_scanline_masked(uint8_t* dest, uint8_t* src, int x1, int x2);
void gr_rle_expand_scanline_masked_8_to_rgb15(uint16_t* dest, uint8_t* src, int x1, int x2);
void gr_rle_expand_scanline(uint8_t* dest, uint8_t* src, int x1, int x2);
void gr_rle_expand_scanline_8_to_rgb15(uint16_t* dest, uint8_t* src, int x1, int x2);

grs_bitmap* rle_expand_texture(grs_bitmap* bmp);

void rle_cache_flush();
