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

#include "vecmat/vecmat.h"
#include "2d/gr.h"
#include "3d/3d.h"

#ifndef DRIVE
#include "robot.h"
#endif
#include "main_shared/piggy.h"

#ifndef DRIVE
#define MAX_POLYGON_MODELS 85
#else
#define MAX_POLYGON_MODELS 300
#define MAX_SUBMODELS 10
#endif

 //used to describe a polygon model
typedef struct polymodel 
{
	int n_models;
	int model_data_size;
	uint8_t* model_data;
	int submodel_ptrs[MAX_SUBMODELS];
	vms_vector submodel_offsets[MAX_SUBMODELS];
	vms_vector submodel_norms[MAX_SUBMODELS];		//norm for sep plane
	vms_vector submodel_pnts[MAX_SUBMODELS];		//point on sep plane 
	fix submodel_rads[MAX_SUBMODELS];				//radius for each submodel
	uint8_t submodel_parents[MAX_SUBMODELS];		//what is parent for each submodel
	vms_vector submodel_mins[MAX_SUBMODELS];
	vms_vector submodel_maxs[MAX_SUBMODELS];
	vms_vector mins, maxs;							//min,max for whole model
	fix rad;
	uint8_t		n_textures;
	uint16_t	first_texture;
	uint8_t		simpler_model;		//alternate model with less detail (0 if none, model_num+1 else)
//	vms_vector min,max;
} polymodel;

//array of pointers to polygon objects
extern polymodel Polygon_models[];

//switch to simpler model when the object has depth 
//greater than this value times its radius.   
extern int Simple_model_threshhold_scale;

//how many polygon objects there are
extern int N_polygon_models;


//array of names of currently-loaded models
extern char Pof_names[MAX_POLYGON_MODELS][13];

void init_polygon_models();

#ifndef DRIVE
int load_polygon_model(char* filename, int n_textures, int first_texture, robot_info* r);
#else
int load_polygon_model(char* filename, int n_textures, grs_bitmap*** textures);
#endif

//draw a polygon model
void draw_polygon_model(vms_vector* pos, vms_matrix* orient, vms_angvec* anim_angles, int model_num, int flags, fix light, fix* glow_values, bitmap_index alt_textures[]);

//fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(char* filename, vms_vector* gun_points, vms_vector* gun_dirs, int* gun_submodels);

//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.
void draw_model_picture(int mn, vms_angvec* orient_angles);

#define MAX_POLYOBJ_TEXTURES 50
extern grs_bitmap * texture_list[MAX_POLYOBJ_TEXTURES];
extern bitmap_index texture_list_index[MAX_POLYOBJ_TEXTURES];
extern g3s_point robot_points[];
