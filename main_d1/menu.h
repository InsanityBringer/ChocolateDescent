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
 * $Source: f:/miner/source/main/rcs/menu.h $
 * $Revision: 2.1 $
 * $Author: mike $
 * $Date: 1995/03/06 16:47:41 $
 *
 * Menu prototypes and variables
 *
 */


#ifndef _MENU_H
#define _MENU_H

 //returns number of item chosen
extern int DoMenu();
extern void do_options_menu();

extern void set_detail_level_parameters(int detail_level);

extern char* menu_difficulty_text[];
extern int Player_default_difficulty;
extern int Max_debris_objects;
extern int Auto_leveling_on;

void do_option(int select);
void do_detail_level_menu_custom(void);
void do_new_game_menu();
void do_multi_player_menu();

#endif
