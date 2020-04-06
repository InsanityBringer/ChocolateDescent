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

int Gr_scanline_darkening_level = GR_FADE_LEVELS;

void gr_linear_darken(uint8_t* dest, int darkening_level, int count, uint8_t* fade_table)
{
	int i;

	for (i = 0; i < count; i++) {
		*dest = fade_table[*dest + (darkening_level * 256)];
		dest++;
	}
}

void gr_linear_stosd(uint8_t* dest, uint8_t color, int count)
{
	int i, x;

	if (count > 3) 
	{
		while ((uintptr_t)(dest) & 0x3) { *dest++ = color; count--; };
		if (count >= 4) 
		{
			x = (color << 24) | (color << 16) | (color << 8) | color;
			while (count > 4) { *(int*)dest = x; dest += 4; count -= 4; };
		}
		while (count > 0) { *dest++ = color; count--; };
	}
	else 
	{
		for (i = 0; i < count; i++)
			* dest++ = color;
	}
}

void gr_uscanline(int x1, int x2, int y)
{
	//	memset(DATA + ROWSIZE*y + x1, COLOR, x2-x1+0);
	//
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS) {
		gr_linear_stosd(DATA + ROWSIZE * y + x1, (uint8_t)COLOR, x2 - x1 + 1);
	}
	else {
		gr_linear_darken(DATA + ROWSIZE * y + x1, Gr_scanline_darkening_level, x2 - x1 + 1, gr_fade_table);
	}
}

void gr_scanline(int x1, int x2, int y)
{
	if ((y < 0) || (y > MAXY)) return;

	if (x2 < x1) x2 ^= x1 ^= x2;

	if (x1 > MAXX) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > MAXX) x2 = MAXX;

	//	memset(DATA + ROWSIZE*y + x1, COLOR, x2-x1+1);
	//	
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS) {
		gr_linear_stosd(DATA + ROWSIZE * y + x1, (uint8_t)COLOR, x2 - x1 + 1);
	}
	else {
		gr_linear_darken(DATA + ROWSIZE * y + x1, Gr_scanline_darkening_level, x2 - x1 + 1, gr_fade_table);
	}
}
