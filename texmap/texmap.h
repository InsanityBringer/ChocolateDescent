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

#include "fix/fix.h"
#include "3d/3d.h"
#include "2d/gr.h"

#define	NUM_LIGHTING_LEVELS 32
#define MAX_TMAP_VERTS (25 + 14)
#define MAX_LIGHTING_VALUE	((NUM_LIGHTING_LEVELS-1)*F1_0/NUM_LIGHTING_LEVELS)
#define MIN_LIGHTING_VALUE	(F1_0/NUM_LIGHTING_LEVELS)

//[ISB] Uncomment for a drug induced nightmare. 
//#define TEXMAP_ANTIALIAS
//[ISB] Uncomment for dithering of lighting. Slow since it's perspective only ATM. 
//#define TEXMAP_DITHER

#ifdef BUILD_DESCENT2
//variables for clipping the texture-mapper to screen region
extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
#endif

 // -------------------------------------------------------------------------------------------------------
//extern fix compute_lighting_value(g3s_point * vertptr); //[ISB] not used?

// -------------------------------------------------------------------------------------------------------
// This is the main texture mapper call.
//	tmap_num references a texture map defined in Texmap_ptrs.
//	nverts = number of vertices
//	vertbuf is a pointer to an array of vertex pointers
extern void draw_tmap(grs_bitmap* bp, int nverts, g3s_point** vertbuf);

// -------------------------------------------------------------------------------------------------------
// Texture map vertex.
//	The fields r,g,b and l are mutually exclusive.  r,g,b are used for rgb lighting.
//	l is used for intensity based lighting.
typedef struct g3ds_vertex {
	fix	x, y, z;
	fix	u, v;
	fix	x2d, y2d;
	fix	l;
	fix	r, g, b;
} g3ds_vertex;

// A texture map is defined as a polygon with u,v coordinates associated with
// one point in the polygon, and a pair of vectors describing the orientation
// of the texture map in the world, from which the deltas Du_dx, Dv_dy, etc.
// are computed.
typedef struct g3ds_tmap {
	int	nv;			// number of vertices
	g3ds_vertex	verts[MAX_TMAP_VERTS];	// up to 8 vertices, this is inefficient, change
} g3ds_tmap; //[ISB] Added 14 to MAX_TMAP_VERTS, because after Tmap1 were two unused arrays that contained storage for 14 vertices. 

// -------------------------------------------------------------------------------------------------------

//	Note:	Not all interpolation method and lighting combinations are supported.
//	Set Interpolation_method to 0/1/2 for linear/linear, perspective/linear, perspective/perspective
extern	int	Interpolation_method;

// Set Lighting_on to 0/1/2 for no lighting/intensity lighting/rgb lighting
extern	int	Lighting_on;

// HACK INTERFACE: how far away the current segment (& thus texture) is
extern	int	Current_seg_depth;
extern	int	Max_perspective_depth;		//	Deepest segment at which perspective interpolation will be used.
extern	int	Max_linear_depth;				//	Deepest segment at which linear interpolation will be used.
extern	int	Max_flat_depth;				//	Deepest segment at which flat shading will be used. (If not flat shading, then what?)

//	These are pointers to texture maps.  If you want to render texture map #7, then you will render
//	the texture map defined by Texmap_ptrs[7].
extern	grs_bitmap Texmap_ptrs[];
extern	grs_bitmap Texmap4_ptrs[];

// Interface for sky renderer //[ISB] it remains a dream
//extern void texture_map_lin_lin_sky(grs_bitmap* srcb, g3ds_tmap* t);
//extern void texture_map_lin_lin_sky_v(grs_bitmap* srcb, g3ds_tmap* t);
//extern void texture_map_hyp_lin_v(grs_bitmap* srcb, g3ds_tmap* t);

extern void ntexture_map_lighted_linear(grs_bitmap* srcb, g3ds_tmap* t);

//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(int nverts, int* vert);

//This is like gr_upoly_tmap() but instead of drawing, it calls the specified
//function with ylr values
void gr_upoly_tmap_ylr(int nverts, int* vert, void(*ylr_func)(int, fix, fix));

extern int Transparency_on, per2_flag;

//	Set to !0 to enable Sim City 2000 (or Eric's Drive Through, or Eric's Game) specific code.
extern	int	SC2000;
