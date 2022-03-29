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

#include "main_shared/digi.h"

#ifdef BUILD_DESCENT2
#include "main_d2/sounds.h"
#else
#include "main_d1/sounds.h"
#endif

#include "main_shared/inferno_shared.h"

#define MAX_ALIASES 20

typedef struct alias 
{
	char alias_name[FILENAME_LEN];
	char file_name[FILENAME_LEN];
} alias;

extern alias alias_list[MAX_ALIASES];
extern int Num_aliases;

typedef struct bitmap_index 
{
	uint16_t	index;
} bitmap_index;

int piggy_init();
void piggy_close();
void piggy_dump_all();
bitmap_index piggy_register_bitmap( grs_bitmap * bmp, const char * name, int in_file );
int piggy_register_sound( digi_sound * snd, const char * name, int in_file );
bitmap_index piggy_find_bitmap( char * name );
int piggy_find_sound(const char * name );

extern int Pigfile_initialized;

#define PIGGY_PAGE_IN(bmp) 							\
do { 																\
	if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT )	{	\
		piggy_bitmap_page_in( bmp ); 						\
	}																\
} while(0)

extern void piggy_bitmap_page_in( bitmap_index bmp );
extern void piggy_bitmap_page_out_all();
extern int piggy_page_flushed;

void piggy_read_bitmap_data(grs_bitmap * bmp);
void piggy_read_sound_data(digi_sound	*snd);

void piggy_load_level_data();

#ifdef BUILD_DESCENT2
#define MAX_BITMAP_FILES	2620 // Upped for CD Enhanced
#else
#define MAX_BITMAP_FILES	1800
#endif
#define MAX_SOUND_FILES		MAX_SOUNDS

extern digi_sound GameSounds[MAX_SOUND_FILES];
extern grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

void piggy_read_sounds();

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(const char *pigname);
