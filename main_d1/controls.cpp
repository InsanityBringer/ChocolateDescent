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
/*
 * $Source: f:/miner/source/main/rcs/controls.c $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:27:11 $
 *
 * Code for controlling player movement
 *
 */

#include <stdlib.h>

#include "bios/mono.h"
#include "bios/key.h"
#include "bios/joy.h"
#include "bios/timer.h"
#include "misc/error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "render.h"
#include "args.h"
#include "2d/palette.h"
#include "bios/mouse.h"
#include "kconfig.h"

//look at keyboard, mouse, joystick, CyberMan, whatever, and set 
//physics vars rotvel, velocity

void read_flying_controls(object* obj)
{
	fix	afterburner_thrust;

	Assert(FrameTime > 0); 		//Get MATT if hit this!

	if (Player_is_dead) 
	{
		vm_vec_zero(&obj->mtype.phys_info.rotthrust);
		vm_vec_zero(&obj->mtype.phys_info.thrust);
		return;
	}

	if ((obj->type != OBJ_PLAYER) || (obj->id != Player_num)) return;	//references to player_ship require that this obj be the player

	//	Couldn't the "50" in the next three lines be changed to "64" with no ill effect?
	obj->mtype.phys_info.rotthrust.x = Controls.pitch_time;
	obj->mtype.phys_info.rotthrust.y = Controls.heading_time;
	obj->mtype.phys_info.rotthrust.z = Controls.bank_time;

	//	mprintf( (0, "Rot thrust = %.3f,%.3f,%.3f\n", f2fl(obj->mtype.phys_info.rotthrust.x),f2fl(obj->mtype.phys_info.rotthrust.y), f2fl(obj->mtype.phys_info.rotthrust.z) ));

	afterburner_thrust = 0;
	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		afterburner_thrust = FrameTime;

	// Set object's thrust vector for forward/backward
	vm_vec_copy_scale(&obj->mtype.phys_info.thrust, &obj->orient.fvec, Controls.forward_thrust_time + afterburner_thrust);

	// slide left/right
	vm_vec_scale_add2(&obj->mtype.phys_info.thrust, &obj->orient.rvec, Controls.sideways_thrust_time);

	// slide up/down
	vm_vec_scale_add2(&obj->mtype.phys_info.thrust, &obj->orient.uvec, Controls.vertical_thrust_time);

	if (obj->mtype.phys_info.flags & PF_WIGGLE) 
	{
		fix swiggle;
		fix_fastsincos(GameTime, &swiggle, NULL);
		vm_vec_scale_add2(&obj->mtype.phys_info.velocity, &obj->orient.uvec, fixmul(swiggle, Player_ship->wiggle));
	}

	// As of now, obj->mtype.phys_info.thrust & obj->mtype.phys_info.rotthrust are 
	// in units of time... In other words, if thrust==FrameTime, that
	// means that the user was holding down the Max_thrust key for the
	// whole frame.  So we just scale them up by the max, and divide by
	// FrameTime to make them independant of framerate

	//	Prevent divide overflows on high frame rates.
	//	In a signed divide, you get an overflow if num >= div<<15
	{
		fix	ft = FrameTime;

		//	Note, you must check for ft < F1_0/2, else you can get an overflow  on the << 15.
		if ((ft < F1_0 / 2) && (ft << 15 <= Player_ship->max_thrust)) 
		{
			mprintf((0, "Preventing divide overflow in controls.c for Max_thrust!\n"));
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale(&obj->mtype.phys_info.thrust, fixdiv(Player_ship->max_thrust, ft));

		if ((ft < F1_0 / 2) && (ft << 15 <= Player_ship->max_rotthrust)) 
		{
			mprintf((0, "Preventing divide overflow in controls.c for max_rotthrust!\n"));
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale(&obj->mtype.phys_info.rotthrust, fixdiv(Player_ship->max_rotthrust, ft));
	}
}
