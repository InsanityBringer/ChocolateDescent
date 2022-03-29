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

#include "platform/mono.h"
#include "2d/gr.h"
#include "wall.h"
#include "switch.h"
#include "inferno.h"
#include "cfile/cfile.h"
#ifdef EDITOR
#include "editor\editor.h"
#endif
#include "segment.h"
#include "misc/error.h"
#include "gameseg.h"
#include "game.h"
#include "bm.h"
#include "vclip.h"
#include "player.h"
#include "gauges.h"
#include "stringtable.h"
#include "fireball.h"
#include "textures.h"
#include "sounds.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

wall Walls[MAX_WALLS];					// Master walls array
int Num_walls = 0;							// Number of walls

wclip WallAnims[MAX_WALL_ANIMS];		// Wall animations
int Num_wall_anims;
//--unused-- int walls_bm_num[MAX_WALL_ANIMS];

//door Doors[MAX_DOORS];					//	Master doors array

active_door ActiveDoors[MAX_DOORS];
int Num_open_doors;						// Number of open doors

//--unused-- grs_bitmap *wall_title_bms[MAX_WALL_ANIMS];

//#define BM_FLAG_TRANSPARENT			1
//#define BM_FLAG_SUPER_TRANSPARENT	2

#ifdef EDITOR
char	Wall_names[7][10] = {
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
};
#endif

// This function determines whether the current segment/side is transparent
//		1 = YES
//		0 = NO
int check_transparency(segment* seg, int side)
{
	if ((seg->sides[side].tmap_num2 & 0x3FFF) == 0) 
	{
		if (GameBitmaps[Textures[seg->sides[side].tmap_num].index].bm_flags & BM_FLAG_TRANSPARENT)
			return 1;
		else
			return 0;
	}

	if (GameBitmaps[Textures[seg->sides[side].tmap_num2 & 0x3FFF].index].bm_flags & BM_FLAG_SUPER_TRANSPARENT)
		return 1;
	else
		return 0;
}

//-----------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_FLY_FLAG				1
//		WID_RENDER_FLAG			2
//		WID_RENDPAST_FLAG			4
//	 Return values:
//		WID_WALL						2	// 0/1/0		wall	
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
//		WID_NO_WALL					5	//	1/0/1		no wall, can fly through
int wall_is_doorway(segment* seg, int side)
{
	int flags, type;
	int state;
	//--Covered by macro	// No child.
	//--Covered by macro	if (seg->children[side] == -1)
	//--Covered by macro		return WID_WALL;

	//--Covered by macro	if (seg->children[side] == -2)
	//--Covered by macro		return WID_EXTERNAL_FLAG;

	//--Covered by macro // No wall present.
	//--Covered by macro	if (seg->sides[side].wall_num == -1)
	//--Covered by macro		return WID_NO_WALL;

	Assert(seg - Segments >= 0 && seg - Segments <= Highest_segment_index);
	Assert(side >= 0 && side < 6);

	type = Walls[seg->sides[side].wall_num].type;
	flags = Walls[seg->sides[side].wall_num].flags;

	if (type == WALL_OPEN)
		return WID_NO_WALL;

	if (type == WALL_ILLUSION) 
	{
		if (Walls[seg->sides[side].wall_num].flags & WALL_ILLUSION_OFF)
			return WID_NO_WALL;
		else 
		{
			if (check_transparency(seg, side))
				return WID_TRANSILLUSORY_WALL;
			else
				return WID_ILLUSORY_WALL;
		}
	}

	if (type == WALL_BLASTABLE) 
	{
		if (flags & WALL_BLASTED)
			return WID_TRANSILLUSORY_WALL;

		if (check_transparency(seg, side))
			return WID_TRANSPARENT_WALL;
		else
			return WID_WALL;
	}

	if (flags & WALL_DOOR_OPENED)
		return WID_TRANSILLUSORY_WALL;

	state = Walls[seg->sides[side].wall_num].state;
	if ((type == WALL_DOOR) && (state == WALL_DOOR_OPENING))
		return WID_TRANSPARENT_WALL;

	// If none of the above flags are set, there is no doorway.
	if (check_transparency(seg, side))
		return WID_TRANSPARENT_WALL;
	else
		return WID_WALL; // There are children behind the door.
}

#ifdef EDITOR
//-----------------------------------------------------------------
// Initializes all the walls (in other words, no special walls)
void wall_init()
{
	int i;

	Num_walls = 0;
	for (i = 0; i < MAX_WALLS; i++) 
	{
		Walls[i].segnum = Walls[i].sidenum = -1;
		Walls[i].type = WALL_NORMAL;
		Walls[i].flags = 0;
		Walls[i].hps = 0;
		Walls[i].trigger = -1;
		Walls[i].clip_num = -1;
		Walls[i].linked_wall = -1;
	}
	Num_open_doors = 0;

}

//-----------------------------------------------------------------
// Initializes one wall.
void wall_reset(segment* seg, int side)
{
	int i;

	i = seg->sides[side].wall_num;

	if (i == -1) 
	{
		mprintf((0, "Resetting Illegal Wall\n"));
		return;
	}

	Walls[i].segnum = seg - Segments;
	Walls[i].sidenum = side;
	Walls[i].type = WALL_NORMAL;
	Walls[i].flags = 0;
	Walls[i].hps = 0;
	Walls[i].trigger = -1;
	Walls[i].clip_num = -1;
	Walls[i].linked_wall = -1;
}
#endif

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(segment* seg, int side, segment* csegp, int cside, int anim_num, int frame_num)
{
	wclip* anim = &WallAnims[anim_num];
	int tmap = anim->frames[frame_num];

	if (Newdemo_state == ND_STATE_PLAYBACK) return;

	if (anim->flags & WCF_TMAP1) 
	{
		seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = tmap;
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_wall_set_tmap_num1(seg - Segments, side, csegp - Segments, cside, tmap);
	}
	else 
	{
		Assert(tmap != 0 && seg->sides[side].tmap_num2 != 0);
		seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = tmap;
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_wall_set_tmap_num2(seg - Segments, side, csegp - Segments, cside, tmap);
	}
}


// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
void blast_blastable_wall(segment* seg, int side)
{
	int Connectside;
	segment* csegp;
	int a, n;

	Assert(seg->sides[side].wall_num != -1);

	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);

	kill_stuck_objects(seg->sides[side].wall_num);
	kill_stuck_objects(csegp->sides[Connectside].wall_num);

	a = Walls[seg->sides[side].wall_num].clip_num;
	n = WallAnims[a].num_frames;

	if (!(WallAnims[Walls[seg->sides[side].wall_num].clip_num].flags & WCF_EXPLODES))
		wall_set_tmap_num(seg, side, csegp, Connectside, a, n - 1);

	Walls[seg->sides[side].wall_num].flags |= WALL_BLASTED;
	Walls[csegp->sides[Connectside].wall_num].flags |= WALL_BLASTED;

	//if this is an exploding wall, explode it
	if (WallAnims[Walls[seg->sides[side].wall_num].clip_num].flags & WCF_EXPLODES)
		explode_wall(seg - Segments, side);
}


//-----------------------------------------------------------------
// Destroys a blastable wall.
void wall_destroy(segment* seg, int side)
{
	Assert(seg->sides[side].wall_num != -1);
	Assert(seg - Segments != 0);

	if (Walls[seg->sides[side].wall_num].type == WALL_BLASTABLE)
		blast_blastable_wall(seg, side);
	else
		Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons)) 
void wall_damage(segment* seg, int side, fix damage)
{
	int a, i, n;

	if (seg->sides[side].wall_num == -1) 
	{
		mprintf((0, "Damaging illegal wall\n"));
		return;
	}

	if (Walls[seg->sides[side].wall_num].type != WALL_BLASTABLE)
		return;

	if (!(Walls[seg->sides[side].wall_num].flags & WALL_BLASTED))
	{
		int Connectside;
		segment* csegp;

		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);

		Walls[seg->sides[side].wall_num].hps -= damage;
		Walls[csegp->sides[Connectside].wall_num].hps -= damage;

		a = Walls[seg->sides[side].wall_num].clip_num;
		n = WallAnims[a].num_frames;

		if (Walls[seg->sides[side].wall_num].hps < WALL_HPS * 1 / n) 
		{
			blast_blastable_wall(seg, side);
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_door_open(seg - Segments, side);
#endif
		}
		else
			for (i = 0; i < n; i++)
				if (Walls[seg->sides[side].wall_num].hps < WALL_HPS * (n - i) / n) 
				{
					wall_set_tmap_num(seg, side, csegp, Connectside, a, i);
				}
	}
}


//-----------------------------------------------------------------
// Opens a door 
void wall_open_door(segment* seg, int side)
{
	wall* w;
	active_door* d;
	int Connectside;
	segment* csegp;

	Assert(seg->sides[side].wall_num != -1); 	//Opening door on illegal wall

	w = &Walls[seg->sides[side].wall_num];

	//kill_stuck_objects(seg->sides[side].wall_num);

	if (w->state == WALL_DOOR_OPENING)		//already opening
		return;

	if (w->state == WALL_DOOR_WAITING)		//open, waiting to close
		return;

	if (w->state != WALL_DOOR_CLOSED) //reuse door
	{		
		int i;

		d = NULL;

		for (i = 0; i < Num_open_doors; i++) //find door
		{
			d = &ActiveDoors[i];

			if (d->front_wallnum[0] == w - Walls || d->back_wallnum[0] == w - Walls || (d->n_parts == 2 && (d->front_wallnum[1] == w - Walls || d->back_wallnum[1] == w - Walls)))
				break;
		}

		Assert(i < Num_open_doors);				//didn't find door!
		Assert(d != NULL); // Get John!

		d->time = WallAnims[w->clip_num].play_time - d->time;

		if (d->time < 0)
			d->time = 0;

	}
	else 
	{											//create new door
		d = &ActiveDoors[Num_open_doors];
		d->time = 0;
		Num_open_doors++;
		Assert(Num_open_doors < MAX_DOORS);
	}

	w->state = WALL_DOOR_OPENING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);

	Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_OPENING;

	//kill_stuck_objects(csegp->sides[Connectside].wall_num);

	d->front_wallnum[0] = seg->sides[side].wall_num;
	d->back_wallnum[0] = csegp->sides[Connectside].wall_num;

	Assert(seg - Segments != -1);

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING) 
	{
		newdemo_record_door_opening(seg - Segments, side);
	}
#endif

	if (w->linked_wall != -1) 
	{
		wall* w2;
		segment* seg2;

		w2 = &Walls[w->linked_wall];
		seg2 = &Segments[w2->segnum];

		Assert(w2->linked_wall == seg->sides[side].wall_num);
		//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED));

		w2->state = WALL_DOOR_OPENING;

		csegp = &Segments[seg2->children[w2->sidenum]];
		Connectside = find_connect_side(seg2, csegp);
		Assert(Connectside != -1);
		Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_OPENING;

		d->n_parts = 2;
		d->front_wallnum[1] = w->linked_wall;
		d->back_wallnum[1] = csegp->sides[Connectside].wall_num;
	}
	else
		d->n_parts = 1;


	if (Newdemo_state != ND_STATE_PLAYBACK)
	{
		// NOTE THE LINK TO ABOVE!!!!
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side);
		if (WallAnims[w->clip_num].open_sound > -1)
			digi_link_sound_to_pos(WallAnims[w->clip_num].open_sound, seg - Segments, side, &cp, 0, F1_0);

	}
}

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void wall_close_door(int door_num)
{
	int p;
	active_door* d;
	int i;

	d = &ActiveDoors[door_num];

	for (p = 0; p < d->n_parts; p++) 
	{
		wall* w;
		int Connectside, side;
		segment* csegp, * seg;

		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;

		Assert(seg->sides[side].wall_num != -1);		//Closing door on illegal wall

		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);

		Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSED;
		Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_CLOSED;

		wall_set_tmap_num(seg, side, csegp, Connectside, w->clip_num, 0);

	}

	for (i = door_num; i < Num_open_doors; i++)
		ActiveDoors[i] = ActiveDoors[i + 1];

	Num_open_doors--;

}


//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
void do_door_open(int door_num)
{
	int p;
	active_door* d;

	Assert(door_num != -1);		//Trying to do_door_open on illegal door

	d = &ActiveDoors[door_num];

	for (p = 0; p < d->n_parts; p++) 
	{
		wall* w;
		int Connectside, side;
		segment* csegp, * seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;

		w = &Walls[d->front_wallnum[p]];
		kill_stuck_objects(d->front_wallnum[p]);
		kill_stuck_objects(d->back_wallnum[p]);

		seg = &Segments[w->segnum];
		side = w->sidenum;

		Assert(seg->sides[side].wall_num != -1);		//Trying to do_door_open on illegal wall

		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);

		d->time += FrameTime;

		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;

		one_frame = time_total / n;

		i = time_elapsed / one_frame;

		if (i < n)
			wall_set_tmap_num(seg, side, csegp, Connectside, w->clip_num, i);

		if (i > n / 2) 
		{
			Walls[seg->sides[side].wall_num].flags |= WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags |= WALL_DOOR_OPENED;
		}

		if (i >= n - 1) 
		{
			wall_set_tmap_num(seg, side, csegp, Connectside, w->clip_num, n - 1);

			// If our door is not automatic just remove it from the list.
			if (!(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO)) 
			{
				for (i = door_num; i < Num_open_doors; i++)
					ActiveDoors[i] = ActiveDoors[i + 1];
				Num_open_doors--;
			}
			else 
			{

				Walls[seg->sides[side].wall_num].state = WALL_DOOR_WAITING;
				Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_WAITING;

				ActiveDoors[Num_open_doors].time = 0;	//counts up
			}
		}

	}

}

int check_poke(int objnum, int segnum, int side)
{
	object* obj = &Objects[objnum];

	//note: don't let objects with zero size block door

	if (obj->size && get_seg_masks(&obj->pos, segnum, obj->size).sidemask & (1 << side))
		return 1;		//pokes through side!
	else
		return 0;		//does not!

}


//-----------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
void do_door_close(int door_num)
{
	int p;
	active_door* d;
	wall* w;

	Assert(door_num != -1);		//Trying to do_door_open on illegal door

	d = &ActiveDoors[door_num];

	w = &Walls[d->front_wallnum[0]];

	//check for objects in doorway before closing
	if (w->flags & WALL_DOOR_AUTO)
		for (p = 0; p < d->n_parts; p++) 
		{
			int Connectside, side;
			segment* csegp, * seg;
			int objnum;

			seg = &Segments[w->segnum];
			side = w->sidenum;

			csegp = &Segments[seg->children[side]];
			Connectside = find_connect_side(seg, csegp);
			Assert(Connectside != -1);

			//go through each object in each of two segments, and see if
			//it pokes into the connecting seg

			for (objnum = seg->objects; objnum != -1; objnum = Objects[objnum].next)
				if (check_poke(objnum, seg - Segments, side))
					return;		//abort!

			for (objnum = csegp->objects; objnum != -1; objnum = Objects[objnum].next)
				if (check_poke(objnum, csegp - Segments, Connectside))
					return;		//abort!
		}

	for (p = 0; p < d->n_parts; p++) 
	{
		wall* w;
		int Connectside, side;
		segment* csegp, * seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;

		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;

		if (seg->sides[side].wall_num == -1) 
		{
			mprintf((0, "Trying to do_door_close on Illegal wall\n"));
			return;
		}

		//if here, must be auto door
		Assert(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO);

		// Otherwise, close it.
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);


		if (Newdemo_state != ND_STATE_PLAYBACK)
			// NOTE THE LINK TO ABOVE!!
			if (p == 0)	//only play one sound for linked doors
				if (d->time == 0) //first time
				{
					vms_vector cp;
					compute_center_point_on_side(&cp, seg, side);
					if (WallAnims[w->clip_num].close_sound > -1)
						digi_link_sound_to_pos(WallAnims[Walls[seg->sides[side].wall_num].clip_num].close_sound, seg - Segments, side, &cp, 0, F1_0);
				}

		d->time += FrameTime;

		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;

		one_frame = time_total / n;

		i = n - time_elapsed / one_frame - 1;

		if (i < n / 2) 
		{
			Walls[seg->sides[side].wall_num].flags &= ~WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags &= ~WALL_DOOR_OPENED;
		}

		// Animate door.
		if (i > 0) 
		{
			wall_set_tmap_num(seg, side, csegp, Connectside, w->clip_num, i);

			Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSING;
			Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_CLOSING;

			ActiveDoors[Num_open_doors].time = 0;		//counts up

		}
		else
			wall_close_door(door_num);
	}
}


//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_off(segment* seg, int side)
{
	segment* csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	if (seg->sides[side].wall_num == -1) 
	{
		mprintf((0, "Trying to shut off illusion illegal wall\n"));
		return;
	}

	Walls[seg->sides[side].wall_num].flags |= WALL_ILLUSION_OFF;
	Walls[csegp->sides[cside].wall_num].flags |= WALL_ILLUSION_OFF;
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_on(segment* seg, int side)
{
	segment* csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	if (seg->sides[side].wall_num == -1) 
	{
		mprintf((0, "Trying to turn on illusion illegal wall\n"));
		return;
	}

	Walls[seg->sides[side].wall_num].flags &= ~WALL_ILLUSION_OFF;
	Walls[csegp->sides[cside].wall_num].flags &= ~WALL_ILLUSION_OFF;
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
int special_boss_opening_allowed(int segnum, int sidenum)
{
	if (Game_mode & GM_MULTI)
		return (Current_level_num == BOSS_LOCKED_DOOR_LEVEL) && (segnum == BOSS_LOCKED_DOOR_SEG) && (sidenum == BOSS_LOCKED_DOOR_SIDE);
	else
		return 0;
}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the object that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
int wall_hit_process(segment* seg, int side, fix damage, int playernum, object* obj)
{
	wall* w;
	fix	show_message;

	Assert(seg - Segments != -1);

	// If it is not a "wall" then just return.
	if (seg->sides[side].wall_num < 0)
		return WHP_NOT_SPECIAL;

	w = &Walls[seg->sides[side].wall_num];

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_wall_hit_process(seg - Segments, side, damage, playernum);

	if (w->type == WALL_BLASTABLE) 
	{
		wall_damage(seg, side, damage);
		return WHP_BLASTABLE;
	}

	if (playernum != Player_num)	//return if was robot fire
		return WHP_NOT_SPECIAL;

	Assert(playernum > -1);

	//	Determine whether player is facing door he hit.  If not, don't say negative
	//	messages because he probably didn't intentionally hit the door.
	if (obj->type == OBJ_PLAYER)
		show_message = (vm_vec_dot(&obj->orient.fvec, &obj->mtype.phys_info.velocity) > 0);
	else
		show_message = 1;

	if (w->keys == KEY_BLUE)
		if (!(Players[playernum].flags & PLAYER_FLAGS_BLUE_KEY)) 
		{
			if (playernum == Player_num)
				if (show_message)
					HUD_init_message("%s %s", TXT_BLUE, TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->keys == KEY_RED)
		if (!(Players[playernum].flags & PLAYER_FLAGS_RED_KEY)) 
		{
			if (playernum == Player_num)
				if (show_message)
					HUD_init_message("%s %s", TXT_RED, TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->keys == KEY_GOLD)
		if (!(Players[playernum].flags & PLAYER_FLAGS_GOLD_KEY)) 
		{
			if (playernum == Player_num)
				if (show_message)
					HUD_init_message("%s %s", TXT_YELLOW, TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->type == WALL_DOOR)
		if ((w->flags & WALL_DOOR_LOCKED) && !(special_boss_opening_allowed(seg - Segments, side))) 
		{
			if (playernum == Player_num)
				if (show_message)
					HUD_init_message(TXT_CANT_OPEN_DOOR);
			return WHP_NO_KEY;
		}
		else 
		{
			if (w->state != WALL_DOOR_OPENING)
			{
				wall_open_door(seg, side);
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_door_open(seg - Segments, side);
#endif
			}
			return WHP_DOOR;

		}

	return WHP_NOT_SPECIAL;		//default is treat like normal wall
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void wall_toggle(segment* seg, int side)
{
	int wall_num;

	Assert(seg - Segments <= Highest_segment_index);
	Assert(side < MAX_SIDES_PER_SEGMENT);

	wall_num = seg->sides[side].wall_num;

	if (wall_num == -1) 
	{
		mprintf((0, "Illegal wall_toggle\n"));
		return;
	}

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_wall_toggle(seg - Segments, side);

	if (Walls[wall_num].type == WALL_BLASTABLE)
		wall_destroy(seg, side);

	if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].state == WALL_DOOR_CLOSED))
		wall_open_door(seg, side);

}


//-----------------------------------------------------------------
// Tidy up Walls array for load/save purposes.
void reset_walls()
{
	int i;

	if (Num_walls < 0) 
	{
		mprintf((0, "Illegal Num_walls\n"));
		return;
	}

	for (i = Num_walls; i < MAX_WALLS; i++) 
	{
		Walls[i].type = WALL_NORMAL;
		Walls[i].flags = 0;
		Walls[i].hps = 0;
		Walls[i].trigger = -1;
		Walls[i].clip_num = -1;
	}
}

void wall_frame_process()
{
	int i;

	for (i = 0; i < Num_open_doors; i++) 
	{
		active_door* d;
		wall* w;

		d = &ActiveDoors[i];
		w = &Walls[d->front_wallnum[0]];

		if (w->state == WALL_DOOR_OPENING)
			do_door_open(i);
		else if (w->state == WALL_DOOR_CLOSING)
			do_door_close(i);
		else if (w->state == WALL_DOOR_WAITING) 
		{
			d->time += FrameTime;
			if (d->time > DOOR_WAIT_TIME) 
			{
				w->state = WALL_DOOR_CLOSING;
				d->time = 0;
			}
		}
	}
}

int	Num_stuck_objects = 0;

stuckobj	Stuck_objects[MAX_STUCK_OBJECTS];

//	An object got stuck in a door (like a flare).
//	Add global entry.
void add_stuck_object(object* objp, int segnum, int sidenum)
{
	int	i;
	int	wallnum;

	wallnum = Segments[segnum].sides[sidenum].wall_num;

	if (wallnum != -1) {
		if (Walls[wallnum].flags & WALL_BLASTED)
			objp->flags |= OF_SHOULD_BE_DEAD;

		for (i = 0; i < MAX_STUCK_OBJECTS; i++) 
		{
			if (Stuck_objects[i].wallnum == -1) 
			{
				Stuck_objects[i].wallnum = wallnum;
				Stuck_objects[i].objnum = objp - Objects;
				Stuck_objects[i].signature = objp->signature;
				// mprintf((0, "Added wall %i at index %i\n", wallnum, i));
				Num_stuck_objects++;
				break;
			}
		}
		if (i == MAX_STUCK_OBJECTS)
			mprintf((1, "Warning: Unable to add object %i which got stuck in wall %i to Stuck_objects\n", objp - Objects, wallnum));
	}
}

//	--------------------------------------------------------------------------------------------------
//	Look at the list of stuck objects, clean up in case an object has gone away, but not been removed here.
//	Removes up to one/frame.
void remove_obsolete_stuck_objects(void)
{
	int	objnum;

	objnum = FrameCount % MAX_STUCK_OBJECTS;

	if (Stuck_objects[objnum].wallnum != -1)
		if ((Stuck_objects[objnum].wallnum == 0) || (Objects[Stuck_objects[objnum].objnum].signature != Stuck_objects[objnum].signature)) 
		{
			Num_stuck_objects--;
			Stuck_objects[objnum].wallnum = -1;
			// -- mprintf((0, "Removing obsolete stuck object %i\n", Stuck_objects[objnum].objnum));
		}

}

//	----------------------------------------------------------------------------------------------------
//	Door with wall index wallnum is opening, kill all objects stuck in it.
void kill_stuck_objects(int wallnum)
{
	int	i;

	if (Num_stuck_objects == 0)
		return;

	Num_stuck_objects = 0;

	for (i = 0; i < MAX_STUCK_OBJECTS; i++)
		if (Stuck_objects[i].wallnum == wallnum) 
		{
			if (Objects[Stuck_objects[i].objnum].type == OBJ_WEAPON) 
			{
				Objects[Stuck_objects[i].objnum].lifeleft = F1_0 / 4;
				mprintf((0, "Removing object %i from wall %i\n", Stuck_objects[i].objnum, wallnum));
			}
			else
				mprintf((0, "Warning: Stuck object of type %i, expected to be of type %i, see wall.c\n", Objects[Stuck_objects[i].objnum].type, OBJ_WEAPON));
			// Int3();	//	What?  This looks bad.  Object is not a weapon and it is stuck in a wall!
			Stuck_objects[i].wallnum = -1;
		}
		else if (Stuck_objects[i].wallnum != -1)
			Num_stuck_objects++;
}

void read_wall(wall* nwall, FILE* fp)
{
	nwall->segnum = file_read_int(fp);
	nwall->sidenum = file_read_int(fp);
	nwall->hps = file_read_int(fp);
	nwall->linked_wall = file_read_int(fp);
	nwall->type = file_read_byte(fp);
	nwall->flags = file_read_byte(fp);
	nwall->state = file_read_byte(fp);
	nwall->trigger = file_read_byte(fp);
	nwall->clip_num = file_read_byte(fp);
	nwall->keys = file_read_byte(fp);
	nwall->pad = file_read_short(fp);
}

void read_active_door(active_door* door, FILE* fp)
{
	door->n_parts = file_read_int(fp);
	door->front_wallnum[0] = file_read_short(fp);
	door->front_wallnum[1] = file_read_short(fp);
	door->back_wallnum[0] = file_read_short(fp);
	door->back_wallnum[1] = file_read_short(fp);
	door->time = file_read_int(fp);
}

void read_trigger(trigger* trig, FILE* fp)
{
	int j;
	trig->type = file_read_byte(fp);
	trig->flags = file_read_short(fp);
	trig->value = file_read_int(fp);
	trig->time = file_read_int(fp);
	trig->link_num = file_read_byte(fp);
	trig->num_links = file_read_short(fp);
	for (j = 0; j < MAX_WALLS_PER_LINK; j++)
		trig->seg[j] = file_read_short(fp);
	for (j = 0; j < MAX_WALLS_PER_LINK; j++)
		trig->side[j] = file_read_short(fp);
}

void write_wall(wall* nwall, FILE* fp)
{
	file_write_int(fp, nwall->segnum);
	file_write_int(fp, nwall->sidenum);
	file_write_int(fp, nwall->hps);
	file_write_int(fp, nwall->linked_wall);
	file_write_byte(fp, nwall->type);
	file_write_byte(fp, nwall->flags);
	file_write_byte(fp, nwall->state);
	file_write_byte(fp, nwall->trigger);
	file_write_byte(fp, nwall->clip_num);
	file_write_byte(fp, nwall->keys);
	file_write_short(fp, nwall->pad);
}

void write_active_door(active_door* door, FILE* fp)
{
	file_write_int(fp, door->n_parts);
	file_write_short(fp, door->front_wallnum[0]);
	file_write_short(fp, door->front_wallnum[1]);
	file_write_short(fp, door->back_wallnum[0]);
	file_write_short(fp, door->back_wallnum[1]);
	file_write_int(fp, door->time);
}

void write_trigger(trigger* trig, FILE* fp)
{
	int j;
	file_write_byte(fp, trig->type);
	file_write_short(fp, trig->flags);
	file_write_int(fp, trig->value);
	file_write_int(fp, trig->time);
	file_write_byte(fp, trig->link_num);
	file_write_short(fp, trig->num_links);
	for (j = 0; j < MAX_WALLS_PER_LINK; j++)
		file_write_short(fp, trig->seg[j]);
	for (j = 0; j < MAX_WALLS_PER_LINK; j++)
		file_write_short(fp, trig->side[j]);
}