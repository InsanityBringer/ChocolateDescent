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

#ifdef EDITOR

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>

#include "2d/gr.h"
#include "ui/ui.h"
#include "platform/key.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "mem/mem.h"

#include "main_d1/inferno.h"
#include "editor.h"
#include "main_d1/segment.h"
#include "main_d1/object.h"

typedef struct sort_element {
	short segnum;
	fix dist;
} sort_element;

//compare the distance of two segments.  slow, since it computes the
//distance each time
int segdist_cmp(const void *a,const void *b)
{
	sort_element* s0 = (sort_element*)a;
	sort_element* s1 = (sort_element*)b;
	return (s0->dist==s1->dist)?0:((s0->dist<s1->dist)?-1:1);

}


//find the distance between a segment and a point
fix compute_dist(segment *seg,vms_vector *pos)
{
	vms_vector delta;

	compute_segment_center(&delta,seg);
	vm_vec_sub2(&delta,pos);

	return vm_vec_mag(&delta);

}

//sort a list of segments, in order of closeness to pos
void sort_seg_list(int n_segs,short *segnumlist,vms_vector *pos)
{
	int i;
	sort_element *sortlist;

	sortlist = (sort_element*)calloc(n_segs,sizeof(*sortlist));

	for (i=0;i<n_segs;i++) {
		sortlist[i].segnum = segnumlist[i];
		sortlist[i].dist = compute_dist(&Segments[segnumlist[i]],pos);
	}

	qsort(sortlist,n_segs,sizeof(*sortlist),segdist_cmp);

	for (i=0;i<n_segs;i++)
		segnumlist[i] = sortlist[i].segnum;

	free(sortlist);
}

int SortSelectedList(void)
{
	sort_seg_list(N_selected_segs,Selected_segs,&ConsoleObject->pos);
	editor_status("%i element selected list sorted.",N_selected_segs);

	return 1;
}

int SelectNextFoundSeg(void)
{
	if (++Found_seg_index >= N_found_segs)
		Found_seg_index = 0;

	Cursegp = &Segments[Found_segs[Found_seg_index]];
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	editor_status("Curseg assigned to next found segment.");

	return 1;
}

int SelectPreviousFoundSeg(void)
{
	if (Found_seg_index > 0)
		Found_seg_index--;
	else
		Found_seg_index = N_found_segs-1;

	Cursegp = &Segments[Found_segs[Found_seg_index]];
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	editor_status("Curseg assigned to previous found segment.");

	return 1;
}

#endif
