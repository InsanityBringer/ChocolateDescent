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

#include "segment.h"
#include "object.h"
#include "switch.h"

 //------------------------------------------------------------
 // A refueling center is one segment... to identify it in the
 // segment structure, the "special" field is set to 
 // SEGMENT_IS_FUELCEN.  The "value" field is then used for how
 // much fuel the center has left, with a maximum value of 100.

 //-------------------------------------------------------------
 // To hook into Inferno:
 // * When all segents are deleted or before a new mine is created
 //   or loaded, call fuelcen_reset().
 // * Add call to fuelcen_create(segment * segp) to make a segment
 //   which isn't a fuel center be a fuel center.  
 // * When a mine is loaded call fuelcen_activate(segp) with each 
 //   new segment as it loads. Always do this.
 // * When a segment is deleted, always call fuelcen_delete(segp).
 // * Call fuelcen_replentish_all() to fill 'em all up, like when
 //   a new game is started.
 // * When an object that needs to be refueled is in a segment, call
 //   fuelcen_give_fuel(segp) to get fuel. (Call once for any refueling
 //   object once per frame with the object's current segment.) This
 //   will return a value between 0 and 100 that tells how much fuel
 //   he got.


 // Destroys all fuel centers, clears segment backpointer array.
void fuelcen_reset();
// Create materialization center
int create_matcen(segment* segp);
// Makes a segment a fuel center.
void fuelcen_create(segment* segp);
// Makes a fuel center active... needs to be called when 
// a segment is loaded from disk.
void fuelcen_activate(segment* segp, int station_type);
// Deletes a segment as a fuel center.
void fuelcen_delete(segment* segp);

// Charges all fuel centers to max capacity.
//void fuelcen_replentish_all(); //[ISB] cut

// Create a matcen robot
extern object* create_morph_robot(segment* segp, vms_vector* object_pos, int object_id);

// Returns the amount of fuel this segment can give up.
// Can be from 0 to 100.
fix fuelcen_give_fuel(segment* segp, fix MaxAmountCanTake);

// Call once per frame.
void fuelcen_update_all();

// Called when hit by laser.
//void fuelcen_damage(segment* segp, fix AmountOfDamage); //[ISB] cut

#ifdef RESTORE_REPAIRCENTER
// Called to repair an object
int refuel_do_repair_effect( object * obj, int first_time, int repair_seg );
#endif

#define MAX_NUM_FUELCENS	50

#define SEGMENT_IS_NOTHING			0
#define SEGMENT_IS_FUELCEN			1
#define SEGMENT_IS_REPAIRCEN		2
#define SEGMENT_IS_CONTROLCEN		3
#define SEGMENT_IS_ROBOTMAKER		4
#define MAX_CENTER_TYPES			5

extern char Special_names[MAX_CENTER_TYPES][11];

// Set to 1 after control center is destroyed.
// Clear by calling fuelcen_reset();
extern int Fuelcen_control_center_destroyed;
extern int Fuelcen_seconds_left;

#ifdef RESTORE_REPAIRCENTER
//do the repair center for this frame
void do_repair_sequence(object *obj);

//see if we should start the repair center
void check_start_repair_center(object *obj);

//if repairing, cut it short
void abort_repair_center();
#endif

typedef struct control_center_triggers {
	short		num_links;
	short 	seg[MAX_WALLS_PER_LINK];
	short		side[MAX_WALLS_PER_LINK];
} control_center_triggers;

// An array of pointers to segments with fuel centers.
typedef struct FuelCenter {
	int			Type;
	int			segnum;
	int8_t			Flag;
	int8_t			Enabled;
	int8_t			Lives;			//	Number of times this can be enabled.
	int8_t			dum1;
	fix 			Capacity;
	fix			MaxCapacity;
	fix			Timer;
	fix			Disable_time;		//	Time until center disabled.
//	object *		last_created_obj;
//	int 			last_created_sig;
	vms_vector	Center;
} FuelCenter;

extern control_center_triggers ControlCenterTriggers;

// The max number of robot centers per mine.
#define MAX_ROBOT_CENTERS  20	

extern int Num_robot_centers;

typedef struct matcen_info {
	int			robot_flags;		// Up to 32 different robots
	fix			hit_points;			// How hard it is to destroy this particular matcen
	fix			interval;			// Interval between materialogrifizations
	short			segnum;				// Segment this is attached to.
	short			fuelcen_num;		// Index in fuelcen array.
} matcen_info;

extern matcen_info RobotCenters[MAX_ROBOT_CENTERS];

extern int Fuelcen_control_center_dead_modelnum;
extern fix Fuelcen_control_center_strength;

#ifdef RESTORE_REPAIRCENTER
extern object *RepairObj;			//which object getting repaired, or NULL
#endif

//	Called when a materialization center gets triggered by the player flying through some trigger!
extern void trigger_matcen(int segnum);

extern void disable_matcens(void);

extern FuelCenter Station[MAX_NUM_FUELCENS];
extern int Num_fuelcenters;

extern void init_all_matcens(void);

void read_matcen(matcen_info* center, FILE* fp);
void read_fuelcen(FuelCenter* center, FILE* fp);
void read_reactor_triggers(control_center_triggers* trigger, FILE* fp);

void write_matcen(matcen_info* center, FILE* fp);
void write_fuelcen(FuelCenter* center, FILE* fp);
void write_reactor_triggers(control_center_triggers* trigger, FILE* fp);

extern fix EnergyToCreateOneRobot;
