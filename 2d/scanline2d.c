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
/*
 * $Source: Smoke:miner:source:2d::RCS:scanline.c $
 * $Revision: 1.6 $
 * $Author: allender $
 * $Date: 1995/09/14 14:24:03 $
 *
 * Graphical routines for drawing solid scanlines.
 *
 * $Log: scanline.c $
 * Revision 1.6  1995/09/14  14:24:03  allender
 * fixed MW compile error
 *
 * Revision 1.5  1995/09/14  13:45:17  allender
 * quick optimization for scanline
 *
 * Revision 1.4  1995/04/27  07:36:05  allender
 * remove some memsets since all old is here now
 *
 * Revision 1.3  1995/04/19  14:35:33  allender
 * *** empty log message ***
 *
 * Revision 1.2  1995/04/18  12:03:40  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/03/09  09:24:06  allender
 * Initial revision
 *
 *
 * --- PC RCS information ---
 * Revision 1.7  1994/11/18  22:50:48  john
 * Changed a bunch of shorts to ints in calls.
 *
 * Revision 1.6  1994/09/02  11:40:32  john
 * fixed bug with urect scanline drakening still
 * only using 16 levels of fade.
 *
 * Revision 1.5  1994/04/08  16:59:12  john
 * Add fading poly's; Made palette fade 32 instead of 16.
 *
 * Revision 1.4  1994/03/22  18:36:27  john
 * Added darkening scanlines
 *
 * Revision 1.3  1993/10/15  16:22:52  john
 * y
 *
 * Revision 1.2  1993/09/08  11:56:29  john
 * neatened
 *
 * Revision 1.1  1993/09/08  11:44:27  john
 * Initial revision
 *
 *
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

	if (count > 3) {
		while ((int)(dest) & 0x3) { *dest++ = color; count--; };
		if (count >= 4) {
			x = (color << 24) | (color << 16) | (color << 8) | color;
			while (count > 4) { *(int*)dest = x; dest += 4; count -= 4; };
		}
		while (count > 0) { *dest++ = color; count--; };
	}
	else {
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
