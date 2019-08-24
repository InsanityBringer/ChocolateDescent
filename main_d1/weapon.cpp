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

#include "game.h"
#include "weapon.h"
#include "platform/mono.h"
#include "player.h"
#include "gauges.h"
#include "misc/error.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "newdemo.h"
#include "multi.h"

//	Note, only Vulcan cannon requires ammo.
//uint8_t	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//uint8_t	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in Weapon_info array.
uint8_t Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = { 0, 11, 12, 13, 14 };
uint8_t Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = { 8, 15, 16, 17, 18 };

int Primary_ammo_max[MAX_PRIMARY_WEAPONS] = { 0, VULCAN_AMMO_MAX, 0, 0, 0 };
uint8_t Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = { 20, 10, 10, 5, 5 };

weapon_info Weapon_info[MAX_WEAPON_TYPES];
int	N_weapon_types = 0;
int8_t	Primary_weapon, Secondary_weapon;

//char	*Primary_weapon_names[MAX_PRIMARY_WEAPONS] = {
//	"Laser Cannon",
//	"Vulcan Cannon",
//	"Spreadfire Cannon",
//	"Plasma Cannon",
//	"Fusion Cannon"
//};

//char	*Secondary_weapon_names[MAX_SECONDARY_WEAPONS] = {
//	"Concussion Missile",
//	"Homing Missile",
//	"Proximity Bomb",
//	"Smart Missile",
//	"Mega Missile"
//};

//char	*Primary_weapon_names_short[MAX_PRIMARY_WEAPONS] = {
//	"Laser",
//	"Vulcan",
//	"Spread",
//	"Plasma",
//	"Fusion"
//};

//char	*Secondary_weapon_names_short[MAX_SECONDARY_WEAPONS] = {
//	"Concsn\nMissile",
//	"Homing\nMissile",
//	"Proxim.\nBomb",
//	"Smart\nMissile",
//	"Mega\nMissile"
//};

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG	
// See weapon.h for bit values
int player_has_weapon(int weapon_num, int secondary_flag)
{
	int	return_value = 0;
	int	weapon_index;

	//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
	//	But energy should not go negative (but it does), so find out why it does!
	if (Players[Player_num].energy < 0)
		Players[Player_num].energy = 0;

	if (!secondary_flag) 
	{
		weapon_index = Primary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].primary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].primary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
			return_value |= HAS_ENERGY_FLAG;

	}
	else 
	{
		weapon_index = Secondary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].secondary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].secondary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
			return_value |= HAS_ENERGY_FLAG;
	}

	return return_value;
}

//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	char* weapon_name;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_weapon(secondary_flag, weapon_num);
#endif

	if (!secondary_flag) 
	{
		if (Primary_weapon != weapon_num) 
		{
			if (wait_for_rearm) digi_play_sample_once(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
#ifdef NETWORK
			if (Game_mode & GM_MULTI) {
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
			if (wait_for_rearm)
				Next_laser_fire_time = GameTime + REARM_TIME;
			else
				Next_laser_fire_time = 0;
			Global_laser_firing_count = 0;
		}
		else 
		{
			if (wait_for_rearm) digi_play_sample(SOUND_ALREADY_SELECTED, F1_0);
		}
		Primary_weapon = weapon_num;
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
	}
	else 
	{
		if (Secondary_weapon != weapon_num) 
		{
			if (wait_for_rearm) digi_play_sample_once(SOUND_GOOD_SELECTION_SECONDARY, F1_0);
#ifdef NETWORK
			if (Game_mode & GM_MULTI) {
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
			if (wait_for_rearm)
				Next_missile_fire_time = GameTime + REARM_TIME;
			else
				Next_missile_fire_time = 0;
			Global_missile_firing_count = 0;
		}
		else 
		{
			if (wait_for_rearm) digi_play_sample_once(SOUND_ALREADY_SELECTED, F1_0);
		}
		Secondary_weapon = weapon_num;
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	}

	if (print_message)
		HUD_init_message("%s %s", weapon_name, TXT_SELECTED);

}

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
	int	weapon_status = player_has_weapon(weapon_num, secondary_flag);
	char* weapon_name;

#ifdef SHAREWARE	// do special hud msg. for picking registered weapon in shareware version.
	if (weapon_num >= NUM_SHAREWARE_WEAPONS) {
		weapon_name = secondary_flag ? SECONDARY_WEAPON_NAMES(weapon_num) : PRIMARY_WEAPON_NAMES(weapon_num);
		HUD_init_message("%s %s!", weapon_name, TXT_NOT_IN_SHAREWARE);
		digi_play_sample(SOUND_BAD_SELECTION, F1_0);
		return;
	}
#endif

	if (!secondary_flag) 
	{
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
		if ((weapon_status & HAS_WEAPON_FLAG) == 0) 
		{
			HUD_init_message("%s %s!", TXT_DONT_HAVE, weapon_name);
			digi_play_sample(SOUND_BAD_SELECTION, F1_0);
			return;
		}
		else if ((weapon_status & HAS_AMMO_FLAG) == 0) 
		{
			HUD_init_message("%s %s!", TXT_DONT_HAVE_AMMO, weapon_name);
			digi_play_sample(SOUND_BAD_SELECTION, F1_0);
			return;
		}
	}
	else 
	{
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
		if (weapon_status != HAS_ALL) 
		{
			HUD_init_message("%s %s%s", TXT_HAVE_NO, weapon_name, TXT_SX);
			digi_play_sample(SOUND_BAD_SELECTION, F1_0);
			return;
		}
	}

	select_weapon(weapon_num, secondary_flag, 1, 1);
}

//	----------------------------------------------------------------------------------------
//	Automatically select next best weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary
void auto_select_weapon(int weapon_type)
{
	int	r;

	if (weapon_type == 0) 
	{
		r = player_has_weapon(Primary_weapon, 0);
		if (r != HAS_ALL) 
		{
			int	cur_weapon;
			int	try_again = 1;

			cur_weapon = Primary_weapon;

			while (try_again) 
			{
				cur_weapon--;
				if (cur_weapon < 0)
					cur_weapon = MAX_PRIMARY_WEAPONS - 1;

				//	Hack alert!  Because the fusion uses 0 energy at the end (it's got the weird chargeup)
				//	it looks like it takes 0 to fire, but it doesn't, so never auto-select.
				if (cur_weapon == FUSION_INDEX)
					continue;

				if (cur_weapon == Primary_weapon) 
				{
					HUD_init_message(TXT_NO_PRIMARY);
					try_again = 0;				// Tried all weapons!
					select_weapon(0, 0, 0, 1);
				}
				else if (player_has_weapon(cur_weapon, 0) == HAS_ALL) 
				{
					select_weapon(cur_weapon, 0, 1, 1);
					try_again = 0;
				}
			}
		}

	}
	else 
	{

		Assert(weapon_type == 1);

		if (Secondary_weapon != PROXIMITY_INDEX) 
		{
			if (!(player_has_weapon(Secondary_weapon, 1) == HAS_ALL)) 
			{
				if (Secondary_weapon > SMART_INDEX)
					if (player_has_weapon(SMART_INDEX, 1) == HAS_ALL) 
					{
						select_weapon(SMART_INDEX, 1, 1, 1);
						goto weapon_selected;
					}
				if (player_has_weapon(HOMING_INDEX, 1) == HAS_ALL)
					select_weapon(HOMING_INDEX, 1, 1, 1);
				else if (player_has_weapon(CONCUSSION_INDEX, 1) == HAS_ALL)
					select_weapon(CONCUSSION_INDEX, 1, 1, 1);
			weapon_selected:;
			}
		}
	}

}

#ifndef RELEASE

//	----------------------------------------------------------------------------------------
//	Show player which weapons he has, how much ammo...
//	Looks like a debug screen now because it writes to mono screen, but that will change... [ISB] well it didn't rip
void show_weapon_status(void)
{
	int	i;

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++) 
	{
		if (Players[Player_num].primary_weapon_flags & (1 << i))
			mprintf((0, "HAVE"));
		else
			mprintf((0, "    "));

		mprintf((0, "  Weapon: %20s, charges: %4i\n", PRIMARY_WEAPON_NAMES(i), Players[Player_num].primary_ammo[i]));
	}

	mprintf((0, "\n"));
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++) 
	{
		if (Players[Player_num].secondary_weapon_flags & (1 << i))
			mprintf((0, "HAVE"));
		else
			mprintf((0, "    "));

		mprintf((0, "  Weapon: %20s, charges: %4i\n", SECONDARY_WEAPON_NAMES(i), Players[Player_num].secondary_ammo[i]));
	}

	mprintf((0, "\n"));
	mprintf((0, "\n"));

}

#endif

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(int weapon_index, int count)
{
	int	num_picked_up;

	if (Players[Player_num].secondary_ammo[weapon_index] >= Secondary_ammo_max[weapon_index]) 
	{
		HUD_init_message("%s %i %ss!", TXT_ALREADY_HAVE, Players[Player_num].secondary_ammo[weapon_index], SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].secondary_weapon_flags |= (1 << weapon_index);
	Players[Player_num].secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (Players[Player_num].secondary_ammo[weapon_index] > Secondary_ammo_max[weapon_index]) 
	{
		num_picked_up = count - (Players[Player_num].secondary_ammo[weapon_index] - Secondary_ammo_max[weapon_index]);
		Players[Player_num].secondary_ammo[weapon_index] = Secondary_ammo_max[weapon_index];
	}

	//if you pick up a homing, and you're currently using concussion,
	//and you had no homings before, then upgrade
	if ((Secondary_weapon < weapon_index) && (weapon_index != PROXIMITY_INDEX)) //  && (Players[Player_num].secondary_ammo[weapon_index] == count))
		select_weapon(weapon_index, 1, 0, 1);

	//if you pick up a concussion, and you've got homing (or smart or mega) selected but are out,
	//then select concussion
	if ((weapon_index != PROXIMITY_INDEX) && (Players[Player_num].secondary_ammo[Secondary_weapon] == 0))
		select_weapon(weapon_index, 1, 0, 1);

	//note: flash for all but concussion was 7,14,21
	if (count > 1) 
	{
		PALETTE_FLASH_ADD(15, 15, 15);
		HUD_init_message("%d %s%s", num_picked_up, SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else 
	{
		PALETTE_FLASH_ADD(10, 10, 10);
		HUD_init_message("%s!", SECONDARY_WEAPON_NAMES(weapon_index));
	}

	return 1;
}

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index)
{
	uint8_t old_flags = Players[Player_num].primary_weapon_flags;
	uint8_t flag = 1 << weapon_index;

	if (Players[Player_num].primary_weapon_flags & flag) //already have
	{
		HUD_init_message("%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;

	if (!(old_flags & flag) && weapon_index > Primary_weapon)
		select_weapon(weapon_index, 0, 0, 1);

	PALETTE_FLASH_ADD(7, 14, 21);
	HUD_init_message("%s!", PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}

//called when ammo (for the vulcan cannon) is picked up
//	Return true if ammo picked up, else return false.
int pick_up_ammo(int class_flag, int weapon_index, int ammo_count)
{
	int old_ammo = class_flag;		//kill warning

	Assert(class_flag == CLASS_PRIMARY && weapon_index == VULCAN_INDEX);

	if (Players[Player_num].primary_ammo[weapon_index] == Primary_ammo_max[weapon_index])
		return 0;

	old_ammo = Players[Player_num].primary_ammo[weapon_index];

	Players[Player_num].primary_ammo[weapon_index] += ammo_count;

	if (Players[Player_num].primary_ammo[weapon_index] > Primary_ammo_max[weapon_index])
		Players[Player_num].primary_ammo[weapon_index] = Primary_ammo_max[weapon_index];

	if (Players[Player_num].primary_weapon_flags & (1 << weapon_index) && weapon_index > Primary_weapon && old_ammo == 0)
		select_weapon(weapon_index, 0, 0, 1);

	return 1;
}
