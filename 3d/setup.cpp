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

#include <stdlib.h>

#include "misc/error.h"

#include "3d/3d.h"
#include "globvars.h"
#include "clipper.h"
//#include "div0.h"

//initialize the 3d system
void g3_init(void)
{
	//	div0_init(DM_ERROR);
	atexit(g3_close);
}

//close down the 3d system
void g3_close(void) {} //[ISB] sure the devs had a really cool thing planned here...

extern void init_interface_vars_to_assembler(void);

//start the frame
void g3_start_frame(void)
{
	fix s;

	//set int w,h & fixed-point w,h/2
	Canv_w2 = (Canvas_width = grd_curcanv->cv_bitmap.bm_w) << 15;
	Canv_h2 = (Canvas_height = grd_curcanv->cv_bitmap.bm_h) << 15;

	//compute aspect ratio for this canvas

	s = fixmuldiv(grd_curscreen->sc_aspect, Canvas_height, Canvas_width);

	//if (s <= 0) //scale x
	if (s <= F1_0)
	{
		Window_scale.x = s;
		Window_scale.y = f1_0;
	}
	else 
	{
		Window_scale.y = fixdiv(f1_0, s);
		Window_scale.x = f1_0;
	}

	Window_scale.z = f1_0;		//always 1

	init_free_points();

	init_interface_vars_to_assembler();		//for the texture-mapper

}

//this doesn't do anything, but is here for completeness
void g3_end_frame(void)
{
	//	Assert(free_point_num==0);
	free_point_num = 0;
}