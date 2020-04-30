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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "fix/fix.h"
#include "platform/mono.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"

#define DIVIDE_SIG_BITS		12
#define Z_SHIFTER 			(30-DIVIDE_SIG_BITS)
#define DIVIDE_TABLE_SIZE	(1<<DIVIDE_SIG_BITS)


extern uint8_t * dest_row_data;
extern int loop_count;

void c_tmap_scanline_flat()
{
	uint8_t *dest;
	int x;

	dest = dest_row_data;

	for (x=loop_count; x >= 0; x-- )
	{
		*dest++ = tmap_flat_color;
	}
}

void c_tmap_scanline_shaded()
{
	int fade;
	uint8_t *dest;
	int x;

	dest = dest_row_data;

	fade = tmap_flat_shade_value<<8;
	for (x=loop_count; x >= 0; x-- ) 
	{
		*dest++ = gr_fade_table[ fade |(*dest)];
	}
}

void c_tmap_scanline_lin_nolight()
{
	uint8_t *dest;
	uint32_t c;
	int x;
	fix u,v,dudx, dvdx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	dest = dest_row_data;

	for (x=loop_count; x >= 0; x-- ) 
	{
		*dest++ = (uint32_t)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
		u += dudx;
		v += dvdx;
	}
}

void c_tmap_scanline_lin_nolight_trans()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, dudx, dvdx;

	u = fx_u;
	v = fx_v * 64;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx * 64;

	dest = dest_row_data;

	for (x = loop_count; x >= 0; x--)
	{
		c = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
		if (c != 255)
			*dest = c;
		dest++;
		u += dudx;
		v += dvdx;
	}
}

void c_tmap_scanline_lin()
{
	uint8_t *dest;
	uint32_t c;
	int x;
	fix u,v,l,dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x=loop_count; x >= 0; x-- ) 
	{
		*dest++ = gr_fade_table[ (l&(0xff00)) + (uint32_t)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
		l += dldx;
		u += dudx;
		v += dvdx;
	}
}

void c_tmap_scanline_lin_trans()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, l, dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v * 64;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx * 64;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x >= 0; x--)
	{
		c = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
		if (c != 255)
			*dest = gr_fade_table[(l & (0xff00)) + c];
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
	}
}

void c_tmap_scanline_per_nolight()
{
	uint8_t *dest;
	uint32_t c;
	int x, localz;
	fix u,v,z,dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx; 
	dzdx = fx_dz_dx;

	dest = dest_row_data;

	for (x=loop_count; x >= 0; x-- ) 
	{
		localz = z >> Z_SHIFTER;
		if (localz == 0) break;
		*dest++ = (uint32_t)pixptr[(((v / localz) & 63) << 6) + ((u / localz) & 63)];
		u += dudx;
		v += dvdx;
		z += dzdx;
	} 
}

void c_tmap_scanline_per_nolight_trans()
{
	uint8_t* dest;
	uint32_t c;
	int x, localz;
	fix u, v, z, dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	dest = dest_row_data;

	for (x = loop_count; x >= 0; x--)
	{
		localz = z >> Z_SHIFTER;
		if (localz == 0) break;
		c = (uint32_t)pixptr[(((v / localz) & 63) << 6) + ((u / localz) & 63)];
		if (c != 255)
			*dest = c;
		dest++;
		u += dudx;
		v += dvdx;
		z += dzdx;
	}
}

void c_tmap_scanline_per()
{
	uint8_t *dest;
	uint32_t c;
	fix localz;
	int x;
	fix u,v,z,l,dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx;// *64; //[ISB] changes to make more accurate to ASM tmapper
	dzdx = fx_dz_dx;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x=loop_count; x >= 0; x-- ) 
	{
		localz = z >> Z_SHIFTER;
		if (localz == 0) break;
		//*dest++ = gr_fade_table[ (l&(0xff00)) + (uint32_t)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
		*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((v / localz) & 63) << 6) + ((u / localz) & 63)]];
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
	}
}

void c_tmap_scanline_per_trans()
{
	uint8_t* dest;
	uint32_t c;
	fix localz;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;// *64; //[ISB] changes to make more accurate to ASM tmapper
	dzdx = fx_dz_dx;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x >= 0; x--)
	{
		localz = z >> Z_SHIFTER;
		if (localz == 0) break;
		//c = (uint32_t)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
		c = (uint32_t)pixptr[(((v / localz) & 63) << 6) + ((u / localz) & 63)];
		if (c != 255)
			*dest = gr_fade_table[(l & (0xff00)) + c];
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
	}
}


extern fix divide_table[];

fix invert_z(fix in)
{
	in >>= Z_SHIFTER;
	return divide_table[(in & (DIVIDE_TABLE_SIZE-1))];
}

int num_left_over;
fix U0, V0, Z0, U1, V1;
uint16_t ut, vt;
uint16_t ui, vi;
#define NBITS 4

extern int fx_u_right, fx_v_right, fx_z_right;

#define C_TMAP_SCANLINE_PLN_LOOP 		*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];\
										ut = (ut + ui);\
										vt = (vt + vi);\
										l += dldx;

void c_tmap_scanline_pln()
{
	uint8_t* dest;
	uint32_t c;
	fix localz;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	if (loop_count == 0) return;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	z = invert_z(z);

	U0 = fixmul(u, z);
	V0 = fixmul(v, z);

	z = fx_z;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		localz = invert_z(z);

		U1 = fixmul(u, localz);
		V1 = fixmul(v, localz);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
		C_TMAP_SCANLINE_PLN_LOOP
	}
	if (loop_count == 0 || num_left_over > 4)
	{
		z = fx_z_right;
		z = invert_z(z);
		U1 = fixmul(fx_u_right, z);
		V1 = fixmul(fx_v_right, z);

		ui = static_cast<uint16_t>(((U1 - U0) / num_left_over) >> 6);
		vi = static_cast<uint16_t>(((V1 - V0) / num_left_over) >> 6);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);
	}
	for (x = num_left_over; x >= 0; x--)
	{
		ut = (ut + ui);
		vt = (vt + vi);
		*dest++ = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];
		l += dldx;
	}
}

#define C_TMAP_SCANLINE_PLT_LOOP 		c =  (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										ut = (ut + ui);\
										vt = (vt + vi);\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;

void c_tmap_scanline_plt()
{
	uint8_t* dest;
	uint32_t c;
	fix localz;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	if (loop_count == 0) return;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	z = invert_z(z);

	U0 = fixmul(u, z);
	V0 = fixmul(v, z);

	z = fx_z;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		localz = invert_z(z);

		U1 = fixmul(u, localz);
		V1 = fixmul(v, localz);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP
		C_TMAP_SCANLINE_PLT_LOOP

	}
	if (loop_count == 0 || num_left_over > 4)
	{
		z = fx_z_right;
		z = invert_z(z);
		U1 = fixmul(fx_u_right, z);
		V1 = fixmul(fx_v_right, z);

		ui = static_cast<uint16_t>(((U1 - U0) / num_left_over) >> 6);
		vi = static_cast<uint16_t>(((V1 - V0) / num_left_over) >> 6);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);
	}
	for (x = num_left_over; x >= 0; x--)
	{
		ut = (ut + ui);
		vt = (vt + vi);
		c = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))]];
		if (c != 255)
			*dest = c;
		dest++;
		l += dldx;
	}
}

#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP *dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										ut = (ut + ui); \
										vt = (vt + vi); \
										l += dldx;


void c_tmap_scanline_pln_nolight()
{
	uint8_t* dest;
	uint32_t c;
	fix localz;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	if (loop_count == 0) return;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	z = invert_z(z);

	U0 = fixmul(u, z);
	V0 = fixmul(v, z);

	z = fx_z;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		localz = invert_z(z);

		U1 = fixmul(u, localz);
		V1 = fixmul(v, localz);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
	}
	if (loop_count == 0 || num_left_over > 4)
	{
		z = fx_z_right;
		z = invert_z(z);
		U1 = fixmul(fx_u_right, z);
		V1 = fixmul(fx_v_right, z);

		ui = static_cast<uint16_t>(((U1 - U0) / num_left_over) >> 6);
		vi = static_cast<uint16_t>(((V1 - V0) / num_left_over) >> 6);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);
	}
	for (x = num_left_over; x >= 0; x--)
	{
		ut = (ut + ui);
		vt = (vt + vi);
		*dest++ = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];
		l += dldx;
	}
}

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP c =  (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];\
										ut = (ut + ui);\
										vt = (vt + vi);\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;

void c_tmap_scanline_plt_nolight()
{
	uint8_t* dest;
	uint32_t c;
	fix localz;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	if (loop_count == 0) return;

	u = fx_u;
	v = fx_v;// *64;
	z = fx_z;
	z = invert_z(z);

	U0 = fixmul(u, z);
	V0 = fixmul(v, z);

	z = fx_z;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		localz = invert_z(z);

		U1 = fixmul(u, localz);
		V1 = fixmul(v, localz);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);

		ui = static_cast<uint16_t>((U1 - U0) >> (NBITS + 6));
		vi = static_cast<uint16_t>((V1 - V0) >> (NBITS + 6));

		U0 = U1;
		V0 = V1;

		C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP

	}
	if (loop_count == 0 || num_left_over > 4)
	{
		z = fx_z_right;
		z = invert_z(z);
		U1 = fixmul(fx_u_right, z);
		V1 = fixmul(fx_v_right, z);

		ui = static_cast<uint16_t>(((U1 - U0) / num_left_over) >> 6);
		vi = static_cast<uint16_t>(((V1 - V0) / num_left_over) >> 6);

		ut = static_cast<uint16_t>(U0 >> 6);
		vt = static_cast<uint16_t>(V0 >> 6);
	}
	for (x = num_left_over; x >= 0; x--)
	{
		ut = (ut + ui);
		vt = (vt + vi);
		c = (uint32_t)pixptr[((ut >> 10) | ((vt >> 10) << 6))];
		if (c != 255)
			*dest = c;
		dest++;
		l += dldx;
	}
}

#define zonk 1

void c_tmap_scanline_editor()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v * 64;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx * 64;
	dzdx = fx_dz_dx;

	dest = dest_row_data;

	if (!Transparency_on) {
		for (x = loop_count; x >= 0; x--) {
			*dest++ = zonk;
			//(uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
	else {
		for (x = loop_count; x >= 0; x--) {
			c = (uint32_t)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
			if (c != 255)
				*dest = zonk;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}
