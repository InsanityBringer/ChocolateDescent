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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3d/3d.h"
#include "misc/error.h"
#include "2d/gr.h"
#include "texmap/texmap.h"
#include "iff/iff.h"
#include "mem/mem.h"
#include "platform/mono.h"

#include "inferno.h"
#include "textures.h"
#include "object.h"
#include "endlevel.h"
#include "fireball.h"
#include "terrain.h"

#define GRID_MAX_SIZE	64
#define GRID_SCALE	i2f(2*20)
#define HEIGHT_SCALE	f1_0

int grid_w, grid_h;

g3s_uvl uvl_list1[] = { {0,0,0}, {f1_0,0,0},  {0,f1_0,0} };
g3s_uvl uvl_list2[] = { {f1_0,0,0}, {f1_0,f1_0,0},  {0,f1_0,0} };

uint8_t* height_array;
uint8_t* light_array;

#define HEIGHT(_i,_j) (height_array[(_i)*grid_w+(_j)])
#define LIGHT(_i,_j) light_array[(_i)*grid_w+(_j)]

//!!#define HEIGHT(_i,_j)	height_array[(grid_h-1-j)*grid_w+(_i)]
//!!#define LIGHT(_i,_j)		light_array[(grid_h-1-j)*grid_w+(_i)]

#define LIGHTVAL(_i,_j) (((fix) LIGHT(_i,_j))<<8)

g3s_point save_row[GRID_MAX_SIZE];

vms_vector start_point;

grs_bitmap* terrain_bm;

//extern fix g3_get_surface_dotprod(g3s_point** list); //[ISB] unused
//#pragma aux g3_get_surface_dotprod "*" parm [esi] value [eax] modify exact [eax];

int terrain_outline = 0;

void render_mine(int start_seg_num, fix eye_offset);

int org_i, org_j;

int mine_tiles_drawn;		//flags to tell if all 4 tiles under mine have drawn

void draw_cell(int i, int j, g3s_point* p0, g3s_point* p1, g3s_point* p2, g3s_point* p3)
{
	g3s_point* pointlist[3];

	pointlist[0] = p0;
	pointlist[1] = p1;
	pointlist[2] = p3;
	uvl_list1[0].l = LIGHTVAL(i, j);
	uvl_list1[1].l = LIGHTVAL(i, j + 1);
	uvl_list1[2].l = LIGHTVAL(i + 1, j);

	uvl_list1[0].u = (i)* f1_0 / 4; uvl_list1[0].v = (j)* f1_0 / 4;
	uvl_list1[1].u = (i)* f1_0 / 4; uvl_list1[1].v = (j + 1) * f1_0 / 4;
	uvl_list1[2].u = (i + 1) * f1_0 / 4;   uvl_list1[2].v = (j)* f1_0 / 4;

	g3_check_and_draw_tmap(3, pointlist, uvl_list1, terrain_bm, NULL, NULL);
	if (terrain_outline) 
	{
		int lsave = Lighting_on;
		Lighting_on = 0;
		gr_setcolor(BM_XRGB(31, 0, 0));
		g3_draw_line(pointlist[0], pointlist[1]);
		g3_draw_line(pointlist[2], pointlist[0]);
		Lighting_on = lsave;
	}

	pointlist[0] = p1;
	pointlist[1] = p2;
	uvl_list2[0].l = LIGHTVAL(i, j + 1);
	uvl_list2[1].l = LIGHTVAL(i + 1, j + 1);
	uvl_list2[2].l = LIGHTVAL(i + 1, j);

	uvl_list2[0].u = (i)* f1_0 / 4; uvl_list2[0].v = (j + 1) * f1_0 / 4;
	uvl_list2[1].u = (i + 1) * f1_0 / 4;   uvl_list2[1].v = (j + 1) * f1_0 / 4;
	uvl_list2[2].u = (i + 1) * f1_0 / 4;   uvl_list2[2].v = (j)* f1_0 / 4;

	g3_check_and_draw_tmap(3, pointlist, uvl_list2, terrain_bm, NULL, NULL);
	if (terrain_outline) 
	{
		int lsave = Lighting_on;
		Lighting_on = 0;
		gr_setcolor(BM_XRGB(31, 0, 0));
		g3_draw_line(pointlist[0], pointlist[1]);
		g3_draw_line(pointlist[1], pointlist[2]);
		g3_draw_line(pointlist[2], pointlist[0]);
		Lighting_on = lsave;
	}

	if (i == org_i && j == org_j)
		mine_tiles_drawn |= 1;
	if (i == org_i - 1 && j == org_j)
		mine_tiles_drawn |= 2;
	if (i == org_i && j == org_j - 1)
		mine_tiles_drawn |= 4;
	if (i == org_i - 1 && j == org_j - 1)
		mine_tiles_drawn |= 8;

	if (mine_tiles_drawn == 0xf)
	{
		render_mine(exit_segnum, 0);
		//draw_exit_model();
		mine_tiles_drawn = -1;
		//if (ext_expl_playing)
		//	draw_fireball(&external_explosion);
	}

}

vms_vector y_cache[256];
uint8_t yc_flags[256];

extern vms_matrix surface_orient;

vms_vector* get_dy_vec(int h)
{
	vms_vector* dyp;

	dyp = &y_cache[h];

	if (!yc_flags[h]) {
		vms_vector tv;

		//@@g3_rotate_delta_y(dyp,h*HEIGHT_SCALE);

		vm_vec_copy_scale(&tv, &surface_orient.uvec, h * HEIGHT_SCALE);
		g3_rotate_delta_vec(dyp, &tv);

		yc_flags[h] = 1;
	}

	return dyp;

}

int im = 1;

void render_terrain(vms_vector* org_point, int org_2dx, int org_2dy)
{
	vms_vector delta_i, delta_j;		//delta_y;
	g3s_point p, last_p, save_p_low, save_p_high;
	g3s_point last_p2;
	int i, j;
	int low_i, high_i, low_j, high_j;
	int viewer_i, viewer_j;
	vms_vector tv;

	mine_tiles_drawn = 0;	//clear flags

	org_i = org_2dy;
	org_j = org_2dx;

	low_i = 0;  high_i = grid_w - 1;
	low_j = 0;  high_j = grid_h - 1;

	//@@start_point.x = org_point->x - GRID_SCALE*(org_i - low_i);
	//@@start_point.z = org_point->z - GRID_SCALE*(org_j - low_j);
	//@@start_point.y = org_point->y;

	memset(yc_flags, 0, 256);

	//Lighting_on = 0;
	Interpolation_method = im;

	vm_vec_copy_scale(&tv, &surface_orient.rvec, GRID_SCALE);
	g3_rotate_delta_vec(&delta_i, &tv);
	vm_vec_copy_scale(&tv, &surface_orient.fvec, GRID_SCALE);
	g3_rotate_delta_vec(&delta_j, &tv);

	vm_vec_scale_add(&start_point, org_point, &surface_orient.rvec, -(org_i - low_i) * GRID_SCALE);
	vm_vec_scale_add2(&start_point, &surface_orient.fvec, -(org_j - low_j) * GRID_SCALE);

	vm_vec_sub(&tv, &Viewer->pos, &start_point);
	viewer_i = vm_vec_dot(&tv, &surface_orient.rvec) / GRID_SCALE;
	viewer_j = vm_vec_dot(&tv, &surface_orient.fvec) / GRID_SCALE;

	//mprintf((0,"viewer_i,j = %d,%d\n",viewer_i,viewer_j));

	if (viewer_i >= high_i) viewer_i = high_i-1;
	if (viewer_j >= high_j) viewer_j = high_j-1;

	g3_rotate_point(&last_p, &start_point);
	save_p_low = last_p;

	for (j = low_j; j <= high_j; j++) 
	{
		g3_add_delta_vec(&save_row[j], &last_p, get_dy_vec(HEIGHT(low_i, j)));
		if (j == high_j)
			save_p_high = last_p;
		else
			g3_add_delta_vec(&last_p, &last_p, &delta_j);
	}

	for (i = low_i; i < viewer_i; i++) 
	{
		g3_add_delta_vec(&save_p_low, &save_p_low, &delta_i);
		last_p = save_p_low;
		g3_add_delta_vec(&last_p2, &last_p, get_dy_vec(HEIGHT(i + 1, low_j)));

		for (j = low_j; j < viewer_j; j++) 
		{
			g3s_point p2;

			g3_add_delta_vec(&p, &last_p, &delta_j);
			g3_add_delta_vec(&p2, &p, get_dy_vec(HEIGHT(i + 1, j + 1)));

			draw_cell(i, j, &save_row[j], &save_row[j + 1], &p2, &last_p2);

			last_p = p;
			save_row[j] = last_p2;
			last_p2 = p2;
		}

		vm_vec_negate(&delta_j);			//don't have a delta sub...

		g3_add_delta_vec(&save_p_high, &save_p_high, &delta_i);
		last_p = save_p_high;
		g3_add_delta_vec(&last_p2, &last_p, get_dy_vec(HEIGHT(i + 1, high_j)));

		for (j = high_j - 1; j >= viewer_j; j--) 
		{
			g3s_point p2;

			g3_add_delta_vec(&p, &last_p, &delta_j);
			g3_add_delta_vec(&p2, &p, get_dy_vec(HEIGHT(i + 1, j)));

			draw_cell(i, j, &save_row[j], &save_row[j + 1], &last_p2, &p2);

			last_p = p;
			if (j >= 0)
				save_row[j + 1] = last_p2;
			last_p2 = p2;

		}

		if (j >= 0)
			save_row[j + 1] = last_p2;

		vm_vec_negate(&delta_j);		//restore sign of j
	}

	//now do i from other end

	vm_vec_negate(&delta_i);		//going the other way now...

	//@@start_point.x += (high_i-low_i)*GRID_SCALE;
	vm_vec_scale_add2(&start_point, &surface_orient.rvec, (high_i - low_i) * GRID_SCALE);
	g3_rotate_point(&last_p, &start_point);
	save_p_low = last_p;

	for (j = low_j; j <= high_j; j++) 
	{
		g3_add_delta_vec(&save_row[j], &last_p, get_dy_vec(HEIGHT(high_i, j)));
		if (j == high_j)
			save_p_high = last_p;
		else
			g3_add_delta_vec(&last_p, &last_p, &delta_j);
	}

	for (i = high_i - 1; i >= viewer_i; i--) 
	{
		g3_add_delta_vec(&save_p_low, &save_p_low, &delta_i);
		last_p = save_p_low;
		g3_add_delta_vec(&last_p2, &last_p, get_dy_vec(HEIGHT(i, low_j)));

		for (j = low_j; j < viewer_j; j++) 
		{
			g3s_point p2;

			g3_add_delta_vec(&p, &last_p, &delta_j);
			g3_add_delta_vec(&p2, &p, get_dy_vec(HEIGHT(i, j + 1)));

			draw_cell(i, j, &last_p2, &p2, &save_row[j + 1], &save_row[j]);

			last_p = p;
			save_row[j] = last_p2;
			last_p2 = p2;
		}

		vm_vec_negate(&delta_j);			//don't have a delta sub...

		g3_add_delta_vec(&save_p_high, &save_p_high, &delta_i);
		last_p = save_p_high;
		g3_add_delta_vec(&last_p2, &last_p, get_dy_vec(HEIGHT(i, high_j)));

		for (j = high_j - 1; j >= viewer_j; j--)
		{
			g3s_point p2;

			g3_add_delta_vec(&p, &last_p, &delta_j);
			g3_add_delta_vec(&p2, &p, get_dy_vec(HEIGHT(i, j)));

			draw_cell(i, j, &p2, &last_p2, &save_row[j + 1], &save_row[j]);

			last_p = p;
			if (j >= 0)
				save_row[j + 1] = last_p2;
			last_p2 = p2;

		}

		if (j >= 0)
			save_row[j + 1] = last_p2;

		vm_vec_negate(&delta_j);		//restore sign of j
	}
}

void free_height_array(void)
{
	free(height_array);
}

void load_terrain(char* filename)
{
	grs_bitmap height_bitmap;
	int iff_error;
	int i, j;
	uint8_t h, min_h, max_h;

	iff_error = iff_read_bitmap(filename, &height_bitmap, BM_LINEAR, NULL);
	if (iff_error != IFF_NO_ERROR) {
		mprintf((1, "File %s - IFF error: %s", filename, iff_errormsg(iff_error)));
		Error("File %s - IFF error: %s", filename, iff_errormsg(iff_error));
	}

	if (height_array)
		free(height_array);
	else
		atexit(free_height_array);		//first time

	grid_w = height_bitmap.bm_w;
	grid_h = height_bitmap.bm_h;

	Assert(grid_w <= GRID_MAX_SIZE);
	Assert(grid_h <= GRID_MAX_SIZE);

	height_array = height_bitmap.bm_data;

	max_h = 0; min_h = 255;
	for (i = 0; i < grid_w; i++)
		for (j = 0; j < grid_h; j++) {

			h = HEIGHT(i, j);

			if (h > max_h)
				max_h = h;

			if (h < min_h)
				min_h = h;
		}

	for (i = 0; i < grid_w; i++)
		for (j = 0; j < grid_h; j++)
			HEIGHT(i, j) -= min_h;


	//	free(height_bitmap.bm_data);

	terrain_bm = terrain_bitmap;

	build_light_table();
}


void get_pnt(vms_vector* p, int i, int j)
{
	p->x = GRID_SCALE * i;
	p->z = GRID_SCALE * j;
	p->y = HEIGHT(i, j) * HEIGHT_SCALE;
}

vms_vector light = { 0x2e14,0xe8f5,0x5eb8 };

fix get_face_light(vms_vector* p0, vms_vector* p1, vms_vector* p2)
{
	vms_vector norm;

	vm_vec_normal(&norm, p0, p1, p2);

	return -vm_vec_dot(&norm, &light);

}


fix get_avg_light(int i, int j)
{
	vms_vector pp, p[6];
	fix sum;
	int f;

	get_pnt(&pp, i, j);
	get_pnt(&p[0], i - 1, j);
	get_pnt(&p[1], i, j - 1);
	get_pnt(&p[2], i + 1, j - 1);
	get_pnt(&p[3], i + 1, j);
	get_pnt(&p[4], i, j + 1);
	get_pnt(&p[5], i - 1, j + 1);

	for (f = 0, sum = 0; f < 6; f++)
		sum += get_face_light(&pp, &p[f], &p[(f + 1) % 5]);

	return sum / 6;
}

void free_light_table(void)
{
	if (light_array)
		free(light_array);
}

void build_light_table()
{
	int i, j;
	fix l, l2, min_l = 0x7fffffff, max_l = 0;

	if (light_array)
		free(light_array);
	else
		atexit(free_light_table);		//first time

	//MALLOC(light_array,uint8_t,grid_w*grid_h); //Won't comile -KRB
	light_array = (uint8_t*)malloc(grid_w * grid_h + (sizeof(uint8_t))); //my hack -KRB
	for (i = 1; i < grid_w; i++)
		for (j = 1; j < grid_h; j++) {
			l = get_avg_light(i, j);

			if (l > max_l)
				max_l = l;

			if (l < min_l)
				min_l = l;

			//printf("light %2d,%2d = %8x\n",i,j,l);
		}

	for (i = 1; i < grid_w; i++)
		for (j = 1; j < grid_h; j++) 
		{

			l = get_avg_light(i, j);

			if (min_l == max_l) {
				LIGHT(i, j) = (uint8_t)(l >> 8);
				continue;
			}

			l2 = fixdiv((l - min_l), (max_l - min_l));

			if (l2 == f1_0)
				l2--;

			LIGHT(i, j) = (uint8_t)(l2 >> 8);

			//printf("light %2d,%2d = %4x\n",i,j,l2>>8);
		}
}
