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
#include <math.h>
#include <string.h>
#include "misc/rand.h"
#include "cfile/cfile.h"
#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "gauges.h"
#include "vclip.h"
#include "fireball.h"
#include "robot.h"
#include "wall.h"
#include "sounds.h"
#include "morph.h"
#include "3d/3d.h"
#include "bm.h"
#include "polyobj.h"
#include "ai.h"
#include "gamemine.h"
#include "gamesave.h"
#include "gameseq.h"
#include "player.h"
#include "collide.h"
#include "laser.h"
#include "network.h"
#include "multi.h"
#include "multibot.h"

// The max number of fuel stations per mine.

fix Fuelcen_refill_speed = i2f(1);
fix Fuelcen_give_amount = i2f(25);
fix Fuelcen_max_amount = i2f(100);

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
fix EnergyToCreateOneRobot = i2f(1);

int Fuelcen_control_center_destroyed = 0;
int Fuelcen_seconds_left = 0;

#define MATCEN_HP_DEFAULT			F1_0*500; // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	F1_0*5;	//  5 seconds

matcen_info RobotCenters[MAX_ROBOT_CENTERS];
int Num_robot_centers;

control_center_triggers ControlCenterTriggers;

FuelCenter Station[MAX_NUM_FUELCENS];
int Num_fuelcenters = 0;

segment* PlayerSegment = NULL;

#ifdef EDITOR
char	Special_names[MAX_CENTER_TYPES][11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void fuelcen_reset()
{
	int i;

	Num_fuelcenters = 0;
	//mprintf( (0, "All fuel centers reset.\n"));

	for (i = 0; i < MAX_SEGMENTS; i++)
		Segments[i].special = SEGMENT_IS_NOTHING;

	Fuelcen_control_center_destroyed = 0;
	Num_robot_centers = 0;
}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
void reset_all_robot_centers()
{
	int i;

	// Remove all materialization centers
	for (i = 0; i < Num_segments; i++)
		if (Segments[i].special == SEGMENT_IS_ROBOTMAKER) 
		{
			Segments[i].special = SEGMENT_IS_NOTHING;
			Segments[i].matcen_num = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a segment into a fully charged up fuel center...
void fuelcen_create(segment* segp)
{
	int	station_type;

	station_type = segp->special;

	switch (station_type) 
	{
	case SEGMENT_IS_NOTHING:
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
		break;
	default:
		Error("Invalid station type %d in fuelcen.c\n", station_type);
	}

	Assert((segp != NULL));
	if (segp == NULL) return;

	Assert(Num_fuelcenters < MAX_NUM_FUELCENS);
	Assert(Num_fuelcenters > -1);

	segp->value = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].MaxCapacity = Fuelcen_max_amount;
	Station[Num_fuelcenters].Capacity = Station[Num_fuelcenters].MaxCapacity;
	Station[Num_fuelcenters].segnum = segp - Segments;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;
	//	Station[Num_fuelcenters].NextRobotType = -1;
	//	Station[Num_fuelcenters].last_created_obj=NULL;
	//	Station[Num_fuelcenters].last_created_sig = -1;
	compute_segment_center(&Station[Num_fuelcenters].Center, segp);

	//	if (station_type == SEGMENT_IS_ROBOTMAKER)
	//		Station[Num_fuelcenters].Capacity = i2f(Difficulty_level + 3);

		//mprintf( (0, "Segment %d is assigned to be fuel center %d.\n", Station[Num_fuelcenters].segnum, Num_fuelcenters ));
	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a matcen that already is a special type into the Station array.
// This function is separate from other fuelcens because we don't want values reset.
void matcen_create(segment* segp)
{
	int	station_type = segp->special;

	Assert((segp != NULL));
	Assert(station_type == SEGMENT_IS_ROBOTMAKER);
	if (segp == NULL) return;

	Assert(Num_fuelcenters < MAX_NUM_FUELCENS);
	Assert(Num_fuelcenters > -1);

	segp->value = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].Capacity = i2f(Difficulty_level + 3);
	Station[Num_fuelcenters].MaxCapacity = Station[Num_fuelcenters].Capacity;

	Station[Num_fuelcenters].segnum = segp - Segments;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;
	//	Station[Num_fuelcenters].NextRobotType = -1;
	//	Station[Num_fuelcenters].last_created_obj=NULL;
	//	Station[Num_fuelcenters].last_created_sig = -1;
	compute_segment_center(&Station[Num_fuelcenters].Center, segp);

	segp->matcen_num = Num_robot_centers;
	Num_robot_centers++;

	RobotCenters[segp->matcen_num].hit_points = MATCEN_HP_DEFAULT;
	RobotCenters[segp->matcen_num].interval = MATCEN_INTERVAL_DEFAULT;
	RobotCenters[segp->matcen_num].segnum = segp - Segments;
	RobotCenters[segp->matcen_num].fuelcen_num = Num_fuelcenters;

	//mprintf( (0, "Segment %d is assigned to be fuel center %d.\n", Station[Num_fuelcenters].segnum, Num_fuelcenters ));
	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a segment that already is a special type into the Station array.
void fuelcen_activate(segment* segp, int station_type)
{
	segp->special = station_type;

	if (segp->special == SEGMENT_IS_ROBOTMAKER)
		matcen_create(segp);
	else
		fuelcen_create(segp);

}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f(30-2*Difficulty_level))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in segment segnum
void trigger_matcen(int segnum)
{
	segment* segp = &Segments[segnum];
	vms_vector	pos, delta;
	FuelCenter* robotcen;
	int			objnum;

	mprintf((0, "Trigger matcen, segment %i\n", segnum));

	Assert(segp->special == SEGMENT_IS_ROBOTMAKER);
	Assert(segp->matcen_num < Num_fuelcenters);
	Assert((segp->matcen_num >= 0) && (segp->matcen_num <= Highest_segment_index));

	robotcen = &Station[RobotCenters[segp->matcen_num].fuelcen_num];

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

	robotcen->Lives--;
	robotcen->Timer = F1_0 * 1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f(Difficulty_level + 3);
	robotcen->Disable_time = MATCEN_LIFE;

	//	Create a bright object in the segment.
	pos = robotcen->Center;
	vm_vec_sub(&delta, &Vertices[Segments[segnum].verts[0]], &robotcen->Center);
	vm_vec_scale_add2(&pos, &delta, F1_0 / 2);
	objnum = obj_create(OBJ_LIGHT, 0, segnum, &pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE);
	if (objnum != -1) 
	{
		Objects[objnum].lifeleft = MATCEN_LIFE;
		Objects[objnum].ctype.light_info.intensity = i2f(8);	//	Light cast by a fuelcen.
	}
	else 
	{
		mprintf((1, "Can't create invisible flare for matcen.\n"));
		Int3();
	}
	//	mprintf((0, "Created invisibile flare, object=%i, segment=%i, pos=%7.3f %7.3f%7.3f\n", objnum, segnum, f2fl(pos.x), f2fl(pos.y), f2fl(pos.z)));
}

#ifdef EDITOR
//------------------------------------------------------------
// Takes away a segment's fuel center properties.
//	Deletes the segment point entry in the FuelCenter list.
void fuelcen_delete(segment* segp)
{
	int i, j;

Restart:;

	for (i = 0; i < Num_fuelcenters; i++) {
		if (Station[i].segnum == segp - Segments) {

			// If Robot maker is deleted, fix Segments and RobotCenters.
			if (Station[i].Type == SEGMENT_IS_ROBOTMAKER) {
				Num_robot_centers--;

				for (j = segp->matcen_num; j < Num_robot_centers; j++)
					RobotCenters[j] = RobotCenters[j + 1];

				for (j = 0; j < Num_fuelcenters; j++) {
					if (Station[j].Type == SEGMENT_IS_ROBOTMAKER)
						if (Segments[Station[j].segnum].matcen_num > segp->matcen_num)
							Segments[Station[j].segnum].matcen_num--;
				}
			}

			Num_fuelcenters--;
			for (j = i; j < Num_fuelcenters; j++) {
				Station[i] = Station[i + 1];
				Segments[Station[i].segnum].value = i;
			}
			segp->special = 0;
			goto Restart;
		}
	}

}
#endif

#define	ROBOT_GEN_TIME (i2f(5))

object* create_morph_robot(segment* segp, vms_vector* object_pos, int object_id)
{
	short		objnum;
	object* obj;
	int		default_behavior;

	Players[Player_num].num_robots_level++;
	Players[Player_num].num_robots_total++;

	objnum = obj_create(OBJ_ROBOT, object_id, segp - Segments, object_pos,
		&vmd_identity_matrix, Polygon_models[Robot_info[object_id].model_num].rad,
		CT_AI, MT_PHYSICS, RT_POLYOBJ);

	if (objnum < 0) 
	{
		mprintf((1, "Can't create morph robot.  Aborting morph.\n"));
		Int3();
		return NULL;
	}

	obj = &Objects[objnum];

	//Set polygon-object-specific data 

	obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
	obj->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
	obj->mtype.phys_info.drag = Robot_info[obj->id].drag;

	obj->mtype.phys_info.flags |= (PF_LEVELLING);

	obj->shields = Robot_info[obj->id].strength;

	default_behavior = AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = AIB_RUN_FROM;

	init_ai_object(obj - Objects, default_behavior, -1);		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	create_n_segment_path(obj, 6, -1);		//	Create a 6 segment path from creation point.

	if (default_behavior == AIB_RUN_FROM)
		Ai_local_info[objnum].mode = AIM_RUN_FROM_OBJECT;

	return obj;
}

int Num_extry_robots = 15;

#ifndef NDEBUG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------
void robotmaker_proc(FuelCenter* robotcen)
{
	fix		dist_to_player;
	vms_vector	cur_object_loc; //, direction;
	int		matcen_num, segnum, objnum;
	object* obj;
	fix		top_time;
	vms_vector	direction;

	if (robotcen->Enabled == 0)
		return;

	if (robotcen->Disable_time > 0) 
	{
		robotcen->Disable_time -= FrameTime;
		if (robotcen->Disable_time <= 0) 
		{
			mprintf((0, "Robot center #%i gets disabled due to time running out.\n", robotcen - Station));
			robotcen->Enabled = 0;
		}
	}

	// mprintf((0, "Capacity of robot maker #%i is %i\n", robotcen - Station, robotcen->Capacity));

	//	No robot making in multiplayer mode.
#ifdef NETWORK
#ifndef SHAREWARE
	if ((Game_mode & GM_MULTI) && (!(Game_mode & GM_MULTI_ROBOTS) || !network_i_am_master()))
		return;
#else
	if (Game_mode & GM_MULTI)
		return;
#endif
#endif

	// Wait until transmorgafier has capacity to make a robot...
	if (robotcen->Capacity <= 0) 
	{
		return;
	}

	matcen_num = Segments[robotcen->segnum].matcen_num;
	//mprintf((0, "Robotmaker #%i flags = %8x\n", matcen_num, RobotCenters[matcen_num].robot_flags));

	if (matcen_num == -1) 
	{
		mprintf((0, "Non-functional robotcen at %d\n", robotcen->segnum));
		return;
	}

	if (RobotCenters[matcen_num].robot_flags == 0) 
	{
		//mprintf((0, "robot_flags = 0 at robot maker #%i\n", RobotCenters[matcen_num].robot_flags));
		return;
	}

	// Wait until we have a free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	if ((Players[Player_num].num_robots_level - Players[Player_num].num_kills_level) >= (Gamesave_num_org_robots + Num_extry_robots)) 
	{
#ifndef NDEBUG
		if (FrameCount > FrameCount_last_msg + 20) 
		{
			mprintf((0, "Cannot morph until you kill one!\n"));
			FrameCount_last_msg = FrameCount;
		}
#endif
		return;
	}

	robotcen->Timer += FrameTime;

	switch (robotcen->Flag) 
	{
	case 0:		// Wait until next robot can generate
		if (Game_mode & GM_MULTI)
		{
			top_time = ROBOT_GEN_TIME;
		}
		else
		{
			dist_to_player = vm_vec_dist_quick(&ConsoleObject->pos, &robotcen->Center);
			top_time = dist_to_player / 64 + P_Rand() * 2 + F1_0 * 2;
			if (top_time > ROBOT_GEN_TIME)
				top_time = ROBOT_GEN_TIME + P_Rand();
			if (top_time < F1_0 * 2)
				top_time = F1_0 * 3 / 2 + P_Rand() * 2;
		}

		// mprintf( (0, "Time between morphs %d seconds, dist_to_player = %7.3f\n", f2i(top_time), f2fl(dist_to_player) ));

		if (robotcen->Timer > top_time) 
		{
			int	count = 0;
			int	i, my_station_num = robotcen - Station;
			object* obj;

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			for (i = 0; i <= Highest_object_index; i++)
				if (Objects[i].type == OBJ_ROBOT)
					//[ISB] This weird cast is needed due to oddness with type promotion in Watcom C. Types don't promote to the larger type of the literal as specified in the C/C++ specs,
					//messing this calculation up. Thanks Arne for pointing this out. 
					if ((int8_t)(Objects[i].matcen_creator ^ 0x80) == my_station_num) 
						count++;
			if (count > Difficulty_level + 3) 
			{
				mprintf((0, "Cannot morph: center %i has already put out %i robots.\n", my_station_num, count));
				robotcen->Timer /= 2;
				return;
			}

			//	Whack on any robot or player in the matcen segment.
			count = 0;
			segnum = robotcen->segnum;
			for (objnum = Segments[segnum].objects; objnum != -1; objnum = Objects[objnum].next) 
			{
				count++;
				if (count > MAX_OBJECTS)
				{
					mprintf((0, "Object list in segment %d is circular.", segnum));
					Int3();
					return;
				}
				if (Objects[objnum].type == OBJ_ROBOT)
				{
					collide_robot_and_materialization_center(&Objects[objnum]);
					robotcen->Timer = top_time / 2;
					return;
				}
				else if (Objects[objnum].type == OBJ_PLAYER)
				{
					collide_player_and_materialization_center(&Objects[objnum]);
					robotcen->Timer = top_time / 2;
					return;
				}
			}

			compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);
			// HACK!!! The 10 under here should be something equal to the 1/2 the size of the segment.
			obj = object_create_explosion(robotcen->segnum, &cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT);

			if (obj)
				extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);

			if (Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1) 
			{
				digi_link_sound_to_pos(Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen->segnum, 0, &cur_object_loc, 0, F1_0);
			}
			robotcen->Flag = 1;
			robotcen->Timer = 0;

		}
		break;
	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > (Vclip[VCLIP_MORPHING_ROBOT].play_time / 2)) 
		{

			robotcen->Capacity -= EnergyToCreateOneRobot;
			robotcen->Flag = 0;

			robotcen->Timer = 0;
			compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);

			// If this is the first materialization, set to valid robot.
			if (RobotCenters[matcen_num].robot_flags != 0)
			{
				int	type;
				uint32_t	flags;
				int8_t	legal_types[32];		//	32 bits in a word, the width of robot_flags.
				int	num_types, robot_index;

				robot_index = 0;
				num_types = 0;
				flags = RobotCenters[matcen_num].robot_flags;
				while (flags)
				{
					if (flags & 1)
						legal_types[num_types++] = robot_index;
					flags >>= 1;
					robot_index++;
				}

				//mprintf((0, "Flags = %08x, %2i legal types to morph: \n", RobotCenters[matcen_num].robot_flags, num_types));
				//for (i=0; i<num_types; i++)
				//	mprintf((0, "%2i ", legal_types[i]));
				//mprintf((0, "\n"));

				if (num_types == 1)
					type = legal_types[0];
				else
					type = legal_types[(P_Rand() * num_types) / 32768];

				mprintf((0, "Morph: (type = %i) (seg = %i) (capacity = %08x)\n", type, robotcen->segnum, robotcen->Capacity));
				obj = create_morph_robot(&Segments[robotcen->segnum], &cur_object_loc, type);
				if (obj != NULL)
				{
#ifndef SHAREWARE
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_create_robot(robotcen - Station, obj - Objects, type);
#endif
#endif
					obj->matcen_creator = robotcen - Station | 0x80;

					// Make object faces player...
					vm_vec_sub(&direction, &ConsoleObject->pos, &obj->pos);
					vm_vector_2_matrix(&obj->orient, &direction, &obj->orient.uvec, NULL);

					morph_start(obj);
					//robotcen->last_created_obj = obj;
					//robotcen->last_created_sig = robotcen->last_created_obj->signature;
				}
				else
					mprintf((0, "Warning: create_morph_robot returned NULL (no objects left?)\n"));

			}

		}
		break;
	default:
		robotcen->Flag = 0;
		robotcen->Timer = 0;
	}
}

#define	BASE_CONTROL_CENTER_EXPLOSION_TIME	30
#define	DIFF_CONTROL_CENTER_EXPLOSION_TIME	(BASE_CONTROL_CENTER_EXPLOSION_TIME + (NDL-Difficulty_level-1)*5)

#define COUNTDOWN_VOICE_TIME (i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME)-fl2f(12.75))

void controlcen_proc(FuelCenter * controlcen)
{
	fix old_time;
	int	fc;

	//	mprintf( (0, "CCT: %.1f\n", f2fl(controlcen->Timer)));

	if (!Fuelcen_control_center_destroyed)	return;

	//	Control center destroyed, rock the player's ship.
	fc = Fuelcen_seconds_left;
	if (fc > 16)
		fc = 16;
	ConsoleObject->mtype.phys_info.rotvel.x += fixmul(P_Rand() - 16384, 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32);
	ConsoleObject->mtype.phys_info.rotvel.z += fixmul(P_Rand() - 16384, 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32);
	//	Hook in the rumble sound effect here.

	old_time = controlcen->Timer;
	controlcen->Timer += RealFrameTime;			//timer_get_approx_seconds
	Fuelcen_seconds_left = DIFF_CONTROL_CENTER_EXPLOSION_TIME - f2i(controlcen->Timer);
	if ((old_time < COUNTDOWN_VOICE_TIME) && (controlcen->Timer >= COUNTDOWN_VOICE_TIME)) 
	{
		digi_play_sample(SOUND_COUNTDOWN_13_SECS, F3_0);
	}
	if (f2i(old_time) != f2i(controlcen->Timer)) 
	{
		if ((Fuelcen_seconds_left >= 0) && (Fuelcen_seconds_left < 10))
			digi_play_sample(SOUND_COUNTDOWN_0_SECS + Fuelcen_seconds_left, F3_0);
		if (Fuelcen_seconds_left == DIFF_CONTROL_CENTER_EXPLOSION_TIME - 1)
			digi_play_sample(SOUND_COUNTDOWN_29_SECS, F3_0);
	}

	if (controlcen->Timer < i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME))
	{
		vms_vector vp;	//,v,c;
		fix size;
		compute_segment_center(&vp, &Segments[controlcen->segnum]);
		size = (0x50000 * f2i(controlcen->Timer) * (FrameTime & 0xF)) / 16;
		size = controlcen->Timer / (fl2f(0.65));
		old_time = old_time / (fl2f(0.65));
		if (size != old_time && (controlcen->Timer > (5 * F1_0))) {			// Every 2 seconds!
			//@@object_create_explosion( controlcen->segnum, &vp, size*10, FrameTime & 7);
			object_create_explosion(controlcen->segnum, &vp, size * 10, VCLIP_SMALL_EXPLOSION);
			digi_play_sample(SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
		}
	}
	else 
	{
		int flash_value;

		if (old_time < i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME))
			digi_play_sample(SOUND_MINE_BLEW_UP, F1_0);

		flash_value = f2i((controlcen->Timer - i2f(DIFF_CONTROL_CENTER_EXPLOSION_TIME)) * (64 / 4));	// 4 seconds to total whiteness
		PALETTE_FLASH_SET(flash_value, flash_value, flash_value);

		//gauge_message( "YOU'RE TOO SLOW! THE MINE BLEW UP!" );
		if (PaletteBlueAdd > 64)
		{
			gr_set_current_canvas(NULL);
			gr_clear_canvas(BM_XRGB(31, 31, 31));		//make screen all white to match palette effect
			reset_cockpit();								//force cockpit redraw next time
			reset_palette_add();							//restore palette for death message
			controlcen->Timer = -1;
			controlcen->MaxCapacity = Fuelcen_max_amount;
			//gauge_message( "Control Center Reset" );
			DoPlayerDead();		//kill_player();
		}
	}
}

#define M_PI 3.14159

//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void fuelcen_update_all()
{
	int i;
	fix AmountToreplenish;

	AmountToreplenish = fixmul(FrameTime, Fuelcen_refill_speed);

	for (i = 0; i < Num_fuelcenters; i++)
	{
		if (Station[i].Type == SEGMENT_IS_ROBOTMAKER) 
		{
			if (!(Game_suspended & SUSP_ROBOTS))
				robotmaker_proc(&Station[i]);
		}
		else if (Station[i].Type == SEGMENT_IS_CONTROLCEN) 
		{
			controlcen_proc(&Station[i]);

		}
		else if ((Station[i].MaxCapacity > 0) && (PlayerSegment != &Segments[Station[i].segnum])) 
		{
			if (Station[i].Capacity < Station[i].MaxCapacity) 
			{
				Station[i].Capacity += AmountToreplenish;
				//mprintf( (0, "Fuel center %d replenished to %d.\n", i, f2i(Station[i].Capacity) ));
				if (Station[i].Capacity >= Station[i].MaxCapacity) 
				{
					Station[i].Capacity = Station[i].MaxCapacity;
					//gauge_message( "Fuel center is fully recharged!    " );
				}
			}
		}
	}
}

//--unused-- //-------------------------------------------------------------
//--unused-- // replenishes all fuel supplies.
//--unused-- void fuelcen_replenish_all()
//--unused-- {
//--unused-- 	int i;
//--unused-- 
//--unused-- 	for (i=0; i<Num_fuelcenters; i++ )	{
//--unused-- 		Station[i].Capacity = Station[i].MaxCapacity;
//--unused-- 	}
//--unused-- 	//mprintf( (0, "All fuel centers are replenished\n" ));
//--unused-- 
//--unused-- }

//-------------------------------------------------------------
fix fuelcen_give_fuel(segment* segp, fix MaxAmountCanTake)
{
	Assert(segp != NULL);

	PlayerSegment = segp;

	if ((segp) && (segp->special == SEGMENT_IS_FUELCEN)) 
	{
		fix amount;

		//		if (Station[segp->value].MaxCapacity<=0)	{
		//			HUD_init_message( "Fuelcenter %d is destroyed.", segp->value );
		//			return 0;
		//		}

		//		if (Station[segp->value].Capacity<=0)	{
		//			HUD_init_message( "Fuelcenter %d is empty.", segp->value );
		//			return 0;
		//		}

		if (MaxAmountCanTake <= 0) {
			//			//gauge_message( "Fueled up!");
			return 0;
		}

		amount = fixmul(FrameTime, Fuelcen_give_amount);

		if (amount > MaxAmountCanTake)
			amount = MaxAmountCanTake;

		//		if (!(Game_mode & GM_MULTI))
		//			if ( Station[segp->value].Capacity < amount  )	{
		//				amount = Station[segp->value].Capacity;
		//				Station[segp->value].Capacity = 0;
		//			} else {
		//				Station[segp->value].Capacity -= amount;
		//			}

		digi_play_sample(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0 / 2);

#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_play_sound(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0 / 2);
#endif

		//HUD_init_message( "Fuelcen %d has %d/%d fuel", segp->value,f2i(Station[segp->value].Capacity),f2i(Station[segp->value].MaxCapacity) );
		return amount;

	}
	else 
	{
		return 0;
	}
}

//--unused-- //-----------------------------------------------------------
//--unused-- // Damages a fuel center
//--unused-- void fuelcen_damage(segment *segp, fix damage )
//--unused-- {
//--unused-- 	//int i;
//--unused-- 	// int	station_num = segp->value;
//--unused-- 
//--unused-- 	Assert( segp != NULL );
//--unused-- 	if ( segp == NULL ) return;
//--unused-- 
//--unused-- 	mprintf((0, "Obsolete function fuelcen_damage() called with seg=%i, damage=%7.3f\n", segp-Segments, f2fl(damage)));
//--unused-- 	switch( segp->special )	{
//--unused-- 	case SEGMENT_IS_NOTHING:
//--unused-- 		return;
//--unused-- 	case SEGMENT_IS_ROBOTMAKER:
//--unused-- //--		// Robotmaker hit by laser
//--unused-- //--		if (Station[station_num].MaxCapacity<=0 )	{
//--unused-- //--			// Shooting a already destroyed materializer
//--unused-- //--		} else {
//--unused-- //--			Station[station_num].MaxCapacity -= damage;
//--unused-- //--			if (Station[station_num].Capacity > Station[station_num].MaxCapacity )	{
//--unused-- //--				Station[station_num].Capacity = Station[station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (Station[station_num].MaxCapacity <= 0 )	{
//--unused-- //--				Station[station_num].MaxCapacity = 0;
//--unused-- //--				// Robotmaker dead
//--unused-- //--				for (i=0; i<6; i++ )
//--unused-- //--					segp->sides[i].tmap_num2 = 0;
//--unused-- //--			}
//--unused-- //--		}
//--unused-- //--		//mprintf( (0, "Materializatormografier has %x capacity left\n", Station[station_num].MaxCapacity ));
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_FUELCEN:	
//--unused-- //--		digi_play_sample( SOUND_REFUEL_STATION_HIT );
//--unused-- //--		if (Station[station_num].MaxCapacity>0 )	{
//--unused-- //--			Station[station_num].MaxCapacity -= damage;
//--unused-- //--			if (Station[station_num].Capacity > Station[station_num].MaxCapacity )	{
//--unused-- //--				Station[station_num].Capacity = Station[station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (Station[station_num].MaxCapacity <= 0 )	{
//--unused-- //--				Station[station_num].MaxCapacity = 0;
//--unused-- //--				digi_play_sample( SOUND_REFUEL_STATION_DESTROYED );
//--unused-- //--			}
//--unused-- //--		} else {
//--unused-- //--			Station[station_num].MaxCapacity = 0;
//--unused-- //--		}
//--unused-- //--		HUD_init_message( "Fuelcenter %d damaged", station_num );
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_REPAIRCEN:
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_CONTROLCEN:
//--unused-- 		break;
//--unused-- 	default:
//--unused-- 		Error( "Invalid type in fuelcen.c" );
//--unused-- 	}
//--unused-- }

#ifdef RESTORE_REPAIRCENTER
//	----------------------------------------------------------------------------------------------------------
fixang my_delta_ang(fixang a,fixang b)
{
	fixang delta0,delta1;

	return (abs(delta0 = a - b) < abs(delta1 = b - a)) ? delta0 : delta1;

}

//	----------------------------------------------------------------------------------------------------------
//return though which side of seg0 is seg1
int john_find_connect_side(int seg0,int seg1)
{
	segment *Seg=&Segments[seg0];
	int i;
 
 	for (i=MAX_SIDES_PER_SEGMENT;i--;) if (Seg->children[i]==seg1) return i;

 	return -1;
 }

//	----------------------------------------------------------------------------------------------------------
vms_angvec start_angles, delta_angles, goal_angles;
vms_vector start_pos, delta_pos, goal_pos;
int FuelStationSeg;
fix current_time,delta_time;
int next_side, side_index;
int * sidelist;
int Repairing;

vms_vector repair_save_uvec;		//the player's upvec when enter repaircen
object *RepairObj=NULL;		//which object getting repaired
int disable_repair_center=0;
fix repair_rate;
#define FULL_REPAIR_RATE i2f(10)

uint8_t save_control_type,save_movement_type;

int SideOrderBack[] = {WFRONT, WRIGHT, WTOP, WLEFT, WBOTTOM, WBACK};
int SideOrderFront[] =  {WBACK, WLEFT, WTOP, WRIGHT, WBOTTOM, WFRONT};
int SideOrderLeft[] =  { WRIGHT, WBACK, WTOP, WFRONT, WBOTTOM, WLEFT };
int SideOrderRight[] =  { WLEFT, WFRONT, WTOP, WBACK, WBOTTOM, WRIGHT };
int SideOrderTop[] =  { WBOTTOM, WLEFT, WBACK, WRIGHT, WFRONT, WTOP };
int SideOrderBottom[] =  { WTOP, WLEFT, WFRONT, WRIGHT, WBACK, WBOTTOM };

int SideUpVector[] = {WBOTTOM, WFRONT, WBOTTOM, WFRONT, WBOTTOM, WBOTTOM };

//	----------------------------------------------------------------------------------------------------------
void refuel_calc_deltas(object *obj, int next_side, int repair_seg)
{
	vms_vector nextcenter, headfvec, *headuvec;
	vms_matrix goal_orient;

	// Find time for this movement
	delta_time = F1_0;		// one second...
		
	// Find start and goal position
	start_pos = obj->pos;
	
	// Find delta position to get to goal position
	compute_segment_center(&goal_pos,&Segments[repair_seg]);
	vm_vec_sub( &delta_pos,&goal_pos,&start_pos);
	
	// Find start angles
	//angles_from_vector(&start_angles,&obj->orient.fvec);
	vm_extract_angles_matrix(&start_angles,&obj->orient);
	
	// Find delta angles to get to goal orientation
	compute_center_point_on_side(&nextcenter,&Segments[repair_seg],next_side);
	vm_vec_sub(&headfvec,&nextcenter,&goal_pos);
	//mprintf( (0, "Next_side = %d, Head fvec = %d,%d,%d\n", next_side, headfvec.x, headfvec.y, headfvec.z ));

	if (next_side == 5)						//last side
		headuvec = &repair_save_uvec;
	else
		headuvec = &Segments[repair_seg].sides[SideUpVector[next_side]].normals[0];

	vm_vector_2_matrix(&goal_orient,&headfvec,headuvec,NULL);
	vm_extract_angles_matrix(&goal_angles,&goal_orient);
	delta_angles.p = my_delta_ang(start_angles.p,goal_angles.p);
	delta_angles.b = my_delta_ang(start_angles.b,goal_angles.b);
	delta_angles.h = my_delta_ang(start_angles.h,goal_angles.h);
	current_time = 0;
	Repairing = 0;
}

//	----------------------------------------------------------------------------------------------------------
//if repairing, cut it short
void abort_repair_center()
{
	if (!RepairObj || side_index==5)
		return;

	current_time = 0;
	side_index = 5;
	next_side = sidelist[side_index];
	refuel_calc_deltas(RepairObj, next_side, FuelStationSeg);
}

//	----------------------------------------------------------------------------------------------------------
void repair_ship_damage()
{
 	//mprintf((0,"Repairing ship damage\n"));
}

//	----------------------------------------------------------------------------------------------------------
int refuel_do_repair_effect( object * obj, int first_time, int repair_seg )	{

	obj->mtype.phys_info.velocity.x = 0;				
	obj->mtype.phys_info.velocity.y = 0;				
	obj->mtype.phys_info.velocity.z = 0;				

	if (first_time)	{
		int entry_side;
		current_time = 0;

		//digi_play_sample( SOUND_REPAIR_STATION_PLAYER_ENTERING, F1_0 );

		entry_side = john_find_connect_side(repair_seg,obj->segnum );
		Assert( entry_side > -1 );

		switch( entry_side )	{
		case WBACK: sidelist = SideOrderBack; break;
		case WFRONT: sidelist = SideOrderFront; break;
		case WLEFT: sidelist = SideOrderLeft; break;
		case WRIGHT: sidelist = SideOrderRight; break;
		case WTOP: sidelist = SideOrderTop; break;
		case WBOTTOM: sidelist = SideOrderBottom; break;
		}
		side_index = 0;
		next_side = sidelist[side_index];

		refuel_calc_deltas(obj,next_side, repair_seg);
	} 

	//update shields
	if (Players[Player_num].shields < MAX_SHIELDS) {	//if above max, don't mess with it

		Players[Player_num].shields += fixmul(FrameTime,repair_rate);

		if (Players[Player_num].shields > MAX_SHIELDS)
			Players[Player_num].shields = MAX_SHIELDS;
	}

	current_time += FrameTime;

	if (current_time >= delta_time )	{
		vms_angvec av;
		obj->pos = goal_pos;
		av	= goal_angles;
		vm_angles_2_matrix(&obj->orient,&av);

		if (side_index >= 5 )	
			return 1;		// Done being repaired...

		if (Repairing==0)		{
			//mprintf( (0, "<MACHINE EFFECT ON SIDE %d>\n", next_side ));
			//digi_play_sample( SOUND_REPAIR_STATION_FIXING );
			Repairing=1;

			/*switch( next_side )	
			{
			case 0:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_1,F1_0 ); break;
			case 1:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_2,F1_0 ); break;
			case 2:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_3,F1_0 ); break;
			case 3:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_4,F1_0 ); break;
			case 4:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_1,F1_0 ); break;
			case 5:	digi_play_sample( SOUND_REPAIR_STATION_FIXING_2,F1_0 ); break;
			}*/
		
			repair_ship_damage();

		}

		if (current_time >= (delta_time+(F1_0/2)) )	{
			current_time = 0;
			// Find next side...
			side_index++;
			if (side_index >= 6 ) return 1;
			next_side = sidelist[side_index];
	
			refuel_calc_deltas(obj, next_side, repair_seg);
		}

	} else {
		fix factor, p,b,h;	
		vms_angvec av;

		factor = fixdiv( current_time,delta_time );

		// Find object's current position
		obj->pos = delta_pos;
		vm_vec_scale( &obj->pos, factor );
		vm_vec_add2( &obj->pos, &start_pos );
			
		// Find object's current orientation
		p	= fixmul(delta_angles.p,factor);
		b	= fixmul(delta_angles.b,factor);
		h	= fixmul(delta_angles.h,factor);
		av.p = (fixang)p + start_angles.p;
		av.b = (fixang)b + start_angles.b;
		av.h = (fixang)h + start_angles.h;
		vm_angles_2_matrix(&obj->orient,&av);

	}

	update_object_seg(obj);		//update segment

	return 0;
}

//	----------------------------------------------------------------------------------------------------------
//do the repair center for this frame
void do_repair_sequence(object *obj)
{
	Assert(obj == RepairObj);

	if (refuel_do_repair_effect( obj, 0, FuelStationSeg )) {
		if (Players[Player_num].shields < MAX_SHIELDS)
			Players[Player_num].shields = MAX_SHIELDS;
		obj->control_type = save_control_type;
		obj->movement_type = save_movement_type;
		disable_repair_center=1;
		RepairObj = NULL;


		//the two lines below will spit the player out of the rapair center,
		//but what happen is that the ship just bangs into the door
		//if (obj->movement_type == MT_PHYSICS)
		//	vm_vec_copy_scale(&obj->mtype.phys_info.velocity,&obj->orient.fvec,i2f(200));
	}

}

//	----------------------------------------------------------------------------------------------------------
//see if we should start the repair center
void check_start_repair_center(object *obj)
{
	if (RepairObj != NULL) return;		//already in repair center

	if (Lsegments[obj->segnum].special_type & SS_REPAIR_CENTER) {

		if (!disable_repair_center) {
			//have just entered repair center

			RepairObj = obj;
			repair_save_uvec = obj->orient.uvec;

			repair_rate = fixmuldiv(FULL_REPAIR_RATE,(MAX_SHIELDS - Players[Player_num].shields),MAX_SHIELDS);

			save_control_type = obj->control_type;
			save_movement_type = obj->movement_type;

			obj->control_type = CT_REPAIRCEN;
			obj->movement_type = MT_NONE;

			FuelStationSeg	= Lsegments[obj->segnum].special_segment;
			Assert(FuelStationSeg != -1);

			if (refuel_do_repair_effect( obj, 1, FuelStationSeg )) {
				Int3();		//can this happen?
				obj->control_type = CT_FLYING;
				obj->movement_type = MT_PHYSICS;
			}
		}
	}
	else
		disable_repair_center=0;

}

#endif

//	--------------------------------------------------------------------------------------------
void disable_matcens(void)
{
	int	i;

	for (i = 0; i < Num_robot_centers; i++) 
	{
		Station[i].Enabled = 0;
		Station[i].Disable_time = 0;
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void init_all_matcens(void)
{
	int	i;

	for (i = 0; i < Num_fuelcenters; i++)
		if (Station[i].Type == SEGMENT_IS_ROBOTMAKER)
		{
			Station[i].Lives = 3;
			Station[i].Enabled = 0;
			Station[i].Disable_time = 0;
#ifndef NDEBUG
			{
				//	Make sure this fuelcen is pointed at by a matcen.
				int	j;
				for (j = 0; j < Num_robot_centers; j++) 
				{
					if (RobotCenters[j].fuelcen_num == i)
						break;
				}
				Assert(j != Num_robot_centers);
			}
#endif
		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	for (i = 0; i < Num_robot_centers; i++) 
	{
		int	fuelcen_num = RobotCenters[i].fuelcen_num;

		Assert(fuelcen_num < Num_fuelcenters);
		Assert(Station[fuelcen_num].Type == SEGMENT_IS_ROBOTMAKER);
	}
#endif
}

void read_matcen(matcen_info* center, FILE* fp)
{
	center->robot_flags = file_read_int(fp);
	center->hit_points = file_read_int(fp);
	center->interval = file_read_int(fp);
	center->segnum = file_read_short(fp);
	center->fuelcen_num = file_read_short(fp);
}

void write_matcen(matcen_info* center, FILE* fp)
{
	file_write_int(fp, center->robot_flags);
	file_write_int(fp, center->hit_points);
	file_write_int(fp, center->interval);
	file_write_short(fp, center->segnum);
	file_write_short(fp, center->fuelcen_num);
}

void read_fuelcen(FuelCenter* center, FILE* fp)
{
	center->Type = file_read_int(fp);
	center->segnum = file_read_int(fp);
	center->Flag = file_read_byte(fp);
	center->Enabled = file_read_byte(fp);
	center->Lives = file_read_byte(fp);
	center->dum1 = file_read_byte(fp);
	center->Capacity = file_read_int(fp);
	center->MaxCapacity = file_read_int(fp);
	center->Timer = file_read_int(fp);
	center->Disable_time = file_read_int(fp);
	center->Center.x = file_read_int(fp);
	center->Center.y = file_read_int(fp);
	center->Center.z = file_read_int(fp);
}

void write_fuelcen(FuelCenter* center, FILE* fp)
{
	file_write_int(fp, center->Type);
	file_write_int(fp, center->segnum);
	file_write_byte(fp, center->Flag);
	file_write_byte(fp, center->Enabled);
	file_write_byte(fp, center->Lives);
	file_write_byte(fp, center->dum1);
	file_write_int(fp, center->Capacity);
	file_write_int(fp, center->MaxCapacity);
	file_write_int(fp, center->Timer);
	file_write_int(fp, center->Disable_time);
	file_write_int(fp, center->Center.x);
	file_write_int(fp, center->Center.y);
	file_write_int(fp, center->Center.z);
}

void read_reactor_triggers(control_center_triggers* trigger, FILE* fp)
{
	int i;

	trigger->num_links = file_read_short(fp);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		trigger->seg[i] = file_read_short(fp);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		trigger->side[i] = file_read_short(fp);
}

void write_reactor_triggers(control_center_triggers* trigger, FILE* fp)
{
	int i;

	file_write_short(fp, trigger->num_links);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		file_write_short(fp, trigger->seg[i]);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		file_write_short(fp, trigger->side[i]);
}
