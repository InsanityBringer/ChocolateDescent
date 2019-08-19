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
 * $Source: f:/miner/source/main/rcs/gamesave.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:30:25 $
 *
 * Headers for gamesave.c
 *
 */

#ifndef _GAMESAVE_H
#define _GAMESAVE_H

#define	NUM_SHAREWARE_LEVELS	7
#define	NUM_REGISTERED_LEVELS	23

extern char* Shareware_level_names[NUM_SHAREWARE_LEVELS];
extern char* Registered_level_names[NUM_REGISTERED_LEVELS];

//void LoadGame(void);
//void SaveGame(void); //[ISB] cut
void get_level_name(void);

//extern int load_game(char *filename);
//extern int save_game(char *filename);

extern int load_level(char* filename);
extern int save_level(char* filename);

//called in place of load_game() to only load the .min data
//extern void load_mine_only(char* filename); //[ISB] cut

extern char Gamesave_current_filename[];

extern int Gamesave_num_org_robots;

//	In dumpmine.c
#ifdef EDITOR
extern void write_game_text_file(char* filename);
#endif

extern	int	Errors_in_mine;

void dump_mine_info(void);

#endif
