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
#include <stdarg.h>
#include <string.h>

#include "platform/posixstub.h"
#include "misc/types.h"
#include "inferno.h"
#include "2d/gr.h"
#include "cfile/cfile.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "sounds.h"
#include "bm.h"
#include "hash.h"
#include "args.h"
#include "2d/palette.h"
#include "gamefont.h"
#include "2d/rle.h"
#include "screens.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile/cfile.h"
#include "newmenu.h"
#include "2d/i_gr.h"

int piggy_is_substitutable_bitmap(char* name, char* subst_name);

//#define NO_DUMP_SOUNDS	1		//if set, dump bitmaps but not sounds

uint8_t* BitmapBits = NULL;
uint8_t* SoundBits = NULL;

typedef struct BitmapFile 
{
	char			name[15];
} BitmapFile;

typedef struct SoundFile 
{
	char			name[15];
} SoundFile;

hashtable AllBitmapsNames;
hashtable AllDigiSndNames;

int Num_bitmap_files = 0;
int Num_sound_files = 0;

digi_sound GameSounds[MAX_SOUND_FILES];
int SoundOffset[MAX_SOUND_FILES];
grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

int Num_bitmap_files_new = 0;
int Num_sound_files_new = 0;
static BitmapFile AllBitmaps[MAX_BITMAP_FILES];
static SoundFile AllSounds[MAX_SOUND_FILES];

int piggy_low_memory = 0;

#define DBM_FLAG_LARGE 	128		// Flags added onto the flags struct in b
#define DBM_FLAG_ABM		64

int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
uint8_t* Piggy_bitmap_cache_data = NULL;
static int GameBitmapOffset[MAX_BITMAP_FILES];
static uint8_t GameBitmapFlags[MAX_BITMAP_FILES];
uint16_t GameBitmapXlat[MAX_BITMAP_FILES];

#define PIGGY_BUFFER_SIZE (2048*1024)

int piggy_page_flushed = 0;

typedef struct DiskBitmapHeader 
{
	char name[8];
	uint8_t dflags;
	uint8_t	width;
	uint8_t height;
	uint8_t flags;
	uint8_t avg_color;
	int offset;
} DiskBitmapHeader;

//[ISB]: The above structure size can vary from system to system, but the size on disk is constant. Calculate that instead. 
#define BITMAP_HEADER_SIZE 17

typedef struct DiskSoundHeader 
{
	char name[8];
	int length;
	int data_length;
	int offset;
} DiskSoundHeader;

#define SOUND_HEADER_SIZE 20

uint8_t BigPig = 0;

void piggy_get_bitmap_name(int i, char* name)
{
	strncpy(name, AllBitmaps[i].name, 12);
	name[12] = 0;
}

bitmap_index piggy_register_bitmap(grs_bitmap* bmp, const char* name, int in_file)
{
	bitmap_index temp;
	Assert(Num_bitmap_files < MAX_BITMAP_FILES);

	temp.index = Num_bitmap_files;

	if (!in_file) 
	{
		if (!BigPig)	gr_bitmap_rle_compress(bmp);
		Num_bitmap_files_new++;
	}

	strncpy(AllBitmaps[Num_bitmap_files].name, name, 12);
	hashtable_insert(&AllBitmapsNames, AllBitmaps[Num_bitmap_files].name, Num_bitmap_files);
	GameBitmaps[Num_bitmap_files] = *bmp;
	if (!in_file) 
	{
		GameBitmapOffset[Num_bitmap_files] = 0;
		GameBitmapFlags[Num_bitmap_files] = bmp->bm_flags;
	}
	Num_bitmap_files++;

	return temp;
}

int piggy_register_sound(digi_sound* snd, char* name, int in_file)
{
	int i;

	Assert(Num_sound_files < MAX_SOUND_FILES);

	strncpy(AllSounds[Num_sound_files].name, name, 12);
	hashtable_insert(&AllDigiSndNames, AllSounds[Num_sound_files].name, Num_sound_files);
	GameSounds[Num_sound_files] = *snd;
	if (!in_file) 
	{
		SoundOffset[Num_sound_files] = 0;
	}

	i = Num_sound_files;

	if (!in_file)
		Num_sound_files_new++;

	Num_sound_files++;
	return i;
}

bitmap_index piggy_find_bitmap(char* name)
{
	bitmap_index bmp;
	int i;

	bmp.index = 0;

	i = hashtable_search(&AllBitmapsNames, name);
	Assert(i != 0);
	if (i < 0)
		return bmp;

	bmp.index = i;
	return bmp;
}

int piggy_find_sound(char* name)
{
	int i;

	i = hashtable_search(&AllDigiSndNames, name);

	if (i < 0)
		return 255;

	return i;
}

CFILE* Piggy_fp = NULL;

void piggy_close_file(void)
{
	if (Piggy_fp) 
	{
		cfclose(Piggy_fp);
		Piggy_fp = NULL;
	}
}

uint8_t bogus_data[64 * 64];
grs_bitmap bogus_bitmap;
uint8_t bogus_bitmap_initialized = 0;
digi_sound bogus_sound;

extern void bm_read_all(CFILE* fp);

int piggy_init()
{
	int sbytes = 0;
	char temp_name_read[16];
	char temp_name[16];
	grs_bitmap temp_bitmap;
	digi_sound temp_sound;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_size, N_bitmaps, N_sounds;
	int i, size, length, x, y;
	const char* filename;
	int read_sounds = 1;
	int Pigdata_start;

	hashtable_init(&AllBitmapsNames, MAX_BITMAP_FILES);
	hashtable_init(&AllDigiSndNames, MAX_SOUND_FILES);

	if (FindArg("-nosound") || (digi_driver_board < 1)) 
	{
		read_sounds = 0;
		mprintf((0, "Not loading sound data!!!!!\n"));
	}

	for (i = 0; i < MAX_SOUND_FILES; i++) 
	{
		GameSounds[i].length = 0;
		GameSounds[i].data = NULL;
		SoundOffset[i] = 0;
	}

	for (i = 0; i < MAX_BITMAP_FILES; i++) 
	{
		GameBitmapXlat[i] = i;
		GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
	}

	if (!bogus_bitmap_initialized) 
	{
		int i;
		uint8_t c;
		bogus_bitmap_initialized = 1;
		memset(&bogus_bitmap, 0, sizeof(grs_bitmap));
		bogus_bitmap.bm_w = bogus_bitmap.bm_h = bogus_bitmap.bm_rowsize = 64;
		bogus_bitmap.bm_data = bogus_data;
		c = gr_find_closest_color(0, 0, 63);
		for (i = 0; i < 4096; i++) bogus_data[i] = c;
		c = gr_find_closest_color(63, 0, 0);
		// Make a big red X !
		for (i = 0; i < 64; i++) 
		{
			bogus_data[i * 64 + i] = c;
			bogus_data[i * 64 + (63 - i)] = c;
		}
		piggy_register_bitmap(&bogus_bitmap, "bogus", 1);
		bogus_sound.length = 64 * 64;
		bogus_sound.data = bogus_data;
		GameBitmapOffset[0] = 0;
	}

	filename = "DESCENT.PIG";

	if (FindArg("-bigpig"))
		BigPig = 1;

	if (FindArg("-lowmem"))
		piggy_low_memory = 1;

	if (FindArg("-nolowmem"))
		piggy_low_memory = 0;

	if (piggy_low_memory)
		digi_lomem = 1;

	if ((i = FindArg("-piggy"))) 
	{
		filename = Args[i + 1];
		mprintf((0, "Using alternate pigfile, '%s'\n", filename));
	}
	Piggy_fp = cfopen(filename, "rb");
	if (Piggy_fp == NULL) return 0;

	//cfread(&Pigdata_start, sizeof(int), 1, Piggy_fp);
	Pigdata_start = CF_ReadInt(Piggy_fp);
#ifdef EDITOR
	if (FindArg("-nobm"))
#endif
	{
		bm_read_all(Piggy_fp);	// Note connection to above if!!!
		cfread(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, Piggy_fp);
	}

	cfseek(Piggy_fp, Pigdata_start, SEEK_SET);
	size = cfilelength(Piggy_fp) - Pigdata_start;
	length = size;
	mprintf((0, "\nReading data (%d KB) ", size / 1024));

	cfread(&N_bitmaps, sizeof(int), 1, Piggy_fp);
	size -= sizeof(int);
	cfread(&N_sounds, sizeof(int), 1, Piggy_fp);
	size -= sizeof(int);

	header_size = (N_bitmaps * BITMAP_HEADER_SIZE) + (N_sounds * SOUND_HEADER_SIZE);

	x = 60; y = 189;

	gr_set_curfont(Gamefonts[GFONT_SMALL]);
	gr_set_fontcolor(gr_find_closest_color_current(20, 20, 20), -1);
	gr_printf(0x8000, y - 10, "%s...", TXT_LOADING_DATA);
	I_DrawCurrentCanvas(0);

	for (i = 0; i < N_bitmaps; i++) 
	{
		//cfread(&bmh, sizeof(DiskBitmapHeader), 1, Piggy_fp);
		//size -= sizeof(DiskBitmapHeader);
		//[ISB] fix platform bugs, hopefully
		/*	char name[8];
	uint8_t dflags;
	uint8_t	width;
	uint8_t height;
	uint8_t flags;
	uint8_t avg_color;
	int offset;*/
		cfread(&bmh.name[0], 8 * sizeof(char), 1, Piggy_fp);
		bmh.dflags = CF_ReadByte(Piggy_fp);
		bmh.width = CF_ReadByte(Piggy_fp);
		bmh.height = CF_ReadByte(Piggy_fp);
		bmh.flags = CF_ReadByte(Piggy_fp);
		bmh.avg_color = CF_ReadByte(Piggy_fp);
		bmh.offset = CF_ReadInt(Piggy_fp);
		memcpy(temp_name_read, bmh.name, 8);
		temp_name_read[8] = 0;
		if (bmh.dflags & DBM_FLAG_ABM)
			sprintf(temp_name, "%s#%d", temp_name_read, bmh.dflags & 63);
		else
			strcpy(temp_name, temp_name_read);
		memset(&temp_bitmap, 0, sizeof(grs_bitmap));
		if (bmh.dflags & DBM_FLAG_LARGE)
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + 256;
		else
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width;
		temp_bitmap.bm_h = bmh.height;
		temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;
		temp_bitmap.bm_data = Piggy_bitmap_cache_data;

		GameBitmapFlags[i + 1] = 0;
		if (bmh.flags & BM_FLAG_TRANSPARENT) GameBitmapFlags[i + 1] |= BM_FLAG_TRANSPARENT;
		if (bmh.flags & BM_FLAG_SUPER_TRANSPARENT) GameBitmapFlags[i + 1] |= BM_FLAG_SUPER_TRANSPARENT;
		if (bmh.flags & BM_FLAG_NO_LIGHTING) GameBitmapFlags[i + 1] |= BM_FLAG_NO_LIGHTING;
		if (bmh.flags & BM_FLAG_RLE) GameBitmapFlags[i + 1] |= BM_FLAG_RLE;

		GameBitmapOffset[i + 1] = bmh.offset + header_size + (sizeof(int) * 2) + Pigdata_start;
		Assert((i + 1) == Num_bitmap_files);
		piggy_register_bitmap(&temp_bitmap, temp_name, 1);
	}

	for (i = 0; i < N_sounds; i++) 
	{
		//cfread(&sndh, sizeof(DiskSoundHeader), 1, Piggy_fp);
		//[ISB] fix platform bugs, hopefully
		cfread(&sndh.name, 8 * sizeof(char), 1, Piggy_fp);
		sndh.length = CF_ReadInt(Piggy_fp);
		sndh.data_length = CF_ReadInt(Piggy_fp);
		sndh.offset = CF_ReadInt(Piggy_fp);
		//size -= sizeof(DiskSoundHeader);
		temp_sound.length = sndh.length;
		temp_sound.data = (uint8_t*)(sndh.offset + header_size + (sizeof(int) * 2) + Pigdata_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + (sizeof(int) * 2) + Pigdata_start;
		memcpy(temp_name_read, sndh.name, 8);
		temp_name_read[8] = 0;
		piggy_register_sound(&temp_sound, temp_name_read, 1);
		sbytes += sndh.length;
		//mprintf(( 0, "%d bytes of sound\n", sbytes ));
	}

	SoundBits = (uint8_t*)malloc(sbytes + 16);
	if (SoundBits == NULL)
		Error("Not enough memory to load DESCENT.PIG sounds\n");

#ifdef EDITOR
	Piggy_bitmap_cache_size = size - header_size - sbytes + 16;
	Assert(Piggy_bitmap_cache_size > 0);
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
#endif
	BitmapBits = (uint8_t*)malloc(Piggy_bitmap_cache_size);
	if (BitmapBits == NULL)
		Error("Not enough memory to load DESCENT.PIG bitmaps\n");
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

	mprintf((0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size / 1024, sbytes / 1024));

	atexit(piggy_close_file);

	//	mprintf( (0, "<<<<Paging in all piggy bitmaps...>>>>>" ));
	//	for (i=0; i < Num_bitmap_files; i++ )	{
	//		bitmap_index bi;
	//		bi.index = i;
	//		PIGGY_PAGE_IN( bi );
	//	}
	//	mprintf( (0, "\n (USed %d / %d KB)\n", Piggy_bitmap_cache_next/1024, (size - header_size - sbytes + 16)/1024 ));
	//	key_getch();

	return 0;
}

int piggy_is_needed(int soundnum)
{
	int i;

	if (!digi_lomem) return 1;

	for (i = 0; i < MAX_SOUNDS; i++) 
	{
		if ((AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum))
			return 1;
	}
	return 0;
}

void piggy_read_sounds()
{
	uint8_t* ptr;
	int i, sbytes;

	ptr = SoundBits;
	sbytes = 0;

	for (i = 0; i < Num_sound_files; i++)
	{
		digi_sound* snd = &GameSounds[i];
		if (SoundOffset[i] > 0) 
		{
			if (piggy_is_needed(i)) 
			{
				cfseek(Piggy_fp, SoundOffset[i], SEEK_SET);

				// Read in the sound data!!!
				snd->data = ptr;
				ptr += snd->length;
				sbytes += snd->length;
				cfread(snd->data, snd->length, 1, Piggy_fp);
			}
		}
	}

	mprintf((0, "\nActual Sound usage: %d KB\n", sbytes / 1024));

}

extern int descent_critical_error;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

const char* crit_errors[16] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media type", "Sector not found", "Printer out of paper", "Write Fault", \
"Read fault", "General Failure", "", "", "" };

void piggy_critical_error()
{
	grs_canvas* save_canv;
	grs_font* save_font;
	int i;
	save_canv = grd_curcanv;
	save_font = grd_curcanv->cv_font;
	gr_palette_load(gr_palette);
	i = nm_messagebox("Disk Error", 2, "Retry", "Exit", "%s\non drive %c:", crit_errors[descent_critical_errcode & 0xf], (descent_critical_deverror & 0xf) + 'A');
	if (i == 1)
		Error("You chose to abort. Rip.\n");
		//exit(1);
	gr_set_current_canvas(save_canv);
	grd_curcanv->cv_font = save_font;
}

void piggy_bitmap_page_in(bitmap_index bitmap)
{
	grs_bitmap* bmp;
	int i, org_i, temp;

	i = bitmap.index;
	Assert(i >= 0);
	Assert(i < MAX_BITMAP_FILES);
	Assert(i < Num_bitmap_files);
	Assert(Piggy_bitmap_cache_size > 0);

	if (i < 1) return;
	if (i >= MAX_BITMAP_FILES) return;
	if (i >= Num_bitmap_files) return;

	if (GameBitmapOffset[i] == 0) return;		// A read-from-disk bitmap!!!

	if (piggy_low_memory) 
	{
		org_i = i;
		i = GameBitmapXlat[i];		// Xlat for low-memory settings!
	}
	bmp = &GameBitmaps[i];

	if (bmp->bm_flags & BM_FLAG_PAGED_OUT) 
	{
		stop_time();

	ReDoIt:
		descent_critical_error = 0;
		cfseek(Piggy_fp, GameBitmapOffset[i], SEEK_SET);
		if (descent_critical_error) 
		{
			piggy_critical_error();
			goto ReDoIt;
		}

		bmp->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
		bmp->bm_flags = GameBitmapFlags[i];

		if (bmp->bm_flags & BM_FLAG_RLE) 
		{
			int zsize = 0;
			descent_critical_error = 0;
			//temp = cfread(&zsize, 1, sizeof(int), Piggy_fp);
			zsize = CF_ReadInt(Piggy_fp);
			if (descent_critical_error) 
			{
				piggy_critical_error();
				goto ReDoIt;
			}

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert(Piggy_bitmap_cache_next + zsize < Piggy_bitmap_cache_size);
			if (Piggy_bitmap_cache_next + zsize >= Piggy_bitmap_cache_size) 
			{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			memcpy(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int));
			Piggy_bitmap_cache_next += sizeof(int);
			descent_critical_error = 0;
			temp = cfread(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, zsize - 4, Piggy_fp);
			if (descent_critical_error)
			{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += zsize - 4;
		}
		else 
		{
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert(Piggy_bitmap_cache_next + (bmp->bm_h * bmp->bm_w) < Piggy_bitmap_cache_size);
			if (Piggy_bitmap_cache_next + (bmp->bm_h * bmp->bm_w) >= Piggy_bitmap_cache_size) 
			{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			temp = cfread(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h * bmp->bm_w, Piggy_fp);
			if (descent_critical_error) 
			{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += bmp->bm_h * bmp->bm_w;
		}

#if 0 //[ISB] ughhhhhhhhhhhhhhhhhhhh
		if (bmp->bm_selector) 
		{
			if (!dpmi_modify_selector_base(bmp->bm_selector, bmp->bm_data))
				Error("Error modifying selector base in piggy.c\n");
		}
#endif
		start_time();
	}

	if (piggy_low_memory) 
	{
		if (org_i != i)
			GameBitmaps[org_i] = GameBitmaps[i];
	}
}

void piggy_bitmap_page_out_all()
{
	int i;

	Piggy_bitmap_cache_next = 0;

	piggy_page_flushed++;

	texmerge_flush();
	rle_cache_flush();

	for (i = 0; i < Num_bitmap_files; i++) 
	{
		if (GameBitmapOffset[i] > 0) // Don't page out bitmaps read from disk!!!
		{
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
			GameBitmaps[i].bm_data = Piggy_bitmap_cache_data;
		}
	}
	mprintf((0, "Flushing piggy bitmap cache\n"));
}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all();
}

#ifdef EDITOR
void piggy_dump_all()
{
	int i, xlat_offset;
	FILE* fp;
#ifndef RELEASE
	FILE* fp1;
	FILE* fp2;
#endif
	char* filename;
	int data_offset;
	int org_offset;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_offset;
	char subst_name[32];

#ifdef NO_DUMP_SOUNDS
	Num_sound_files = 0;
	Num_sound_files_new = 0;
#endif

	//	{
	//	bitmap_index bi;
	//	bi.index = 614;
	//	PIGGY_PAGE_IN( bi );
	//	count_colors( bi.index, &GameBitmaps[bi.index] );
	//	key_getch();
	//	}
	//	{
	//	bitmap_index bi;
	//	bi.index = 478;
	//	PIGGY_PAGE_IN( bi );
	//	Int3();
	//	count_colors( bi.index, &GameBitmaps[bi.index] );
	//	key_getch();
	//	}
	//	{
	//	bitmap_index bi;
	//	bi.index = 1398;
	//	PIGGY_PAGE_IN( bi );
	//	count_colors( bi.index, &GameBitmaps[bi.index] );
	//	key_getch();
	//	}
	//	{
	//	bitmap_index bi;
	//	bi.index = 642;
	//	PIGGY_PAGE_IN( bi );
	//	count_colors( bi.index, &GameBitmaps[bi.index] );
	//	key_getch();
	//	}
	//	{
	//	bitmap_index bi;
	//	bi.index = 529;
	//	PIGGY_PAGE_IN( bi );
	//	count_colors( bi.index, &GameBitmaps[bi.index] );
	//	key_getch();
	//	}
	//	exit(0);
	//
	if ((Num_bitmap_files_new == 0) && (Num_sound_files_new == 0))
		return;

	mprintf((0, "Paging in all piggy bitmaps..."));
	for (i = 0; i < Num_bitmap_files; i++) {
		bitmap_index bi;
		bi.index = i;
		PIGGY_PAGE_IN(bi);
	}
	mprintf((0, "\n"));

	piggy_close_file();

	mprintf((0, "Creating DESCENT.PIG..."));
	filename = "DESCENT.PIG";
	if ((i = FindArg("-piggy"))) {
		filename = Args[i + 1];
		mprintf((0, "Dumping alternate pigfile, '%s'\n", filename));
	}
	mprintf((0, "\nDumping bitmaps..."));

	fp = fopen(filename, "wb");
	Assert(fp != NULL);

#ifndef RELEASE
	fp1 = fopen("piggy.lst", "wt");
	fp2 = fopen("piggy.all", "wt");
#endif

	i = 0;
	fwrite(&i, sizeof(int), 1, fp);
	bm_write_all(fp);
	xlat_offset = ftell(fp);
	fwrite(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, fp);
	i = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fwrite(&i, sizeof(int), 1, fp);
	fseek(fp, i, SEEK_SET);

	Num_bitmap_files--;
	fwrite(&Num_bitmap_files, sizeof(int), 1, fp);
	Num_bitmap_files++;
	fwrite(&Num_sound_files, sizeof(int), 1, fp);

	header_offset = ftell(fp);
	header_offset += ((Num_bitmap_files - 1) * sizeof(DiskBitmapHeader)) + (Num_sound_files * sizeof(DiskSoundHeader));
	data_offset = header_offset;

	for (i = 1; i < Num_bitmap_files; i++) {
		int* size;
		grs_bitmap* bmp;

		{
			char* p, * p1;
			p = strchr(AllBitmaps[i].name, '#');
			if (p) {
				int n;
				p1 = p; p1++;
				n = atoi(p1);
				*p = 0;
#ifndef RELEASE
				if (n == 0) {
					fprintf(fp2, "%s.abm\n", AllBitmaps[i].name);
				}
#endif
				memcpy(bmh.name, AllBitmaps[i].name, 8);
				Assert(n <= 63);
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			}
			else {
#ifndef RELEASE
				fprintf(fp2, "%s.bbm\n", AllBitmaps[i].name);
#endif
				memcpy(bmh.name, AllBitmaps[i].name, 8);
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[i];

		Assert(!(bmp->bm_flags & BM_FLAG_PAGED_OUT));

#ifndef RELEASE
		fprintf(fp1, "BMP: %s, size %d bytes", AllBitmaps[i].name, bmp->bm_rowsize * bmp->bm_h);
#endif
		org_offset = ftell(fp);
		bmh.offset = data_offset - header_offset;
		fseek(fp, data_offset, SEEK_SET);

		if (bmp->bm_flags & BM_FLAG_RLE) {
			size = (int*)bmp->bm_data;
			fwrite(bmp->bm_data, sizeof(uint8_t), *size, fp);
			data_offset += *size;
			//bmh.data_length = *size;
#ifndef RELEASE
			fprintf(fp1, ", and is already compressed to %d bytes.\n", *size);
#endif
		}
		else {
			fwrite(bmp->bm_data, sizeof(uint8_t), bmp->bm_rowsize * bmp->bm_h, fp);
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			//bmh.data_length = bmp->bm_rowsize * bmp->bm_h;
#ifndef RELEASE
			fprintf(fp1, ".\n");
#endif
		}
		fseek(fp, org_offset, SEEK_SET);
		if (GameBitmaps[i].bm_w > 255) {
			Assert(GameBitmaps[i].bm_w < 512);
			bmh.width = GameBitmaps[i].bm_w - 256;
			bmh.dflags |= DBM_FLAG_LARGE;
		}
		else {
			bmh.width = GameBitmaps[i].bm_w;
		}
		Assert(GameBitmaps[i].bm_h < 256);
		bmh.height = GameBitmaps[i].bm_h;
		bmh.flags = GameBitmaps[i].bm_flags;
		if (piggy_is_substitutable_bitmap(AllBitmaps[i].name, subst_name)) {
			bitmap_index other_bitmap;
			other_bitmap = piggy_find_bitmap(subst_name);
			GameBitmapXlat[i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
			//mprintf(( 0, "Skipping bitmap %d\n", i ));
			//mprintf(( 0, "Marking '%s' as substitutible\n", AllBitmaps[i].name ));
		}
		else {
#ifdef BUILD_PSX_DATA
			count_colors(i, &GameBitmaps[i]);
#endif
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color = GameBitmaps[i].avg_color;
		fwrite(&bmh, sizeof(DiskBitmapHeader), 1, fp);			// Mark as a bitmap
	}

	mprintf((0, "\nDumping sounds..."));

	for (i = 0; i < Num_sound_files; i++) {
		digi_sound* snd;

		snd = &GameSounds[i];
		strcpy(sndh.name, AllSounds[i].name);
		sndh.length = GameSounds[i].length;
		sndh.offset = data_offset - header_offset;

		org_offset = ftell(fp);
		fseek(fp, data_offset, SEEK_SET);

		sndh.data_length = GameSounds[i].length;
		fwrite(snd->data, sizeof(uint8_t), snd->length, fp);
		data_offset += snd->length;
		fseek(fp, org_offset, SEEK_SET);
		fwrite(&sndh, sizeof(DiskSoundHeader), 1, fp);			// Mark as a bitmap

#ifndef RELEASE
		fprintf(fp1, "SND: %s, size %d bytes\n", AllSounds[i].name, snd->length);

		fprintf(fp2, "%s.raw\n", AllSounds[i].name);
#endif
	}

	fseek(fp, xlat_offset, SEEK_SET);
	fwrite(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, fp);

	fclose(fp);

	mprintf((0, "\n"));

	mprintf((0, " Dumped %d assorted bitmaps.\n", Num_bitmap_files));
	mprintf((0, " Dumped %d assorted sounds.\n", Num_sound_files));

#ifndef RELEASE
	fprintf(fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files);
	fprintf(fp1, " Dumped %d assorted sounds.\n", Num_sound_files);

	fclose(fp1);
	fclose(fp2);
#endif

	// Never allow the game to run after building pig.
	exit(0);
}

#endif

void piggy_close()
{
	if (BitmapBits)
		free(BitmapBits);

	if (SoundBits)
		free(SoundBits);

	hashtable_free(&AllBitmapsNames);
	hashtable_free(&AllDigiSndNames);
}

int piggy_does_bitmap_exist_slow(char* name)
{
	int i;

	for (i = 0; i < Num_bitmap_files; i++) {
		if (!strcmp(AllBitmaps[i].name, name))
			return 1;
	}
	return 0;
}


#define NUM_GAUGE_BITMAPS 10
const char* gauge_bitmap_names[NUM_GAUGE_BITMAPS] = { "gauge01", "gauge02", "gauge06", "targ01", "targ02", "targ03", "targ04", "targ05", "targ06", "gauge18" };

int piggy_is_gauge_bitmap(char* base_name)
{
	int i;
	for (i = 0; i < NUM_GAUGE_BITMAPS; i++) 
	{
		if (!_stricmp(base_name, gauge_bitmap_names[i]))
			return 1;
	}

	return 0;
}

int piggy_is_substitutable_bitmap(char* name, char* subst_name)
{
	int frame;
	char* p;
	char base_name[16];

	strcpy(subst_name, name);
	p = strchr(subst_name, '#');
	if (p) 
	{
		frame = atoi(&p[1]);
		*p = 0;
		strcpy(base_name, subst_name);
		if (!piggy_is_gauge_bitmap(base_name)) 
		{
			sprintf(subst_name, "%s#%d", base_name, frame + 1);
			if (piggy_does_bitmap_exist_slow(subst_name)) 
			{
				if (frame & 1) 
				{
					sprintf(subst_name, "%s#%d", base_name, frame - 1);
					return 1;
				}
			}
		}
	}
	strcpy(subst_name, name);
	return 0;
}
