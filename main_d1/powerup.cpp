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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "3d/3d.h"
#include "platform/mono.h"
#include "inferno.h"
#include "object.h"
#include "game.h"
#include "fireball.h"
#include "powerup.h"
#include "gauges.h"
#include "sounds.h"
#include "player.h"
#include "wall.h"
#include "text.h"
#include "weapon.h"
#include "laser.h"
#include "scores.h"
#include "multi.h"
#include "newdemo.h"

#ifdef EDITOR
#include "2d/gr.h"	//	for powerup outline drawing
#include "editor\editor.h"
#endif

#define	ENERGY_MAX	i2f(200)
#define	SHIELD_MAX	i2f(200)

int N_powerup_types = 0;
powerup_type_info Powerup_info[MAX_POWERUP_TYPES];

//process this powerup for this frame
void do_powerup_frame(object* obj)
{
	vclip_info_t* vci = &obj->rtype.vclip_info;
	vclip* vc = &Vclip[vci->vclip_num];

	vci->frametime -= FrameTime;

	while (vci->frametime < 0) {

		vci->frametime += vc->frame_time;

		vci->framenum++;
		if (vci->framenum >= vc->num_frames)
			vci->framenum = 0;
	}

	if (obj->lifeleft <= 0) {
		object_create_explosion(obj->segnum, &obj->pos, fl2f(3.5), VCLIP_POWERUP_DISAPPEARANCE);

		if (Vclip[VCLIP_POWERUP_DISAPPEARANCE].sound_num > -1)
			digi_link_sound_to_object(Vclip[VCLIP_POWERUP_DISAPPEARANCE].sound_num, obj - Objects, 0, F1_0);
	}
}

#ifdef EDITOR
//extern fix blob_vertices[];
extern grs_point blob_vertices[]; //for mac code

//	blob_vertices has 3 vertices in it, 4th must be computed
void draw_blob_outline(void)
{
	fix	v3x, v3y;

	//v3x = blob_vertices[4] - blob_vertices[2] + blob_vertices[0];
	v3x = blob_vertices[2].x - blob_vertices[1].x + blob_vertices[0].x;
	//v3y = blob_vertices[5] - blob_vertices[3] + blob_vertices[1];
	v3y = blob_vertices[2].y - blob_vertices[1].y + blob_vertices[0].y;

	gr_setcolor(BM_XRGB(63, 63, 63));

	mprintf((0, "[%7.3f %7.3f]  [%7.3f %7.3f]  [%7.3f %7.3f]\n", f2fl(blob_vertices[0].x), f2fl(blob_vertices[0].y), f2fl(blob_vertices[1].x), f2fl(blob_vertices[1].y), f2fl(blob_vertices[2].x), f2fl(blob_vertices[2].y)));

	gr_line(blob_vertices[0].x, blob_vertices[0].y, blob_vertices[1].x, blob_vertices[1].y);
	gr_line(blob_vertices[1].x, blob_vertices[1].y, blob_vertices[2].x, blob_vertices[2].y);
	gr_line(blob_vertices[2].x, blob_vertices[2].y, v3x, v3y);

	gr_line(v3x, v3y, blob_vertices[0].x, blob_vertices[0].y);
}
#endif

void draw_powerup(object* obj)
{
#ifdef EDITOR
	blob_vertices[0].x = 0x80000;
#endif

	draw_object_blob(obj, Vclip[obj->rtype.vclip_info.vclip_num].frames[obj->rtype.vclip_info.framenum]);

#ifdef EDITOR
	if ((Function_mode == FMODE_EDITOR) && (Cur_object_index == obj - Objects))
		if (blob_vertices[0].x != 0x80000)
			draw_blob_outline();
#endif

}

//void mprintf_powerup_info(void)
//{
//	int		i;
//mprintf((0, "Powerup: %s\n", text));
//for (i=0; i<5; i++) {
//	char	has_text[12];
//
//	if (Players[Player_num].primary_weapon_flags & (1 << i))
//		strcpy(has_text,"PRESENT");
//	else
//		strcpy(has_text,"NOPE   ");
//	has_text[8] = 0;
//
//	mprintf((0, "Weapon %i = %s, ammo = %6i, name = %s\n", i, has_text, Players[Player_num].primary_ammo[i], Primary_weapon_names[i]));
//}
//
//for (i=0; i<5; i++) {
//	char	has_text[12];
//
//	if (Players[Player_num].secondary_weapon_flags & (1 << i))
//		strcpy(has_text,"PRESENT");
//	else
//		strcpy(has_text,"NOPE   ");
//
//	mprintf((0, "Weapon %i = %s, ammo = %6i, name = %s\n", i, has_text, Players[Player_num].secondary_ammo[i], Secondary_weapon_names[i]));
//}
//}

void powerup_basic(int redadd, int greenadd, int blueadd, int score, const char* format, ...)
{
	char		text[120];
	va_list	args;

	va_start(args, format);
	vsprintf(text, format, args);
	va_end(args);

	PALETTE_FLASH_ADD(redadd, greenadd, blueadd);

	HUD_init_message(text);

	//mprintf_powerup_info();

	add_points_to_score(score);

}

//#ifndef RELEASE
//	Give the megawow powerup!
void do_megawow_powerup(int quantity)
{
	int i;

	powerup_basic(30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
#ifndef SHAREWARE
	Players[Player_num].primary_weapon_flags = 0xff;
	Players[Player_num].secondary_weapon_flags = 0xff;
#else
	Players[Player_num].primary_weapon_flags = 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
	Players[Player_num].secondary_weapon_flags = 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);
#endif
	for (i = 0; i < 3; i++)
		Players[Player_num].primary_ammo[i] = 200;

	for (i = 0; i < 3; i++)
		Players[Player_num].secondary_ammo[i] = quantity;

#ifndef SHAREWARE
	for (i = 3; i < 5; i++)
		Players[Player_num].primary_ammo[i] = 200;

	for (i = 3; i < 5; i++)
		Players[Player_num].secondary_ammo[i] = quantity / 5;
#endif

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

	Players[Player_num].energy = F1_0 * 200;
	Players[Player_num].shields = F1_0 * 200;
	Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
	Players[Player_num].laser_level = MAX_LASER_LEVEL;
	update_laser_weapon_info();

}
//#endif

int pick_up_energy(void)
{
	int	used = 0;

	if (Players[Player_num].energy < ENERGY_MAX) {
		Players[Player_num].energy += 3 * F1_0 + 3 * F1_0 * (NDL - Difficulty_level);
		if (Players[Player_num].energy > ENERGY_MAX)
			Players[Player_num].energy = ENERGY_MAX;
		powerup_basic(15, 15, 7, ENERGY_SCORE, "%s %s %d", TXT_ENERGY, TXT_BOOSTED_TO, f2ir(Players[Player_num].energy));
		used = 1;
	}
	else
		HUD_init_message(TXT_MAXED_OUT, TXT_ENERGY);

	return used;
}

int pick_up_vulcan_ammo(void)
{
	int	used = 0;

	int	pwsave = Primary_weapon;		// Ugh, save selected primary weapon around the picking up of the ammo.  I apologize for this code.  Matthew A. Toschlog
	if (pick_up_ammo(CLASS_PRIMARY, VULCAN_INDEX, VULCAN_AMMO_AMOUNT)) 
	{
		powerup_basic(7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
		used = 1;
	}
	else 
	{
		HUD_init_message("%s %d %s!", TXT_ALREADY_HAVE, f2i(VULCAN_AMMO_SCALE * Primary_ammo_max[VULCAN_INDEX]), TXT_VULCAN_ROUNDS);
		used = 0;
	}
	Primary_weapon = pwsave;

	return used;
}

//	returns true if powerup consumed
int do_powerup(object* obj)
{
	int used = 0;
	int vulcan_ammo_to_add_with_cannon;

	if ((Player_is_dead) || (ConsoleObject->type == OBJ_GHOST))
		return 0;

	switch (obj->id) 
	{
	case POW_EXTRA_LIFE:
		Players[Player_num].lives++;
		powerup_basic(15, 15, 15, 0, TXT_EXTRA_LIFE);
		used = 1;
		break;
	case POW_ENERGY:
		used = pick_up_energy();
		break;
	case POW_SHIELD_BOOST:
		if (Players[Player_num].shields < SHIELD_MAX) 
		{
			Players[Player_num].shields += 3 * F1_0 + 3 * F1_0 * (NDL - Difficulty_level);
			if (Players[Player_num].shields > SHIELD_MAX)
				Players[Player_num].shields = SHIELD_MAX;
			powerup_basic(0, 0, 15, SHIELD_SCORE, "%s %s %d", TXT_SHIELD, TXT_BOOSTED_TO, f2ir(Players[Player_num].shields));
			used = 1;
		}
		else
			HUD_init_message(TXT_MAXED_OUT, TXT_SHIELD);
		break;
	case POW_LASER:
		if (Players[Player_num].laser_level >= MAX_LASER_LEVEL) 
		{
			Players[Player_num].laser_level = MAX_LASER_LEVEL;
			HUD_init_message(TXT_MAXED_OUT, TXT_LASER);
		}
		else 
		{
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_laser_level(Players[Player_num].laser_level, Players[Player_num].laser_level + 1);
			Players[Player_num].laser_level++;
			powerup_basic(10, 0, 10, LASER_SCORE, "%s %s %d", TXT_LASER, TXT_BOOSTED_TO, Players[Player_num].laser_level + 1);
			update_laser_weapon_info();
			used = 1;
		}
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_energy();
		break;
	case POW_MISSILE_1:
		used = pick_up_secondary(CONCUSSION_INDEX, 1);
		break;
	case POW_MISSILE_4:
		used = pick_up_secondary(CONCUSSION_INDEX, 4);
		break;

	case POW_KEY_BLUE:
		if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY)
			break;
#ifdef NETWORK
		multi_send_play_sound(Powerup_info[obj->id].hit_sound, F1_0);
#endif
		digi_play_sample(Powerup_info[obj->id].hit_sound, F1_0);
		Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY;
		powerup_basic(0, 0, 15, KEY_SCORE, "%s %s", TXT_BLUE, TXT_ACCESS_GRANTED);
		if (Game_mode & GM_MULTI)
			used = 0;
		else
			used = 1;
		break;
	case POW_KEY_RED:
		if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)
			break;
#ifdef NETWORK
		multi_send_play_sound(Powerup_info[obj->id].hit_sound, F1_0);
#endif
		digi_play_sample(Powerup_info[obj->id].hit_sound, F1_0);
		Players[Player_num].flags |= PLAYER_FLAGS_RED_KEY;
		powerup_basic(15, 0, 0, KEY_SCORE, "%s %s", TXT_RED, TXT_ACCESS_GRANTED);
		if (Game_mode & GM_MULTI)
			used = 0;
		else
			used = 1;
		break;
	case POW_KEY_GOLD:
		if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)
			break;
#ifdef NETWORK
		multi_send_play_sound(Powerup_info[obj->id].hit_sound, F1_0);
#endif
		digi_play_sample(Powerup_info[obj->id].hit_sound, F1_0);
		Players[Player_num].flags |= PLAYER_FLAGS_GOLD_KEY;
		powerup_basic(15, 15, 7, KEY_SCORE, "%s %s", TXT_YELLOW, TXT_ACCESS_GRANTED);
		if (Game_mode & GM_MULTI)
			used = 0;
		else
			used = 1;
		break;
	case POW_QUAD_FIRE:
		if (!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)) 
		{
			Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
			powerup_basic(15, 15, 7, QUAD_FIRE_SCORE, "%s!", TXT_QUAD_LASERS);
			update_laser_weapon_info();
			used = 1;
		}
		else
			HUD_init_message("%s %s!", TXT_ALREADY_HAVE, TXT_QUAD_LASERS);
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_energy();
		break;
	case	POW_VULCAN_WEAPON:
		if ((used = pick_up_primary(VULCAN_INDEX)) != 0) 
		{
			vulcan_ammo_to_add_with_cannon = obj->ctype.powerup_info.count;
			if (vulcan_ammo_to_add_with_cannon < VULCAN_WEAPON_AMMO_AMOUNT) vulcan_ammo_to_add_with_cannon = VULCAN_WEAPON_AMMO_AMOUNT;
			pick_up_ammo(CLASS_PRIMARY, VULCAN_INDEX, vulcan_ammo_to_add_with_cannon);
		}
		if (!used)
			used = pick_up_vulcan_ammo();
		break;
	case	POW_SPREADFIRE_WEAPON:
		used = pick_up_primary(SPREADFIRE_INDEX);
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_energy();
		break;
	case	POW_PLASMA_WEAPON:
		used = pick_up_primary(PLASMA_INDEX);
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_energy();
		break;
	case	POW_FUSION_WEAPON:
		used = pick_up_primary(FUSION_INDEX);
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_energy();
		break;

	case	POW_PROXIMITY_WEAPON:
		used = pick_up_secondary(PROXIMITY_INDEX, 4);
		break;
	case	POW_SMARTBOMB_WEAPON:
		used = pick_up_secondary(SMART_INDEX, 1);
		break;
	case	POW_MEGA_WEAPON:
		used = pick_up_secondary(MEGA_INDEX, 1);
		break;
	case	POW_VULCAN_AMMO: 
	{
		used = pick_up_vulcan_ammo();
		if (!used && !(Game_mode & GM_MULTI))
			used = pick_up_vulcan_ammo();
		break;
	}
	break;
	case	POW_HOMING_AMMO_1:
		used = pick_up_secondary(HOMING_INDEX, 1);
		break;
	case	POW_HOMING_AMMO_4:
		used = pick_up_secondary(HOMING_INDEX, 4);
		break;
	case	POW_CLOAK:
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) 
		{
			HUD_init_message("%s %s!", TXT_ALREADY_ARE, TXT_CLOAKED);
			break;
		}
		else 
		{
			Players[Player_num].cloak_time = GameTime;	//	Not! changed by awareness events (like player fires laser).
			Players[Player_num].flags |= PLAYER_FLAGS_CLOAKED;
			ai_do_cloak_stuff();
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
#endif
			powerup_basic(-10, -10, -10, CLOAK_SCORE, "%s!", TXT_CLOAKING_DEVICE);
			used = 1;
			break;
		}
	case	POW_INVULNERABILITY:
		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) 
		{
			HUD_init_message("%s %s!", TXT_ALREADY_ARE, TXT_INVULNERABLE);
			break;
		}
		else 
		{
			Players[Player_num].invulnerable_time = GameTime;
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			powerup_basic(7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
			used = 1;
			break;
		}
#ifndef RELEASE
	case	POW_MEGAWOW:
		do_megawow_powerup(50);
		used = 1;
		break;
#endif

	default:
		break;
	}

	//always say used, until physics problem (getting stuck on unused powerup)
	//is solved.  Note also the break statements above that are commented out
	//!!	used=1;

	if (used && Powerup_info[obj->id].hit_sound > -1) 
	{
#ifdef NETWORK
		if (Game_mode & GM_MULTI) // Added by Rob, take this out if it turns out to be not good for net games!
			multi_send_play_sound(Powerup_info[obj->id].hit_sound, F1_0);
#endif
		digi_play_sample(Powerup_info[obj->id].hit_sound, F1_0);
	}

	return used;

}
