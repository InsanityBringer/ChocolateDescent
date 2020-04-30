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

#include "fix/fix.h"
#include "vecmat/vecmat.h"
#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "bm.h"
#include "3d/3d.h"
#include "game.h"

// Global array of vertices, common to one mine.
vms_vector Vertices[MAX_VERTICES];
g3s_point Segment_points[MAX_VERTICES];

fix FrameTime = 0;		// Time since last frame, in seconds
fix GameTime = 0;		//	Time in game, in seconds

//How many frames we've rendered
int FrameCount = 0;

//	This is the global mine which create_new_mine returns.
segment	Segments[MAX_SEGMENTS];
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, Vertices, pointed to by Vp)
int		Num_vertices = 0;
int		Num_segments = 0;

int		Highest_vertex_index = 0;
int		Highest_segment_index = 0;

//	Translate table to get opposite side of a face on a segment.
char	Side_opposite[MAX_SIDES_PER_SEGMENT] = { WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK };

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

#ifdef PASSWORD
#define encrypt(a,b,c,d)	a ^ TOLOWER((((int) PASSWORD)>>24)&255), \
									b ^ TOLOWER((((int) PASSWORD)>>16)&255), \
									c ^ TOLOWER((((int) PASSWORD)>>8)&255), \
									d ^ TOLOWER((((int) PASSWORD))&255)
#else
#define encrypt(a,b,c,d) a,b,c,d
#endif

int8_t Side_to_verts[MAX_SIDES_PER_SEGMENT][4] = {
			encrypt(7,6,2,3),			// left
			encrypt(0,4,7,3),			// top
			encrypt(0,1,5,4),			// right
			encrypt(2,6,5,1),			// bottom
			encrypt(4,5,6,7),			// back
			encrypt(3,2,1,0),			// front
};

//	Note, this MUST be the same as Side_to_verts, it is an int for speed reasons.
int Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4] = {
			encrypt(7,6,2,3),			// left
			encrypt(0,4,7,3),			// top
			encrypt(0,1,5,4),			// right
			encrypt(2,6,5,1),			// bottom
			encrypt(4,5,6,7),			// back
			encrypt(3,2,1,0),			// front
};

// Texture map stuff

int NumTextures = 0;
bitmap_index Textures[MAX_TEXTURES];		// All textures.

fix	Next_laser_fire_time;			//	Time at which player can next fire his selected laser.
fix	Next_missile_fire_time;			//	Time at which player can next fire his selected missile.
//--unused-- fix	Laser_delay_time = F1_0/6;		//	Delay between laser fires.

#define DEFAULT_DIFFICULTY		1

int	Difficulty_level = DEFAULT_DIFFICULTY;	//	Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
int	Detail_level = NUM_DETAIL_LEVELS - 2;		//	Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest

char	Menu_pcx_name[13];

