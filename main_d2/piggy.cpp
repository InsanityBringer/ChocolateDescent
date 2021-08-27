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

#ifdef WINDOWS
#include "desw.h"
#include "win\ds.h"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "misc/types.h"
#include "inferno.h"
#include "2d/gr.h"
#include "mem/mem.h"
#include "iff/iff.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "sounds.h"
#include "songs.h"
#include "bm.h"
#include "bmread.h"
#include "misc/hash.h"
#include "misc/args.h"
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
#include "misc/byteswap.h"
#include "platform/findfile.h"

//#include "unarj.h" //[ISB] goddamnit

//#define NO_DUMP_SOUNDS        1               //if set, dump bitmaps but not sounds

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"


#ifdef SHAREWARE
#define DEFAULT_HAMFILE         "d2demo.ham"
#define DEFAULT_PIGFILE         DEFAULT_PIGFILE_SHAREWARE
#define DEFAULT_SNDFILE			"descent2.s11"
#else
#define DEFAULT_HAMFILE         "descent2.ham"
#define DEFAULT_PIGFILE         DEFAULT_PIGFILE_REGISTERED
#define DEFAULT_SNDFILE	 		((digi_sample_rate==SAMPLE_RATE_22K)?"descent2.s22":"descent2.s11")
#endif	// end of ifdef SHAREWARE

uint8_t* BitmapBits = NULL;
uint8_t* SoundBits = NULL;

typedef struct BitmapFile {
	char                    name[15];
} BitmapFile;

typedef struct SoundFile {
	char                    name[15];
} SoundFile;

hashtable AllBitmapsNames;
hashtable AllDigiSndNames;

int Num_bitmap_files = 0;
int Num_sound_files = 0;

digi_sound GameSounds[MAX_SOUND_FILES];
int SoundOffset[MAX_SOUND_FILES];
grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

alias alias_list[MAX_ALIASES];
int Num_aliases = 0;

int Must_write_hamfile = 0;
int Num_bitmap_files_new = 0;
int Num_sound_files_new = 0;
BitmapFile AllBitmaps[MAX_BITMAP_FILES];
static SoundFile AllSounds[MAX_SOUND_FILES];

int piggy_low_memory = 0;

int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
uint8_t* Piggy_bitmap_cache_data = NULL;
static int GameBitmapOffset[MAX_BITMAP_FILES];
static uint8_t GameBitmapFlags[MAX_BITMAP_FILES];
uint16_t GameBitmapXlat[MAX_BITMAP_FILES];

#define PIGGY_BUFFER_SIZE (2400*1024)

#ifdef MACINTOSH
#define PIGGY_SMALL_BUFFER_SIZE (1400*1024)		// size of buffer when piggy_low_memory is set

#ifdef SHAREWARE
#undef PIGGY_BUFFER_SIZE
#undef PIGGY_SMALL_BUFFER_SIZE

#define PIGGY_BUFFER_SIZE (2000*1024)
#define PIGGY_SMALL_BUFFER_SIZE (1100 * 1024)
#endif		// SHAREWARE

#endif

int piggy_page_flushed = 0;

#define DBM_FLAG_ABM            64

typedef struct DiskBitmapHeader
{
	char name[8];
	uint8_t dflags;                   //bits 0-5 anim frame num, bit 6 abm flag
	uint8_t width;                    //low 8 bits here, 4 more bits in wh_extra
	uint8_t height;                   //low 8 bits here, 4 more bits in wh_extra
	uint8_t   wh_extra;               //bits 0-3 width, bits 4-7 height
	uint8_t flags;
	uint8_t avg_color;
	int offset;
} DiskBitmapHeader;

//[ISB]: The above structure size can vary from system to system, but the size on disk is constant. Calculate that instead. 
#define BITMAP_HEADER_SIZE 18

typedef struct DiskSoundHeader
{
	char name[8];
	int length;
	int data_length;
	int offset;
} DiskSoundHeader;

//Not as much as a problem since its longword aligned, but just in case
#define SOUND_HEADER_SIZE 20

uint8_t BigPig = 0;

void piggy_write_pigfile(const char* filename); 
static void write_int(int i, FILE* file);
int piggy_is_substitutable_bitmap(char* name, char* subst_name);

#ifdef EDITOR
void swap_0_255(grs_bitmap* bmp)
{
	int i;

	for (i = 0; i < bmp->bm_h * bmp->bm_w; i++) {
		if (bmp->bm_data[i] == 0)
			bmp->bm_data[i] = 255;
		else if (bmp->bm_data[i] == 255)
			bmp->bm_data[i] = 0;
	}
}
#endif

bitmap_index piggy_register_bitmap(grs_bitmap* bmp, const char* name, int in_file)
{
	bitmap_index temp;
	Assert(Num_bitmap_files < MAX_BITMAP_FILES);

	temp.index = Num_bitmap_files;

	if (!in_file)
	{
#ifdef EDITOR
		if (FindArg("-macdata"))
			swap_0_255(bmp);
#endif
		if (!BigPig)  gr_bitmap_rle_compress(bmp);
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

int piggy_register_sound(digi_sound* snd, const char* name, int in_file)
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
	char* t;

	bmp.index = 0;

	if ((t = strchr(name, '#')) != NULL)
		* t = 0;

	for (i = 0; i < Num_aliases; i++)
		if (_strfcmp(name, alias_list[i].alias_name) == 0)
		{
			if (t) //extra stuff for ABMs
			{
				static char temp[FILENAME_LEN];
				_splitpath(alias_list[i].file_name, NULL, NULL, temp, NULL);
				name = temp;
				strcat(name, "#");
				strcat(name, t + 1);
			}
			else
				name = alias_list[i].file_name;
			break;
		}

	if (t)
		* t = '#';

	i = hashtable_search(&AllBitmapsNames, name);
	Assert(i != 0);
	if (i < 0)
		return bmp;

	bmp.index = i;
	return bmp;
}

int piggy_find_sound(const char* name)
{
	int i;

	i = hashtable_search(&AllDigiSndNames, const_cast<char*>(name));

	if (i < 0)
		return 255;

	return i;
}

CFILE* Piggy_fp = NULL;

#define FILENAME_LEN 13

char Current_pigfile[FILENAME_LEN] = "";

void piggy_close_file()
{
	if (Piggy_fp)
	{
		cfclose(Piggy_fp);
		Piggy_fp = NULL;
		Current_pigfile[0] = 0;
	}
}

int Pigfile_initialized = 0;

#define PIGFILE_ID              'GIPP'          //PPIG
#define PIGFILE_VERSION         2

extern char CDROM_dir[];

int request_cd(void);


//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
CFILE* copy_pigfile_from_cd(const char* filename)
{
	Error("Cannot copy PIG file from CD. stub function\n");
	return NULL;
}


//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void piggy_init_pigfile(const char* filename)
{
	int i;
	char temp_name[16];
	char temp_name_read[16];
	grs_bitmap temp_bitmap;
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_size, data_start;
#ifdef MACINTOSH
	char name[255];		// filename + path for the mac
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char name[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

	piggy_close_file();             //close old pig if still open

#ifdef SHAREWARE                //rename pigfile for shareware
	if (strfcmp(filename, DEFAULT_PIGFILE_REGISTERED) == 0)
		filename = DEFAULT_PIGFILE_SHAREWARE;
#endif

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(name, filename, CHOCOLATE_SYSTEM_FILE_DIR);
	Piggy_fp = cfopen(name, "rb");
#else
	Piggy_fp = cfopen(filename, "rb");
#endif

	if (!Piggy_fp)
	{
#ifdef EDITOR
		return;         //if editor, ok to not have pig, because we'll build one
#else
		//Piggy_fp = copy_pigfile_from_cd(filename); //[ISB] no
#endif
	}

	if (Piggy_fp) //make sure pig is valid type file & is up-to-date
	{
		int pig_id, pig_version;

		pig_id = cfile_read_int(Piggy_fp);
		pig_version = cfile_read_int(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION)
		{
			cfclose(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

	if (!Piggy_fp)
	{
#ifdef EDITOR
		return;         //if editor, ok to not have pig, because we'll build one
#else
		Error("Cannot load required file <%s>", filename);
#endif
	}

	strncpy(Current_pigfile, filename, sizeof(Current_pigfile));
	N_bitmaps = cfile_read_int(Piggy_fp);
	header_size = N_bitmaps * BITMAP_HEADER_SIZE;
	data_start = header_size + cftell(Piggy_fp);
	data_size = cfilelength(Piggy_fp) - data_start;
	Num_bitmap_files = 1;

	for (i = 0; i < N_bitmaps; i++)
	{
		cfread(bmh.name, 8, 1, Piggy_fp);
		bmh.dflags = CF_ReadByte(Piggy_fp);
		bmh.width = CF_ReadByte(Piggy_fp);
		bmh.height = CF_ReadByte(Piggy_fp);
		bmh.wh_extra = CF_ReadByte(Piggy_fp);
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
		temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short)(bmh.wh_extra & 0x0f) << 8);
		temp_bitmap.bm_h = bmh.height + ((short)(bmh.wh_extra & 0xf0) << 4);
		temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;
		temp_bitmap.bm_data = Piggy_bitmap_cache_data;

		GameBitmapFlags[i + 1] = 0;
		if (bmh.flags & BM_FLAG_TRANSPARENT) GameBitmapFlags[i + 1] |= BM_FLAG_TRANSPARENT;
		if (bmh.flags & BM_FLAG_SUPER_TRANSPARENT) GameBitmapFlags[i + 1] |= BM_FLAG_SUPER_TRANSPARENT;
		if (bmh.flags & BM_FLAG_NO_LIGHTING) GameBitmapFlags[i + 1] |= BM_FLAG_NO_LIGHTING;
		if (bmh.flags & BM_FLAG_RLE) GameBitmapFlags[i + 1] |= BM_FLAG_RLE;
		if (bmh.flags & BM_FLAG_RLE_BIG) GameBitmapFlags[i + 1] |= BM_FLAG_RLE_BIG;

		GameBitmapOffset[i + 1] = bmh.offset + data_start;
		Assert((i + 1) == Num_bitmap_files);
		piggy_register_bitmap(&temp_bitmap, temp_name, 1);
	}

#ifdef EDITOR
	Piggy_bitmap_cache_size = data_size + (data_size / 10);   //extra mem for new bitmaps
	Assert(Piggy_bitmap_cache_size > 0);
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
#endif
	BitmapBits = (uint8_t*)malloc(Piggy_bitmap_cache_size);
	if (BitmapBits == NULL)
		Error("Not enough memory to load bitmaps\n");
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

#if defined(MACINTOSH) && defined(SHAREWARE)
	//	load_exit_models();
#endif

	Pigfile_initialized = 1;
}

#define FILENAME_LEN 13
#define MAX_BITMAPS_PER_BRUSH 30

extern int compute_average_pixel(grs_bitmap * newbm);

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(const char* pigname)
{
	int i;
	char temp_name[16];
	char temp_name_read[16];
	grs_bitmap temp_bitmap;
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_size, data_start;
	int must_rewrite_pig = 0;
#ifdef MACINTOSH
	char name[255];
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char name[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

#ifdef SHAREWARE                //rename pigfile for shareware
	if (strfcmp(pigname, DEFAULT_PIGFILE_REGISTERED) == 0)
		pigname = DEFAULT_PIGFILE_SHAREWARE;
#endif

	if (_strnfcmp(Current_pigfile, pigname, sizeof(Current_pigfile)) == 0)
		return;         //already have correct pig

	if (!Pigfile_initialized) //have we ever opened a pigfile?
	{
		piggy_init_pigfile(pigname);            //..no, so do initialization stuff
		return;
	}
	else
		piggy_close_file();             //close old pig if still open

	Piggy_bitmap_cache_next = 0;            //free up cache

	strncpy(Current_pigfile, pigname, sizeof(Current_pigfile));

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(name, pigname, CHOCOLATE_SYSTEM_FILE_DIR);
	Piggy_fp = cfopen(name, "rb");
#else
	Piggy_fp = cfopen(pigname, "rb");
#endif

#ifndef EDITOR
	//if (!Piggy_fp)
	//	Piggy_fp = copy_pigfile_from_cd(pigname);
#endif

	if (Piggy_fp) //make sure pig is valid type file & is up-to-date
	{
		int pig_id, pig_version;

		pig_id = cfile_read_int(Piggy_fp);
		pig_version = cfile_read_int(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION)
		{
			cfclose(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

#ifndef EDITOR
	if (!Piggy_fp) Error("Piggy_fp not defined in piggy_new_pigfile.");
#endif

	if (Piggy_fp)
	{

		N_bitmaps = cfile_read_int(Piggy_fp);
		header_size = N_bitmaps * BITMAP_HEADER_SIZE;
		data_start = header_size + cftell(Piggy_fp);
		data_size = cfilelength(Piggy_fp) - data_start;

		for (i = 1; i <= N_bitmaps; i++)
		{
			cfread(bmh.name, 8, 1, Piggy_fp);
			bmh.dflags = CF_ReadByte(Piggy_fp);
			bmh.width = CF_ReadByte(Piggy_fp);
			bmh.height = CF_ReadByte(Piggy_fp);
			bmh.wh_extra = CF_ReadByte(Piggy_fp);
			bmh.flags = CF_ReadByte(Piggy_fp);
			bmh.avg_color = CF_ReadByte(Piggy_fp);
			bmh.offset = CF_ReadInt(Piggy_fp);

			memcpy(temp_name_read, bmh.name, 8);
			temp_name_read[8] = 0;

			if (bmh.dflags & DBM_FLAG_ABM)
				sprintf(temp_name, "%s#%d", temp_name_read, bmh.dflags & 63);
			else
				strcpy(temp_name, temp_name_read);

			//Make sure name matches
			if (strcmp(temp_name, AllBitmaps[i].name))
			{
				//Int3();       //this pig is out of date.  Delete it
				must_rewrite_pig = 1;
			}

			strcpy(AllBitmaps[i].name, temp_name);

			memset(&temp_bitmap, 0, sizeof(grs_bitmap));

			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short)(bmh.wh_extra & 0x0f) << 8);
			temp_bitmap.bm_h = bmh.height + ((short)(bmh.wh_extra & 0xf0) << 4);
			temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
			temp_bitmap.avg_color = bmh.avg_color;
			temp_bitmap.bm_data = Piggy_bitmap_cache_data;

			GameBitmapFlags[i] = 0;

			if (bmh.flags & BM_FLAG_TRANSPARENT) GameBitmapFlags[i] |= BM_FLAG_TRANSPARENT;
			if (bmh.flags & BM_FLAG_SUPER_TRANSPARENT) GameBitmapFlags[i] |= BM_FLAG_SUPER_TRANSPARENT;
			if (bmh.flags & BM_FLAG_NO_LIGHTING) GameBitmapFlags[i] |= BM_FLAG_NO_LIGHTING;
			if (bmh.flags & BM_FLAG_RLE) GameBitmapFlags[i] |= BM_FLAG_RLE;
			if (bmh.flags & BM_FLAG_RLE_BIG) GameBitmapFlags[i] |= BM_FLAG_RLE_BIG;

			GameBitmapOffset[i] = bmh.offset + data_start;

			GameBitmaps[i] = temp_bitmap;
		}
	}
	else
		N_bitmaps = 0;          //no pigfile, so no bitmaps

#ifndef EDITOR

	Assert(N_bitmaps == Num_bitmap_files - 1);

#else

	if (must_rewrite_pig || (N_bitmaps < Num_bitmap_files - 1)) {
		int size;

		//re-read the bitmaps that aren't in this pig

		for (i = N_bitmaps + 1; i < Num_bitmap_files; i++) {
			char* p;

			p = strchr(AllBitmaps[i].name, '#');

			if (p) {                //this is an ABM
				char abmname[FILENAME_LEN];
				int fnum;
				grs_bitmap* bm[MAX_BITMAPS_PER_BRUSH];
				int iff_error;          //reference parm to avoid warning message
				uint8_t newpal[768];
				char basename[FILENAME_LEN];
				int nframes;

				strcpy(basename, AllBitmaps[i].name);
				basename[p - AllBitmaps[i].name] = 0;             //cut off "#nn" part

				sprintf(abmname, "%s.abm", basename);

				iff_error = iff_read_animbrush(abmname, bm, MAX_BITMAPS_PER_BRUSH, &nframes, newpal);

				if (iff_error != IFF_NO_ERROR) {
					mprintf((1, "File %s - IFF error: %s", abmname, iff_errormsg(iff_error)));
					Error("File %s - IFF error: %s", abmname, iff_errormsg(iff_error));
				}

				for (fnum = 0; fnum < nframes; fnum++) {
					char tempname[20];
					int SuperX;

					sprintf(tempname, "%s#%d", basename, fnum);

					//SuperX = (GameBitmaps[i+fnum].bm_flags&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					SuperX = (GameBitmapFlags[i + fnum] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
					//above makes assumption that supertransparent color is 254

					if (iff_has_transparency)
						gr_remap_bitmap_good(bm[fnum], newpal, iff_transparent_color, SuperX);
					else
						gr_remap_bitmap_good(bm[fnum], newpal, -1, SuperX);

					bm[fnum]->avg_color = compute_average_pixel(bm[fnum]);

#ifdef EDITOR
					if (FindArg("-macdata"))
						swap_0_255(bm[fnum]);
#endif
					if (!BigPig) gr_bitmap_rle_compress(bm[fnum]);

					if (bm[fnum]->bm_flags & BM_FLAG_RLE)
						size = *((int*)bm[fnum]->bm_data);
					else
						size = bm[fnum]->bm_w * bm[fnum]->bm_h;

					memcpy(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], bm[fnum]->bm_data, size);
					free(bm[fnum]->bm_data);
					bm[fnum]->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
					Piggy_bitmap_cache_next += size;

					GameBitmaps[i + fnum] = *bm[fnum];

					// -- mprintf( (0, "U" ));
					free(bm[fnum]);
				}

				i += nframes - 1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM

				grs_bitmap* newbm;
				uint8_t newpal[256 * 3];
				int iff_error;
				char bbmname[FILENAME_LEN];
				int SuperX;

				MALLOC(newbm, grs_bitmap, 1);

				sprintf(bbmname, "%s.bbm", AllBitmaps[i].name);
				iff_error = iff_read_bitmap(bbmname, newbm, BM_LINEAR, newpal);

				//newbm->bm_handle = 0;
				if (iff_error != IFF_NO_ERROR) 
				{
					mprintf((1, "File %s - IFF error: %s", bbmname, iff_errormsg(iff_error)));
					Error("File %s - IFF error: %s", bbmname, iff_errormsg(iff_error));
				}

				SuperX = (GameBitmapFlags[i] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
				//above makes assumption that supertransparent color is 254

				if (iff_has_transparency)
					gr_remap_bitmap_good(newbm, newpal, iff_transparent_color, SuperX);
				else
					gr_remap_bitmap_good(newbm, newpal, -1, SuperX);

				newbm->avg_color = compute_average_pixel(newbm);

#ifdef EDITOR
				if (FindArg("-macdata"))
					swap_0_255(newbm);
#endif
				if (!BigPig)  gr_bitmap_rle_compress(newbm);

				if (newbm->bm_flags & BM_FLAG_RLE)
					size = *((int*)newbm->bm_data);
				else
					size = newbm->bm_w * newbm->bm_h;

				memcpy(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], newbm->bm_data, size);
				free(newbm->bm_data);
				newbm->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
				Piggy_bitmap_cache_next += size;

				GameBitmaps[i] = *newbm;

				free(newbm);

				// -- mprintf( (0, "U" ));
			}
		}

		//@@Dont' do these things which are done when writing
		//@@for (i=0; i < Num_bitmap_files; i++ )       {
		//@@    bitmap_index bi;
		//@@    bi.index = i;
		//@@    PIGGY_PAGE_IN( bi );
		//@@}
		//@@
		//@@piggy_close_file();

		piggy_write_pigfile(pigname);

		Current_pigfile[0] = 0;                 //say no pig, to force reload

		piggy_new_pigfile(pigname);             //read in just-generated pig
	}
#endif  //ifdef EDITOR

}

uint8_t bogus_data[64 * 64];
grs_bitmap bogus_bitmap;
uint8_t bogus_bitmap_initialized = 0;
digi_sound bogus_sound;

extern void bm_read_all(CFILE* fp);

#define HAMFILE_ID              '!MAH'          //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              'DNSD'          //DSND
#define SNDFILE_VERSION 1

int read_hamfile()
{
	CFILE* ham_fp = NULL;
	int ham_id, ham_version;
#ifdef MACINTOSH
	char name[255];
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char name[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

#ifdef MACINTOSH
	sprintf(name, ":Data:%s", DEFAULT_HAMFILE);
	ham_fp = cfopen(name, "rb");
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(name, DEFAULT_HAMFILE, CHOCOLATE_SYSTEM_FILE_DIR);
	ham_fp = cfopen(name, "rb");
#else
	ham_fp = cfopen(DEFAULT_HAMFILE, "rb");
#endif

	if (ham_fp == NULL) {
		Must_write_hamfile = 1;
		return 0;
	}

	//make sure ham is valid type file & is up-to-date
	ham_id = cfile_read_int(ham_fp);
	ham_version = cfile_read_int(ham_fp);
	if (ham_id != HAMFILE_ID || ham_version != HAMFILE_VERSION)
	{
		Must_write_hamfile = 1;
		cfclose(ham_fp);						//out of date ham
		return 0;
	}

#ifndef EDITOR
	{
		bm_read_all(ham_fp);  // Note connection to above if!!!
		cfread(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, ham_fp);
#ifdef MACINTOSH
		{
			int i;

			for (i = 0; i < MAX_BITMAP_FILES; i++)
				GameBitmapXlat[i] = SWAPSHORT(GameBitmapXlat[i]);
		}
#endif
	}
#endif

	cfclose(ham_fp);

	return 1;
}

int read_sndfile()
{
	CFILE* snd_fp = NULL;
	int snd_id, snd_version;
	int N_sounds;
	int sound_start;
	int header_size;
	int i, size, length;
	DiskSoundHeader sndh;
	digi_sound temp_sound;
	char temp_name_read[16];
	int sbytes = 0;
#ifdef MACINTOSH
	char name[255];
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char name[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

#ifdef MACINTOSH
	sprintf(name, ":Data:%s", DEFAULT_SNDFILE);
	snd_fp = cfopen(name, "rb");
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(name, DEFAULT_SNDFILE, CHOCOLATE_SYSTEM_FILE_DIR);
	snd_fp = cfopen(name, "rb");
#else
	snd_fp = cfopen(DEFAULT_SNDFILE, "rb");
#endif

	if (snd_fp == NULL)
		return 0;

	//make sure soundfile is valid type file & is up-to-date
	snd_id = cfile_read_int(snd_fp);
	snd_version = cfile_read_int(snd_fp);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION)
	{
		cfclose(snd_fp);						//out of date sound file
		return 0;
	}

	N_sounds = cfile_read_int(snd_fp);

	sound_start = cftell(snd_fp);
	size = cfilelength(snd_fp) - sound_start;
	length = size;
	mprintf((0, "\nReading data (%d KB) ", size / 1024));

	header_size = N_sounds * SOUND_HEADER_SIZE;

	//Read sounds

	for (i = 0; i < N_sounds; i++)
	{
		cfread(sndh.name, 8, 1, snd_fp);
		sndh.length = cfile_read_int(snd_fp);
		sndh.data_length = cfile_read_int(snd_fp);
		sndh.offset = cfile_read_int(snd_fp);
		//		cfread( &sndh, sizeof(DiskSoundHeader), 1, snd_fp );
				//size -= sizeof(DiskSoundHeader);
		temp_sound.length = sndh.length;
		temp_sound.data = (uint8_t*)(sndh.offset + header_size + sound_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + sound_start;
		memcpy(temp_name_read, sndh.name, 8);
		temp_name_read[8] = 0;
		piggy_register_sound(&temp_sound, temp_name_read, 1);
#ifdef MACINTOSH
		if (piggy_is_needed(i))
#endif		// note link to if.
			sbytes += sndh.length;
		//mprintf(( 0, "%d bytes of sound\n", sbytes ));
	}

	SoundBits = (uint8_t*)malloc(sbytes + 16);
	if (SoundBits == NULL)
		Error("Not enough memory to load sounds\n");

	mprintf((0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size / 1024, sbytes / 1024));

	//	piggy_read_sounds(snd_fp);

	cfclose(snd_fp);

	return 1;
}

int piggy_init(void)
{
	int ham_ok = 0, snd_ok = 0;
	int i;

	hashtable_init(&AllBitmapsNames, MAX_BITMAP_FILES);
	hashtable_init(&AllDigiSndNames, MAX_SOUND_FILES);

	for (i = 0; i < MAX_SOUND_FILES; i++)
	{
		GameSounds[i].length = 0;
		GameSounds[i].data = NULL;
		SoundOffset[i] = 0;
	}

	for (i = 0; i < MAX_BITMAP_FILES; i++)
		GameBitmapXlat[i] = i;

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

	if (FindArg("-bigpig"))
		BigPig = 1;

	if (FindArg("-lowmem"))
		piggy_low_memory = 1;

	if (FindArg("-nolowmem"))
		piggy_low_memory = 0;

	if (piggy_low_memory)
		digi_lomem = 1;

	if ((i = FindArg("-piggy")))
		Error("-piggy no longer supported");

	WIN(DDGRLOCK(dd_grd_curcanv));
	gr_set_curfont(SMALL_FONT);
	gr_set_fontcolor(gr_find_closest_color_current(20, 20, 20), -1);
	gr_printf(0x8000, grd_curcanv->cv_h - 20, "%s...", TXT_LOADING_DATA);
	WIN(DDGRUNLOCK(dd_grd_curcanv));

#ifdef EDITOR
	piggy_init_pigfile(DEFAULT_PIGFILE);
#endif

	ham_ok = read_hamfile();

	snd_ok = read_sndfile();

	atexit(piggy_close);

	mprintf((0, "HamOk=%d SndOk=%d\n", ham_ok, snd_ok));
	return (ham_ok && snd_ok);               //read ok
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


void piggy_read_sounds(void)
{
	CFILE* fp = NULL;
	uint8_t* ptr;
	int i, sbytes;
#ifdef MACINTOSH
	char name[255];
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char name[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

	ptr = SoundBits;
	sbytes = 0;

#ifdef MACINTOSH
	sprintf(name, ":Data:%s", DEFAULT_SNDFILE);
	fp = cfopen(name, "rb");
#elif defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(name, DEFAULT_SNDFILE, CHOCOLATE_SYSTEM_FILE_DIR);
	fp = cfopen(name, "rb");
#else
	fp = cfopen(DEFAULT_SNDFILE, "rb");
#endif

	if (fp == NULL)
		return;

	for (i = 0; i < Num_sound_files; i++) {
		digi_sound* snd = &GameSounds[i];

		if (SoundOffset[i] > 0) {
			if (piggy_is_needed(i)) {
				cfseek(fp, SoundOffset[i], SEEK_SET);

				// Read in the sound data!!!
				snd->data = ptr;
				ptr += snd->length;
				sbytes += snd->length;
				cfread(snd->data, snd->length, 1, fp);
			}
			else
				snd->data = (uint8_t*)-1;
		}
	}

	cfclose(fp);

	mprintf((0, "\nActual Sound usage: %d KB\n", sbytes / 1024));

}


extern int descent_critical_error;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

const char* crit_errors[13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media type", "Sector not found", "Printer out of paper", "Write Fault", \
"Read fault", "General Failure" };

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
		exit(1);
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

	if (GameBitmapOffset[i] == 0) return;         // A read-from-disk bitmap!!!

	if (piggy_low_memory) {
		org_i = i;
		i = GameBitmapXlat[i];          // Xlat for low-memory settings!
	}

	bmp = &GameBitmaps[i];

	if (bmp->bm_flags & BM_FLAG_PAGED_OUT) {
		stop_time();

	ReDoIt:
		descent_critical_error = 0;
		cfseek(Piggy_fp, GameBitmapOffset[i], SEEK_SET);
		if (descent_critical_error) {
			piggy_critical_error();
			goto ReDoIt;
		}

		bmp->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
		bmp->bm_flags = GameBitmapFlags[i];

		if (bmp->bm_flags & BM_FLAG_RLE)
		{
			int zsize = 0;
			descent_critical_error = 0;
			zsize = cfile_read_int(Piggy_fp);
			if (descent_critical_error)
			{
				piggy_critical_error();
				goto ReDoIt;
			}

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			//Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );      
			if (Piggy_bitmap_cache_next + zsize >= Piggy_bitmap_cache_size)
			{
				Int3();
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
			if (Piggy_bitmap_cache_next + (bmp->bm_h * bmp->bm_w) >= Piggy_bitmap_cache_size) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			temp = cfread(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h * bmp->bm_w, Piggy_fp);
			if (descent_critical_error) {
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += bmp->bm_h * bmp->bm_w;
		}

		//@@if ( bmp->bm_selector ) {
		//@@#if !defined(WINDOWS) && !defined(MACINTOSH)
		//@@	if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
		//@@		Error( "Error modifying selector base in piggy.c\n" );
		//@@#endif
		//@@}

		start_time();
	}

	if (piggy_low_memory) {
		if (org_i != i)
			GameBitmaps[org_i] = GameBitmaps[i];
	}

	//@@Removed from John's code:
	//@@#ifndef WINDOWS
	//@@    if ( bmp->bm_selector ) {
	//@@            if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
	//@@                    Error( "Error modifying selector base in piggy.c\n" );
	//@@    }
	//@@#endif

}

void piggy_bitmap_page_out_all()
{
	int i;

	Piggy_bitmap_cache_next = 0;

	piggy_page_flushed++;

	texmerge_flush();
	rle_cache_flush();

	for (i = 0; i < Num_bitmap_files; i++) {
		if (GameBitmapOffset[i] > 0) {       // Don't page out bitmaps read from disk!!!
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

void change_filename_ext(char* dest, const char* src, const char* ext);

void piggy_write_pigfile(const char* filename)
{
	FILE* pig_fp;
	int bitmap_data_start, data_offset;
	DiskBitmapHeader bmh;
	int org_offset;
	char subst_name[32];
	int i;
	FILE* fp1, * fp2;
	char tname[FILENAME_LEN];
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

	// -- mprintf( (0, "Paging in all piggy bitmaps..." ));
	for (i = 0; i < Num_bitmap_files; i++) {
		bitmap_index bi;
		bi.index = i;
		PIGGY_PAGE_IN(bi);
	}
	// -- mprintf( (0, "\n" ));

	piggy_close_file();

	// -- mprintf( (0, "Creating %s...",filename ));

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(filename_full_path, filename, CHOCOLATE_SYSTEM_FILE_DIR);
	pig_fp = fopen(filename_full_path, "wb");
#else
	pig_fp = fopen(filename, "wb");       //open PIG file
#endif
	Assert(pig_fp != NULL);

	write_int(PIGFILE_ID, pig_fp);
	write_int(PIGFILE_VERSION, pig_fp);

	Num_bitmap_files--;
	fwrite(&Num_bitmap_files, sizeof(int), 1, pig_fp);
	Num_bitmap_files++;

	bitmap_data_start = ftell(pig_fp);
	bitmap_data_start += (Num_bitmap_files - 1) * sizeof(DiskBitmapHeader);
	data_offset = bitmap_data_start;

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	change_filename_ext(tname, filename, "lst");
	get_full_file_path(filename_full_path, tname, CHOCOLATE_SYSTEM_FILE_DIR);
	fp1 = fopen(filename_full_path, "wt");
	change_filename_ext(tname, filename, "all");
	get_full_file_path(filename_full_path, tname, CHOCOLATE_SYSTEM_FILE_DIR);
	fp2 = fopen(filename_full_path, "wt");
#else
	change_filename_ext(tname, filename, "lst");
	fp1 = fopen(tname, "wt");
	change_filename_ext(tname, filename, "all");
	fp2 = fopen(tname, "wt");
#endif

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
				if (fp2 && n == 0)
					fprintf(fp2, "%s.abm\n", AllBitmaps[i].name);
				memcpy(bmh.name, AllBitmaps[i].name, 8);
				Assert(n <= 63);
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			}
			else {
				if (fp2)
					fprintf(fp2, "%s.bbm\n", AllBitmaps[i].name);
				memcpy(bmh.name, AllBitmaps[i].name, 8);
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[i];

		Assert(!(bmp->bm_flags & BM_FLAG_PAGED_OUT));

		if (fp1)
			fprintf(fp1, "BMP: %s, size %d bytes", AllBitmaps[i].name, bmp->bm_rowsize * bmp->bm_h);
		org_offset = ftell(pig_fp);
		bmh.offset = data_offset - bitmap_data_start;
		fseek(pig_fp, data_offset, SEEK_SET);

		if (bmp->bm_flags & BM_FLAG_RLE) {
			size = (int*)bmp->bm_data;
			fwrite(bmp->bm_data, sizeof(uint8_t), *size, pig_fp);
			data_offset += *size;
			if (fp1)
				fprintf(fp1, ", and is already compressed to %d bytes.\n", *size);
		}
		else {
			fwrite(bmp->bm_data, sizeof(uint8_t), bmp->bm_rowsize * bmp->bm_h, pig_fp);
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			if (fp1)
				fprintf(fp1, ".\n");
		}
		fseek(pig_fp, org_offset, SEEK_SET);
		Assert(GameBitmaps[i].bm_w < 4096);
		bmh.width = (GameBitmaps[i].bm_w & 0xff);
		bmh.wh_extra = ((GameBitmaps[i].bm_w >> 8) & 0x0f);
		Assert(GameBitmaps[i].bm_h < 4096);
		bmh.height = GameBitmaps[i].bm_h;
		bmh.wh_extra |= ((GameBitmaps[i].bm_h >> 4) & 0xf0);
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
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color = GameBitmaps[i].avg_color;
		fwrite(&bmh, sizeof(DiskBitmapHeader), 1, pig_fp);                    // Mark as a bitmap
	}

	fclose(pig_fp);

	mprintf((0, " Dumped %d assorted bitmaps.\n", Num_bitmap_files));
	fprintf(fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files);

	fclose(fp1);
	fclose(fp2);

}

static void write_int(int i, FILE* file)
{
	if (fwrite(&i, sizeof(i), 1, file) != 1)
		Error("Error reading int in gamesave.c");

}

void piggy_dump_all()
{
	int i, xlat_offset;
	FILE* ham_fp;
	int org_offset, data_offset;
	DiskSoundHeader sndh;
	int sound_data_start;
	FILE* fp1, * fp2;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif

#ifdef NO_DUMP_SOUNDS
	Num_sound_files = 0;
	Num_sound_files_new = 0;
#endif

	if (!Must_write_hamfile && (Num_bitmap_files_new == 0) && (Num_sound_files_new == 0))
		return;

	if ((i = FindArg("-piggy")))
		Error("-piggy no longer supported");

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(filename_full_path, "ham.lst", CHOCOLATE_SYSTEM_FILE_DIR);
	fp1 = fopen(filename_full_path, "wt");
	get_full_file_path(filename_full_path, "ham.all", CHOCOLATE_SYSTEM_FILE_DIR);
	fp2 = fopen(filename_full_path, "wt");
#else
	fp1 = fopen("ham.lst", "wt");
	fp2 = fopen("ham.all", "wt");
#endif

	if (Must_write_hamfile || Num_bitmap_files_new) {

		mprintf((0, "Creating %s...", DEFAULT_HAMFILE));

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		get_full_file_path(filename_full_path, DEFAULT_HAMFILE, CHOCOLATE_SYSTEM_FILE_DIR);
		ham_fp = fopen(filename_full_path, "wb");
#else
		ham_fp = fopen(DEFAULT_HAMFILE, "wb");                       //open HAM file
#endif
		Assert(ham_fp != NULL);

		write_int(HAMFILE_ID, ham_fp);
		write_int(HAMFILE_VERSION, ham_fp);

		bm_write_all(ham_fp);
		xlat_offset = ftell(ham_fp);
		fwrite(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, ham_fp);
		//Dump bitmaps

		if (Num_bitmap_files_new)
			piggy_write_pigfile(DEFAULT_PIGFILE);

		//free up memeory used by new bitmaps
		for (i = Num_bitmap_files - Num_bitmap_files_new; i < Num_bitmap_files; i++)
			free(GameBitmaps[i].bm_data);

		//next thing must be done after pig written
		fseek(ham_fp, xlat_offset, SEEK_SET);
		fwrite(GameBitmapXlat, sizeof(uint16_t) * MAX_BITMAP_FILES, 1, ham_fp);

		fclose(ham_fp);
		mprintf((0, "\n"));
	}

	if (Num_sound_files_new) {

		mprintf((0, "Creating %s...", DEFAULT_HAMFILE));
		// Now dump sound file
#if defined (CHOCOLATE_USE_LOCALIZED_PATHS)
		get_full_file_path(filename_full_path, DEFAULT_SNDFILE, CHOCOLATE_SYSTEM_FILE_DIR);
		ham_fp = fopen(filename_full_path, "wb");
#else
		ham_fp = fopen(DEFAULT_SNDFILE, "wb");
#endif
		Assert(ham_fp != NULL);

		write_int(SNDFILE_ID, ham_fp);
		write_int(SNDFILE_VERSION, ham_fp);

		fwrite(&Num_sound_files, sizeof(int), 1, ham_fp);

		mprintf((0, "\nDumping sounds..."));

		sound_data_start = ftell(ham_fp);
		sound_data_start += Num_sound_files * sizeof(DiskSoundHeader);
		data_offset = sound_data_start;

		for (i = 0; i < Num_sound_files; i++) {
			digi_sound* snd;

			snd = &GameSounds[i];
			strcpy(sndh.name, AllSounds[i].name);
			sndh.length = GameSounds[i].length;
			sndh.offset = data_offset - sound_data_start;

			org_offset = ftell(ham_fp);
			fseek(ham_fp, data_offset, SEEK_SET);

			sndh.data_length = GameSounds[i].length;
			fwrite(snd->data, sizeof(uint8_t), snd->length, ham_fp);
			data_offset += snd->length;
			fseek(ham_fp, org_offset, SEEK_SET);
			fwrite(&sndh, sizeof(DiskSoundHeader), 1, ham_fp);                    // Mark as a bitmap

			fprintf(fp1, "SND: %s, size %d bytes\n", AllSounds[i].name, snd->length);
			fprintf(fp2, "%s.raw\n", AllSounds[i].name);
		}

		fclose(ham_fp);
		mprintf((0, "\n"));
	}

	fprintf(fp1, "Total sound size: %d bytes\n", data_offset - sound_data_start);
	mprintf((0, " Dumped %d assorted sounds.\n", Num_sound_files));
	fprintf(fp1, " Dumped %d assorted sounds.\n", Num_sound_files);

	fclose(fp1);
	fclose(fp2);

	// Never allow the game to run after building ham.
	exit(0);
}

#endif

void piggy_close()
{
	piggy_close_file();

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


#define NUM_GAUGE_BITMAPS 23
const char* gauge_bitmap_names[NUM_GAUGE_BITMAPS] = {
	"gauge01", "gauge01b",
	"gauge02", "gauge02b",
	"gauge06", "gauge06b",
	"targ01", "targ01b",
	"targ02", "targ02b",
	"targ03", "targ03b",
	"targ04", "targ04b",
	"targ05", "targ05b",
	"targ06", "targ06b",
	"gauge18", "gauge18b",
	"gauss1", "helix1",
	"phoenix1"
};


int piggy_is_gauge_bitmap(char* base_name)
{
	int i;
	for (i = 0; i < NUM_GAUGE_BITMAPS; i++) {
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
	if (p) {
		frame = atoi(&p[1]);
		*p = 0;
		strcpy(base_name, subst_name);
		if (!piggy_is_gauge_bitmap(base_name)) {
			sprintf(subst_name, "%s#%d", base_name, frame + 1);
			if (piggy_does_bitmap_exist_slow(subst_name)) {
				if (frame & 1) {
					sprintf(subst_name, "%s#%d", base_name, frame - 1);
					return 1;
				}
			}
		}
	}
	strcpy(subst_name, name);
	return 0;
}



#ifdef WINDOWS
//	New Windows stuff

//	windows bitmap page in
//		Page in a bitmap, if ddraw, then page it into a ddsurface in 
//		'video' memory.  if that fails, page it in normally.

void piggy_bitmap_page_in_w(bitmap_index bitmap, int ddraw)
{
}


//	Essential when switching video modes!

void piggy_bitmap_page_out_all_w()
{
}


#endif


