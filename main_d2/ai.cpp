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

#include <stdio.h>
#include <stdlib.h>

#include "inferno.h"
#include "game.h"
#include "platform/mono.h"
#include "3d/3d.h"

#include "object.h"
#include "render.h"
#include "misc/error.h"
#include "ai.h"
#include "aistruct.h"
#include "laser.h"
#include "fvi.h"
#include "polyobj.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "digi.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "platform/timer.h"
#include "sounds.h"
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "network.h"
#include "gameseq.h"
#include "platform/key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "fuelcen.h"
#include "controls.h"
#include "kconfig.h"
#include "misc/rand.h"

#ifdef EDITOR
#include "editor\editor.h"
#endif

#include "string.h"

#ifndef NDEBUG
#include <time.h>
#endif

//	---------- John: These variables must be saved as part of gamesave. ----------
int				Ai_initialized = 0;
int				Overall_agitation;
ai_local			Ai_local_info[MAX_OBJECTS];
point_seg		Point_segs[MAX_POINT_SEGS];
point_seg* Point_segs_free_ptr = Point_segs;
ai_cloak_info	Ai_cloak_info[MAX_AI_CLOAK_INFO];
fix				Boss_cloak_start_time = 0;
fix				Boss_cloak_end_time = 0;
fix				Last_teleport_time = 0;
fix				Boss_teleport_interval = F1_0 * 8;
fix				Boss_cloak_interval = F1_0 * 10;					//	Time between cloaks
fix				Boss_cloak_duration = BOSS_CLOAK_DURATION;
fix				Last_gate_time = 0;
fix				Gate_interval = F1_0 * 6;
fix				Boss_dying_start_time;
fix				Boss_hit_time;
int8_t				Boss_dying, Boss_dying_sound_playing, unused123, unused234;

// -- MK, 10/21/95, unused! -- int				Boss_been_hit=0;


//	---------- John: End of variables which must be saved as part of gamesave. ----------


// -- uint8_t	Boss_cloaks[NUM_D2_BOSSES] = 					{1,1,1,1,1,1};		// Set int8_t if this boss can cloak

uint8_t	Boss_teleports[NUM_D2_BOSSES] = { 1,1,1,1,1,1, 1,1 };		// Set int8_t if this boss can teleport
uint8_t	Boss_spew_more[NUM_D2_BOSSES] = { 0,1,0,0,0,0, 0,0 };		//	If set, 50% of time, spew two bots.
uint8_t	Boss_spews_bots_energy[NUM_D2_BOSSES] = { 1,1,0,1,0,1, 1,1 };		//	Set int8_t if boss spews bots when hit by energy weapon.
uint8_t	Boss_spews_bots_matter[NUM_D2_BOSSES] = { 0,0,1,1,1,1, 0,1 };		//	Set int8_t if boss spews bots when hit by matter weapon.
uint8_t	Boss_invulnerable_energy[NUM_D2_BOSSES] = { 0,0,1,1,0,0, 0,0 };		//	Set int8_t if boss is invulnerable to energy weapons.
uint8_t	Boss_invulnerable_matter[NUM_D2_BOSSES] = { 0,0,0,0,1,1, 1,0 };		//	Set int8_t if boss is invulnerable to matter weapons.
uint8_t	Boss_invulnerable_spot[NUM_D2_BOSSES] = { 0,0,0,0,0,1, 0,1 };		//	Set int8_t if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

int				ai_evaded = 0;

// -- int8_t	Super_boss_gate_list[MAX_GATE_INDEX] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};

int	Robot_firing_enabled = 1;
int	Animation_enabled = 1;

#ifndef NDEBUG
int	Ai_info_enabled = 0;
#endif


//	These globals are set by a call to find_vector_intersection, which is a slow routine,
//	so we don't want to call it again (for this object) unless we have to.
vms_vector	Hit_pos;
int			Hit_type, Hit_seg;
fvi_info		Hit_data;

int					Num_awareness_events = 0;
awareness_event	Awareness_events[MAX_AWARENESS_EVENTS];

vms_vector		Believed_player_pos;
int	Believed_player_seg;

#ifndef NDEBUG
//	Index into this array with ailp->mode
const char* mode_text[18] = {
	{"STILL"},
	{"WANDER"},
	{"FOL_PATH"},
	{"CHASE_OBJ"},
	{"RUN_FROM"},
	{"BEHIND"},
	{"FOL_PATH2"},
	{"OPEN_DOOR"},
	{"GOTO_PLR"},
	{"GOTO_OBJ"},
	{"SN_ATT"},
	{"SN_FIRE"},
	{"SN_RETR"},
	{"SN_RTBK"},
	{"SN_WAIT"},
	{"TH_ATTACK"},
	{"TH_RETREAT"},
	{"TH_WAIT"},

};

//	Index into this array with aip->behavior
const char	behavior_text[6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

//	Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
const char	state_text[8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR_",
};


#endif

// Current state indicates where the robot current is, or has just done.
//	Transition table between states for an AI object.
//	 First dimension is trigger event.
//	Second dimension is current state.
//	 Third dimension is goal state.
//	Result is new goal state.
//	ERR_ means something impossible has happened.
int8_t Ai_transition_table[AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
		//	Event = AIE_FIRE, a nearby object fired
		//	none			rest			srch			lock			flin			fire			reco				// CURRENT is rows, GOAL is columns
		{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	none
		{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	rest
		{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	search
		{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	lock
		{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},		//	flinch
		{	AIS_ERR_,	AIS_FIRE,	AIS_FIRE,	AIS_FIRE,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	fire
		{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}		//	recoil
		},

	//	Event = AIE_HITT, a nearby object was hit (or a wall was hit)
	{
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}
	},

	//	Event = AIE_COLL, player collided with robot
	{
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_LOCK,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}
	},

	//	Event = AIE_HURT, player hurt robot (by firing at and hitting it)
	//	Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN}
	}
};



fix	Dist_to_last_fired_upon_player_pos = 0;

// --------------------------------------------------------------------------------------------------------------------
void init_ai_frame(void)
{
	int	ab_state;

	Dist_to_last_fired_upon_player_pos = vm_vec_dist_quick(&Last_fired_upon_player_pos, &Believed_player_pos);

	ab_state = Afterburner_charge && Controls.afterburner_state && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER);

	if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON) || ab_state) {
		ai_do_cloak_stuff();
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Return firing status.
//	If ready to fire a weapon, return true, else return false.
//	Ready to fire a weapon if next_fire <= 0 or next_fire2 <= 0.
int ready_to_fire(robot_info* robptr, ai_local* ailp)
{
	if (robptr->weapon_type2 != -1)
		return (ailp->next_fire <= 0) || (ailp->next_fire2 <= 0);
	else
		return (ailp->next_fire <= 0);
}

// --------------------------------------------------------------------------------------------------------------------
//	Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70
void make_nearby_robot_snipe(void)
{
	int	bfs_length, i;
	short	bfs_list[MNRS_SEG_MAX];

	create_bfs_list(ConsoleObject->segnum, bfs_list, &bfs_length, MNRS_SEG_MAX);

	for (i = 0; i < bfs_length; i++) {
		int	objnum = Segments[bfs_list[i]].objects;
		while (objnum != -1) {
			object* objp = &Objects[objnum];
			robot_info* robptr = &Robot_info[objp->id];

			if ((objp->type == OBJ_ROBOT) && (objp->id != ROBOT_BRAIN)) {
				if ((objp->ctype.ai_info.behavior != AIB_SNIPE) && (objp->ctype.ai_info.behavior != AIB_RUN_FROM) && !Robot_info[objp->id].boss_flag && !robptr->companion) {
					objp->ctype.ai_info.behavior = AIB_SNIPE;
					Ai_local_info[objnum].mode = AIM_SNIPE_ATTACK;
					mprintf((0, "Making robot #%i go into snipe mode!\n", objnum));
					return;
				}
			}
			objnum = objp->next;
		}
	}

	mprintf((0, "Couldn't find a robot to make snipe!\n"));

}

int	Ai_last_missile_camera;

int	Robots_kill_robots_cheat = 0;

// --------------------------------------------------------------------------------------------------------------------
void do_ai_frame(object* obj)
{
	int			objnum = obj - Objects;
	ai_static* aip = &obj->ctype.ai_info;
	ai_local* ailp = &Ai_local_info[objnum];
	fix			dist_to_player;
	vms_vector	vec_to_player;
	fix			dot;
	robot_info* robptr;
	int			player_visibility = -1;
	int			obj_ref;
	int			object_animates;
	int			new_goal_state;
	int			visibility_and_vec_computed = 0;
	int			previous_visibility;
	vms_vector	gun_point;
	vms_vector	vis_vec_pos;

	ailp->next_action_time -= FrameTime;

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {
			obj->mtype.phys_info.rotthrust.x = (obj->mtype.phys_info.rotthrust.x * 15) / 16;
			obj->mtype.phys_info.rotthrust.y = (obj->mtype.phys_info.rotthrust.y * 15) / 16;
			obj->mtype.phys_info.rotthrust.z = (obj->mtype.phys_info.rotthrust.z * 15) / 16;
			if (!aip->SKIP_AI_COUNT)
				obj->mtype.phys_info.flags &= ~PF_USES_THRUST;
		}
		return;
	}

	robptr = &Robot_info[obj->id];
	Assert(robptr->always_0xabcd == 0xabcd);

	if (do_any_robot_dying_frame(obj))
		return;

	//	Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	//	Else, you can turn a big nasty robot into a wimp by firing flares at it.
	//	This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ready_to_fire(robptr, ailp)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3();	//	This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	mprintf_animation_info((obj));

	if (!Do_ai_flag)
		return;

	if (Break_on_object != -1)
		if ((obj - Objects) == Break_on_object)
			Int3();	//	Contact Mike: This is a debug break
#endif

	// mprintf((0, "Object %i: behavior = %02x, mode = %i, awareness = %i, time = %7.3f\n", obj-Objects, aip->behavior, ailp->mode, ailp->player_awareness_type, f2fl(ailp->player_awareness_time)));
	// mprintf((0, "Object %i: behavior = %02x, mode = %i, awareness = %i, cur=%i, goal=%i\n", obj-Objects, aip->behavior, ailp->mode, ailp->player_awareness_type, aip->CURRENT_STATE, aip->GOAL_STATE));

//	Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR));
	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		// mprintf((0, "Object %i behavior is %i, setting to AIB_NORMAL, fix in editor!\n", objnum, aip->behavior));
		aip->behavior = AIB_NORMAL;
	}

	Assert(obj->segnum != -1);
	Assert(obj->id < N_robot_types);

	obj_ref = objnum ^ FrameCount;

	if (ailp->next_fire > -F1_0 * 8)
		ailp->next_fire -= FrameTime;

	if (robptr->weapon_type2 != -1) {
		if (ailp->next_fire2 > -F1_0 * 8)
			ailp->next_fire2 -= FrameTime;
	}
	else
		ailp->next_fire2 = F1_0 * 8;

	if (ailp->time_since_processed < F1_0 * 256)
		ailp->time_since_processed += FrameTime;

	previous_visibility = ailp->previous_visibility;	//	Must get this before we toast the master copy!

	// -- (No robots have this behavior...)
	// -- //	Deal with cloaking for robots which are cloaked except just before firing.
	// -- if (robptr->cloak_type == RI_CLOAKED_EXCEPT_FIRING)
	// -- 	if (ailp->next_fire < F1_0/2)
	// -- 		aip->CLOAKED = 1;
	// -- 	else
	// -- 		aip->CLOAKED = 0;

	//	If only awake because of a camera, make that the believed player position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (Ai_last_missile_camera != -1))
		Believed_player_pos = Objects[Ai_last_missile_camera].pos;
	else {
		if (Robots_kill_robots_cheat) {
			vis_vec_pos = obj->pos;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility) {
				int	ii, min_obj = -1;
				fix	min_dist = F1_0 * 200, cur_dist;

				for (ii = 0; ii <= Highest_object_index; ii++)
					if ((Objects[ii].type == OBJ_ROBOT) && (ii != objnum)) {
						cur_dist = vm_vec_dist_quick(&obj->pos, &Objects[ii].pos);

						if (cur_dist < F1_0 * 100)
							if (object_to_object_visibility(obj, &Objects[ii], FQ_TRANSWALL))
								if (cur_dist < min_dist) {
									min_obj = ii;
									min_dist = cur_dist;
								}
					}
				if (min_obj != -1) {
					Believed_player_pos = Objects[min_obj].pos;
					Believed_player_seg = Objects[min_obj].segnum;
					vm_vec_normalized_dir_quick(&vec_to_player, &Believed_player_pos, &obj->pos);
				}
				else
					goto _exit_cheat;
			}
			else
				goto _exit_cheat;
		}
		else {
		_exit_cheat:
			visibility_and_vec_computed = 0;
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				Believed_player_pos = ConsoleObject->pos;
			else
				Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO - 1)].last_position;
		}
	}
	dist_to_player = vm_vec_dist_quick(&Believed_player_pos, &obj->pos);
	//	if (robptr->companion)
	//		mprintf((0, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n", objnum, obj->segnum, f2fl(dist_to_player), mode_text[ailp->mode], behavior_text[aip->behavior-0x80], aip->hide_index, aip->path_length));

		//	If this robot can fire, compute visibility from gun position.
		//	Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	if ((previous_visibility || !(obj_ref & 3)) && ready_to_fire(robptr, ailp) && (dist_to_player < F1_0 * 200) && (robptr->n_guns) && !(robptr->attack_type)) {
		//	Since we passed ready_to_fire(), either next_fire or next_fire2 <= 0.  calc_gun_point from relevant one.
		//	If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
		if (ailp->next_fire <= 0)
			calc_gun_point(&gun_point, obj, aip->CURRENT_GUN);
		else
			calc_gun_point(&gun_point, obj, 0);
		vis_vec_pos = gun_point;
	}
	else {
		vis_vec_pos = obj->pos;
		vm_vec_zero(&gun_point);
		// mprintf((0, "Visibility = %i, computed from center.\n", player_visibility));
	}

	// MK: Debugging, July 26, 1995!
	// if (objnum == 1)
	// {
	// compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
	// mprintf((0, "Frame %i: dist=%7.3f, vecdot = %7.3f, mode=%i\n", FrameCount, f2fl(dist_to_player), f2fl(vm_vec_dot(&vec_to_player, &obj->orient.fvec)), ailp->mode));
	// }
		//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
		//	Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(Game_mode & GM_MULTI) && (robptr->companion != 1) && (robptr->thief != 1))
		if (Overall_agitation > 70) {
			if ((dist_to_player < F1_0 * 200) && (P_Rand() < FrameTime / 4)) {
				if (P_Rand() * (Overall_agitation - 40) > F1_0 * 5) {
					// -- mprintf((0, "(1) Object #%i going from still to path in frame %i.\n", objnum, FrameCount));
					create_path_to_player(obj, 4 + Overall_agitation / 8 + Difficulty_level, 1);
					return;
				}
			}
		}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If retry count not 0, then add it into consecutive_retries.
	//	If it is 0, cut down consecutive_retries.
	//	This is largely a hack to speed up physics and deal with stupid AI.  This is low level
	//	communication between systems of a sort that should not be done.
	if ((ailp->retry_count) && !(Game_mode & GM_MULTI)) {
		ailp->consecutive_retries += ailp->retry_count;
		ailp->retry_count = 0;
		if (ailp->consecutive_retries > 3) {
			switch (ailp->mode) {
			case AIM_GOTO_PLAYER:
				// -- mprintf((0, "Buddy stuck going to player...\n"));
				// -- Buddy_got_stuck = 1;
				move_towards_segment_center(obj);
				create_path_to_player(obj, 100, 1);
				// -- Buddy_got_stuck = 0;
				break;
			case AIM_GOTO_OBJECT:
				// -- mprintf((0, "Buddy stuck going to object...\n"));
				Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
				//					if (obj->segnum == ConsoleObject->segnum) {
				//						if (Point_segs[aip->hide_index + aip->cur_path_index].segnum == obj->segnum)
				//							if ((aip->cur_path_index + aip->PATH_DIR >= 0) && (aip->cur_path_index + aip->PATH_DIR < aip->path_length-1))
				//								aip->cur_path_index += aip->PATH_DIR;
				//					}
				break;
			case AIM_CHASE_OBJECT:
				// -- mprintf((0, "(2) Object #%i, retries while chasing, creating path to player in frame %i\n", objnum, FrameCount));
				create_path_to_player(obj, 4 + Overall_agitation / 8 + Difficulty_level, 1);
				break;
			case AIM_STILL:
				if (robptr->attack_type)
					move_towards_segment_center(obj);
				else if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION) || (aip->behavior == AIB_FOLLOW)))	//	Behavior is still, so don't follow path.
					attempt_to_resume_path(obj);
				break;
			case AIM_FOLLOW_PATH:
				// mprintf((0, "Object %i following path got %i retries in frame %i\n", obj-Objects, ailp->consecutive_retries, FrameCount));
				if (Game_mode & GM_MULTI) {
					ailp->mode = AIM_STILL;
				}
				else
					attempt_to_resume_path(obj);
				break;
			case AIM_RUN_FROM_OBJECT:
				move_towards_segment_center(obj);
				obj->mtype.phys_info.velocity.x = 0;
				obj->mtype.phys_info.velocity.y = 0;
				obj->mtype.phys_info.velocity.z = 0;
				create_n_segment_path(obj, 5, -1);
				ailp->mode = AIM_RUN_FROM_OBJECT;
				break;
			case AIM_BEHIND:
				mprintf((0, "Hiding robot (%i) collided much.\n", obj - Objects));
				move_towards_segment_center(obj);
				obj->mtype.phys_info.velocity.x = 0;
				obj->mtype.phys_info.velocity.y = 0;
				obj->mtype.phys_info.velocity.z = 0;
				break;
			case AIM_OPEN_DOOR:
				create_n_segment_path_to_door(obj, 5, -1);
				break;
#ifndef NDEBUG
			case AIM_FOLLOW_PATH_2:
				Int3();	//	Should never happen!
				break;
#endif
			}
			ailp->consecutive_retries = 0;
		}
	}
	else
		ailp->consecutive_retries /= 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If in materialization center, exit
	if (!(Game_mode & GM_MULTI) && (Segment2s[obj->segnum].special == SEGMENT_IS_ROBOTMAKER)) {
		if (Station[Segment2s[obj->segnum].value].Enabled) {
			ai_follow_path(obj, 1, 1, NULL);		// 1 = player is visible, which might be a lie, but it works.
			return;
		}
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease player awareness due to the passage of time.
	if (ailp->player_awareness_type) {
		if (ailp->player_awareness_time > 0) {
			ailp->player_awareness_time -= FrameTime;
			if (ailp->player_awareness_time <= 0) {
				ailp->player_awareness_time = F1_0 * 2;	//new: 11/05/94
				ailp->player_awareness_type--;	//new: 11/05/94
			}
		}
		else {
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0 * 2;
			// aip->GOAL_STATE = AIS_REST;
		}
	}
	else
		aip->GOAL_STATE = AIS_REST;							//new: 12/13/94


	if (Player_is_dead && (ailp->player_awareness_type == 0))
		if ((dist_to_player < F1_0 * 200) && (P_Rand() < FrameTime / 8)) {
			if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(obj, 30))
					return;
#ifndef SHAREWARE
				ai_multi_send_robot_position(objnum, -1);
#endif

				if (!((ailp->mode == AIM_FOLLOW_PATH) && (aip->cur_path_index < aip->path_length - 1)))
					if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM)) {
						if (dist_to_player < F1_0 * 30)
							create_n_segment_path(obj, 5, 1);
						else
							create_path_to_player(obj, 20, 1);
					}
			}
		}

	// -- 	//	Make sure that if this guy got hit or bumped, then he's chasing player.
	// -- 	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
	// -- 		if ((ailp->mode != AIM_BEHIND) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (!robptr->companion) && (!robptr->thief) && (obj->id != ROBOT_BRAIN)) {
	// -- 			ailp->mode = AIM_CHASE_OBJECT;
	// -- 			ailp->player_awareness_type = 0;
	// -- 			ailp->player_awareness_time = 0;
	// -- 		}
	// -- 	}

		//	Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		if (player_visibility == 1)	//	Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility = 2;
	}
	else if (((obj_ref & 3) == 0) && !previous_visibility && (dist_to_player < F1_0 * 100)) {
		fix	sval, rval;

		rval = P_Rand();
		sval = (dist_to_player * (Difficulty_level + 1)) / 64;

		// -- mprintf((0, "Object #%3i: dist = %7.3f, rval = %8x, sval = %8x", obj-Objects, f2fl(dist_to_player), rval, sval));
		if ((fixmul(rval, sval) < FrameTime) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
			ailp->player_awareness_time = F1_0 * 3;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility == 1) {
				player_visibility = 2;
				// -- mprintf((0, "...SWITCH!"));
			}
		}

		// -- mprintf((0, "\n"));
	}


	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aip->GOAL_STATE == AIS_FLIN) && (aip->CURRENT_STATE == AIS_FLIN))
		aip->GOAL_STATE = AIS_LOCK;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Note: Should only do these two function calls for objects which animate
	if (Animation_enabled && (dist_to_player < F1_0 * 100)) { // && !(Game_mode & GM_MULTI)) {
		object_animates = do_silly_animation(obj);
		if (object_animates)
			ai_frame_animation(obj);
		//mprintf((0, "Object %i: goal=%i, current=%i\n", obj-Objects, obj->ctype.ai_info.GOAL_STATE, obj->ctype.ai_info.CURRENT_STATE));
	}
	else {
		//	If Object is supposed to animate, but we don't let it animate due to distance, then
		//	we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;		//	If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (Robot_info[obj->id].boss_flag) {
	case 0:
		break;

	case 1:
	case 2:
		mprintf((1, "Warning: D1 boss detected.  Not supported!\n"));
		break;

	default: {
		int	pv;
		fix	dtp = dist_to_player / 4;

		if (aip->GOAL_STATE == AIS_FLIN)
			aip->GOAL_STATE = AIS_FIRE;
		if (aip->CURRENT_STATE == AIS_FLIN)
			aip->CURRENT_STATE = AIS_FIRE;

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

		pv = player_visibility;

		//	If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
			pv = 0;
			dtp = vm_vec_dist_quick(&ConsoleObject->pos, &obj->pos) / 4;
		}

		do_boss_stuff(obj, pv);
	}
			 break;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Time-slice, don't process all the time, purely an efficiency hack.
	//	Guys whose behavior is station and are not at their hide segment get processed anyway.
	if (!((aip->behavior == AIB_SNIPE) && (ailp->mode != AIM_SNIPE_WAIT)) && !robptr->companion && !robptr->thief && (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION - 1)) { // If robot got hit, he gets to attack player always!
#ifndef NDEBUG
		if (Break_on_object != objnum) {	//	don't time slice if we're interested in this object.
#endif
			if ((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != obj->segnum)) {
				if (dist_to_player > F1_0 * 250)	//	station guys not at home always processed until 250 units away.
					return;
			}
			else if ((!ailp->previous_visibility) && ((dist_to_player >> 7) > ailp->time_since_processed)) {	//	128 units away (6.4 segments) processed after 1 second.
				if (robptr->thief)
					mprintf((0, "T"));
				return;
			}
#ifndef NDEBUG
		}
#endif
	}

	//	Reset time since processed, but skew objects so not everything processed synchronously, else
	//	we get fast frames with the occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp->time_since_processed = -((objnum & 0x03) * FrameTime) / 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Perform special ability
	switch (obj->id) {
	case ROBOT_BRAIN:
		//	Robots function nicely if behavior is Station.  This means they won't move until they
		//	can see the player, at which time they will start wandering about opening doors.
		if (ConsoleObject->segnum == obj->segnum) {
			if (!ai_multiplayer_awareness(obj, 97))
				return;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			move_away_from_player(obj, &vec_to_player, 0);
			ai_multi_send_robot_position(objnum, -1);
		}
		else if (ailp->mode != AIM_STILL) {
			int	r;

			r = openable_doors_in_segment(obj->segnum);
			if (r != -1) {
				ailp->mode = AIM_OPEN_DOOR;
				aip->GOALSIDE = r;
			}
			else if (ailp->mode != AIM_FOLLOW_PATH) {
				if (!ai_multiplayer_awareness(obj, 50))
					return;
				create_n_segment_path_to_door(obj, 8 + Difficulty_level, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
				ai_multi_send_robot_position(objnum, -1);
			}

			if (ailp->next_action_time < 0) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility) {
					make_nearby_robot_snipe();
					ailp->next_action_time = (NDL - Difficulty_level) * 2 * F1_0;
				}
			}
		}
		else {
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility) {
				if (!ai_multiplayer_awareness(obj, 50))
					return;
				create_n_segment_path_to_door(obj, 8 + Difficulty_level, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
				ai_multi_send_robot_position(objnum, -1);
			}
		}
		break;
	default:
		break;
	}

	if (aip->behavior == AIB_SNIPE) {
		if ((Game_mode & GM_MULTI) && !robptr->thief) {
			aip->behavior = AIB_NORMAL;
			ailp->mode = AIM_CHASE_OBJECT;
			return;
		}

		if (!(obj_ref & 3) || previous_visibility) {
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			//	If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
			if (ailp->mode == AIM_STILL)
				if (player_visibility || (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION))
					ailp->mode = AIM_SNIPE_ATTACK;

			if (!robptr->thief && (ailp->mode != AIM_STILL))
				do_snipe_frame(obj, dist_to_player, player_visibility, &vec_to_player);
		}
		else if (!robptr->thief && !robptr->companion)
			return;
	}

	//	More special ability stuff, but based on a property of a robot, not its ID.
	if (robptr->companion) {

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_escort_frame(obj, dist_to_player, player_visibility);

		if (obj->ctype.ai_info.danger_laser_num != -1) {
			object* dobjp = &Objects[obj->ctype.ai_info.danger_laser_num];

			if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == obj->ctype.ai_info.danger_laser_signature)) {
				fix	circle_distance;
				// -- mprintf((0, "Evading!  "));
				circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
				ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, circle_distance, 1, player_visibility);
			}
		}

		if (ready_to_fire(robptr, ailp)) {
			int	do_stuff = 0;
			if (openable_doors_in_segment(obj->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + 2 * aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if ((ailp->mode == AIM_GOTO_PLAYER) && (dist_to_player < 3 * MIN_ESCORT_DISTANCE / 2) && (vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player) > -F1_0 / 4)) {
				// mprintf((0, "Firing at player because dot = %7.3f\n", f2fl(vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player))));
				do_stuff = 1;
			}
			else
				; // mprintf((0, "Not Firing at player because dot = %7.3f, dist = %7.3f\n", f2fl(vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player)), f2fl(dist_to_player)));

			if (do_stuff) {
				Laser_create_new_easy(&obj->orient.fvec, &obj->pos, obj - Objects, FLARE_ID, 1);
				ailp->next_fire = F1_0 / 2;
				if (!Buddy_allowed_to_talk)	//	If buddy not talking, make him fire flares less often.
					ailp->next_fire += P_Rand() * 4;
			}

		}
	}

	if (robptr->thief) {

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_thief_frame(obj, dist_to_player, player_visibility, &vec_to_player);

		if (ready_to_fire(robptr, ailp)) {
			int	do_stuff = 0;
			if (openable_doors_in_segment(obj->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + 2 * aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;

			if (do_stuff) {
				//	@mk, 05/08/95: Firing flare from center of object, this is dumb...
				Laser_create_new_easy(&obj->orient.fvec, &obj->pos, obj - Objects, FLARE_ID, 1);
				ailp->next_fire = F1_0 / 2;
				if (Stolen_item_index == 0)		//	If never stolen an item, fire flares less often (bad: Stolen_item_index wraps, but big deal)
					ailp->next_fire += P_Rand() * 4;
			}
		}
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	switch (ailp->mode) {
	case AIM_CHASE_OBJECT: {		// chasing player, sort of, chase if far, back off if close, circle in between
		fix	circle_distance;

		circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
		//	Green guy doesn't get his circle distance boosted, else he might never attack.
		if (robptr->attack_type != 1)
			circle_distance += (objnum & 0xf) * F1_0 / 2;

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

		//	@mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
		if ((player_visibility < 2) && (previous_visibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
			// -- mprintf((0, "I used to be able to see the player!\n"));
			if (!ai_multiplayer_awareness(obj, 53)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip))
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				return;
			}
			// -- mprintf((0, "(3) Object #%i going from chase to player path in frame %i.\n", objnum, FrameCount));
			create_path_to_player(obj, 8, 1);
			ai_multi_send_robot_position(objnum, -1);
		}
		else if ((player_visibility == 0) && (dist_to_player > F1_0 * 80) && (!(Game_mode & GM_MULTI))) {
			//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
			//	This has one desirable benefit of avoiding physics retries.
			if (aip->behavior == AIB_STATION) {
				ailp->goal_segment = aip->hide_segment;
				// -- mprintf((0, "(1) Object #%i going from chase to STATION in frame %i.\n", objnum, FrameCount));
				create_path_to_station(obj, 15);
			} // -- this looks like a dumb thing to do...robots following paths far away from you! else create_n_segment_path(obj, 5, -1);
			break;
		}

		if ((aip->CURRENT_STATE == AIS_REST) && (aip->GOAL_STATE == AIS_REST)) {
			if (player_visibility) {
				if (P_Rand() < FrameTime * player_visibility) {
					if (dist_to_player / 256 < P_Rand() * player_visibility) {
						// mprintf((0, "Object %i searching for player.\n", obj-Objects));
						aip->GOAL_STATE = AIS_SRCH;
						aip->CURRENT_STATE = AIS_SRCH;
					}
				}
			}
		}

		if (GameTime - ailp->time_player_seen > CHASE_TIME_LENGTH) {

			if (Game_mode & GM_MULTI)
				if (!player_visibility && (dist_to_player > F1_0 * 70)) {
					ailp->mode = AIM_STILL;
					return;
				}

			if (!ai_multiplayer_awareness(obj, 64)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip))
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				return;
			}
			// -- bad idea, robots charge player they've never seen! -- mprintf((0, "(4) Object #%i going from chase to player path in frame %i.\n", objnum, FrameCount));
			// -- bad idea, robots charge player they've never seen! -- create_path_to_player(obj, 10, 1);
			// -- bad idea, robots charge player they've never seen! -- ai_multi_send_robot_position(objnum, -1);
		}
		else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
			if (!ai_multiplayer_awareness(obj, 70)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip))
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				return;
			}
			ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, circle_distance, 0, player_visibility);

			if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
				if (player_visibility) // == 2)
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
			}

			if (ai_evaded) {
				ai_multi_send_robot_position(objnum, 1);
				ai_evaded = 0;
			}
			else
				ai_multi_send_robot_position(objnum, -1);

			do_firing_stuff(obj, player_visibility, &vec_to_player);
		}
		break;
	}

	case AIM_RUN_FROM_OBJECT:
		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

		if (player_visibility) {
			if (ailp->player_awareness_type == 0)
				ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;

		}

		//	If in multiplayer, only do if player visible.  If not multiplayer, do always.
		if (!(Game_mode & GM_MULTI) || player_visibility)
			if (ai_multiplayer_awareness(obj, 75)) {
				ai_follow_path(obj, player_visibility, previous_visibility, &vec_to_player);
				ai_multi_send_robot_position(objnum, -1);
			}

		if (aip->GOAL_STATE != AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;
		else if (aip->CURRENT_STATE == AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;

		//	Bad to let run_from robot fire at player because it will cause a war in which it turns towards the
		//	player to fire and then towards its goal to move.
		// do_firing_stuff(obj, player_visibility, &vec_to_player);
		//	Instead, do this:
		//	(Note, only drop if player is visible.  This prevents the bombs from being a giveaway, and
		//	also ensures that the robot is moving while it is dropping.  Also means fewer will be dropped.)
		if ((ailp->next_fire <= 0) && (player_visibility))
		{
			vms_vector	fire_vec, fire_pos;

			if (!ai_multiplayer_awareness(obj, 75))
				return;

			fire_vec = obj->orient.fvec;
			vm_vec_negate(&fire_vec);
			vm_vec_add(&fire_pos, &obj->pos, &fire_vec);

			if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
				Laser_create_new_easy(&fire_vec, &fire_pos, obj - Objects, ROBOT_SUPERPROX_ID, 1);
			else
				Laser_create_new_easy(&fire_vec, &fire_pos, obj - Objects, PROXIMITY_ID, 1);

			ailp->next_fire = (F1_0 / 2) * (NDL + 5 - Difficulty_level);		//	Drop a proximity bomb every 5 seconds.

#ifndef SHAREWARE
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
			{
				ai_multi_send_robot_position(obj - Objects, -1);
				if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					multi_send_robot_fire(obj - Objects, -2, &fire_vec);
				else
					multi_send_robot_fire(obj - Objects, -1, &fire_vec);
			}
#endif	
#endif
		}
		break;

	case AIM_GOTO_PLAYER:
	case AIM_GOTO_OBJECT:
		ai_follow_path(obj, 2, previous_visibility, &vec_to_player);		//	Follows path as if player can see robot.
		ai_multi_send_robot_position(objnum, -1);
		break;

	case AIM_FOLLOW_PATH: {
		int	anger_level = 65;

		if (aip->behavior == AIB_STATION)
			if (Point_segs[aip->hide_index + aip->path_length - 1].segnum == aip->hide_segment) {
				anger_level = 64;
				// mprintf((0, "Object %i, station, lowering anger to 64.\n"));
			}

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

		if (Game_mode & (GM_MODEM | GM_SERIAL))
			if (!player_visibility && (dist_to_player > F1_0 * 70)) {
				ailp->mode = AIM_STILL;
				return;
			}

		if (!ai_multiplayer_awareness(obj, anger_level)) {
			if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			}
			return;
		}

		ai_follow_path(obj, player_visibility, previous_visibility, &vec_to_player);

		if (aip->GOAL_STATE != AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;
		else if (aip->CURRENT_STATE == AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;

		if (aip->behavior != AIB_RUN_FROM)
			do_firing_stuff(obj, player_visibility, &vec_to_player);

		if ((player_visibility == 2) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && (aip->behavior != AIB_RUN_FROM) && (obj->id != ROBOT_BRAIN) && (robptr->companion != 1) && (robptr->thief != 1)) {
			if (robptr->attack_type == 0)
				ailp->mode = AIM_CHASE_OBJECT;
			//	This should not just be distance based, but also time-since-player-seen based.
		}
		else if ((dist_to_player > F1_0 * (20 * (2 * Difficulty_level + robptr->pursuit)))
			&& (GameTime - ailp->time_player_seen > (F1_0 / 2 * (Difficulty_level + robptr->pursuit)))
			&& (player_visibility == 0)
			&& (aip->behavior == AIB_NORMAL)
			&& (ailp->mode == AIM_FOLLOW_PATH)) {
			ailp->mode = AIM_STILL;
			aip->hide_index = -1;
			aip->path_length = 0;
		}

		ai_multi_send_robot_position(objnum, -1);

		break;
	}

	case AIM_BEHIND:
		if (!ai_multiplayer_awareness(obj, 71)) {
			if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			}
			return;
		}

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

		if (player_visibility == 2) {
			//	Get behind the player.
			//	Method:
			//		If vec_to_player dot player_rear_vector > 0, behind is goal.
			//		Else choose goal with larger dot from left, right.
			vms_vector	goal_point, goal_vector, vec_to_goal, rand_vec;
			fix			dot;

			dot = vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player);
			if (dot > 0) {			//	Remember, we're interested in the rear vector dot being < 0.
				goal_vector = ConsoleObject->orient.fvec;
				vm_vec_negate(&goal_vector);
				// -- mprintf((0, "Goal is BEHIND\n"));
			}
			else {
				fix	dot;
				dot = vm_vec_dot(&ConsoleObject->orient.rvec, &vec_to_player);
				goal_vector = ConsoleObject->orient.rvec;
				if (dot > 0) {
					vm_vec_negate(&goal_vector);
					// -- mprintf((0, "Goal is LEFT\n"));
				}
				else
					; // -- mprintf((0, "Goal is RIGHT\n"));
			}

			vm_vec_scale(&goal_vector, 2 * (ConsoleObject->size + obj->size + (((objnum * 4 + FrameCount) & 63) << 12)));
			vm_vec_add(&goal_point, &ConsoleObject->pos, &goal_vector);
			make_random_vector(&rand_vec);
			vm_vec_scale_add2(&goal_point, &rand_vec, F1_0 * 8);
			vm_vec_sub(&vec_to_goal, &goal_point, &obj->pos);
			vm_vec_normalize_quick(&vec_to_goal);
			move_towards_vector(obj, &vec_to_goal, 0);
			ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
			ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
		}

		if (aip->GOAL_STATE != AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;
		else if (aip->CURRENT_STATE == AIS_FLIN)
			aip->GOAL_STATE = AIS_LOCK;

		ai_multi_send_robot_position(objnum, -1);
		break;

	case AIM_STILL:
		if ((dist_to_player < F1_0 * 120 + Difficulty_level * F1_0 * 20) || (ailp->player_awareness_type >= PA_WEAPON_ROBOT_COLLISION - 1)) {
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			//	turn towards vector if visible this time or last time, or P_Rand
			// new!
			if ((player_visibility == 2) || (previous_visibility == 2)) { // -- MK, 06/09/95:  || ((P_Rand() > 0x4000) && !(Game_mode & GM_MULTI))) {
				if (!ai_multiplayer_awareness(obj, 71)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_multi_send_robot_position(objnum, -1);
			}

			do_firing_stuff(obj, player_visibility, &vec_to_player);
			if (player_visibility == 2) {		//	Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
				if (robptr->attack_type == 1) {
					aip->behavior = AIB_NORMAL;
					if (!ai_multiplayer_awareness(obj, 80)) {
						if (maybe_ai_do_actual_firing_stuff(obj, aip))
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
					ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
					if (ai_evaded) {
						ai_multi_send_robot_position(objnum, 1);
						ai_evaded = 0;
					}
					else
						ai_multi_send_robot_position(objnum, -1);
				}
				else {
					//	Robots in hover mode are allowed to evade at half normal speed.
					if (!ai_multiplayer_awareness(obj, 81)) {
						if (maybe_ai_do_actual_firing_stuff(obj, aip))
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
					ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 1, player_visibility);
					if (ai_evaded) {
						ai_multi_send_robot_position(objnum, -1);
						ai_evaded = 0;
					}
					else
						ai_multi_send_robot_position(objnum, -1);
				}
			}
			else if ((obj->segnum != aip->hide_segment) && (dist_to_player > F1_0 * 80) && (!(Game_mode & GM_MULTI))) {
				//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
				//	This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->goal_segment = aip->hide_segment;
					// -- mprintf((0, "(2) Object #%i going from STILL to STATION in frame %i.\n", objnum, FrameCount));
					create_path_to_station(obj, 15);
				}
				break;
			}
		}

		break;
	case AIM_OPEN_DOOR: {		// trying to open a door.
		vms_vector	center_point, goal_vector;
		Assert(obj->id == ROBOT_BRAIN);		//	Make sure this guy is allowed to be in this mode.

		if (!ai_multiplayer_awareness(obj, 62))
			return;
		compute_center_point_on_side(&center_point, &Segments[obj->segnum], aip->GOALSIDE);
		vm_vec_sub(&goal_vector, &center_point, &obj->pos);
		vm_vec_normalize_quick(&goal_vector);
		ai_turn_towards_vector(&goal_vector, obj, robptr->turn_time[Difficulty_level]);
		move_towards_vector(obj, &goal_vector, 0);
		ai_multi_send_robot_position(objnum, -1);

		break;
	}

	case AIM_SNIPE_WAIT:
		break;
	case AIM_SNIPE_RETREAT:
		// -- if (ai_multiplayer_awareness(obj, 53))
		// -- 	if (ailp->next_fire < -F1_0)
		// -- 		ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
		break;
	case AIM_SNIPE_RETREAT_BACKWARDS:
	case AIM_SNIPE_ATTACK:
	case AIM_SNIPE_FIRE:
		if (ai_multiplayer_awareness(obj, 53)) {
			ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			if (robptr->thief)
				ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
			break;
		}
		break;

	case AIM_THIEF_WAIT:
	case AIM_THIEF_ATTACK:
	case AIM_THIEF_RETREAT:
	case AIM_WANDER:	//	Used for Buddy Bot
		break;

	default:
		mprintf((0, "Unknown mode = %i in robot %i, behavior = %i\n", ailp->mode, obj - Objects, aip->behavior));
		ailp->mode = AIM_CHASE_OBJECT;
		break;
	}		// end:	switch (ailp->mode) {

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If the robot can see you, increase his awareness of you.
	//	This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1);	//	Means it didn't get initialized!
	compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
	if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW) && (!robptr->thief)) {
		if ((ailp->player_awareness_type == 0) && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
		// mprintf((0, "Setting current to goal (%i) because object doesn't animate.\n", aip->GOAL_STATE));
	}

	Assert(ailp->player_awareness_type <= AIE_MAX);
	Assert(aip->CURRENT_STATE < AIS_MAX);
	Assert(aip->GOAL_STATE < AIS_MAX);

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (ailp->player_awareness_type) {
		new_goal_state = Ai_transition_table[ailp->player_awareness_type - 1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) {
			//	Decrease awareness, else this robot will flinch every frame.
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0 * 3;
		}

		if (new_goal_state == AIS_ERR_)
			new_goal_state = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = new_goal_state;

	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == AIS_FIRE)) {
		int	i, num_guns;
		num_guns = Robot_info[obj->id].n_guns;
		for (i = 0; i < num_guns; i++)
			ailp->goal_state[i] = AIS_FIRE;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if (ready_to_fire(robptr, ailp) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN) && (obj->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
		case	AIS_NONE:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			dot = vm_vec_dot(&obj->orient.fvec, &vec_to_player);
			if (dot >= F1_0 / 2)
				if (aip->GOAL_STATE == AIS_REST)
					aip->GOAL_STATE = AIS_SRCH;
			break;
		case	AIS_REST:
			if (aip->GOAL_STATE == AIS_REST) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (ready_to_fire(robptr, ailp) && (player_visibility)) {
					// mprintf((0, "Setting goal state to fire from rest.\n"));
					aip->GOAL_STATE = AIS_FIRE;
				}
			}
			break;
		case	AIS_SRCH:
			if (!ai_multiplayer_awareness(obj, 60))
				return;

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_multi_send_robot_position(objnum, -1);
			}
			break;
		case	AIS_LOCK:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!(Game_mode & GM_MULTI) || (player_visibility)) {
				if (!ai_multiplayer_awareness(obj, 68))
					return;

				if (player_visibility == 2) {	//	@mk, 09/21/95, require that they be looking towards you to turn towards you.
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		case	AIS_FIRE:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION - 1))) {
					if (Game_mode & GM_MULTI) {
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
				}
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_multi_send_robot_position(objnum, -1);
			}

			//	Fire at player, if appropriate.
			ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);

			break;
		case	AIS_RECO:
			if (!(obj_ref & 3)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility == 2) {
					if (!ai_multiplayer_awareness(obj, 69))
						return;
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				} // -- MK, 06/09/95: else if (!(Game_mode & GM_MULTI)) {
			}
			break;
		case	AIS_FLIN:
			// mprintf((0, "State = flinch, goal = %i.\n", aip->GOAL_STATE));
			break;
		default:
			mprintf((1, "Unknown mode for AI object #%i\n", objnum));
			aip->GOAL_STATE = AIS_REST;
			aip->CURRENT_STATE = AIS_REST;
			break;
		}
	} // end of: if (aip->GOAL_STATE != AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aip->CURRENT_GUN++;
		if (aip->CURRENT_GUN >= Robot_info[obj->id].n_guns)
			if ((robptr->n_guns == 1) || (robptr->weapon_type2 == -1))	//	Two weapon types hack.
				aip->CURRENT_GUN = 0;
			else
				aip->CURRENT_GUN = 1;
	}

}

//	-----------------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int	i;

	for (i = 0; i < MAX_AI_CLOAK_INFO; i++) {
		Ai_cloak_info[i].last_position = ConsoleObject->pos;
		Ai_cloak_info[i].last_segment = ConsoleObject->segnum;
		Ai_cloak_info[i].last_time = GameTime;
	}

	//	Make work for control centers.
	Believed_player_pos = Ai_cloak_info[0].last_position;
	Believed_player_seg = Ai_cloak_info[0].last_segment;

}

//	-----------------------------------------------------------------------------------
//	Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(object* objp, int type)
{
	//	If player cloaked and hit a robot, then increase awareness
	if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_WEAPON_WALL_COLLISION) || (type == PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (Num_awareness_events < MAX_AWARENESS_EVENTS) {
		if ((type == PA_WEAPON_WALL_COLLISION) || (type == PA_WEAPON_ROBOT_COLLISION))
			if (objp->id == VULCAN_ID)
				if (P_Rand() > 3276)
					return 0;		//	For vulcan cannon, only about 1/10 actually cause awareness

		Awareness_events[Num_awareness_events].segnum = objp->segnum;
		Awareness_events[Num_awareness_events].pos = objp->pos;
		Awareness_events[Num_awareness_events].type = type;
		Num_awareness_events++;
	}
	else {
		//		Int3();		// Hey -- Overflowed Awareness_events, make more or something
								// This just gets ignored, so you can just continue.
	}
	return 1;

}

// ----------------------------------------------------------------------------------
//	Robots will become aware of the player based on something that occurred.
//	The object (probably player or weapon) which created the awareness is objp.
void create_awareness_event(object* objp, int type)
{
	//	If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS)) {
		if (add_awareness_event(objp, type)) {
			if (((P_Rand() * (type + 4)) >> 15) > 4)
				Overall_agitation++;
			if (Overall_agitation > OVERALL_AGITATION_MAX)
				Overall_agitation = OVERALL_AGITATION_MAX;
		}
	}
}

int8_t	New_awareness[MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux(int segnum, int type, int level)
{
	int	j;

	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;

	// Process children.
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++)
		if (IS_CHILD(Segments[segnum].children[j]))
			if (level <= 3)
				if (type == 4)
					pae_aux(Segments[segnum].children[j], type - 1, level + 1);
				else
					pae_aux(Segments[segnum].children[j], type, level + 1);
}


// ----------------------------------------------------------------------------------
void process_awareness_events(void)
{
	int	i;

	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS)) {
		memset(New_awareness, 0, sizeof(New_awareness[0]) * (Highest_segment_index + 1));

		for (i = 0; i < Num_awareness_events; i++)
			pae_aux(Awareness_events[i].segnum, Awareness_events[i].type, 1);

	}

	Num_awareness_events = 0;
}

// ----------------------------------------------------------------------------------
void set_player_awareness_all(void)
{
	int	i;

	process_awareness_events();

	for (i = 0; i <= Highest_object_index; i++)
		if (Objects[i].control_type == CT_AI) {
			if (New_awareness[Objects[i].segnum] > Ai_local_info[i].player_awareness_type) {
				Ai_local_info[i].player_awareness_type = New_awareness[Objects[i].segnum];
				Ai_local_info[i].player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
			}

			//	Clear the bit that says this robot is only awake because a camera woke it up.
			if (New_awareness[Objects[i].segnum] > Ai_local_info[i].player_awareness_type)
				Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

#ifndef NDEBUG
int	Ai_dump_enable = 0;

FILE* Ai_dump_file = NULL;

char	Ai_error_message[128] = "";

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(const char* msg)
{
	int	tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf(Ai_error_message, "%s\n", msg);
	//dump_ai_objects_all();
	Ai_error_message[0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
void turn_off_ai_dump(void)
{
	if (Ai_dump_file != NULL)
		fclose(Ai_dump_file);

	Ai_dump_file = NULL;
}

#endif

extern void do_boss_dying_frame(object* objp);

// ----------------------------------------------------------------------------------
//	Do things which need to get done for all AI objects each frame.
//	This includes:
//		Setting player_awareness (a fix, time in seconds which object is aware of player)
void do_ai_frame_all(void)
{
#ifndef NDEBUG
	//dump_ai_objects_all();
#endif

	set_player_awareness_all();

	if (Ai_last_missile_camera != -1) {
		//	Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
		if (((FrameCount & 0x0f) == 0) || (Objects[Ai_last_missile_camera].type != OBJ_WEAPON)) {
			int	i;

			Ai_last_missile_camera = -1;
			for (i = 0; i <= Highest_object_index; i++)
				if (Objects[i].type == OBJ_ROBOT)
					Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
	}

	//	(Moved here from do_boss_stuff() because that only gets called if robot aware of player.)
	if (Boss_dying) {
		int	i;

		for (i = 0; i <= Highest_object_index; i++)
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[Objects[i].id].boss_flag)
					do_boss_dying_frame(&Objects[i]);
	}
}


extern int Final_boss_is_dead;
extern fix Boss_invulnerable_dot;

//	Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	Overall_agitation = 0;
	Final_boss_is_dead = 0;

	Buddy_objnum = 0;
	Buddy_allowed_to_talk = 0;

	Boss_invulnerable_dot = F1_0 / 4 - i2f(Difficulty_level) / 8;
	Boss_dying_start_time = 0;
}

#include "cfile/cfile.h"

int ai_save_state(FILE* fp)
{
	/*fwrite(&Ai_initialized, sizeof(int), 1, fp);
	fwrite(&Overall_agitation, sizeof(int), 1, fp);

	fwrite(Ai_local_info, sizeof(ai_local) * MAX_OBJECTS, 1, fp);
	fwrite(Point_segs, sizeof(point_seg) * MAX_POINT_SEGS, 1, fp);
	fwrite(Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1, fp);

	fwrite(&Boss_cloak_start_time, sizeof(fix), 1, fp);
	fwrite(&Boss_cloak_end_time, sizeof(fix), 1, fp);
	fwrite(&Last_teleport_time, sizeof(fix), 1, fp);
	fwrite(&Boss_teleport_interval, sizeof(fix), 1, fp);
	fwrite(&Boss_cloak_interval, sizeof(fix), 1, fp);
	fwrite(&Boss_cloak_duration, sizeof(fix), 1, fp);
	fwrite(&Last_gate_time, sizeof(fix), 1, fp);
	fwrite(&Gate_interval, sizeof(fix), 1, fp);
	fwrite(&Boss_dying_start_time, sizeof(fix), 1, fp);
	fwrite(&Boss_dying, sizeof(int), 1, fp);
	fwrite(&Boss_dying_sound_playing, sizeof(int), 1, fp);
	fwrite(&Boss_hit_time, sizeof(fix), 1, fp);
	// -- MK, 10/21/95, unused! -- fwrite( &Boss_been_hit, sizeof(int), 1, fp );

	fwrite(&Escort_kill_object, sizeof(Escort_kill_object), 1, fp);
	fwrite(&Escort_last_path_created, sizeof(Escort_last_path_created), 1, fp);
	fwrite(&Escort_goal_object, sizeof(Escort_goal_object), 1, fp);
	fwrite(&Escort_special_goal, sizeof(Escort_special_goal), 1, fp);
	fwrite(&Escort_goal_index, sizeof(Escort_goal_index), 1, fp);
	fwrite(&Stolen_items, sizeof(Stolen_items[0]) * MAX_STOLEN_ITEMS, 1, fp);

	{ int temp;
	temp = Point_segs_free_ptr - Point_segs;
	fwrite(&temp, sizeof(int), 1, fp);
	}

	fwrite(&Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1, fp);
	fwrite(&Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1, fp);

	if (Num_boss_gate_segs)
		fwrite(Boss_gate_segs, sizeof(Boss_gate_segs[0]), Num_boss_gate_segs, fp);

	if (Num_boss_teleport_segs)
		fwrite(Boss_teleport_segs, sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs, fp);*/

	int i;

	F_WriteInt(fp, Ai_initialized);
	F_WriteInt(fp, Overall_agitation);
	for (i = 0; i < MAX_OBJECTS; i++)
		P_WriteAILocals(&Ai_local_info[i], fp);
	for (i = 0; i < MAX_POINT_SEGS; i++)
		P_WriteSegPoint(&Point_segs[i], fp);
	for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
		P_WriteCloakInfo(&Ai_cloak_info[i], fp);

	F_WriteInt(fp, Boss_cloak_start_time);
	F_WriteInt(fp, Boss_cloak_end_time);
	F_WriteInt(fp, Last_teleport_time);
	F_WriteInt(fp, Boss_teleport_interval);
	F_WriteInt(fp, Boss_cloak_interval);
	F_WriteInt(fp, Boss_cloak_duration);
	F_WriteInt(fp, Last_gate_time);
	F_WriteInt(fp, Gate_interval);
	F_WriteInt(fp, Boss_dying_start_time);
	F_WriteInt(fp, Boss_dying);
	F_WriteInt(fp, Boss_dying_sound_playing);
	F_WriteInt(fp, Boss_hit_time);

	F_WriteInt(fp, Escort_kill_object);
	F_WriteInt(fp, Escort_last_path_created);
	F_WriteInt(fp, Escort_goal_object);
	F_WriteInt(fp, Escort_special_goal);
	F_WriteInt(fp, Escort_goal_index);
	for (i = 0; i < MAX_STOLEN_ITEMS; i++)
		F_WriteByte(fp, Stolen_items[i]);

	{ 
		int temp;
		temp = Point_segs_free_ptr - Point_segs;
		//fwrite(&temp, sizeof(int), 1, fp);
		F_WriteInt(fp, temp);
	}

	F_WriteInt(fp, Num_boss_teleport_segs);
	F_WriteInt(fp, Num_boss_gate_segs);

	//if (Num_boss_gate_segs)
	for (int i = 0; i < Num_boss_gate_segs; i++)
	{
		F_WriteShort(fp, Boss_gate_segs[i]);
	}
	for (int i = 0; i < Num_boss_teleport_segs; i++)
	{
		F_WriteShort(fp, Boss_teleport_segs[i]);
	}
		//fwrite(Boss_gate_segs, sizeof(Boss_gate_segs[0]), Num_boss_gate_segs, fp);

	//if (Num_boss_teleport_segs)
		//fwrite(Boss_teleport_segs, sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs, fp);

	return 1;
}

int ai_restore_state(FILE* fp, int version)
{
	int i;
	Ai_initialized = F_ReadInt(fp);
	Overall_agitation = F_ReadInt(fp);
	for (i = 0; i < MAX_OBJECTS; i++)
		P_ReadAILocals(&Ai_local_info[i], fp);
	for (i = 0; i < MAX_POINT_SEGS; i++)
		P_ReadSegPoint(&Point_segs[i], fp);
	for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
		P_ReadCloakInfo(&Ai_cloak_info[i], fp);

	Boss_cloak_start_time = F_ReadInt(fp);
	Boss_cloak_end_time = F_ReadInt(fp);
	Last_teleport_time = F_ReadInt(fp);
	Boss_teleport_interval = F_ReadInt(fp);
	Boss_cloak_interval = F_ReadInt(fp);
	Boss_cloak_duration = F_ReadInt(fp);
	Last_gate_time = F_ReadInt(fp);
	Gate_interval = F_ReadInt(fp);
	Boss_dying_start_time = F_ReadInt(fp);
	Boss_dying = F_ReadInt(fp);
	Boss_dying_sound_playing = F_ReadInt(fp);
	Boss_hit_time = F_ReadInt(fp);

	if (version >= 8)
	{
		Escort_kill_object = F_ReadInt(fp);
		Escort_last_path_created = F_ReadInt(fp);
		Escort_goal_object = F_ReadInt(fp);
		Escort_special_goal = F_ReadInt(fp);
		Escort_goal_index = F_ReadInt(fp);
		for (i = 0; i < MAX_STOLEN_ITEMS; i++)
			Stolen_items[i] = F_ReadByte(fp);
	}
	else 
	{
		Escort_kill_object = -1;
		Escort_last_path_created = 0;
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_special_goal = -1;
		Escort_goal_index = -1;

		for (i = 0; i < MAX_STOLEN_ITEMS; i++)
			Stolen_items[i] = 255;
	}

	if (version >= 15)
	{
		int	temp = F_ReadInt(fp);
		//fread(&temp, sizeof(int), 1, fp);
		Point_segs_free_ptr = &Point_segs[temp];
	}
	else
		ai_reset_all_paths();

	if (version >= 21)
	{
		//fread(&Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1, fp);
		//fread(&Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1, fp);
		Num_boss_teleport_segs = F_ReadInt(fp);
		Num_boss_gate_segs = F_ReadInt(fp);

		if (Num_boss_teleport_segs > MAX_BOSS_TELEPORT_SEGS)
			Error("ai_restore_state: Too many boss teleport segments.\n");
		if (Num_boss_gate_segs > MAX_BOSS_TELEPORT_SEGS)
			Error("ai_restore_state: Too many boss gate segemnets.\n");

		if (Num_boss_gate_segs)
			for (i = 0; i < Num_boss_gate_segs; i++)
				Boss_gate_segs[i] = F_ReadShort(fp);
			//fread(Boss_gate_segs, sizeof(Boss_gate_segs[0]), Num_boss_gate_segs, fp);

		if (Num_boss_teleport_segs)
			for (i = 0; i < Num_boss_teleport_segs; i++)
				Boss_teleport_segs[i] = F_ReadShort(fp);
			//fread(Boss_teleport_segs, sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs, fp);
	}
	else 
	{
		// -- Num_boss_teleport_segs = 1;
		// -- Num_boss_gate_segs = 1;
		// -- Boss_teleport_segs[0] = 0;
		// -- Boss_gate_segs[0] = 0;
		//	Note: Maybe better to leave alone...will probably be ok.
		mprintf((1, "Warning: If you fight the boss, he might teleport to segment #0!\n"));
	}


	/*fread(&Ai_initialized, sizeof(int), 1, fp);
	fread(&Overall_agitation, sizeof(int), 1, fp);
	fread(Ai_local_info, sizeof(ai_local) * MAX_OBJECTS, 1, fp);
	fread(Point_segs, sizeof(point_seg) * MAX_POINT_SEGS, 1, fp);
	fread(Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1, fp);
	fread(&Boss_cloak_start_time, sizeof(fix), 1, fp);
	fread(&Boss_cloak_end_time, sizeof(fix), 1, fp);
	fread(&Last_teleport_time, sizeof(fix), 1, fp);
	fread(&Boss_teleport_interval, sizeof(fix), 1, fp);
	fread(&Boss_cloak_interval, sizeof(fix), 1, fp);
	fread(&Boss_cloak_duration, sizeof(fix), 1, fp);
	fread(&Last_gate_time, sizeof(fix), 1, fp);
	fread(&Gate_interval, sizeof(fix), 1, fp);
	fread(&Boss_dying_start_time, sizeof(fix), 1, fp);
	fread(&Boss_dying, sizeof(int), 1, fp);
	fread(&Boss_dying_sound_playing, sizeof(int), 1, fp);
	fread(&Boss_hit_time, sizeof(fix), 1, fp);
	// -- MK, 10/21/95, unused! -- fread( &Boss_been_hit, sizeof(int), 1, fp );

	if (version >= 8) {
		fread(&Escort_kill_object, sizeof(Escort_kill_object), 1, fp);
		fread(&Escort_last_path_created, sizeof(Escort_last_path_created), 1, fp);
		fread(&Escort_goal_object, sizeof(Escort_goal_object), 1, fp);
		fread(&Escort_special_goal, sizeof(Escort_special_goal), 1, fp);
		fread(&Escort_goal_index, sizeof(Escort_goal_index), 1, fp);
		fread(&Stolen_items, sizeof(Stolen_items[0]) * MAX_STOLEN_ITEMS, 1, fp);
	}
	else {
		int	i;

		Escort_kill_object = -1;
		Escort_last_path_created = 0;
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_special_goal = -1;
		Escort_goal_index = -1;

		for (i = 0; i < MAX_STOLEN_ITEMS; i++) 
		{
			Stolen_items[i] = 255;
		}

	}

	if (version >= 15) 
	{
		int	temp;
		fread(&temp, sizeof(int), 1, fp);
		Point_segs_free_ptr = &Point_segs[temp];
	}
	else
		ai_reset_all_paths();

	if (version >= 21) 
	{
		fread(&Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1, fp);
		fread(&Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1, fp);

		if (Num_boss_gate_segs)
			fread(Boss_gate_segs, sizeof(Boss_gate_segs[0]), Num_boss_gate_segs, fp);

		if (Num_boss_teleport_segs)
			fread(Boss_teleport_segs, sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs, fp);
	}
	else {
		// -- Num_boss_teleport_segs = 1;
		// -- Num_boss_gate_segs = 1;
		// -- Boss_teleport_segs[0] = 0;
		// -- Boss_gate_segs[0] = 0;
		//	Note: Maybe better to leave alone...will probably be ok.
		mprintf((1, "Warning: If you fight the boss, he might teleport to segment #0!\n"));
	}*/


	return 1;
}
