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

#include "vecmat/vecmat.h"
#include "object.h"
#include "game.h"

#define MAX_GUNS 8		//should be multiple of 4 for uint8_t array

//Animation states
#define AS_REST			0
#define AS_ALERT			1
#define AS_FIRE			2
#define AS_RECOIL			3
#define AS_FLINCH			4
#define N_ANIM_STATES	5

#define	RI_CLOAKED_NEVER					0
#define	RI_CLOAKED_ALWAYS					1
#define	RI_CLOAKED_EXCEPT_FIRING		2

//describes the position of a certain joint
typedef struct jointpos 
{
	short jointnum;
	vms_angvec angles;
} jointpos;

//describes a list of joint positions
typedef struct jointlist 
{
	short n_joints;
	short offset;
} jointlist;

#ifdef BUILD_DESCENT2
//robot info flags
#define RIF_BIG_RADIUS	1	//pad the radius to fix robots firing through walls
#define RIF_THIEF			2	//this guy steals!

//	Robot information
typedef struct robot_info 
{
	int			model_num;							// which polygon model?
	vms_vector	gun_points[MAX_GUNS];			// where each gun model is
	uint8_t			gun_submodels[MAX_GUNS];		// which submodel is each gun in?

	short 		exp1_vclip_num;
	short			exp1_sound_num;

	short 		exp2_vclip_num;
	short			exp2_sound_num;

	int8_t			weapon_type;
	int8_t			weapon_type2;						//	Secondary weapon number, -1 means none, otherwise gun #0 fires this weapon.
	int8_t			n_guns;								// how many different gun positions
	int8_t			contains_id;						//	ID of powerup this robot can contain.

	int8_t			contains_count;					//	Max number of things this instance can contain.
	int8_t			contains_prob;						//	Probability that this instance will contain something in N/16
	int8_t			contains_type;						//	Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	int8_t			kamikaze;							//	!0 means commits suicide when hits you, strength thereof. 0 means no.

	short			score_value;						//	Score from this robot.
	int8_t			badass;								//	Dies with badass explosion, and strength thereof, 0 means NO.
	int8_t			energy_drain;						//	Points of energy drained at each collision.

	fix			lighting;							// should this be here or with polygon model?
	fix			strength;							// Initial shields of robot

	fix		mass;										// how heavy is this thing?
	fix		drag;										// how much drag does it have?

	fix		field_of_view[NDL];					// compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
	fix		firing_wait[NDL];						//	time in seconds between shots
	fix		firing_wait2[NDL];					//	time in seconds between shots
	fix		turn_time[NDL];						// time in seconds to rotate 360 degrees in a dimension
// -- unused, mk, 05/25/95	fix		fire_power[NDL];						//	damage done by a hit from this robot
// -- unused, mk, 05/25/95	fix		shield[NDL];							//	shield strength of this robot
	fix		max_speed[NDL];						//	maximum speed attainable by this robot
	fix		circle_distance[NDL];				//	distance at which robot circles player

	int8_t		rapidfire_count[NDL];				//	number of shots fired rapidly
	int8_t		evade_speed[NDL];						//	rate at which robot can evade shots, 0=none, 4=very fast
	int8_t		cloak_type;								//	0=never, 1=always, 2=except-when-firing
	int8_t		attack_type;							//	0=firing, 1=charge (like green guy)

	uint8_t		see_sound;								//	sound robot makes when it first sees the player
	uint8_t		attack_sound;							//	sound robot makes when it attacks the player
	uint8_t		claw_sound;								//	sound robot makes as it claws you (attack_type should be 1)
	uint8_t		taunt_sound;							//	sound robot makes after you die

	int8_t		boss_flag;								//	0 = not boss, 1 = boss.  Is that surprising?
	int8_t		companion;								//	Companion robot, leads you to things.
	int8_t		smart_blobs;							//	how many smart blobs are emitted when this guy dies!
	int8_t		energy_blobs;							//	how many smart blobs are emitted when this guy gets hit by energy weapon!

	int8_t		thief;									//	!0 means this guy can steal when he collides with you!
	int8_t		pursuit;									//	!0 means pursues player after he goes around a corner.  4 = 4/2 pursue up to 4/2 seconds after becoming invisible if up to 4 segments away
	int8_t		lightcast;								//	Amount of light cast. 1 is default.  10 is very large.
	int8_t		death_roll;								//	0 = dies without death roll. !0 means does death roll, larger = faster and louder

	//boss_flag, companion, thief, & pursuit probably should also be bits in the flags byte.
	uint8_t		flags;									// misc properties
	uint8_t		pad[3];									// alignment

	uint8_t		deathroll_sound;						// if has deathroll, what sound?
	uint8_t		glow;										// apply this light to robot itself. stored as 4:4 fixed-point
	uint8_t		behavior;								//	Default behavior.
	uint8_t		aim;										//	255 = perfect, less = more likely to miss.  0 != random, would look stupid.  0=45 degree spread.  Specify in bitmaps.tbl in range 0.0..1.0

	//animation info
	jointlist anim_states[MAX_GUNS+1][N_ANIM_STATES];

	int		always_0xabcd;							// debugging

} robot_info;

#define	MAX_ROBOT_TYPES	85			// maximum number of robot types

#else
	
//	Robot information
typedef struct robot_info {
	int			model_num;							// which polygon model?
	int			n_guns;								// how many different gun positions
	vms_vector	gun_points[MAX_GUNS];			// where each gun model is
	uint8_t			gun_submodels[MAX_GUNS];		// which submodel is each gun in?
	short 		exp1_vclip_num;
	short			exp1_sound_num;
	short 		exp2_vclip_num;
	short			exp2_sound_num;
	short			weapon_type;
	int8_t			contains_id;						//	ID of powerup this robot can contain.
	int8_t			contains_count;					//	Max number of things this instance can contain.
	int8_t			contains_prob;						//	Probability that this instance will contain something in N/16
	int8_t			contains_type;						//	Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	int			score_value;						//	Score from this robot.
	fix			lighting;							// should this be here or with polygon model?
	fix			strength;							// Initial shields of robot

	fix		mass;										// how heavy is this thing?
	fix		drag;										// how much drag does it have?

	fix		field_of_view[NDL];					// compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
	fix		firing_wait[NDL];						//	time in seconds between shots
	fix		turn_time[NDL];						// time in seconds to rotate 360 degrees in a dimension
	fix		fire_power[NDL];						//	damage done by a hit from this robot
	fix		shield[NDL];							//	shield strength of this robot
	fix		max_speed[NDL];						//	maximum speed attainable by this robot
	fix		circle_distance[NDL];				//	distance at which robot circles player

	int8_t		rapidfire_count[NDL];				//	number of shots fired rapidly
	int8_t		evade_speed[NDL];						//	rate at which robot can evade shots, 0=none, 4=very fast
	int8_t		cloak_type;								//	0=never, 1=always, 2=except-when-firing
	int8_t		attack_type;							//	0=firing, 1=charge (like green guy)
	int8_t		boss_flag;								//	0 = not boss, 1 = boss.  Is that surprising?
	uint8_t		see_sound;								//	sound robot makes when it first sees the player
	uint8_t		attack_sound;							//	sound robot makes when it attacks the player
	uint8_t		claw_sound;								//	sound robot makes as it claws you (attack_type should be 1)

	//animation info
	jointlist anim_states[MAX_GUNS + 1][N_ANIM_STATES];

	int		always_0xabcd;							// debugging

} robot_info;

#define	MAX_ROBOT_TYPES	30				// maximum number of robot types
#endif

#define	ROBOT_NAME_LENGTH	16
extern char	Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//the array of robots types
extern robot_info Robot_info[];			// Robot info for AI system, loaded from bitmaps.tbl.

//how many kinds of robots
extern	int	N_robot_types;		// Number of robot types.  We used to assume this was the same as N_polygon_models.

//test data for one robot
#define MAX_ROBOT_JOINTS 1600
extern jointpos Robot_joints[MAX_ROBOT_JOINTS];
extern int	N_robot_joints;

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector *gun_point,object *obj,int gun_num);
//void calc_gun_point(vms_vector *gun_point,int objnum,int gun_num);

//	Tells joint positions for a gun to be in a specified state.
//	A gun can have associated with it any number of joints.  In order to tell whether a gun is a certain
//	state (such as FIRE or ALERT), you should call this function and check the returned joint positions
//	against the robot's gun's joint positions.  This function should also be called to determine how to
//	move a gun into a desired position.
//	For now (May 30, 1994), it is assumed that guns will linearly interpolate from one joint position to another.
//	There is no ordering of joint movement, so it's impossible to guarantee that a strange starting position won't
//	cause a gun to move through a robot's body, for example.

//	Given:
//		jp_list_ptr		pointer to list of joint angles, on exit, this is pointing at a static array
//		robot_type		type of robot for which to get joint information.  A particular type, not an instance of a robot.
//		gun_num			gun number.  If in 0..Robot_info[robot_type].n_guns-1, then it is a gun, else it refers to non-animating parts of robot.
//		state				state about which to get information.  Legal states in range 0..N_ANIM_STATES-1, defined in robot.h, are:
//								AS_REST, AS_ALERT, AS_FIRE, AS_RECOIL, AS_FLINCH

//	On exit:
//		Returns number of joints in list.
//		jp_list_ptr is stuffed with a pointer to a static array of joint positions.  This pointer is valid forever.
extern int robot_get_anim_state(jointpos **jp_list_ptr,int robot_type,int gun_num,int state);
