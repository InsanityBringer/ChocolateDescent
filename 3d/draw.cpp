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

#include "misc/error.h"
#include "3d/3d.h"
#include "globvars.h"
#include "texmap/texmap.h"
#include "clipper.h"
#include "misc/types.h"

void (*tmap_drawer_ptr)(grs_bitmap* bm, int nv, g3s_point** vertlist) = draw_tmap;
void (*flat_drawer_ptr)(int nv, int* vertlist) = gr_upoly_tmap;
int (*line_drawer_ptr)(fix x0, fix y0, fix x1, fix y1) = gr_line;

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(void (*tmap_drawer)(grs_bitmap* bm, int nv, g3s_point** vertlist), void (*flat_drawer)(int nv, int* vertlist), int (*line_drawer)(fix x0, fix y0, fix x1, fix y1))
{
	tmap_drawer_ptr = (tmap_drawer) ? tmap_drawer : draw_tmap;
	flat_drawer_ptr = (flat_drawer) ? flat_drawer : gr_upoly_tmap;
	line_drawer_ptr = (line_drawer) ? line_drawer : gr_line;
}

//deal with a clipped line
dbool must_clip_line(g3s_point* p0, g3s_point* p1, uint8_t codes_or)
{
	dbool ret;

	if ((p0->p3_flags & PF_TEMP_POINT) || (p1->p3_flags & PF_TEMP_POINT))

		ret = 0;		//line has already been clipped, so give up

	else {

		clip_line(&p0, &p1, codes_or);

		ret = g3_draw_line(p0, p1);
	}

	//free temp points

	if (p0->p3_flags & PF_TEMP_POINT)
		free_temp_point(p0);

	if (p1->p3_flags & PF_TEMP_POINT)
		free_temp_point(p1);

	return ret;
}

//draws a line. takes two points.  returns true if drew
dbool g3_draw_line(g3s_point* p0, g3s_point* p1)
{
	uint8_t codes_or;

	if (p0->p3_codes & p1->p3_codes)
		return 0;

	codes_or = p0->p3_codes | p1->p3_codes;

	if (codes_or & CC_BEHIND)
		return must_clip_line(p0, p1, codes_or);

	if (!(p0->p3_flags & PF_PROJECTED))
		g3_project_point(p0);

	if (p0->p3_flags & PF_OVERFLOW)
		return must_clip_line(p0, p1, codes_or);


	if (!(p1->p3_flags & PF_PROJECTED))
		g3_project_point(p1);

	if (p1->p3_flags & PF_OVERFLOW)
		return must_clip_line(p0, p1, codes_or);

	return (dbool)(*line_drawer_ptr)(p0->p3_sx, p0->p3_sy, p1->p3_sx, p1->p3_sy);
}

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
dbool g3_check_normal_facing(vms_vector* v, vms_vector* norm)
{
	vms_vector tempv;

	vm_vec_sub(&tempv, &View_position, v);

	return (vm_vec_dot(&tempv, norm) > 0);
}

dbool do_facing_check(vms_vector* norm, g3s_point** vertlist, vms_vector* p)
{
	if (norm) {		//have normal

		Assert(norm->x || norm->y || norm->z);

		return g3_check_normal_facing(p, norm);
	}
	else {	//normal not specified, so must compute

		vms_vector tempv;

		//get three points (rotated) and compute normal

		vm_vec_perp(&tempv, &vertlist[0]->p3_vec, &vertlist[1]->p3_vec, &vertlist[2]->p3_vec);

		return (vm_vec_dot(&tempv, &vertlist[1]->p3_vec) < 0);
	}
}

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
dbool g3_check_and_draw_poly(int nv, g3s_point** pointlist, vms_vector* norm, vms_vector* pnt)
{
	if (do_facing_check(norm, pointlist, pnt))
		return g3_draw_poly(nv, pointlist);
	else
		return 255;
}

dbool g3_check_and_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm, vms_vector* norm, vms_vector* pnt)
{
	if (do_facing_check(norm, pointlist, pnt))
		return g3_draw_tmap(nv, pointlist, uvl_list, bm);
	else
		return 255;
}

//deal with face that must be clipped
dbool must_clip_flat_face(int nv, g3s_codes cc)
{
	int i;
	dbool ret = 0; //[ISB] initalize
	g3s_point** bufptr;

	bufptr = clip_polygon(Vbuf0, Vbuf1, &nv, &cc);

	if (nv > 0 && !(cc.low &CC_BEHIND) && !cc.high) {

		for (i = 0; i < nv; i++) {
			g3s_point* p = bufptr[i];

			if (!(p->p3_flags & PF_PROJECTED))
				g3_project_point(p);

			if (p->p3_flags & PF_OVERFLOW) {
				ret = 1;
				goto free_points;
			}

			Vertex_list[i * 2] = p->p3_sx;
			Vertex_list[i * 2 + 1] = p->p3_sy;
		}

		(*flat_drawer_ptr)(nv, (int*)Vertex_list);
	}
	else
		ret = 1;

	//free temp points
free_points:
	;

	for (i = 0; i < nv; i++)
		if (Vbuf1[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point(Vbuf1[i]);

	//	Assert(free_point_num==0);

	return ret;
}

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_poly(int nv, g3s_point** pointlist)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;

	for (i = 0; i < nv; i++) {

		bufptr[i] = pointlist[i];

		cc.high &= bufptr[i]->p3_codes;
		cc.low |= bufptr[i]->p3_codes;
	}

	if (cc.high)
		return 1;	//all points off screen

	if (cc.low )
		return must_clip_flat_face(nv, cc);

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++) {
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			g3_project_point(p);

		if (p->p3_flags & PF_OVERFLOW)
			return must_clip_flat_face(nv, cc);

		Vertex_list[i * 2] = p->p3_sx;
		Vertex_list[i * 2 + 1] = p->p3_sy;
	}

	(*flat_drawer_ptr)(nv, (int*)Vertex_list);

	return 0;	//say it drew
}

dbool must_clip_tmap_face(int nv, g3s_codes cc, grs_bitmap* bm);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;

	for (i = 0; i < nv; i++) {
		g3s_point* p;

		p = bufptr[i] = pointlist[i];

		cc.high &= p->p3_codes;
		cc.low |= p->p3_codes;

		p->p3_u = uvl_list[i].u;
		p->p3_v = uvl_list[i].v;
		p->p3_l = uvl_list[i].l;

		p->p3_flags |= PF_UVS + PF_LS;

	}

	if (cc.high)
		return 1;	//all points off screen

	if (cc.low )
		return must_clip_tmap_face(nv, cc, bm);

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++) 
	{
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			g3_project_point(p);

		if (p->p3_flags & PF_OVERFLOW) 
		{
			Int3();		//should not overflow after clip
			return 255;
		}
	}

	(*tmap_drawer_ptr)(bm, nv, bufptr);

	return 0;	//say it drew
}

dbool must_clip_tmap_face(int nv, g3s_codes cc, grs_bitmap* bm)
{
	g3s_point** bufptr;
	int i;

	bufptr = clip_polygon(Vbuf0, Vbuf1, &nv, &cc);

	if (nv && !(cc.low &CC_BEHIND) && !cc.high) {

		for (i = 0; i < nv; i++) {
			g3s_point* p = bufptr[i];

			if (!(p->p3_flags & PF_PROJECTED))
				g3_project_point(p);

			if (p->p3_flags & PF_OVERFLOW) {
				Int3();		//should not overflow after clip
				goto free_points;
			}
		}

		(*tmap_drawer_ptr)(bm, nv, bufptr);
	}

free_points:
	;

	for (i = 0; i < nv; i++)
		if (bufptr[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point(bufptr[i]);

	//	Assert(free_point_num==0);

	return 0;

}

int checkmuldiv(fix* r, fix a, fix b, fix c);

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int g3_draw_sphere(g3s_point* pnt, fix rad)
{
	if (!(pnt->p3_codes & CC_BEHIND)) 
	{

		if (!(pnt->p3_flags & PF_PROJECTED))
			g3_project_point(pnt);

		if (!(pnt->p3_codes & PF_OVERFLOW)) 
		{
			fix r2, t;

			r2 = fixmul(rad, Matrix_scale.x);
			if (checkmuldiv(&t, r2, Canv_w2, pnt->p3_z))
				return gr_disk(pnt->p3_sx, pnt->p3_sy, t);
		}
	}

	return 0;
}
