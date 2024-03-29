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

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/gr.h"
#include "2d/palette.h"
#include "newmenu.h"
#include "inferno.h"
#ifdef EDITOR
#include "editor\editor.h"
#endif
#include "misc/error.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "cfile/cfile.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "powerup.h"
#include "hostage.h"
#include "weapon.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
#include "polyobj.h"
#include "stringtable.h"
#include "gamefont.h"
#include "gamesave.h"

#ifdef EDITOR
#ifdef SHAREWARE
char* Shareware_level_names[NUM_SHAREWARE_LEVELS] = {
	"level01.sdl",
	"level02.sdl",
	"level03.sdl",
	"level04.sdl",
	"level05.sdl",
	"level06.sdl",
	"level07.sdl"
};
#else
char* Shareware_level_names[NUM_SHAREWARE_LEVELS] = {
	"level01.rdl",
	"level02.rdl",
	"level03.rdl",
	"level04.rdl",
	"level05.rdl",
	"level06.rdl",
	"level07.rdl"
};
#endif

char* Registered_level_names[NUM_REGISTERED_LEVELS] = {
	"level08.rdl",
	"level09.rdl",
	"level10.rdl",
	"level11.rdl",
	"level12.rdl",
	"level13.rdl",
	"level14.rdl",
	"level15.rdl",
	"level16.rdl",
	"level17.rdl",
	"level18.rdl",
	"level19.rdl",
	"level20.rdl",
	"level21.rdl",
	"level22.rdl",
	"level23.rdl",
	"level24.rdl",
	"level25.rdl",
	"level26.rdl",
	"level27.rdl",
	"levels1.rdl",
	"levels2.rdl",
	"levels3.rdl"
};
#endif

#ifdef HOSTAGE_FACES
void load_hostage_data(CFILE* fp, int do_read);
void save_hostage_data(FILE* fp);
#endif

char Gamesave_current_filename[128];

#define GAME_VERSION					25
#define GAME_COMPATIBLE_VERSION	22

#define MENU_CURSOR_X_MIN			MENU_X
#define MENU_CURSOR_X_MAX			MENU_X+6

#define HOSTAGE_DATA_VERSION	0

//Start old wall structures

typedef struct v16_wall {
	int8_t  type; 			  	// What kind of special wall.
	int8_t	flags;				// Flags for the wall.		
	fix   hps;				  	// "Hit points" of the wall. 
	int8_t	trigger;				// Which trigger is associated with the wall.
	int8_t	clip_num;			// Which	animation associated with the wall. 
	int8_t	keys;
} v16_wall;

typedef struct v19_wall {
	int	segnum, sidenum;	// Seg & side for this wall
	int8_t	type; 			  	// What kind of special wall.
	int8_t	flags;				// Flags for the wall.		
	fix   hps;				  	// "Hit points" of the wall. 
	int8_t	trigger;				// Which trigger is associated with the wall.
	int8_t	clip_num;			// Which	animation associated with the wall. 
	int8_t	keys;
	int	linked_wall;		// number of linked wall
} v19_wall;

typedef struct v19_door {
	int		n_parts;					// for linked walls
	short 	seg[2]; 					// Segment pointer of door.
	short 	side[2];					// Side number of door.
	short 	type[2];					// What kind of door animation.
	fix 		open;						//	How long it has been open.
} v19_door;

//End old wall structures

struct {
	uint16_t 	fileinfo_signature;
	uint16_t	fileinfo_version;
	int		fileinfo_sizeof;
} game_top_fileinfo;    // Should be same as first two fields below...

struct {
	uint16_t 	fileinfo_signature;
	uint16_t	fileinfo_version;
	int		fileinfo_sizeof;
	char		mine_filename[15];
	int		level;
	int		player_offset;				// Player info
	int		player_sizeof;
	int		object_offset;				// Object info
	int		object_howmany;
	int		object_sizeof;
	int		walls_offset;
	int		walls_howmany;
	int		walls_sizeof;
	int		doors_offset;
	int		doors_howmany;
	int		doors_sizeof;
	int		triggers_offset;
	int		triggers_howmany;
	int		triggers_sizeof;
	int		links_offset;
	int		links_howmany;
	int		links_sizeof;
	int		control_offset;
	int		control_howmany;
	int		control_sizeof;
	int		matcen_offset;
	int		matcen_howmany;
	int		matcen_sizeof;
} game_fileinfo;

#ifdef EDITOR
extern char mine_filename[];
extern int save_mine_data_compiled(FILE* SaveFile);
//--unused-- #else
//--unused-- char mine_filename[128];
#endif

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

#ifdef EDITOR
//	Return true if this level has a name of the form "level??"
//	Note that a pathspec can appear at the beginning of the filename.
int is_real_level(char* filename)
{
	int	len = strlen(filename);

	if (len < 6)
		return 0;

	//mprintf((0, "String = [%s]\n", &filename[len-11]));
	return !_strnfcmp(&filename[len - 11], "level", 5);

}

void convert_name_to_CDL(char* dest, char* src)
{
	int i;

	strcpy(dest, src);

#ifdef SHAREWARE
	for (i = 1; i < strlen(dest); i++)
	{
		if (dest[i] == '.' || dest[i] == ' ' || dest[i] == 0)
		{
			dest[i] = '.';
			dest[i + 1] = 'S';
			dest[i + 2] = 'D';
			dest[i + 3] = 'L';
			dest[i + 4] = 0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i] = '.';
		dest[i + 1] = 'S';
		dest[i + 2] = 'D';
		dest[i + 3] = 'L';
		dest[i + 4] = 0;
		return;
	}
#else
	for (i = 1; i < strlen(dest); i++)
	{
		if (dest[i] == '.' || dest[i] == ' ' || dest[i] == 0)
		{
			dest[i] = '.';
			dest[i + 1] = 'R';
			dest[i + 2] = 'D';
			dest[i + 3] = 'L';
			dest[i + 4] = 0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i] = '.';
		dest[i + 1] = 'R';
		dest[i + 2] = 'D';
		dest[i + 3] = 'L';
		dest[i + 4] = 0;
		return;
	}
#endif
}
#endif

void convert_name_to_LVL(char* dest, char* src)
{
	int i;

	strcpy(dest, src);

	for (i = 1; i < (int)strlen(dest); i++)
	{
		if (dest[i] == '.' || dest[i] == ' ' || dest[i] == 0)
		{
			dest[i] = '.';
			dest[i + 1] = 'L';
			dest[i + 2] = 'V';
			dest[i + 3] = 'L';
			dest[i + 4] = 0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i] = '.';
		dest[i + 1] = 'L';
		dest[i + 2] = 'V';
		dest[i + 3] = 'L';
		dest[i + 4] = 0;
		return;
	}
}

//--unused-- vms_angvec zero_angles={0,0,0};

#define vm_angvec_zero(v) do {(v)->p=(v)->b=(v)->h=0;} while (0)

int Gamesave_num_players = 0;

int N_save_pof_names = 25;
char Save_pof_names[MAX_POLYGON_MODELS][13];

void check_and_fix_matrix(vms_matrix* m);

void verify_object(object* obj) {

	obj->lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now

	if (obj->type == OBJ_ROBOT) 
	{
		Gamesave_num_org_robots++;

		// Make sure valid id...
		if (obj->id >= N_robot_types)
			obj->id = obj->id % N_robot_types;

		// Make sure model number & size are correct...		
		if (obj->render_type == RT_POLYOBJ) 
		{
			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;
		}

		if (obj->movement_type == MT_PHYSICS) {
			obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
			obj->mtype.phys_info.drag = Robot_info[obj->id].drag;
		}
	}
	else //Robots taken care of above
	{
		if (obj->render_type == RT_POLYOBJ) 
		{
			int i;
			char* name = Save_pof_names[obj->rtype.pobj_info.model_num];

			for (i = 0; i < N_polygon_models; i++)
				if (!_stricmp(Pof_names[i], name)) //found it!	
				{		
					// mprintf((0,"Mapping <%s> to %d (was %d)\n",name,i,obj->rtype.pobj_info.model_num));
					obj->rtype.pobj_info.model_num = i;
					break;
				}
		}
	}

	if (obj->type == OBJ_POWERUP) 
	{
		if (obj->id >= N_powerup_types) 
		{
			obj->id = 0;
			Assert(obj->render_type != RT_POLYOBJ);
		}
		obj->control_type = CT_POWERUP;

		obj->size = Powerup_info[obj->id].size;
	}

	if (obj->type == OBJ_WEAPON) 
	{
		if (obj->id >= N_weapon_types) 
		{
			obj->id = 0;
			Assert(obj->render_type != RT_POLYOBJ);
		}
	}

	if (obj->type == OBJ_CNTRLCEN) 
	{
		int i;

		obj->render_type = RT_POLYOBJ;
		obj->control_type = CT_CNTRLCEN;

		// Make model number is correct...	
		for (i = 0; i < Num_total_object_types; i++)
			if (ObjType[i] == OL_CONTROL_CENTER)
			{
				obj->rtype.pobj_info.model_num = ObjId[i];
				obj->shields = ObjStrength[i];
				break;
			}
	}

	if (obj->type == OBJ_PLAYER) 
	{
		if (obj == ConsoleObject)
			init_player_object();
		else
			if (obj->render_type == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
				obj->rtype.pobj_info.model_num = Player_ship->model_num;

		//Make sure orient matrix is orthogonal
		check_and_fix_matrix(&obj->orient);

		obj->id = Gamesave_num_players++;
	}

	if (obj->type == OBJ_HOSTAGE) 
	{

		if (obj->id > N_hostage_types)
			obj->id = 0;

		obj->render_type = RT_HOSTAGE;
		obj->control_type = CT_POWERUP;
	}
}

static int read_int(CFILE* file)
{
	int i;

	if (cfread(&i, sizeof(i), 1, file) != 1)
		Error("Error reading int in gamesave.c");

	return i;
}

static fix read_fix(CFILE* file)
{
	fix f;

	if (cfread(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fix in gamesave.c");

	return f;
}

static short read_short(CFILE* file)
{
	short s;

	if (cfread(&s, sizeof(s), 1, file) != 1)
		Error("Error reading short in gamesave.c");

	return s;
}

static short read_fixang(CFILE* file)
{
	fixang f;

	if (cfread(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fixang in gamesave.c");

	return f;
}

static int8_t read_byte(CFILE* file)
{
	int8_t b;

	if (cfread(&b, sizeof(b), 1, file) != 1)
		Error("Error reading byte in gamesave.c");

	return b;
}

static void read_vector(vms_vector* v, CFILE* file)
{
	//v->x = read_fix(file);
	//v->y = read_fix(file);
	//v->z = read_fix(file);
	v->x = cfile_read_int(file);
	v->y = cfile_read_int(file);
	v->z = cfile_read_int(file);
}

static void read_matrix(vms_matrix* m, CFILE* file)
{
	read_vector(&m->rvec, file);
	read_vector(&m->uvec, file);
	read_vector(&m->fvec, file);
}

static void read_angvec(vms_angvec* v, CFILE* file)
{
	v->p = cfile_read_short(file);
	v->b = cfile_read_short(file);
	v->h = cfile_read_short(file);
}

//-----------------------------------------------------------------------------
// [ISB]: Extra data readers
//-----------------------------------------------------------------------------
static void gs_read_wall(wall* nwall, CFILE* fp)
{
	nwall->segnum = cfile_read_int(fp);
	nwall->sidenum = cfile_read_int(fp);
	nwall->hps = cfile_read_int(fp);
	nwall->linked_wall = cfile_read_int(fp);
	nwall->type = cfile_read_byte(fp);
	nwall->flags = cfile_read_byte(fp);
	nwall->state = cfile_read_byte(fp);
	nwall->trigger = cfile_read_byte(fp);
	nwall->clip_num = cfile_read_byte(fp);
	nwall->keys = cfile_read_byte(fp);
	nwall->pad = cfile_read_short(fp);
}

static void gs_read_wall_v16(wall* nwall, CFILE* fp)
{
	nwall->segnum = nwall->sidenum = nwall->linked_wall = 0;
	nwall->type = cfile_read_byte(fp);
	nwall->flags = cfile_read_byte(fp);
	nwall->hps = cfile_read_int(fp);
	nwall->trigger = cfile_read_byte(fp);
	nwall->clip_num = cfile_read_byte(fp);
	nwall->keys = cfile_read_byte(fp);
}

static void gs_read_wall_v19(wall* nwall, CFILE* fp)
{
	nwall->segnum = cfile_read_int(fp);
	nwall->sidenum = cfile_read_int(fp);
	nwall->type = cfile_read_byte(fp);
	nwall->flags = cfile_read_byte(fp);
	nwall->hps = cfile_read_int(fp);
	nwall->trigger = cfile_read_byte(fp);
	nwall->clip_num = cfile_read_byte(fp);
	nwall->keys = cfile_read_byte(fp);
	nwall->linked_wall = cfile_read_int(fp);
	nwall->state = WALL_DOOR_CLOSED;
}

static void gs_read_active_door(active_door* door, CFILE* fp)
{
	door->n_parts = cfile_read_int(fp);
	door->front_wallnum[0] = cfile_read_short(fp);
	door->front_wallnum[1] = cfile_read_short(fp);
	door->back_wallnum[0] = cfile_read_short(fp);
	door->back_wallnum[1] = cfile_read_short(fp);
	door->time = cfile_read_int(fp);
}

static void gs_read_active_door_v19(active_door* door, CFILE* fp)
{
	v19_door oldDoor;
	int p;

	oldDoor.n_parts = cfile_read_int(fp);
	oldDoor.seg[0] = cfile_read_short(fp);
	oldDoor.seg[1] = cfile_read_short(fp);
	oldDoor.side[0] = cfile_read_short(fp);
	oldDoor.side[1] = cfile_read_short(fp);
	oldDoor.type[0] = cfile_read_short(fp);
	oldDoor.type[1] = cfile_read_short(fp);
	oldDoor.open = cfile_read_int(fp);

	door->n_parts = oldDoor.n_parts;
	for (p = 0; p < oldDoor.n_parts; p++)
	{
		int cseg, cside;

		cseg = Segments[oldDoor.seg[p]].children[oldDoor.side[p]];
		cside = find_connect_side(&Segments[oldDoor.seg[p]], &Segments[cseg]);

		door->front_wallnum[p] = Segments[oldDoor.seg[p]].sides[oldDoor.side[p]].wall_num;
		door->back_wallnum[p] = Segments[cseg].sides[cside].wall_num;
	}
}

static void gs_read_reactor_trigger(control_center_triggers* trigger, CFILE* fp)
{
	int i;

	trigger->num_links = cfile_read_short(fp);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		trigger->seg[i] = cfile_read_short(fp);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		trigger->side[i] = cfile_read_short(fp);
}

static void gs_read_matcen(matcen_info* center, CFILE* fp)
{
	center->robot_flags = cfile_read_int(fp);
	center->hit_points = cfile_read_int(fp);
	center->interval = cfile_read_int(fp);
	center->segnum = cfile_read_short(fp);
	center->fuelcen_num = cfile_read_short(fp);
}

#ifdef EDITOR
static void gs_write_int(int i, FILE* file)
{
	/*if (fwrite(&i, sizeof(i), 1, file) != 1)
		Error("Error reading int in gamesave.c");*/
	file_write_int(file, i);

}

static void gs_write_fix(fix f, FILE* file)
{
	/*if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fix in gamesave.c");*/
	file_write_int(file, f);
}

static void gs_write_short(short s, FILE* file)
{
	/*if (fwrite(&s, sizeof(s), 1, file) != 1)
		Error("Error reading short in gamesave.c");*/
	file_write_short(file, s);
}

static void gs_write_fixang(fixang f, FILE* file)
{
	/*if (fwrite(&f, sizeof(f), 1, file) != 1)
		Error("Error reading fixang in gamesave.c");*/
	file_write_short(file, f);
}

static void gs_write_byte(int8_t b, FILE* file)
{
	/*if (fwrite(&b, sizeof(b), 1, file) != 1)
		Error("Error reading byte in gamesave.c");*/
	file_write_byte(file, b);
}

static void gr_write_vector(vms_vector* v, FILE* file)
{
	gs_write_fix(v->x, file);
	gs_write_fix(v->y, file);
	gs_write_fix(v->z, file);
}

static void gs_write_matrix(vms_matrix* m, FILE* file)
{
	gr_write_vector(&m->rvec, file);
	gr_write_vector(&m->uvec, file);
	gr_write_vector(&m->fvec, file);
}

static void gs_write_angvec(vms_angvec* v, FILE* file)
{
	gs_write_fixang(v->p, file);
	gs_write_fixang(v->b, file);
	gs_write_fixang(v->h, file);
}

#endif

//reads one object of the given version from the given file
void read_object(object* obj, CFILE* f, int version)
{
	obj->type = cfile_read_byte(f);
	obj->id = cfile_read_byte(f);

	obj->control_type = cfile_read_byte(f);
	obj->movement_type = cfile_read_byte(f);
	obj->render_type = cfile_read_byte(f);
	obj->flags = cfile_read_byte(f);

	obj->segnum = cfile_read_short(f);
	obj->attached_obj = -1;

	read_vector(&obj->pos, f);
	read_matrix(&obj->orient, f);

	obj->size = cfile_read_int(f);
	obj->shields = cfile_read_int(f);

	read_vector(&obj->last_pos, f);

	obj->contains_type = cfile_read_byte(f);
	obj->contains_id = cfile_read_byte(f);
	obj->contains_count = cfile_read_byte(f);

	switch (obj->movement_type) 
	{
	case MT_PHYSICS:
		read_vector(&obj->mtype.phys_info.velocity, f);
		read_vector(&obj->mtype.phys_info.thrust, f);

		obj->mtype.phys_info.mass = cfile_read_int(f);
		obj->mtype.phys_info.drag = cfile_read_int(f);
		obj->mtype.phys_info.brakes = cfile_read_int(f);

		read_vector(&obj->mtype.phys_info.rotvel, f);
		read_vector(&obj->mtype.phys_info.rotthrust, f);

		obj->mtype.phys_info.turnroll = cfile_read_short(f);
		obj->mtype.phys_info.flags = cfile_read_short(f);
		break;

	case MT_SPINNING:
		read_vector(&obj->mtype.spin_rate, f);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type) 
	{

	case CT_AI: 
	{
		int i;

		obj->ctype.ai_info.behavior = cfile_read_byte(f);

		for (i = 0; i < MAX_AI_FLAGS; i++)
			obj->ctype.ai_info.flags[i] = cfile_read_byte(f);

		obj->ctype.ai_info.hide_segment = cfile_read_short(f);
		obj->ctype.ai_info.hide_index = cfile_read_short(f);
		obj->ctype.ai_info.path_length = cfile_read_short(f);
		obj->ctype.ai_info.cur_path_index = cfile_read_short(f);

		obj->ctype.ai_info.follow_path_start_seg = cfile_read_short(f);
		obj->ctype.ai_info.follow_path_end_seg = cfile_read_short(f);

		break;
	}

	case CT_EXPLOSION:

		obj->ctype.expl_info.spawn_time = cfile_read_int(f);
		obj->ctype.expl_info.delete_time = cfile_read_int(f);
		obj->ctype.expl_info.delete_objnum = cfile_read_short(f);
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

		break;

	case CT_WEAPON:

		//do I really need to read these?  Are they even saved to disk?
		obj->ctype.laser_info.parent_type = cfile_read_short(f);
		obj->ctype.laser_info.parent_num = cfile_read_short(f);
		obj->ctype.laser_info.parent_signature = cfile_read_int(f);

		break;

	case CT_LIGHT:

		obj->ctype.light_info.intensity = cfile_read_int(f);
		break;

	case CT_POWERUP:

		if (version >= 25)
			obj->ctype.powerup_info.count = cfile_read_int(f);
		else
			obj->ctype.powerup_info.count = 1;

		if (obj->id == POW_VULCAN_WEAPON)
			obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

		break;


	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
		break;

	case CT_SLEW:		//the player is generally saved as slew
		break;

	case CT_CNTRLCEN:
		break;

	case CT_MORPH:
	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
	default:
		Int3();

	}

	switch (obj->render_type)
	{

	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: 
	{
		int i, tmo;

		obj->rtype.pobj_info.model_num = cfile_read_int(f);

		for (i = 0; i < MAX_SUBMODELS; i++)
			read_angvec(&obj->rtype.pobj_info.anim_angles[i], f);

		obj->rtype.pobj_info.subobj_flags = cfile_read_int(f);

		tmo = cfile_read_int(f);

#ifndef EDITOR
		obj->rtype.pobj_info.tmap_override = tmo;
#else
		if (tmo == -1)
			obj->rtype.pobj_info.tmap_override = -1;
		else {
			int xlated_tmo = tmap_xlate_table[tmo];
			if (xlated_tmo < 0) {
				mprintf((0, "Couldn't find texture for demo object, model_num = %d\n", obj->rtype.pobj_info.model_num));
				Int3();
				xlated_tmo = 0;
			}
			obj->rtype.pobj_info.tmap_override = xlated_tmo;
		}
#endif

		obj->rtype.pobj_info.alt_textures = 0;

		break;
	}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:

		obj->rtype.vclip_info.vclip_num = cfile_read_int(f);
		obj->rtype.vclip_info.frametime = cfile_read_int(f);
		obj->rtype.vclip_info.framenum = cfile_read_byte(f);

		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

}

#ifdef EDITOR

//writes one object to the given file
void write_object(object* obj, FILE* f)
{
	file_write_byte(f, obj->type);
	file_write_byte(f, obj->id);

	file_write_byte(f, obj->control_type);
	file_write_byte(f, obj->movement_type);
	file_write_byte(f, obj->render_type);
	file_write_byte(f, obj->flags);

	file_write_short(f, obj->segnum);

	gr_write_vector(&obj->pos, f);
	gs_write_matrix(&obj->orient, f);

	file_write_int(f, obj->size);
	file_write_int(f, obj->shields);

	gr_write_vector(&obj->last_pos, f);

	file_write_byte(f, obj->contains_type);
	file_write_byte(f, obj->contains_id);
	file_write_byte(f, obj->contains_count);

	switch (obj->movement_type)
	{
	case MT_PHYSICS:
		gr_write_vector(&obj->mtype.phys_info.velocity, f);
		gr_write_vector(&obj->mtype.phys_info.thrust, f);

		file_write_int(f, obj->mtype.phys_info.mass);
		file_write_int(f, obj->mtype.phys_info.drag);
		file_write_int(f, obj->mtype.phys_info.brakes);

		gr_write_vector(&obj->mtype.phys_info.rotvel, f);
		gr_write_vector(&obj->mtype.phys_info.rotthrust, f);

		file_write_short(f, obj->mtype.phys_info.turnroll);
		file_write_short(f, obj->mtype.phys_info.flags);
		break;

	case MT_SPINNING:
		gr_write_vector(&obj->mtype.spin_rate, f);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type)
	{

	case CT_AI:
	{
		int i;

		file_write_byte(f, obj->ctype.ai_info.behavior);

		for (i = 0; i < MAX_AI_FLAGS; i++)
			file_write_byte(f, obj->ctype.ai_info.flags[i]);

		file_write_short(f, obj->ctype.ai_info.hide_segment);
		file_write_short(f, obj->ctype.ai_info.hide_index);
		file_write_short(f, obj->ctype.ai_info.path_length);
		file_write_short(f, obj->ctype.ai_info.cur_path_index);

		file_write_short(f, obj->ctype.ai_info.follow_path_start_seg);
		file_write_short(f, obj->ctype.ai_info.follow_path_end_seg);

		break;
	}

	case CT_EXPLOSION:

		file_write_int(f, obj->ctype.expl_info.spawn_time);
		file_write_int(f, obj->ctype.expl_info.delete_time);
		file_write_short(f, obj->ctype.expl_info.delete_objnum);

		break;

	case CT_WEAPON:

		//do I really need to write these objects?

		file_write_short(f, obj->ctype.laser_info.parent_type);
		file_write_short(f, obj->ctype.laser_info.parent_num);
		file_write_int(f, obj->ctype.laser_info.parent_signature);

		break;

	case CT_LIGHT:

		file_write_int(f, obj->ctype.light_info.intensity);
		break;

	case CT_POWERUP:

		file_write_int(f, obj->ctype.powerup_info.count);

		break;


	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
		break;

	case CT_SLEW:		//the player is generally saved as slew
		break;

	case CT_CNTRLCEN:
		break;

	case CT_MORPH:
	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
	default:
		Int3();

	}

	switch (obj->render_type)
	{

	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ:
	{
		int i;

		file_write_int(f, obj->rtype.pobj_info.model_num);

		for (i = 0; i < MAX_SUBMODELS; i++)
			gs_write_angvec(&obj->rtype.pobj_info.anim_angles[i], f);

		file_write_int(f, obj->rtype.pobj_info.subobj_flags);

		file_write_int(f, obj->rtype.pobj_info.tmap_override);

		break;
	}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:

		file_write_int(f, obj->rtype.vclip_info.vclip_num);
		file_write_int(f, obj->rtype.vclip_info.frametime);
		file_write_byte(f, obj->rtype.vclip_info.framenum);

		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

}
#endif

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int load_game_data(CFILE* LoadFile)
{
	int i, j;
	int start_offset;

	start_offset = cftell(LoadFile);

	//===================== READ FILE INFO ========================

	// Set default values
	game_fileinfo.level = -1;
	game_fileinfo.player_offset = -1;
	game_fileinfo.player_sizeof = sizeof(player);
	game_fileinfo.object_offset = -1;
	game_fileinfo.object_howmany = 0;
	game_fileinfo.object_sizeof = sizeof(object);
	game_fileinfo.walls_offset = -1;
	game_fileinfo.walls_howmany = 0;
	game_fileinfo.walls_sizeof = sizeof(wall);
	game_fileinfo.doors_offset = -1;
	game_fileinfo.doors_howmany = 0;
	game_fileinfo.doors_sizeof = sizeof(active_door);
	game_fileinfo.triggers_offset = -1;
	game_fileinfo.triggers_howmany = 0;
	game_fileinfo.triggers_sizeof = sizeof(trigger);
	game_fileinfo.control_offset = -1;
	game_fileinfo.control_howmany = 0;
	game_fileinfo.control_sizeof = sizeof(control_center_triggers);
	game_fileinfo.matcen_offset = -1;
	game_fileinfo.matcen_howmany = 0;
	game_fileinfo.matcen_sizeof = sizeof(matcen_info);

	// Read in game_top_fileinfo to get size of saved fileinfo.

	if (cfseek(LoadFile, start_offset, SEEK_SET))
		Error("Error seeking in gamesave.c");

	if (cfread(&game_top_fileinfo, sizeof(game_top_fileinfo), 1, LoadFile) != 1)
		Error("Error reading game_top_fileinfo in gamesave.c");

	// Check signature
	if (game_top_fileinfo.fileinfo_signature != 0x6705)
		return -1;

	// Check version number
	if (game_top_fileinfo.fileinfo_version < GAME_COMPATIBLE_VERSION)
		return -1;

	// Now, Read in the fileinfo
	if (cfseek(LoadFile, start_offset, SEEK_SET))
		Error("Error seeking to game_fileinfo in gamesave.c");

	game_fileinfo.fileinfo_signature = cfile_read_short(LoadFile);

	game_fileinfo.fileinfo_version = cfile_read_short(LoadFile);
	game_fileinfo.fileinfo_sizeof = cfile_read_int(LoadFile);
	for (i = 0; i < 15; i++)
		game_fileinfo.mine_filename[i] = cfile_read_byte(LoadFile);
	game_fileinfo.level = cfile_read_int(LoadFile);
	game_fileinfo.player_offset = cfile_read_int(LoadFile);				// Player info
	game_fileinfo.player_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.object_offset = cfile_read_int(LoadFile);				// Object info
	game_fileinfo.object_howmany = cfile_read_int(LoadFile);
	game_fileinfo.object_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.walls_offset = cfile_read_int(LoadFile);
	game_fileinfo.walls_howmany = cfile_read_int(LoadFile);
	game_fileinfo.walls_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.doors_offset = cfile_read_int(LoadFile);
	game_fileinfo.doors_howmany = cfile_read_int(LoadFile);
	game_fileinfo.doors_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.triggers_offset = cfile_read_int(LoadFile);
	game_fileinfo.triggers_howmany = cfile_read_int(LoadFile);
	game_fileinfo.triggers_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.links_offset = cfile_read_int(LoadFile);
	game_fileinfo.links_howmany = cfile_read_int(LoadFile);
	game_fileinfo.links_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.control_offset = cfile_read_int(LoadFile);
	game_fileinfo.control_howmany = cfile_read_int(LoadFile);
	game_fileinfo.control_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.matcen_offset = cfile_read_int(LoadFile);
	game_fileinfo.matcen_howmany = cfile_read_int(LoadFile);
	game_fileinfo.matcen_sizeof = cfile_read_int(LoadFile);

	if (game_top_fileinfo.fileinfo_version >= 14) //load mine filename
	{
		cfile_get_string(&Current_level_name[0], LEVEL_NAME_LEN, LoadFile);
	}
	else
		Current_level_name[0] = 0;

	if (game_top_fileinfo.fileinfo_version >= 19) //load pof names
	{
		N_save_pof_names = (int)cfile_read_short(LoadFile);
		if (N_save_pof_names > MAX_POLYGON_MODELS)
		{
			//Some levels seem to exceed these limits. Still need to read all of them or else the file pointer won't be at the right place. 
			char* badLevelHack = (char*)malloc(N_save_pof_names * FILENAME_LEN * sizeof(char));

			cfread(badLevelHack, N_save_pof_names, FILENAME_LEN, LoadFile);
			memcpy(Save_pof_names, badLevelHack, MAX_POLYGON_MODELS * FILENAME_LEN * sizeof(char));

			free(badLevelHack);
		}
		else
		{
			cfread(Save_pof_names, N_save_pof_names, FILENAME_LEN, LoadFile);
		}
	}

	//===================== READ PLAYER INFO ==========================
	Object_next_signature = 0;

	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (game_fileinfo.object_offset > -1) 
	{
		if (cfseek(LoadFile, game_fileinfo.object_offset, SEEK_SET))
			Error("Error seeking to object_offset in gamesave.c");

		if (game_fileinfo.object_howmany > MAX_OBJECTS)
			Error("Level contains over MAX_OBJECTS(%d) objects.", MAX_OBJECTS);

		for (i = 0; i < game_fileinfo.object_howmany; i++) 
		{
			read_object(&Objects[i], LoadFile, game_top_fileinfo.fileinfo_version);

			Objects[i].signature = Object_next_signature++;
			verify_object(&Objects[i]);
		}
	}

	//===================== READ WALL INFO ============================
	if (game_fileinfo.walls_offset > -1)
	{
		if (!cfseek(LoadFile, game_fileinfo.walls_offset, SEEK_SET)) 
		{
			if (game_fileinfo.walls_howmany >= MAX_WALLS)
				Error("Level contains over MAX_WALLS(%d) walls.", MAX_WALLS);

			for (i = 0; i < game_fileinfo.walls_howmany; i++) 
			{
				if (game_top_fileinfo.fileinfo_version >= 20) 
					gs_read_wall(&Walls[i], LoadFile);
				else if (game_top_fileinfo.fileinfo_version >= 17)
					gs_read_wall_v19(&Walls[i], LoadFile);
				else 
					gs_read_wall_v16(&Walls[i], LoadFile);
			}
		}
	}
	
	//===================== READ DOOR INFO ============================
	if (game_fileinfo.doors_offset > -1)
	{
		if (!cfseek(LoadFile, game_fileinfo.doors_offset, SEEK_SET)) 
		{
			mprintf((1, "load_game_data: active doors present"));

			if (game_fileinfo.doors_howmany > MAX_DOORS)
				Error("Level contains over MAX_DOORS(%d) active doors.", MAX_DOORS);

			for (i = 0; i < game_fileinfo.doors_howmany; i++) 
			{
				if (game_top_fileinfo.fileinfo_version >= 20) 
					gs_read_active_door(&ActiveDoors[i], LoadFile);
				else 
					gs_read_active_door_v19(&ActiveDoors[i], LoadFile);
			}
		}
	}

	//==================== READ TRIGGER INFO ==========================
	if (game_fileinfo.triggers_offset > -1)
	{
		if (!cfseek(LoadFile, game_fileinfo.triggers_offset, SEEK_SET)) 
		{
			if (game_fileinfo.triggers_howmany > MAX_TRIGGERS)
				Error("Level contains more than MAX_TRIGGERS(%d) triggers.", MAX_TRIGGERS);

			for (i = 0; i < game_fileinfo.triggers_howmany; i++) 
			{
				Triggers[i].type = cfile_read_byte(LoadFile);
				Triggers[i].flags = cfile_read_short(LoadFile);
				Triggers[i].value = cfile_read_int(LoadFile);
				Triggers[i].time = cfile_read_int(LoadFile);
				Triggers[i].link_num = cfile_read_byte(LoadFile);
				Triggers[i].num_links = cfile_read_short(LoadFile);
				for (j = 0; j < MAX_WALLS_PER_LINK; j++)
					Triggers[i].seg[j] = cfile_read_short(LoadFile);
				for (j = 0; j < MAX_WALLS_PER_LINK; j++)
					Triggers[i].side[j] = cfile_read_short(LoadFile);
			}
		}
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============
	if (game_fileinfo.control_offset > -1)
	{
		if (!cfseek(LoadFile, game_fileinfo.control_offset, SEEK_SET)) 
		{
			for (i = 0; i < game_fileinfo.control_howmany; i++)
			{
				//[ISB] there's only one...
				gs_read_reactor_trigger(&ControlCenterTriggers, LoadFile);
			}
		}
	}

	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============
	if (game_fileinfo.matcen_offset > -1)
	{
		int	j;

		if (!cfseek(LoadFile, game_fileinfo.matcen_offset, SEEK_SET)) 
		{
			if (game_fileinfo.matcen_howmany > MAX_ROBOT_CENTERS)
				Error("Level contains over MAX_ROBOT_CENTERS(%d) matcens.", MAX_ROBOT_CENTERS);

			// mprintf((0, "Reading %i materialization centers.\n", game_fileinfo.matcen_howmany));
			for (i = 0; i < game_fileinfo.matcen_howmany; i++) 
			{
				//Assert(sizeof(RobotCenters[i]) == game_fileinfo.matcen_sizeof);
				//if (cfread(&RobotCenters[i], game_fileinfo.matcen_sizeof, 1, LoadFile) != 1)
				//	Error("Error reading RobotCenters in gamesave.c", i);
				gs_read_matcen(&RobotCenters[i], LoadFile);
				//	Set links in RobotCenters to Station array
				for (j = 0; j <= Highest_segment_index; j++)
					if (Segments[j].special == SEGMENT_IS_ROBOTMAKER)
						if (Segments[j].matcen_num == i)
							RobotCenters[i].fuelcen_num = Segments[j].value;

				// mprintf((0, "   %i: flags = %08x\n", i, RobotCenters[i].robot_flags));
			}
		}
	}


	//========================= UPDATE VARIABLES ======================

	reset_objects(game_fileinfo.object_howmany);

	for (i = 0; i < MAX_OBJECTS; i++) 
	{
		Objects[i].next = Objects[i].prev = -1;
		if (Objects[i].type != OBJ_NONE) 
		{
			int objsegnum = Objects[i].segnum;

			if (objsegnum > Highest_segment_index)		//bogus object
				Objects[i].type = OBJ_NONE;
			else
			{
				Objects[i].segnum = -1;			//avoid Assert()
				obj_link(i, objsegnum);
			}
		}
	}

	clear_transient_objects(1);		//1 means clear proximity bombs

	// Make sure non-transparent doors are set correctly.
	for (i = 0; i < Num_segments; i++)
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) 
		{
			side* sidep = &Segments[i].sides[j];
			if ((sidep->wall_num != -1) && (Walls[sidep->wall_num].clip_num != -1)) 
			{
				//mprintf((0, "Checking Wall %d\n", Segments[i].sides[j].wall_num));
				if (WallAnims[Walls[sidep->wall_num].clip_num].flags & WCF_TMAP1) 
				{
					//mprintf((0, "Fixing non-transparent door.\n"));
					sidep->tmap_num = WallAnims[Walls[sidep->wall_num].clip_num].frames[0];
					sidep->tmap_num2 = 0;
				}
			}
		}


	Num_walls = game_fileinfo.walls_howmany;
	reset_walls();

	Num_open_doors = game_fileinfo.doors_howmany;
	Num_triggers = game_fileinfo.triggers_howmany;

	Num_robot_centers = game_fileinfo.matcen_howmany;

	//fix old wall structs
	if (game_top_fileinfo.fileinfo_version < 17) 
	{
		int segnum, sidenum, wallnum;

		for (segnum = 0; segnum <= Highest_segment_index; segnum++)
			for (sidenum = 0; sidenum < 6; sidenum++)
				if ((wallnum = Segments[segnum].sides[sidenum].wall_num) != -1) 
				{
					Walls[wallnum].segnum = segnum;
					Walls[wallnum].sidenum = sidenum;
				}
	}

#ifndef NDEBUG
	{
		int	sidenum;
		for (sidenum = 0; sidenum < 6; sidenum++) 
		{
			int	wallnum = Segments[Highest_segment_index].sides[sidenum].wall_num;
			if (wallnum != -1)
				if ((Walls[wallnum].segnum != Highest_segment_index) || (Walls[wallnum].sidenum != sidenum))
					Int3();	//	Error.  Bogus walls in this segment.
								// Consult Yuan or Mike.
		}
	}
#endif

	//create_local_segment_data();

	fix_object_segs();

#ifndef NDEBUG
	dump_mine_info();
#endif

	if (game_top_fileinfo.fileinfo_version < GAME_VERSION)
		return 1;		//means old version
	else
		return 0;
}


int check_segment_connections(void);

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(CFILE* LoadFile);
int load_mine_data_compiled(CFILE* LoadFile);

#define LEVEL_FILE_VERSION		1

#ifndef RELEASE
char* Level_being_loaded = NULL;
#endif


//loads a level (.LVL) file from disk
int load_level(char* filename_passed)
{
#ifdef EDITOR
	int use_compiled_level = 1;
#endif
	CFILE* LoadFile;
	char filename[128];
	int sig, version, minedata_offset, gamedata_offset, hostagetext_offset;
	int mine_err, game_err;

#ifndef RELEASE
	Level_being_loaded = filename_passed;
#endif

	strcpy(filename, filename_passed);

#ifdef EDITOR
	//check extension to see what file type this is
	strupr(filename);

	if (strstr(filename, ".LVL"))
		use_compiled_level = 0;
#ifdef SHAREWARE
	else if (!strstr(filename, ".SDL")) 
	{
		convert_name_to_CDL(filename, filename_passed);
		use_compiled_level = 1;
	}
#else
	else if (!strstr(filename, ".RDL")) 
	{
		convert_name_to_CDL(filename, filename_passed);
		use_compiled_level = 1;
	}
#endif

	// If we're trying to load a CDL, and we can't find it, and we have
	// the editor compiled in, then load the LVL.
	if ((!cfexist(filename)) && use_compiled_level) 
	{
		convert_name_to_LVL(filename, filename_passed);
		use_compiled_level = 0;
	}
#endif

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char full_path_filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
	get_full_file_path(full_path_filename, filename, CHOCOLATE_MISSIONS_DIR);

	LoadFile = cfopen(full_path_filename, "rb");
	if (!LoadFile) 
	{
		LoadFile = cfopen(filename, "rb");
	}
#else
	LoadFile = cfopen(filename, "rb");
#endif

#ifdef EDITOR
	if (!LoadFile)
	{
		mprintf((0, "load_level: Can't open file <%s>\n", filename));
		return 1;
	}
#else
	if (!LoadFile)
		Error("load_level: Can't open file <%s>\n", filename);
#endif

	strcpy(Gamesave_current_filename, filename);

	sig = read_int(LoadFile);
	version = read_int(LoadFile);
	minedata_offset = read_int(LoadFile);
	gamedata_offset = read_int(LoadFile);
	hostagetext_offset = read_int(LoadFile);

	if (sig != 'PLVL')
		Error("Level file doesn't have PLVL signature.");

	cfseek(LoadFile, minedata_offset, SEEK_SET);
#ifdef EDITOR
	if (!use_compiled_level)
		mine_err = load_mine_data(LoadFile);
	else
#endif
		//NOTE LINK TO ABOVE!!
		mine_err = load_mine_data_compiled(LoadFile);

	if (mine_err == -1)	//error!!
		return 1;

	cfseek(LoadFile, gamedata_offset, SEEK_SET);
	game_err = load_game_data(LoadFile);

	if (game_err == -1)	//error!!
		return 1;

#ifdef HOSTAGE_FACES
	cfseek(LoadFile, hostagetext_offset, SEEK_SET);
	load_hostage_data(LoadFile, (version >= 1));
#endif

	//======================== CLOSE FILE =============================

	cfclose(LoadFile);

#ifdef EDITOR
	write_game_text_file(filename);
	if (Errors_in_mine) 
	{
		if (is_real_level(filename)) 
		{
			char  ErrorMessage[200];

			sprintf(ErrorMessage, "Warning: %i errors in %s!\n", Errors_in_mine, Level_being_loaded);
			stop_time();
			gr_palette_load(gr_palette);
			nm_messagebox(NULL, 1, "Continue", ErrorMessage);
			start_time();
		}
		else
			mprintf((1, "Error: %i errors in %s.\n", Errors_in_mine, Level_being_loaded));
	}
#endif

#ifdef EDITOR
	//If an old version, ask the use if he wants to save as new version
	if (((LEVEL_FILE_VERSION > 1) && version < LEVEL_FILE_VERSION) || mine_err == 1 || game_err == 1) 
	{
		char  ErrorMessage[200];

		sprintf(ErrorMessage, "You just loaded a old version level.  Would\n"
			"you like to save it as a current version level?");

		stop_time();
		gr_palette_load(gr_palette);
		if (nm_messagebox(NULL, 2, "Don't Save", "Save", ErrorMessage) == 1)
			save_level(filename);
		start_time();
	}
#endif

#ifdef EDITOR
	if (Function_mode == FMODE_EDITOR)
		editor_status("Loaded NEW mine %s, \"%s\"", filename, Current_level_name);
#endif

#ifdef EDITOR
	if (check_segment_connections())
		nm_messagebox("ERROR", 1, "Ok",
			"Connectivity errors detected in\n"
			"mine.  See monochrome screen for\n"
			"details, and contact Matt or Mike.");
#endif

	return 0;
}

#ifdef EDITOR
int get_level_name()
{
	//NO_UI!!!	UI_WINDOW 				*NameWindow = NULL;
	//NO_UI!!!	UI_GADGET_INPUTBOX	*NameText;
	//NO_UI!!!	UI_GADGET_BUTTON 		*QuitButton;
	//NO_UI!!!
	//NO_UI!!!	// Open a window with a quit button
	//NO_UI!!!	NameWindow = ui_open_window( 20, 20, 300, 110, WIN_DIALOG );
	//NO_UI!!!	QuitButton = ui_add_gadget_button( NameWindow, 150-24, 60, 48, 40, "Done", NULL );
	//NO_UI!!!
	//NO_UI!!!	ui_wprintf_at( NameWindow, 10, 12,"Please enter a name for this mine:" );
	//NO_UI!!!	NameText = ui_add_gadget_inputbox( NameWindow, 10, 30, LEVEL_NAME_LEN, LEVEL_NAME_LEN, Current_level_name );
	//NO_UI!!!
	//NO_UI!!!	NameWindow->keyboard_focus_gadget = (UI_GADGET *)NameText;
	//NO_UI!!!	QuitButton->hotkey = KEY_ENTER;
	//NO_UI!!!
	//NO_UI!!!	ui_gadget_calc_keys(NameWindow);
	//NO_UI!!!
	//NO_UI!!!	while (!QuitButton->pressed && last_keypress!=KEY_ENTER) {
	//NO_UI!!!		ui_mega_process();
	//NO_UI!!!		ui_window_do_gadgets(NameWindow);
	//NO_UI!!!	}
	//NO_UI!!!
	//NO_UI!!!	strcpy( Current_level_name, NameText->text );
	//NO_UI!!!
	//NO_UI!!!	if ( NameWindow!=NULL )	{
	//NO_UI!!!		ui_close_window( NameWindow );
	//NO_UI!!!		NameWindow = NULL;
	//NO_UI!!!	}
	//NO_UI!!!

	newmenu_item m[2];

	m[0].type = NM_TYPE_TEXT; m[0].text = "Please enter a name for this mine:";
	m[1].type = NM_TYPE_INPUT; m[1].text = Current_level_name; m[1].text_len = LEVEL_NAME_LEN;

	newmenu_do(NULL, "Enter mine name", 2, m, NULL);

	return 0;
}
#endif


#ifdef EDITOR

static void gs_write_game_data(FILE* SaveFile)
{
	int i;
	file_write_short(SaveFile, game_fileinfo.fileinfo_signature);

	file_write_short(SaveFile, game_fileinfo.fileinfo_version);
	file_write_int(SaveFile, game_fileinfo.fileinfo_sizeof);
	for (i = 0; i < 15; i++)
		file_write_byte(SaveFile, game_fileinfo.mine_filename[i]);
	file_write_int(SaveFile, game_fileinfo.level);
	file_write_int(SaveFile, game_fileinfo.player_offset);				// Player info
	file_write_int(SaveFile, game_fileinfo.player_sizeof);
	file_write_int(SaveFile, game_fileinfo.object_offset);				// Object info
	file_write_int(SaveFile, game_fileinfo.object_howmany);
	file_write_int(SaveFile, game_fileinfo.object_sizeof);
	file_write_int(SaveFile, game_fileinfo.walls_offset);
	file_write_int(SaveFile, game_fileinfo.walls_howmany);
	file_write_int(SaveFile, game_fileinfo.walls_sizeof);
	file_write_int(SaveFile, game_fileinfo.doors_offset);
	file_write_int(SaveFile, game_fileinfo.doors_howmany);
	file_write_int(SaveFile, game_fileinfo.doors_sizeof);
	file_write_int(SaveFile, game_fileinfo.triggers_offset);
	file_write_int(SaveFile, game_fileinfo.triggers_howmany);
	file_write_int(SaveFile, game_fileinfo.triggers_sizeof);
	file_write_int(SaveFile, game_fileinfo.links_offset);
	file_write_int(SaveFile, game_fileinfo.links_howmany);
	file_write_int(SaveFile, game_fileinfo.links_sizeof);
	file_write_int(SaveFile, game_fileinfo.control_offset);
	file_write_int(SaveFile, game_fileinfo.control_howmany);
	file_write_int(SaveFile, game_fileinfo.control_sizeof);
	file_write_int(SaveFile, game_fileinfo.matcen_offset);
	file_write_int(SaveFile, game_fileinfo.matcen_howmany);
	file_write_int(SaveFile, game_fileinfo.matcen_sizeof);
}

int	Errors_in_mine;

// -----------------------------------------------------------------------------
// Save game
int save_game_data(FILE* SaveFile)
{
	int  player_offset, object_offset, walls_offset, doors_offset, triggers_offset, control_offset, matcen_offset; //, links_offset;
	int start_offset, end_offset;

	start_offset = ftell(SaveFile);

	//===================== SAVE FILE INFO ========================

	game_fileinfo.fileinfo_signature = 0x6705;
	game_fileinfo.fileinfo_version = GAME_VERSION;
	game_fileinfo.level = Current_level_num;
	game_fileinfo.fileinfo_sizeof = 119; //[ISB] canonical size rather than aligned size
	game_fileinfo.player_offset = -1;
	game_fileinfo.player_sizeof = sizeof(player);
	game_fileinfo.object_offset = -1;
	game_fileinfo.object_howmany = Highest_object_index + 1;
	game_fileinfo.object_sizeof = sizeof(object);
	game_fileinfo.walls_offset = -1;
	game_fileinfo.walls_howmany = Num_walls;
	game_fileinfo.walls_sizeof = sizeof(wall);
	game_fileinfo.doors_offset = -1;
	game_fileinfo.doors_howmany = Num_open_doors;
	game_fileinfo.doors_sizeof = sizeof(active_door);
	game_fileinfo.triggers_offset = -1;
	game_fileinfo.triggers_howmany = Num_triggers;
	game_fileinfo.triggers_sizeof = sizeof(trigger);
	game_fileinfo.control_offset = -1;
	game_fileinfo.control_howmany = 1;
	game_fileinfo.control_sizeof = sizeof(control_center_triggers);
	game_fileinfo.matcen_offset = -1;
	game_fileinfo.matcen_howmany = Num_robot_centers;
	game_fileinfo.matcen_sizeof = sizeof(matcen_info);

	// Write the fileinfo
	gs_write_game_data(SaveFile);

	// Write the mine name
	char c;
	for (int i = 0; i < LEVEL_NAME_LEN; i++) //[ISB] tbh this is overkill
	{
		c = Current_level_name[i];
		file_write_byte(SaveFile, c);
		if (c == 0) break;
	}

	file_write_short(SaveFile, (short)N_save_pof_names);
	for (int i = 0; i < N_polygon_models; i++)
	{
		fwrite(&Pof_names[i], 1, 13, SaveFile); //also overkill but I'm paranoid
	}

	//==================== SAVE PLAYER INFO ===========================

	player_offset = ftell(SaveFile);
	//fwrite(&Players[Player_num], sizeof(player), 1, SaveFile);
	//Well this isn't very useful
	write_player_file(&Players[Player_num], SaveFile);

	//==================== SAVE OBJECT INFO ===========================

	object_offset = ftell(SaveFile);
	{
		int i;
		for (i = 0; i < game_fileinfo.object_howmany; i++)
			write_object(&Objects[i], SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls_offset = ftell(SaveFile);
	for (int i = 0; i < game_fileinfo.walls_howmany; i++)
		write_wall(&Walls[i], SaveFile);

	//==================== SAVE DOOR INFO =============================

	doors_offset = ftell(SaveFile);
	for (int i = 0; i < game_fileinfo.doors_howmany; i++)
		write_active_door(&ActiveDoors[i], SaveFile);

	//==================== SAVE TRIGGER INFO =============================

	triggers_offset = ftell(SaveFile);
	for (int i = 0; i < game_fileinfo.triggers_howmany; i++)
		write_trigger(&Triggers[i], SaveFile);

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control_offset = ftell(SaveFile);
	write_reactor_triggers(&ControlCenterTriggers, SaveFile);

	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen_offset = ftell(SaveFile);
	// mprintf((0, "Writing %i materialization centers\n", game_fileinfo.matcen_howmany));
	for (int i = 0; i < game_fileinfo.matcen_howmany; i++)
		write_matcen(&RobotCenters[i], SaveFile);

	//============= REWRITE FILE INFO, TO SAVE OFFSETS ===============

	// Update the offset fields
	game_fileinfo.player_offset = player_offset;
	game_fileinfo.object_offset = object_offset;
	game_fileinfo.walls_offset = walls_offset;
	game_fileinfo.doors_offset = doors_offset;
	game_fileinfo.triggers_offset = triggers_offset;
	game_fileinfo.control_offset = control_offset;
	game_fileinfo.matcen_offset = matcen_offset;


	end_offset = ftell(SaveFile);

	// Write the fileinfo
	fseek(SaveFile, start_offset, SEEK_SET);  // Move to TOF
	gs_write_game_data(SaveFile);

	// Go back to end of data
	fseek(SaveFile, end_offset, SEEK_SET);

	return 0;
}

int save_mine_data(FILE* SaveFile);

// -----------------------------------------------------------------------------
// Save game
int save_level_sub(char* filename, int compiled_version)
{
	FILE* SaveFile;
	char temp_filename[128];
	int sig = 'PLVL', version = LEVEL_FILE_VERSION;
	int minedata_offset, gamedata_offset, hostagetext_offset;

	minedata_offset = gamedata_offset = hostagetext_offset = 0; //[ISB] Avoid CRT runtime error with uninitialized variables.

	if (!compiled_version) {
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				char  ErrorMessage[200];

				sprintf(ErrorMessage, "Warning: %i errors in this mine!\n", Errors_in_mine);
				stop_time();
				gr_palette_load(gr_palette);

				if (nm_messagebox(NULL, 2, "Cancel Save", "Save", ErrorMessage) != 1) {
					start_time();
					return 1;
				}
				start_time();
			}
			else
				mprintf((1, "Error: %i errors in this mine.  See the 'txm' file.\n", Errors_in_mine));
		}
		convert_name_to_LVL(temp_filename, filename);
	}
	else {
		convert_name_to_CDL(temp_filename, filename);
	}

	SaveFile = fopen(temp_filename, "wb");
	if (!SaveFile)
	{
		char ErrorMessage[256];

		char fname[20];
		_splitpath(temp_filename, NULL, NULL, fname, NULL);

		sprintf(ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n"
			, temp_filename, fname);
		stop_time();
		gr_palette_load(gr_palette);
		nm_messagebox(NULL, 1, "Ok", ErrorMessage);
		start_time();
		return 1;
	}

	if (Current_level_name[0] == 0)
		strcpy(Current_level_name, "Untitled");

	clear_transient_objects(1);		//1 means clear proximity bombs

	compress_objects();		//after this, Highest_object_index == num objects

	//make sure player is in a segment
	if (update_object_seg(&Objects[Players[0].objnum]) == 0) {
		if (ConsoleObject->segnum > Highest_segment_index)
			ConsoleObject->segnum = 0;
		compute_segment_center(&ConsoleObject->pos, &(Segments[ConsoleObject->segnum]));
	}

	fix_object_segs();

	//Write the header

	gs_write_int(sig, SaveFile);
	gs_write_int(version, SaveFile);

	//save placeholders
	gs_write_int(minedata_offset, SaveFile);
	gs_write_int(gamedata_offset, SaveFile);
	gs_write_int(hostagetext_offset, SaveFile);

	//Now write the damn data

	minedata_offset = ftell(SaveFile);
	if (!compiled_version)
		save_mine_data(SaveFile);
	else
		save_mine_data_compiled(SaveFile);
	gamedata_offset = ftell(SaveFile);
	save_game_data(SaveFile);
	hostagetext_offset = ftell(SaveFile);

#ifdef HOSTAGE_FACES
	save_hostage_data(SaveFile);
#endif

	fseek(SaveFile, sizeof(sig) + sizeof(version), SEEK_SET);
	gs_write_int(minedata_offset, SaveFile);
	gs_write_int(gamedata_offset, SaveFile);
	gs_write_int(hostagetext_offset, SaveFile);

	//==================== CLOSE THE FILE =============================
	fclose(SaveFile);

	if (!compiled_version) 
	{
		if (Function_mode == FMODE_EDITOR)
			editor_status("Saved mine %s, \"%s\"", filename, Current_level_name);
	}

	return 0;

}

int save_level(char* filename)
{
	int r1;

	// Save normal version...
	r1 = save_level_sub(filename, 0);

	// Save compiled version...
	save_level_sub(filename, 1);

	return r1;
}


#ifdef HOSTAGE_FACES
void save_hostage_data(FILE* fp)
{
	int i, num_hostages = 0;

	// Find number of hostages in mine...
	for (i = 0; i <= Highest_object_index; i++) {
		int num;
		if (Objects[i].type == OBJ_HOSTAGE) {
			num = Objects[i].id;
#ifndef SHAREWARE
			if (num < 0 || num >= MAX_HOSTAGES || Hostage_face_clip[Hostages[num].vclip_num].num_frames <= 0)
				num = 0;
#else
			num = 0;
#endif
			if (num + 1 > num_hostages)
				num_hostages = num + 1;
		}
	}

	gs_write_int(HOSTAGE_DATA_VERSION, fp);

	for (i = 0; i < num_hostages; i++) {
		gs_write_int(Hostages[i].vclip_num, fp);
		fputs(Hostages[i].text, fp);
		fputc('\n', fp);		//fgets wants a newline
	}
}
#endif	//HOSTAGE_FACES

#endif	//EDITOR

#ifndef NDEBUG
void dump_mine_info(void)
{
	int	segnum, sidenum;
	fix	min_u, max_u, min_v, max_v, min_l, max_l, max_sl;

	min_u = F1_0 * 1000;
	min_v = min_u;
	min_l = min_u;

	max_u = -min_u;
	max_v = max_u;
	max_l = max_u;

	max_sl = 0;

	for (segnum = 0; segnum <= Highest_segment_index; segnum++)
	{
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) 
		{
			int	vertnum;
			side* sidep = &Segments[segnum].sides[sidenum];

			if (Segments[segnum].static_light > max_sl)
				max_sl = Segments[segnum].static_light;

			for (vertnum = 0; vertnum < 4; vertnum++) 
			{
				if (sidep->uvls[vertnum].u < min_u)
					min_u = sidep->uvls[vertnum].u;
				else if (sidep->uvls[vertnum].u > max_u)
					max_u = sidep->uvls[vertnum].u;

				if (sidep->uvls[vertnum].v < min_v)
					min_v = sidep->uvls[vertnum].v;
				else if (sidep->uvls[vertnum].v > max_v)
					max_v = sidep->uvls[vertnum].v;

				if (sidep->uvls[vertnum].l < min_l)
					min_l = sidep->uvls[vertnum].l;
				else if (sidep->uvls[vertnum].l > max_l)
					max_l = sidep->uvls[vertnum].l;
			}

		}
	}

	//	mprintf((0, "Smallest uvl = %7.3f %7.3f %7.3f.  Largest uvl = %7.3f %7.3f %7.3f\n", f2fl(min_u), f2fl(min_v), f2fl(min_l), f2fl(max_u), f2fl(max_v), f2fl(max_l)));
	//	mprintf((0, "Static light maximum = %7.3f\n", f2fl(max_sl)));
	//	mprintf((0, "Number of walls: %i\n", Num_walls));

}

#endif

#ifdef HOSTAGE_FACES
void load_hostage_data(CFILE* fp, int do_read)
{
	int version, i, num, num_hostages;

	hostage_init_all();

	num_hostages = 0;

	// Find number of hostages in mine...
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_HOSTAGE) {
			num = Objects[i].id;
			if (num + 1 > num_hostages)
				num_hostages = num + 1;

			if (Hostages[num].objnum != -1) {		//slot already used
				num = hostage_get_next_slot();		//..so get new slot
				if (num + 1 > num_hostages)
					num_hostages = num + 1;
				Objects[i].id = num;
			}

			if (num > -1 && num < MAX_HOSTAGES) {
				Assert(Hostages[num].objnum == -1);		//make sure not used
				// -- Matt -- commented out by MK on 11/19/94, hit often in level 3, level 4.  Assert(Hostages[num].objnum == -1);		//make sure not used
				Hostages[num].objnum = i;
				Hostages[num].objsig = Objects[i].signature;
			}
		}
	}

	if (do_read) {
		version = read_int(fp);

		for (i = 0; i < num_hostages; i++) {

			Assert(Hostages[i].objnum != -1);		//make sure slot filled in

			Hostages[i].vclip_num = read_int(fp);

#ifndef SHAREWARE
			if (Hostages[i].vclip_num < 0 || Hostages[i].vclip_num >= MAX_HOSTAGES || Hostage_face_clip[Hostages[i].vclip_num].num_frames <= 0)
				Hostages[i].vclip_num = 0;

			Assert(Hostage_face_clip[Hostages[i].vclip_num].num_frames);
#endif

			cfgets(Hostages[i].text, HOSTAGE_MESSAGE_LEN, fp);

			if (Hostages[i].text[strlen(Hostages[i].text) - 1] == '\n')
				Hostages[i].text[strlen(Hostages[i].text) - 1] = 0;
		}
	}
	else
		for (i = 0; i < num_hostages; i++) {
			Assert(Hostages[i].objnum != -1);		//make sure slot filled in
			Hostages[i].vclip_num = 0;
		}

}
#endif	//HOSTAGE_FACES
