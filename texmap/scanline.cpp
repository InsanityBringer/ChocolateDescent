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

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "fix/fix.h"
#include "platform/mono.h"
#include "misc/error.h"
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

	/*if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) < 0)
	{
		printf("Underflow drawing unlit flat scanline\n");
	}
	if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing unlit flat scanline\n");
	}*/

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

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest++ = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
			u += dudx;
			v += dvdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
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

	/*if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing linear texture scanline\n");
	}*/

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

int hack1, hack2;
//[ISB] The perspective texture mapper had some serious problems with negative coordinates. This hack should hopefully prevent it..
//HACK: OPTIMIZE ME
inline uint32_t TMap_PickCoord(int u, int v, int z)
{
	hack1 = u < 0 ? 1 : 0;
	hack2 = v < 0 ? 1 : 0;
	u /= z;
	v /= z;

	if (hack1) u--;
	if (hack2) v--;

	return ((v & 63) << 6) | (u & 63);
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
	/*if (((fx_xleft)+(bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing unlit perspective texture scanline\n");
	}*/

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest++ = (uint32_t)pixptr[TMap_PickCoord(u, v, z)];
			/*if (u < 0 || v < 0)
				c = 0xC0;
			else
				c = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];
			*dest++ = c;*/
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

	/*if (((fx_xleft) + (bytes_per_row * fx_y) + (fx_xright - fx_xleft + 1)) > (640 * 480))
	{
		printf("Overflow drawing perspective texture scanline\n");
	}*/ //[ISB] hopefully this overflow detector isn't needed anymore

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

#define NBITS 4
#define ZSHIFT 4
uint16_t ut, vt;
uint16_t ui, vi;
fix U0, V0, Z0, U1, V1;

fix pdiv(int a, int b)
{
	return (fix)((((int64_t)a << ZSHIFT) / b) << (16 - ZSHIFT));
}

//even and odd
#define C_TMAP_SCANLINE_PLN_LOOP 		ut = (ut + ui);\
										vt = (vt + vi);\
										*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];\
										l += dldx;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];\
										l += dldx;

#define C_TMAP_SCANLINE_PLT_LOOP 		ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;

#define C_TMAP_SCANLINE_PLN_LOOP_F 				ut = (ut + ui);\
												vt = (vt + vi);\
												*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];\
												l += dldx;\
												if (--num_left_over == 0) return;\
												ut = (ut + ui);\
												vt = (vt + vi);\
												*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];\
												l += dldx;\
												if (--num_left_over == 0) return;

#define C_TMAP_SCANLINE_PLT_LOOP_F 		ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;

int loop_count, num_left_over;
dbool new_end;

void c_tmap_scanline_pln()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;
	short cl;

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

	loop_count = fx_xright - fx_xleft + 1;

	//Painful code to try to replicate the ASM drawer. aaa
	//Wants to be dword aligned.
	while ((uintptr_t)(dest) & 3)
	{
		c = (uint32_t)pixptr[TMap_PickCoord(u, v, z)];
		if (c != 255)
			*dest = gr_fade_table[(l & (0xff00)) + c]; //oh yeah first ~3 pixels don't check transparency, not that it's relevant
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;
		if (--loop_count == 0) return; //none to do anymore
	}

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = pdiv(v, z);
	U0 = pdiv(u, z);

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = pdiv(v, z);
		U1 = pdiv(u, z);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
		}
		else
		{
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
		}
	}

	if (num_left_over == 0) return;

	int zcmp = z * 2 + z;
	int localz = z + dzdx;

	if (localz >= 0)
	{
		localz <<= 2;
		if (zcmp < localz) //Under certain circumstances, the weirder finishing code can be used. Replicate this.
		{
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;

			cl = 1;
			//z went negative.
			//this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
			//though we might only plot one more pixel.
			while (z < 0 && cl != NBITS)
			{
				u -= (dudx >> cl);
				v -= (dvdx >> cl);
				z -= (dzdx >> cl);

				cl++;
			}
			if (z <= (1 << (ZSHIFT + 1)))
			{
				z = (1 << (ZSHIFT + 1));
			}

			V1 = pdiv(v, z);
			U1 = pdiv(u, z);

			ut = static_cast<uint16_t>(U0 >> 6);
			vt = static_cast<uint16_t>(V0 >> 6);

			ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
			vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

			U0 = U1;
			V0 = V1;

			if (!Transparency_on)
			{
				C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
			}
			else
			{
				C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
			}

			Int3();
			return;
		}
	}
	
	
	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			//*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)]];
			*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[TMap_PickCoord(u, v, z)]];
			//*dest++ = 15;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	else
	{
		for (x = num_left_over; x > 0; --x)
		{
			//c = (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
			c = (uint32_t)pixptr[TMap_PickCoord(u, v, z)];
			if (c != 255)
				*dest = gr_fade_table[(l & (0xff00)) + c];
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	
}

//even and odd
#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP 		ut = (ut + ui);\
										vt = (vt + vi);\
										*dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										l += dldx;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										*dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										l += dldx;

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP 		ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;

#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F 				ut = (ut + ui);\
												vt = (vt + vi);\
												*dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
												l += dldx;\
												if (--num_left_over == 0) return;\
												ut = (ut + ui);\
												vt = (vt + vi);\
												*dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
												l += dldx;\
												if (--num_left_over == 0) return;

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F 		ut = (ut + ui);\
										vt = (vt + vi);\
										c = pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;\
										ut = (ut + ui);\
										vt = (vt + vi);\
										c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;

void c_tmap_scanline_pln_nolight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;
	short cl;

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

	loop_count = fx_xright - fx_xleft + 1;

	//Painful code to try to replicate the ASM drawer. aaa
	//Wants to be dword aligned.
	while ((uintptr_t)(dest) & 3)
	{
		c = (uint32_t)pixptr[TMap_PickCoord(u, v, z)];
		if (c != 255)
			*dest = c; //oh yeah first ~3 pixels don't check transparency, not that it's relevant
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;
		if (--loop_count == 0) return; //none to do anymore
	}

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = pdiv(v, z);
	U0 = pdiv(u, z);

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = pdiv(v, z);
		U1 = pdiv(u, z);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
		}
		else
		{
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
		}
	}

	if (num_left_over == 0) return;

	int zcmp = z * 2 + z;
	int localz = z + dzdx;

	if (localz >= 0)
	{
		localz <<= 2;
		if (zcmp < localz) //Under certain circumstances, the weirder finishing code can be used. Replicate this.
		{
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;

			cl = 1;
			//z went negative.
			//this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
			//though we might only plot one more pixel.
			while (z < 0 && cl != NBITS)
			{
				u -= (dudx >> cl);
				v -= (dvdx >> cl);
				z -= (dzdx >> cl);

				cl++;
			}
			if (z <= (1 << (ZSHIFT + 1)))
			{
				z = (1 << (ZSHIFT + 1));
			}

			V1 = pdiv(v, z);
			U1 = pdiv(u, z);

			ut = static_cast<uint16_t>(U0 >> 6);
			vt = static_cast<uint16_t>(V0 >> 6);

			ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
			vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

			U0 = U1;
			V0 = V1;

			if (!Transparency_on)
			{
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
			}
			else
			{
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
			}

			Int3();
			return;
		}
	}


	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			//*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)]];
			*dest++ = pixptr[TMap_PickCoord(u, v, z)];
			//*dest++ = 15;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	else
	{
		for (x = num_left_over; x > 0; --x)
		{
			//c = (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
			c = (uint32_t)pixptr[TMap_PickCoord(u, v, z)];
			if (c != 255)
				*dest = c;
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}

}

