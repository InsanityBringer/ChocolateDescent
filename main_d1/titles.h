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
 * $Source: f:/miner/source/main/rcs/titles.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:27:40 $
 *
 * .
 *
 *
 */

#ifndef _TITLES_H
#define _TITLES_H

#ifndef RELEASE
extern int	Skip_briefing_screens;
#else
#define Skip_briefing_screens 0
#endif

extern char Briefing_text_filename[13];
extern char Ending_text_filename[13];

extern int show_title_screen(const char* filename, int allow_keys);
//extern int show_briefing_screen(const char* filename, int allow_keys); //[ISB] these don't match definition in C file
//extern void show_title_flick(const char* name, int allow_keys);
extern void do_briefing_screens(int level_num);
extern void do_end_game(void);
extern char* get_briefing_screen(int level_num);

void title_save_game();

#endif
