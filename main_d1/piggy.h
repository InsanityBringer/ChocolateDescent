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
 * $Source: f:/miner/source/main/rcs/piggy.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:31:21 $
 *
 * Interface to piggy functions.
 *
 */



#ifndef _PIGGY_H
#define _PIGGY_H

#include "digi.h"
#include "sounds.h"

typedef struct bitmap_index {
	uint16_t	index;
} bitmap_index;

int piggy_init();
void piggy_close();
void piggy_dump_all();
bitmap_index piggy_register_bitmap(grs_bitmap* bmp, const char* name, int in_file);
int piggy_register_sound(digi_sound* snd, char* name, int in_file);
bitmap_index piggy_find_bitmap(char* name);
int piggy_find_sound(char* name);

#define PIGGY_PAGE_IN(bmp) 							\
do { 																\
	if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT )	{	\
		piggy_bitmap_page_in( bmp ); 						\
	}																\
} while(0)
//		mprintf(( 0, "Paging in '%s' from file '%s', line %d\n", #bmp, __FILE__,__LINE__ ));	\

extern void piggy_bitmap_page_in(bitmap_index bmp);
extern void piggy_bitmap_page_out_all();
extern int piggy_page_flushed;

//void piggy_read_bitmap_data(grs_bitmap* bmp);
//void piggy_read_sound_data(digi_sound* snd);

void piggy_load_level_data();

#ifdef SHAREWARE
#define MAX_BITMAP_FILES	1500
#define MAX_SOUND_FILES		MAX_SOUNDS
#else
#define MAX_BITMAP_FILES	1800
#define MAX_SOUND_FILES		MAX_SOUNDS
#endif

extern digi_sound GameSounds[MAX_SOUND_FILES];
extern grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

void piggy_read_sounds();


#endif
