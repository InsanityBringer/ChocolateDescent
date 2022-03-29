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

#pragma once

#include "object.h"

//interpreter gets 1000 vecs but morph gets 200... thanks guys?
#define MAX_VECS 1000

typedef struct morph_data 
{
	object *obj;			//object which is morphing
	vms_vector morph_vecs[MAX_VECS];
	vms_vector morph_deltas[MAX_VECS];
	fix morph_times[MAX_VECS];
	int submodel_active[MAX_SUBMODELS];		//which submodels are active
	int n_morphing_points[MAX_SUBMODELS];	//how many active points in each part
	int submodel_startpoints[MAX_SUBMODELS];	//first point for each submodel
	int n_submodels_active;
	uint8_t morph_save_control_type;
	uint8_t morph_save_movement_type;
	physics_info morph_save_phys_info;
	int Morph_sig;
 } morph_data;

#define MAX_MORPH_OBJECTS 5

extern morph_data morph_objects[];

void morph_start(object *obj);
void draw_morph_object(object *obj);

//process the morphing object for one frame
void do_morph_frame(object *obj);

//called at the start of a level
void init_morphs();

extern morph_data *find_morph_data(object *obj);
