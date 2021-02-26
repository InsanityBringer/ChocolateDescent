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

#define VESA 0
#define NUM_TMAPS 16

#define HEADLIGHT_LIGHTING 0

#define WIREFRAME 0
#define PERSPECTIVE 1

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform/mono.h"
#include "fix/fix.h"
#include "3d/3d.h"
#include "2d/gr.h"
#include "misc/error.h"
//#include "platform/key.h" //[ISB] i'm sure there's some good reason why the texture mapping code needs the keyboard API

#include "texmap/texmap.h"
#include "texmapl.h"
#include "2d/rle.h"
#include "scanline.h"
//[ISB] WHAT IS PROJECT DESIGN
//#include "..\main\textures.h" //[ISB] cherry on top: unused

#define	EDITOR_TMAP	1		//if in, include extra stuff

#define F15_5 (F1_0*15 + F0_5)

// 1 means enable special sc2000 code, else enable only for Descent
#define	SC2000K	0
int	SC2000 = SC2000K;

// Temporary texture map, interface from Matt's 3d system to Mike's texture mapper.
g3ds_tmap Tmap1;

grs_bitmap Texmap_ptrs[NUM_TMAPS];
grs_bitmap Texmap4_ptrs[NUM_TMAPS];

fix Range_max = 0; // debug, kill me

int	Interpolation_method = 0;	// 0 = choose best method
int	Lighting_on;				// initialize to no lighting
int	Tmap_flat_flag = 0;		//	1 = render texture maps as flat shaded polygons.
int	Current_seg_depth;		// HACK INTERFACE: how far away the current segment (& thus texture) is
int	Max_perspective_depth;
int	Max_linear_depth;
int	Max_flat_depth;

#ifndef BUILD_DESCENT2
extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
#else
int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
#endif

// These variables are the interface to assembler.  They get set for each texture map, which is a real waste of time.
//	They should be set only when they change, which is generally when the window bounds change.  And, even still, it's
//	a pretty bad interface.
int	bytes_per_row = -1;
//int	write_buffer;
uint8_t* write_buffer;
int  	window_left;
int	window_right;
int	window_top;
int	window_bottom;
int  	window_width;
int  	window_height;
#ifdef EDITOR_TMAP
#ifdef BUILD_DESCENT2
#define	MAX_Y_POINTERS	1024
#else
#define	MAX_Y_POINTERS	480
#endif
#else
#define	MAX_Y_POINTERS	480
#endif
int	y_pointers[MAX_Y_POINTERS];

short	pixel_data_selector;		// selector for current pixel data for texture mapper

fix fix_recip[FIX_RECIP_TABLE_SIZE];

int	Lighting_enabled;
int	Fix_recip_table_computed = 0;

fix fx_l, fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx, fx_dl_dx;
int fx_xleft, fx_xright, fx_y;
unsigned char* pixptr;
int per2_flag = 0;
int Transparency_on = 0;
int dither_intensity_lighting = 0;

uint8_t* tmap_flat_cthru_table;
uint8_t tmap_flat_color;
uint8_t tmap_flat_shade_value;



// -------------------------------------------------------------------------------------
void init_fix_recip_table(void)
{
	int	i;

	fix_recip[0] = F1_0;

	for (i = 1; i < FIX_RECIP_TABLE_SIZE; i++)
		fix_recip[i] = F1_0 / i;

	Fix_recip_table_computed = 1;
}

// -------------------------------------------------------------------------------------
//	Initialize interface variables to assembler.
//	These things used to be constants.  This routine is now (10/6/93) getting called for
//	every texture map.  It should get called whenever the window changes, or, preferably,
//	not at all.  I'm pretty sure these variables are only being used for range checking.
void init_interface_vars_to_assembler(void)
{
	grs_bitmap* bp;
	bp = &grd_curcanv->cv_bitmap;

	Assert(bp != NULL);
	Assert(bp->bm_data != NULL);
	Assert(bp->bm_h <= MAX_Y_POINTERS);

	//	If bytes_per_row has changed, create new table of pointers.
	if (bytes_per_row != (int)bp->bm_rowsize) 
	{
		int	y_val, i;

		bytes_per_row = (int)bp->bm_rowsize;

		y_val = 0;
		for (i = 0; i < MAX_Y_POINTERS; i++) 
		{
			y_pointers[i] = y_val;
			y_val += bytes_per_row;
		}
	}

	write_buffer = bp->bm_data;

	window_left = 0;
	window_right = (int)bp->bm_w - 1;
	window_top = 0;
	window_bottom = (int)bp->bm_h - 1;

	Window_clip_left = window_left;
	Window_clip_right = window_right;
	Window_clip_top = window_top;
	Window_clip_bot = window_bottom;

	window_width = bp->bm_w;
	window_height = bp->bm_h;

	if (!Fix_recip_table_computed)
		init_fix_recip_table();
}

//int tmap_set_selector(int selector, void* buffer, unsigned int size); [ISB] unused?

// -------------------------------------------------------------------------------------
//                             VARIABLES
extern short	pixel_data_selector;		// selector for current pixel data for texture mapper
extern g3ds_tmap Tmap1;

// -------------------------------------------------------------------------------------
//	Returns number preceding val modulo modulus.
//	prevmod(3,4) = 2
//	prevmod(0,4) = 3
int prevmod(int val, int modulus)
{
	if (val > 0)
		return val - 1;
	else
		return modulus - 1;
	//	return (val + modulus - 1) % modulus;
}


//	Returns number succeeding val modulo modulus.
//	succmod(3,4) = 0
//	succmod(0,4) = 1
int succmod(int val, int modulus)
{
	if (val < modulus - 1)
		return val + 1;
	else
		return 0;

	//	return (val + 1) % modulus;
}

// -------------------------------------------------------------------------------------
//	Select topmost vertex (minimum y coordinate) and bottommost (maximum y coordinate) in
//	texture map.  If either is part of a horizontal edge, then select leftmost vertex for
//	top, rightmost vertex for bottom.
//	Important: Vertex is selected with integer precision.  So, if there are vertices at
//	(0.0,0.7) and (0.5,0.3), the first vertex is selected, because they y coordinates are
//	considered the same, so the smaller x is favored.
//	Parameters:
//		nv		number of vertices
//		v3d	pointer to 3d vertices containing u,v,x2d,y2d coordinates
//	Results in:
//		*min_y_ind
//		*max_y_ind
// -------------------------------------------------------------------------------------
void compute_y_bounds(g3ds_tmap* t, int* vlt, int* vlb, int* vrt, int* vrb, int* bottom_y_ind)
{
	int	i;
	int	min_y, max_y;
	int	min_y_ind;
	int	original_vrt;
	fix	min_x;

	// Scan all vertices, set min_y_ind to vertex with smallest y coordinate.
	min_y = f2i(t->verts[0].y2d);
	max_y = min_y;
	min_y_ind = 0;
	min_x = f2i(t->verts[0].x2d);
	*bottom_y_ind = 0;

	for (i = 1; i < t->nv; i++) 
	{
		if (f2i(t->verts[i].y2d) < min_y)
		{
			min_y = f2i(t->verts[i].y2d);
			min_y_ind = i;
			min_x = f2i(t->verts[i].x2d);
		}
		else if (f2i(t->verts[i].y2d) == min_y) 
		{
			if (f2i(t->verts[i].x2d) < min_x) 
			{
				min_y_ind = i;
				min_x = f2i(t->verts[i].x2d);
			}
		}
		if (f2i(t->verts[i].y2d) > max_y) 
		{
			max_y = f2i(t->verts[i].y2d);
			*bottom_y_ind = i;
		}
	}

	// Set "vertex left top", etc. based on vertex with topmost y coordinate
	*vlt = min_y_ind;
	*vrt = *vlt;
	*vlb = prevmod(*vlt, t->nv);
	*vrb = succmod(*vrt, t->nv);

	// If right edge is horizontal, then advance along polygon bound until it no longer is or until all
	// vertices have been examined.
	// (Left edge cannot be horizontal, because *vlt is set to leftmost point with highest y coordinate.)
	original_vrt = *vrt;

	while (f2i(t->verts[*vrt].y2d) == f2i(t->verts[*vrb].y2d)) 
	{
		if (succmod(*vrt, t->nv) == original_vrt) 
		{
			break;
		}
		*vrt = succmod(*vrt, t->nv);
		*vrb = succmod(*vrt, t->nv);
	}
}

fix compute_du_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].u - t->verts[top_vertex].u, recip_dy);
}

fix compute_dv_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].v - t->verts[top_vertex].v, recip_dy);
}

fix compute_dl_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].l - t->verts[top_vertex].l, recip_dy);
}

fix compute_dx_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d, recip_dy);
}

fix compute_du_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t->verts[bottom_vertex].u, t->verts[bottom_vertex].z) - fixmul(t->verts[top_vertex].u, t->verts[top_vertex].z), recip_dy);
}

fix compute_dv_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t->verts[bottom_vertex].v, t->verts[bottom_vertex].z) - fixmul(t->verts[top_vertex].v, t->verts[top_vertex].z), recip_dy);
}

fix compute_dz_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].z - t->verts[top_vertex].z, recip_dy);
}
int Skip_short_flag = 0;

extern void c_tmap_scanline_editor();

// -------------------------------------------------------------------------------------
//	Texture map current scanline in perspective.
// -------------------------------------------------------------------------------------
void ntmap_scanline_lighted(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright)
{
	fix	u, v, l;
	fix	dx, recip_dx;

	fix	du_dx, dv_dx, dz_dx, z;

	u = uleft;
	v = vleft;
	l = lleft;

	fx_xright = f2i(xright);
	fx_xleft = f2i(xleft);

	dx = fx_xright - fx_xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	if (fx_xright > Window_clip_right)
		fx_xright = Window_clip_right;

	// setup to call assembler scanline renderer
	if (dx < FIX_RECIP_TABLE_SIZE)
		recip_dx = fix_recip[dx];
	else
		recip_dx = F1_0 / dx;

	du_dx = fixmul(uright - uleft, recip_dx);
	dv_dx = fixmul(vright - vleft, recip_dx);
	dz_dx = fixmul(zright - zleft, recip_dx);

	z = zleft;

	fx_u = uleft;
	fx_v = vleft;
	fx_z = zleft;
	fx_du_dx = du_dx;
	fx_dv_dx = dv_dx;
	fx_dz_dx = dz_dx;
	fx_y = y;
	pixptr = srcb->bm_data;

	switch (Lighting_enabled) 
	{
	case 0:
		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

		c_tmap_scanline_pln_nolight();
		break;
	case 1: 
	{
		fix	mul_thing;

		if (lleft < 0) lleft = 0;
		if (lright < 0) lright = 0;
		if (lleft > (NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2)) lleft = (NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2);
		if (lright > (NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2)) lright = (NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2);

		fx_l = lleft;
		fx_dl_dx = fixmul(lright - lleft, recip_dx);

		//	This is a pretty ugly hack to prevent lighting overflows.
		mul_thing = dx * fx_dl_dx;
		if (lleft + mul_thing < 0)
			fx_dl_dx += 12;
		else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2))
			fx_dl_dx -= 12;

		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

#ifdef TEXMAP_ANTIALIAS
		c_tmap_scanline_pln_aa();
#else
#ifdef TEXMAP_DITHER
		c_tmap_scanline_per_dither();
#else
		c_tmap_scanline_per_16();
#endif
#endif
		break;
	}
	case 2:
#ifdef EDITOR_TMAP
		fx_xright = f2i(xright);
		fx_xleft = f2i(xleft);

		c_tmap_scanline_editor();
#else
		Int3();	//	Illegal, called an editor only routine!
#endif
		break;
	}

}

int Do_vertical_scan = 0;

int	Break_on_flat = 0;

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void ntexture_map_lighted(grs_bitmap* srcb, g3ds_tmap* t)
{
	int	vlt, vrt, vlb, vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy, boty, y, dy;
	fix	dx_dy_left, dx_dy_right;
	fix	du_dy_left, du_dy_right;
	fix	dv_dy_left, dv_dy_right;
	fix	dz_dy_left, dz_dy_right;
	fix	dl_dy_left, dl_dy_right;
	fix	recip_dyl, recip_dyr;
	int	max_y_vertex;
	fix	xleft, xright, uleft, vleft, uright, vright, zleft, zright, lleft, lright;
	int	next_break_left, next_break_right;

	g3ds_vertex* v3d;

	v3d = t->verts;

	//[ISB] more uninitalized memory warnings
	lright = lleft = 0;

	// Determine top and bottom y coords.
	compute_y_bounds(t, &vlt, &vlb, &vrt, &vrb, &max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);
	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	// Set amount to change x coordinate for each advance to next scanline.
	dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0 / dy;

	dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dyl);
	du_dy_left = compute_du_dy(t, vlt, vlb, recip_dyl);
	dv_dy_left = compute_dv_dy(t, vlt, vlb, recip_dyl);
	dz_dy_left = compute_dz_dy(t, vlt, vlb, recip_dyl);

	dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0 / dy;

	du_dy_right = compute_du_dy(t, vrt, vrb, recip_dyr);
	dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dyr);
	dv_dy_right = compute_dv_dy(t, vrt, vrb, recip_dyr);
	dz_dy_right = compute_dz_dy(t, vrt, vrb, recip_dyr);

	if (Lighting_enabled) 
	{
		dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	zleft = v3d[vlt].z;
	zright = v3d[vrt].z;

	uleft = fixmul(v3d[vlt].u, zleft);
	uright = fixmul(v3d[vrt].u, zright);
	vleft = fixmul(v3d[vlt].v, zleft);
	vright = fixmul(v3d[vrt].v, zright);

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) 
	{
		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left)
		{
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) 
			{
				vlt = vlb;
				vlb = prevmod(vlb, t->nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0 / dy;

			dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			zleft = v3d[vlt].z;
			uleft = fixmul(v3d[vlt].u, zleft);
			vleft = fixmul(v3d[vlt].v, zleft);
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy(t, vlt, vlb, recip_dy);
			dv_dy_left = compute_dv_dy(t, vlt, vlb, recip_dy);
			dz_dy_left = compute_dz_dy(t, vlt, vlb, recip_dy);

			if (Lighting_enabled) 
			{
				dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) 
		{
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) 
			{
				vrt = vrb;
				vrb = succmod(vrb, t->nv);
			}

			next_break_right = f2i(v3d[vrb].y2d);

			dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0 / dy;

			dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);

			xright = v3d[vrt].x2d;
			zright = v3d[vrt].z;
			uright = fixmul(v3d[vrt].u, zright);
			vright = fixmul(v3d[vrt].v, zright);

			du_dy_right = compute_du_dy(t, vrt, vrb, recip_dy);
			dv_dy_right = compute_dv_dy(t, vrt, vrb, recip_dy);
			dz_dy_right = compute_dz_dy(t, vrt, vrb, recip_dy);

			if (Lighting_enabled)
			{
				dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) 
		{
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		}
		else
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

		zleft += dz_dy_left;
		zright += dz_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.
	ntmap_scanline_lighted(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);
}


// -------------------------------------------------------------------------------------
//	Texture map current scanline using linear interpolation.
// -------------------------------------------------------------------------------------
void ntmap_scanline_lighted_linear(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix lleft, fix lright)
{
	fix	u, v, l;
	fix	dx, recip_dx;

	fix	du_dx, dv_dx, dl_dx;

	u = uleft;
	v = vleft;
	l = lleft;

	dx = f2i(xright) - f2i(xleft);
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	// setup to call assembler scanline renderer
	if (dx < FIX_RECIP_TABLE_SIZE)
		recip_dx = fix_recip[dx];
	else
		recip_dx = F1_0 / dx;

	du_dx = fixmul(uright - uleft, recip_dx);
	dv_dx = fixmul(vright - vleft, recip_dx);

	fx_u = uleft;
	fx_v = vleft;
	fx_du_dx = du_dx;
	fx_dv_dx = dv_dx;
	fx_y = y;
	fx_xright = f2i(xright);
	fx_xleft = f2i(xleft);

	if (fx_xright > Window_clip_right)
		fx_xright = Window_clip_right;

	pixptr = srcb->bm_data;

	switch (Lighting_enabled) 
	{
	case 0:
		c_tmap_scanline_lin_nolight();
		break;
	case 1:
		if (lleft < F1_0 / 2)
			lleft = F1_0 / 2;
		if (lright < F1_0 / 2)
			lright = F1_0 / 2;

		if (lleft > MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS)
			lleft = MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS;
		if (lright > MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS)
			lright = MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS;

		fx_l = lleft;
		dl_dx = fixmul(lright - lleft, recip_dx);
		fx_dl_dx = dl_dx;
		c_tmap_scanline_lin_16();
		break;
	case 2:
#ifdef EDITOR_TMAP
		fx_xright = f2i(xright);
		fx_xleft = f2i(xleft);
		//asm_tmap_scanline_matt();
#else
		Int3();	//	Illegal, called an editor only routine!
#endif
		break;
	}
}

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void ntexture_map_lighted_linear(grs_bitmap* srcb, g3ds_tmap* t)
{
	int	vlt, vrt, vlb, vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy, boty, y, dy;
	fix	dx_dy_left, dx_dy_right;
	fix	du_dy_left, du_dy_right;
	fix	dv_dy_left, dv_dy_right;
	fix	dl_dy_left, dl_dy_right;
	int	max_y_vertex;
	fix	xleft, xright, uleft, vleft, uright, vright, lleft, lright;
	int	next_break_left, next_break_right;
	fix	recip_dyl, recip_dyr;

	//[ISB]possibly uninitalized memory?
	lleft = lright = 0;

	g3ds_vertex* v3d;

	v3d = t->verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t, &vlt, &vlb, &vrt, &vrb, &max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);

	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0 / dy;

	dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0 / dy;

	// Set amount to change x coordinate for each advance to next scanline.
	dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dyl);
	dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dyr);

	du_dy_left = compute_du_dy_lin(t, vlt, vlb, recip_dyl);
	du_dy_right = compute_du_dy_lin(t, vrt, vrb, recip_dyr);

	dv_dy_left = compute_dv_dy_lin(t, vlt, vlb, recip_dyl);
	dv_dy_right = compute_dv_dy_lin(t, vrt, vrb, recip_dyr);

	if (Lighting_enabled) 
	{
		dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	uleft = v3d[vlt].u;
	uright = v3d[vrt].u;
	vleft = v3d[vlt].v;
	vright = v3d[vrt].v;

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) 
	{
		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) 
		{
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) 
			{
				vlt = vlb;
				vlb = prevmod(vlb, t->nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0 / dy;

			dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			uleft = v3d[vlt].u;
			vleft = v3d[vlt].v;
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy_lin(t, vlt, vlb, recip_dy);
			dv_dy_left = compute_dv_dy_lin(t, vlt, vlb, recip_dy);

			if (Lighting_enabled) 
			{
				dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) 
		{
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) 
			{
				vrt = vrb;
				vrb = succmod(vrb, t->nv);
			}

			dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0 / dy;

			next_break_right = f2i(v3d[vrb].y2d);
			dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);

			xright = v3d[vrt].x2d;
			uright = v3d[vrt].u;
			vright = v3d[vrt].v;

			du_dy_right = compute_du_dy_lin(t, vrt, vrb, recip_dy);
			dv_dy_right = compute_dv_dy_lin(t, vrt, vrb, recip_dy);

			if (Lighting_enabled) 
			{
				dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) 
		{
			ntmap_scanline_lighted_linear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		}
		else
			ntmap_scanline_lighted_linear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;
	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.
	ntmap_scanline_lighted_linear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);
}

// fix	DivNum = F1_0*12;

extern void draw_tmap_flat(grs_bitmap* bp, int nverts, g3s_point** vertbuf);

// -------------------------------------------------------------------------------------
// Interface from Matt's data structures to Mike's texture mapper.
// -------------------------------------------------------------------------------------
void draw_tmap(grs_bitmap* bp, int nverts, g3s_point** vertbuf)
{
	int	i;

	//	These variables are used in system which renders texture maps which lie on one scanline as a line.
#if SC2000K
	int	flat_flag;							// Set to 1 and remains 1 so long as all y coords are the same (in integer portion).
	int	last_y;								// Y coordinate of previous vertex.
	fix	min_x = 0xfff0000, max_x = 0;	// Minimum and maximum bounds of line to render in place of flat texture map.
#endif
	// fix	div_numerator;
	int	lighting_on_save = Lighting_on;

	Assert(nverts <= MAX_TMAP_VERTS);
#ifdef USE_MULT_CODE
	if (!divide_table_filled) fill_divide_table();
#endif

	//	If no transparency and seg depth is large, render as flat shaded.
	if ((Current_seg_depth > Max_linear_depth) && ((bp->bm_flags & 3) == 0)) 
	{
		draw_tmap_flat(bp, nverts, vertbuf);
		return;
	}

	if (bp->bm_flags & BM_FLAG_RLE)
		bp = rle_expand_texture(bp);		// Expand if rle'd

	Transparency_on = bp->bm_flags & BM_FLAG_TRANSPARENT;
	if (bp->bm_flags & BM_FLAG_NO_LIGHTING)
		Lighting_on = 0;

	// Setup texture map in Tmap1
	Tmap1.nv = nverts;						// Initialize number of vertices

	for (i = 0; i < nverts; i++) 
	{
		g3ds_vertex* tvp = &Tmap1.verts[i];
		g3s_point* vp = vertbuf[i];

		tvp->x2d = vp->p3_sx;
		tvp->y2d = vp->p3_sy;

		//	Check for overflow on fixdiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
		if (vp->p3_z < 256) //[ISB] change from Descent 2, guess ntmap wasn't used? heeh. This bodes well...
		{
			vp->p3_z = 256;
			// Int3();		// we would overflow if we divided!
		}

		tvp->z = fixdiv(F1_0 * 12, vp->p3_z);
		tvp->u = vp->p3_u << 6; //* bp->bm_w;
		tvp->v = vp->p3_v << 6; //* bp->bm_h;

		Assert(Lighting_on < 3);

		if (Lighting_on)
			tvp->l = vp->p3_l * NUM_LIGHTING_LEVELS;
	}

	Lighting_enabled = Lighting_on;

	// Now, call my texture mapper.
	if (Lighting_on) 
	{
		switch (Interpolation_method) // 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
		{
		case 0:								// choose best interpolation
			per2_flag = 1;
			if (Current_seg_depth > Max_perspective_depth)
				ntexture_map_lighted_linear(bp, &Tmap1);
			else
				ntexture_map_lighted(bp, &Tmap1);
			break;
		case 1:								// linear interpolation
			per2_flag = 1;
			ntexture_map_lighted_linear(bp, &Tmap1);
			break;
		case 2:								// perspective every 8th pixel interpolation
			per2_flag = 1;
			ntexture_map_lighted(bp, &Tmap1);
			break;
		case 3:								// perspective every pixel interpolation
			per2_flag = 0;					// this hack means do divide every pixel
			ntexture_map_lighted(bp, &Tmap1);
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
	}
	else 
	{
		switch (Interpolation_method) // 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
		{	
		case 0:								// choose best interpolation
			per2_flag = 1;
			if (Current_seg_depth > Max_perspective_depth)
				ntexture_map_lighted_linear(bp, &Tmap1);
			else
				ntexture_map_lighted(bp, &Tmap1);
			break;
		case 1:								// linear interpolation
			per2_flag = 1;
			ntexture_map_lighted_linear(bp, &Tmap1);
			break;
		case 2:								// perspective every 8th pixel interpolation
			per2_flag = 1;
			ntexture_map_lighted(bp, &Tmap1);
			break;
		case 3:								// perspective every pixel interpolation
			per2_flag = 0;					// this hack means do divide every pixel
			ntexture_map_lighted(bp, &Tmap1);
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
	}

	Lighting_on = lighting_on_save;
}

