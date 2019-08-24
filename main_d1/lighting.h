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
#include "vecmat/vecmat.h"
#include "object.h"

#define MAX_DIST		0x400000		//no light beyond this dist
#define MAX_DIST_LOG	5				// log(MAX_DIST-expressed-as-integer)
#define MAX_LIGHT		0x10000		//max value

#define	NEAREST_LIGHT_DIST	(F1_0*60)
#define	MIN_LIGHT_DIST			(F1_0*4)

#define BEAM_CUTOFF	0xa000		//what is out of beam?

extern fix	Beam_brightness;
extern fix	Dynamic_light[MAX_VERTICES];

extern void set_dynamic_light(void);

//Compute the lighting from the headlight for a given vertex on a face.
//Takes:
//  point - the 3d coords of the point
//  face_light - a scale factor derived from the surface normal of the face
//If no surface normal effect is wanted, pass F1_0 for face_light
fix compute_headlight_light(vms_vector* point, fix face_light);

//compute the average dynamic light in a segment.  Takes the segment number
fix compute_seg_dynamic_light(int segnum);

//compute the lighting for an object.  Takes a pointer to the object,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
fix compute_object_light(object* obj, vms_vector* rotated_pnt);
