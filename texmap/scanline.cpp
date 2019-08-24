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
 * $Source: f:/miner/source/texmap/rcs/scanline.c $
 * $Revision: 1.2 $
 * $Author: john $
 * $Date: 1995/02/20 18:23:39 $
 *
 * Routines to draw the texture mapped scanlines.
 *
 * $Log: scanline.c $
 * Revision 1.2  1995/02/20  18:23:39  john
 * Added new module for C versions of inner loops.
 *
 * Revision 1.1  1995/02/20  17:42:27  john
 * Initial revision
 *
 *
 */

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include "fix/fix.h"
#include "platform/mono.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "texmap/texmap.h"
#include "texmapl.h"
#include "scanline.h"

void c_tmap_scanline_flat()
{
	uint8_t* dest;
	int x;

	//[ISB] godawful hack from the ASM
	if (fx_y > window_bottom)
		return;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) < 0)
	{
		printf("Underflow drawing unlit flat scanline\n");
	}
	if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing unlit flat scanline\n");
	}

	for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
	{
		*dest++ = tmap_flat_color;
	}
}

void c_tmap_scanline_shaded()
{
	int fade;
	uint8_t* dest;
	int x;

	//[ISB] godawful hack from the ASM
	if (fx_y > window_bottom)
		return;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	fade = tmap_flat_shade_value << 8;
	for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
	{
		*dest++ = gr_fade_table[fade | (*dest)];
	}
}

void c_tmap_scanline_lin_nolight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, dudx, dvdx;

	u = fx_u;
	v = fx_v * 64;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx * 64;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	if (!Transparency_on) {
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) {
			*dest++ = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
			u += dudx;
			v += dvdx;
		}
	}
	else {
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) {
			c = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
			if (c != 255)
				* dest = c;
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
}


void c_tmap_scanline_lin()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, l, dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing linear texture scanline\n");
	}

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((f2i(v) & 63) * 64) + (f2i(u) & 63)]];
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[((f2i(v) & 63) * 64) + (f2i(u) & 63)];
			if (c != 255)
				* dest = gr_fade_table[(l & (0xff00)) + c];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
}


void c_tmap_scanline_per_nolight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));
	if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing unlit perspective texture scanline\n");
	}

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest++ = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			//c = (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
			c = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];

			if (c != 255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

void c_tmap_scanline_per()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	if (((fx_xleft) + (bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing perspective texture scanline\n");
	}

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			//*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)]];
			*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)]];
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			//c = (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
			c = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];
			if (c != 255)
				*dest = gr_fade_table[(l & (0xff00)) + c];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;
		}
	}
}
