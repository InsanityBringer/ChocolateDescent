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

#ifdef EDITOR

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "platform/posixstub.h"

#include "misc/types.h"
#include "inferno.h"
#include "2d/gr.h"
#include "bm.h"
#include "mem/mem.h"
#include "cfile/cfile.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "object.h"
#include "vclip.h"
#include "main_shared/effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"

#include "iff/iff.h"
#include "cfile/cfile.h"

#include "hostage.h"
#include "powerup.h"
#include "laser.h"
#include "sounds.h"
#include "main_shared/piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "main_shared/compbit.h"
#include "misc/args.h"


#include "editor\texpage.h"

#define BM_NONE			-1
#define BM_COCKPIT		 0
#define BM_TEXTURES		 2
#define BM_UNUSED		 	 3
#define BM_VCLIP		 	 4
#define BM_EFFECTS	    5
#define BM_ECLIP 	 		 6
#define BM_WEAPON			 7
#define BM_DEMO	 		 8
#define BM_ROBOTEX	    9
#define BM_WALL_ANIMS	12
#define BM_WCLIP 			13
#define BM_ROBOT			14
#define BM_GAUGES			20

#define MAX_BITMAPS_PER_BRUSH 30

extern player_ship only_player_ship;		// In bm.c

static short		N_ObjBitmaps = 0;
static short		N_ObjBitmapPtrs = 0;
static int			Num_robot_ais = 0;
int	TmapList[MAX_TEXTURES];
char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];
char	Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//	For the sake of LINT, defining prototypes to module's functions
void bm_read_robot_ai(void);
void bm_read_powerup(int unused_flag);
void bm_read_hostage(void);
void bm_read_robot(void);
void bm_read_weapon(int unused_flag);
void bm_read_object(void);
void bm_read_exitmodel(void);
void bm_read_player_ship(void);
void bm_read_some_file(void);
void bm_read_sound(void);
void verify_textures(void);
void bm_read_hostage_face(void);


//---------------- Internal variables ---------------------------
static int 			Registered_only = 0;		//	Gets set by ! in column 1.
static int			SuperX = -1;
static int			Installed = 0;
static char* arg;
static short 		tmap_count = 0;
static short 		texture_count = 0;
static short 		clip_count = 0;
static short 		clip_num;
static short 		sound_num;
static short 		frames;
static float 		time;
static int			hit_sound = -1;
static int8_t 		bm_flag = BM_NONE;
static int 			abm_flag = 0;
static int 			rod_flag = 0;
static short		wall_open_sound, wall_close_sound, wall_explodes, wall_blastable, wall_hidden;
float		vlighting = 0;
static int			obj_eclip;
static char* dest_bm;		//clip number to play when destroyed
static int			dest_vclip;		//what vclip to play when exploding
static int			dest_eclip;		//what eclip to play when exploding
static fix			dest_size;		//3d size of explosion
static int			crit_clip;		//clip number to play when destroyed
static int			crit_flag;		//flag if this is a destroyed eclip
static int			tmap1_flag;		//flag if this is used as tmap_num (not tmap_num2)
static int			num_sounds = 0;



//------------------- Useful macros and variables ---------------
#define REMOVE_EOL(s)		remove_char((s),'\n')
#define REMOVE_COMMENTS(s)	remove_char((s),';')
#define REMOVE_DOTS(s)  	remove_char((s),'.')

#define IFTOK(str) if (!strcmp(arg, str))
char* space = { " \t" };
//--unused-- char *equal = { "=" };
char* equal_space = { " \t=" };

void remove_char(char* s, char c)
{
	char* p;
	p = strchr(s, c);
	if (p)*p = '\0';
}

//---------------------------------------------------------------
int compute_average_pixel(grs_bitmap* newbm)
{
	int	row, column, color;
	unsigned char* pptr;
	int	total_red, total_green, total_blue;

	pptr = newbm->bm_data;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	for (row = 0; row < newbm->bm_h; row++)
		for (column = 0; column < newbm->bm_w; column++) {
			color = *pptr++;
			total_red += gr_palette[color * 3];
			total_green += gr_palette[color * 3 + 1];
			total_blue += gr_palette[color * 3 + 2];
		}

	total_red /= (newbm->bm_h * newbm->bm_w);
	total_green /= (newbm->bm_h * newbm->bm_w);
	total_blue /= (newbm->bm_h * newbm->bm_w);

	return BM_XRGB(total_red / 2, total_green / 2, total_blue / 2);
}

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

bitmap_index bm_load_sub(char* filename)
{
	bitmap_index bitmap_num;
	grs_bitmap* newbm;
	uint8_t newpal[256 * 3];
	int iff_error;		//reference parm to avoid warning message
	char fname[20];

	bitmap_num.index = 0;

#ifdef SHAREWARE
	if (Registered_only)
	{
		//mprintf( 0, "Skipping registered-only bitmap '%s'\n", filename );
		return bitmap_num;
	}
#endif

	_splitpath(filename, NULL, NULL, fname, NULL);

	bitmap_num = piggy_find_bitmap(fname);
	if (bitmap_num.index)
	{
		//mprintf(( 0, "Found bitmap '%s' in pig!\n", fname ));
		return bitmap_num;
	}

	MALLOC(newbm, grs_bitmap, 1);
	//newbm = (grs_bitmap*)malloc(1 * sizeof(grs_bitmap));
	iff_error = iff_read_bitmap(filename, newbm, BM_LINEAR, newpal);
	newbm->bm_selector = 0;
	if (iff_error != IFF_NO_ERROR)
	{
		mprintf((1, "File %s - IFF error: %s", filename, iff_errormsg(iff_error)));
		Error("File %s - IFF error: %s", filename, iff_errormsg(iff_error));
	}

	if (iff_has_transparency)
		gr_remap_bitmap_good(newbm, newpal, iff_transparent_color, SuperX);
	else
		gr_remap_bitmap_good(newbm, newpal, -1, SuperX);

	newbm->avg_color = compute_average_pixel(newbm);

	mprintf((0, "N"));
	bitmap_num = piggy_register_bitmap(newbm, fname, 0);
	mem_free(newbm);
	return bitmap_num;
}

void ab_load(char* filename, bitmap_index bmp[], int* nframes)
{
	grs_bitmap* bm[MAX_BITMAPS_PER_BRUSH];
	bitmap_index bi;
	int i;
	int iff_error;		//reference parm to avoid warning message
	uint8_t newpal[768];
	char fname[20];
	char tempname[20];

#ifdef SHAREWARE
	if (Registered_only)
	{
		Assert(bogus_bitmap_initialized != 0);
		mprintf((0, "Skipping registered-only animation '%s'\n", filename));
		bmp[0] = &bogus_bitmap;
		*nframes = 1;
		return;
	}
#endif


	_splitpath(filename, NULL, NULL, fname, NULL);

	for (i = 0; i < MAX_BITMAPS_PER_BRUSH; i++)
	{
		sprintf(tempname, "%s#%d", fname, i);
		bi = piggy_find_bitmap(tempname);
		if (!bi.index)
			break;
		bmp[i] = bi;
		//mprintf(( 0, "Found animation frame %d, %s, in piggy file\n", i, tempname ));
	}

	if (i)
	{
		*nframes = i;
		return;
	}

	iff_error = iff_read_animbrush(filename, bm, MAX_BITMAPS_PER_BRUSH, nframes, &newpal[0]);
	if (iff_error != IFF_NO_ERROR)
	{
		mprintf((1, "File %s - IFF error: %s", filename, iff_errormsg(iff_error)));
		Error("File %s - IFF error: %s", filename, iff_errormsg(iff_error));
	}

	for (i = 0; i < *nframes; i++)
	{
		bitmap_index new_bmp;
		sprintf(tempname, "%s#%d", fname, i);
		if (iff_has_transparency)
			gr_remap_bitmap_good(bm[i], newpal, iff_transparent_color, SuperX);
		else
			gr_remap_bitmap_good(bm[i], newpal, -1, SuperX);

		bm[i]->avg_color = compute_average_pixel(bm[i]);

		new_bmp = piggy_register_bitmap(bm[i], tempname, 0);
		mem_free(bm[i]);
		bmp[i] = new_bmp;
		mprintf((0, "Registering frame %d, %s, in piggy file\n", i, tempname));
	}
}

int ds_load(char* filename)
{
	int i;
	CFILE* cfp;
	digi_sound newbm;
	char fname[20];
	char rawname[100];

#ifdef SHAREWARE
	if (Registered_only)
	{
		//mprintf( 0, "Skipping registered-only sound '%s'\n", filename );
		return &bogus_sound;
	}
#endif

	_splitpath(filename, NULL, NULL, fname, NULL);
	_makepath(rawname, NULL, NULL, fname, ".RAW");

	i = piggy_find_sound(fname);
	if (i != 255) {
		return i;
	}

	cfp = cfopen(rawname, "rb");

	if (cfp != NULL)
	{
		newbm.length = cfilelength(cfp);
		MALLOC(newbm.data, uint8_t, newbm.length);
		//newbm.data = (uint8_t*)malloc(new.length * sizeof(uint8_t));
		cfread(newbm.data, 1, newbm.length, cfp);
		cfclose(cfp);
		mprintf((0, "S"));
		mprintf((0, "<%s>", rawname));
	}
	else
	{
		mprintf((1, "Warning: Couldn't find '%s'\n", filename));
		return 255;
	}
	i = piggy_register_sound(&newbm, fname, 0);
	return i;
}

//parse a float
float get_float()
{
	char* xarg;

	xarg = strtok(NULL, space);
	return atof(xarg);
}

//parse an int
int get_int()
{
	char* xarg;

	xarg = strtok(NULL, space);
	return atoi(xarg);
}

// rotates a byte left one bit, preserving the bit falling off the right
//void
//rotate_left(char *c)
//{
//	int found;
//
//	found = 0;
//	if (*c & 0x80)
//		found = 1;
//	*c = *c << 1;
//	if (found)
//		*c |= 0x01;
//}

#define LINEBUF_SIZE 600

int	linenum;

//-----------------------------------------------------------------
// Initializes all bitmaps from BITMAPS.TBL file.
int bm_init_use_tbl()
{
	CFILE* InfoFile;
	char	inputline[LINEBUF_SIZE];
	int	i, have_bin_tbl;

	init_polygon_models();

	ObjType[0] = OL_PLAYER;
	ObjId[0] = 0;
	Num_total_object_types = 1;

	for (i = 0; i < MAX_SOUNDS; i++) {
		Sounds[i] = 255;
		AltSounds[i] = 255;
	}

	for (i = 0; i < MAX_TEXTURES; i++) {
		TmapInfo[i].eclip_num = -1;
		TmapInfo[i].flags = 0;
	}

#ifndef SHAREWARE
	for (i = 0; i < MAX_HOSTAGES; i++)
		Hostage_face_clip[i].num_frames = 0;
#endif

	Num_effects = 0;
	for (i = 0; i < MAX_EFFECTS; i++) {
		//Effects[i].bm_ptr = (grs_bitmap **) -1;
		Effects[i].changing_wall_texture = -1;
		Effects[i].changing_object_texture = -1;
		Effects[i].segnum = -1;
		Effects[i].vc.num_frames = -1;		//another mark of being unused
	}

	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = Dead_modelnums[i] = -1;

	Num_vclips = 0;
	for (i = 0; i < VCLIP_MAXNUM; i++) {
		Vclip[i].num_frames = -1;
		Vclip[i].flags = 0;
	}

	for (i = 0; i < MAX_WALL_ANIMS; i++)
		WallAnims[i].num_frames = -1;
	Num_wall_anims = 0;

	setbuf(stdout, NULL);	// unbuffered output via printf

	if (Installed)
		return 1;

	Installed = 1;

	piggy_init();

	if (FindArg("-nobm")) {
		piggy_read_sounds();
		return 0;
	}

	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	InfoFile = cfopen("BITMAPS.TBL", "rb");
	if (InfoFile == NULL) {
		InfoFile = cfopen("BITMAPS.BIN", "rb");
		if (InfoFile == NULL)
			Error("Missing BITMAPS.TBL and BITMAPS.BIN file\n");
		have_bin_tbl = 1;
	}
	linenum = 0;

	cfseek(InfoFile, 0L, SEEK_SET);

	while (cfgets(inputline, LINEBUF_SIZE, InfoFile))
	{
		int l;
		char* temp_ptr;

		linenum++;

		if (have_bin_tbl) // is this a binary tbl file
		{
			for (i = 0; i < strlen(inputline) - 1; i++)
			{
				encode_rotate_left(&(inputline[i]));
				inputline[i] = inputline[i] ^ BITMAP_TBL_XOR;
				encode_rotate_left(&(inputline[i]));
			}
		}
		else
		{
			while (inputline[(l = strlen(inputline)) - 2] == '\\')
			{
				cfgets(inputline + l - 2, LINEBUF_SIZE - (l - 2), InfoFile);
				linenum++;
			}
		}

		REMOVE_EOL(inputline);
		if (strchr(inputline, ';') != NULL) REMOVE_COMMENTS(inputline);

		if (strlen(inputline) == LINEBUF_SIZE - 1)
			Error("Possible line truncation in BITMAPS.TBL on line %d\n", linenum);

		SuperX = -1;

		if ((temp_ptr = strstr(inputline, "superx=")))
		{
			SuperX = atoi(&temp_ptr[7]);
		}

		arg = strtok(inputline, space);
		while (arg != NULL)
		{
			if (arg[0] == '@') //[ISB] validate when we know it isn't null
			{
				arg++;
				Registered_only = 1;
			}
			else
				Registered_only = 0;

			// Check all possible flags and defines.
			if (*arg == '$') bm_flag = BM_NONE; // reset to no flags as default.

			IFTOK("$COCKPIT") 			bm_flag = BM_COCKPIT;
			else IFTOK("$GAUGES") { bm_flag = BM_GAUGES;   clip_count = 0; }
		else IFTOK("$SOUND") 		bm_read_sound();
		else IFTOK("$DOOR_ANIMS")	bm_flag = BM_WALL_ANIMS;
		else IFTOK("$WALL_ANIMS")	bm_flag = BM_WALL_ANIMS;
	else IFTOK("$TEXTURES") 	bm_flag = BM_TEXTURES;
	else IFTOK("$VCLIP") { bm_flag = BM_VCLIP;		vlighting = 0;	clip_count = 0; }
	else IFTOK("$ECLIP") { bm_flag = BM_ECLIP;		vlighting = 0;	clip_count = 0; obj_eclip = 0; dest_bm = NULL; dest_vclip = -1; dest_eclip = -1; dest_size = -1; crit_clip = -1; crit_flag = 0; sound_num = -1; }
else IFTOK("$WCLIP") { bm_flag = BM_WCLIP;		vlighting = 0;	clip_count = 0; wall_explodes = wall_blastable = 0; wall_open_sound = wall_close_sound = -1; tmap1_flag = 0; wall_hidden = 0; }

			else IFTOK("$EFFECTS") { bm_flag = BM_EFFECTS;	clip_num = 0; }

#ifdef EDITOR
			else IFTOK("!METALS_FLAG")		TextureMetals = texture_count;
			else IFTOK("!LIGHTS_FLAG")		TextureLights = texture_count;
			else IFTOK("!EFFECTS_FLAG")	TextureEffects = texture_count;
#endif

			else IFTOK("lighting") 			TmapInfo[texture_count - 1].lighting = fl2f(get_float());
			else IFTOK("damage") 			TmapInfo[texture_count - 1].damage = fl2f(get_float());
			else IFTOK("volatile") 			TmapInfo[texture_count - 1].flags |= TMI_VOLATILE;
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
			else IFTOK("dest_bm")			dest_bm = strtok(NULL, space);
			else IFTOK("dest_vclip")		dest_vclip = get_int();
			else IFTOK("dest_eclip")		dest_eclip = get_int();
			else IFTOK("dest_size")			dest_size = fl2f(get_float());
			else IFTOK("crit_clip")			crit_clip = get_int();
			else IFTOK("crit_flag")			crit_flag = get_int();
			else IFTOK("sound_num") 		sound_num = get_int();
			else IFTOK("frames") 			frames = get_int();
			else IFTOK("time") 				time = get_float();
			else IFTOK("obj_eclip")			obj_eclip = get_int();
			else IFTOK("hit_sound") 		hit_sound = get_int();
			else IFTOK("abm_flag")			abm_flag = get_int();
			else IFTOK("tmap1_flag")		tmap1_flag = get_int();
			else IFTOK("vlighting")			vlighting = get_float();
			else IFTOK("rod_flag")			rod_flag = get_int();
			else IFTOK("superx") 			get_int();
			else IFTOK("open_sound") 		wall_open_sound = get_int();
			else IFTOK("close_sound") 		wall_close_sound = get_int();
			else IFTOK("explodes")	 		wall_explodes = get_int();
			else IFTOK("blastable")	 		wall_blastable = get_int();
			else IFTOK("hidden")	 			wall_hidden = get_int();
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai();

			else IFTOK("$POWERUP") { bm_read_powerup(0);		continue; }
			else IFTOK("$POWERUP_UNUSED") { bm_read_powerup(1);		continue; }
			else IFTOK("$HOSTAGE") { bm_read_hostage();		continue; }
			else IFTOK("$HOSTAGE_FACE") { bm_read_hostage_face(); continue; }
			else IFTOK("$ROBOT") { bm_read_robot();			continue; }
			else IFTOK("$WEAPON") { bm_read_weapon(0);		continue; }
			else IFTOK("$WEAPON_UNUSED") { bm_read_weapon(1);		continue; }
			else IFTOK("$OBJECT") { bm_read_object();		continue; }
			else IFTOK("$PLAYER_SHIP") { bm_read_player_ship();	continue; }

			else {		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg == '\t') || (*arg == ' ')) arg++;
				if (*arg == '\0') { break; }

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
				bm_read_some_file();

			}

			arg = strtok(NULL, equal_space);
			continue;
		}
	}

	NumTextures = texture_count;
	Num_tmaps = tmap_count;

	cfclose(InfoFile);

	atexit(bm_close);

	Assert(N_robot_types == Num_robot_ais);		//should be one ai info per robot

	init_endlevel();		//this is here so endlevel bitmaps go into pig

	verify_textures();

	//check for refereced but unused clip count
	for (i = 0; i < MAX_EFFECTS; i++)
		if ((
			(Effects[i].changing_wall_texture != -1) ||
			(Effects[i].changing_object_texture != -1)
			)
			&& (Effects[i].vc.num_frames == -1))
			Error("EClip %d referenced (by polygon object?), but not defined", i);

#ifndef NDEBUG
	{
		int used;
		for (i = used = 0; i < num_sounds; i++)
			if (Sounds[i] != 255)
				used++;
		mprintf((0, "Sound slots used: %d of %d, highest index %d\n", used, MAX_SOUNDS, num_sounds));
	}
#endif

	piggy_read_sounds();

#ifdef EDITOR
	piggy_dump_all();
#endif

	return 0;
}

void verify_textures()
{
	grs_bitmap* bmp;
	int i, j;
	j = 0;
	for (i = 0; i < Num_tmaps; i++) {
		bmp = &GameBitmaps[Textures[i].index];
		if ((bmp->bm_w != 64) || (bmp->bm_h != 64) || (bmp->bm_rowsize != 64)) {
			mprintf((1, "ERROR: Texture '%s' isn't 64x64 !\n", TmapInfo[i].filename));
			j++;
		}
	}
	if (j) exit(1);
}

//--unused-- void dump_all_transparent_textures()
//--unused-- {
//--unused-- 	FILE * fp;
//--unused-- 	int i,j,k;
//--unused-- 	uint8_t * p;
//--unused-- 	fp = fopen( "XPARENT.LST", "wt" );
//--unused-- 	for (i=0; i<Num_tmaps; i++ )	{
//--unused-- 		k = 0; 
//--unused-- 		p = Textures[i]->bm_data;
//--unused-- 		for (j=0; j<64*64; j++ )
//--unused-- 			if ( (*p++)==255 ) k++;
//--unused-- 		if ( k )	{
//--unused-- 			fprintf( fp, "'%s' has %d transparent pixels\n", TmapInfo[i].filename, k );
//--unused-- 		}				
//--unused-- 	}
//--unused-- 	fclose(fp);	
//--unused-- }


void bm_close()
{
	if (Installed)
	{
		piggy_close();
		Installed = 0;
	}
}

void set_lighting_flag(int8_t* bp)
{
	if (vlighting < 0)
		*bp |= BM_FLAG_NO_LIGHTING;
	else
		*bp &= (0xff ^ BM_FLAG_NO_LIGHTING);
}

void set_texture_name(char* name)
{
	strcpy(TmapInfo[texture_count].filename, name);
	REMOVE_DOTS(TmapInfo[texture_count].filename);
}

void bm_read_eclip()
{
	bitmap_index bitmap;

	Assert(clip_num < MAX_EFFECTS);

	if (clip_num + 1 > Num_effects)
		Num_effects = clip_num + 1;

	Effects[clip_num].flags = 0;

	if (!abm_flag) {
		bitmap = bm_load_sub(arg);

		Effects[clip_num].vc.play_time = fl2f(time);
		Effects[clip_num].vc.num_frames = frames;
		Effects[clip_num].vc.frame_time = fl2f(time) / frames;

		Assert(clip_count < frames);
		Effects[clip_num].vc.frames[clip_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_flags);

		Assert(!obj_eclip);		//obj eclips for non-abm files not supported!
		Assert(crit_flag == 0);

		if (clip_count == 0) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
			TmapList[tmap_count++] = texture_count;
			Textures[texture_count] = bitmap;
			set_texture_name(arg);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			TmapInfo[texture_count].eclip_num = clip_num;
			NumTextures = texture_count;
		}

		clip_count++;

	}
	else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;

		ab_load(arg, bm, &Effects[clip_num].vc.num_frames);

		//printf("EC%d.", clip_num);
		Effects[clip_num].vc.play_time = fl2f(time);
		Effects[clip_num].vc.frame_time = Effects[clip_num].vc.play_time / Effects[clip_num].vc.num_frames;

		clip_count = 0;
		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
		Effects[clip_num].vc.frames[clip_count] = bm[clip_count];

		if (!obj_eclip && !crit_flag) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
			TmapList[tmap_count++] = texture_count;
			Textures[texture_count] = bm[clip_count];
			set_texture_name(arg);
			Assert(texture_count < MAX_TEXTURES);
			TmapInfo[texture_count].eclip_num = clip_num;
			texture_count++;
			NumTextures = texture_count;
		}

		if (obj_eclip) {

			if (Effects[clip_num].changing_object_texture == -1) {		//first time referenced
				Effects[clip_num].changing_object_texture = N_ObjBitmaps;		// XChange ObjectBitmaps
				N_ObjBitmaps++;
			}

			ObjBitmaps[Effects[clip_num].changing_object_texture] = Effects[clip_num].vc.frames[0];
		}

		//if for an object, Effects_bm_ptrs set in object load 

		for (clip_count = 1; clip_count < Effects[clip_num].vc.num_frames; clip_count++) {
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
			Effects[clip_num].vc.frames[clip_count] = bm[clip_count];
		}

	}

	Effects[clip_num].crit_clip = crit_clip;
	Effects[clip_num].sound_num = sound_num;

	if (dest_bm) {			//deal with bitmap for blown up clip
		char short_name[13];
		int i;
		strcpy(short_name, dest_bm);
		REMOVE_DOTS(short_name);
		for (i = 0; i < texture_count; i++)
			if (!_stricmp(TmapInfo[i].filename, short_name))
				break;
		if (i == texture_count) {
			Textures[texture_count] = bm_load_sub(dest_bm);
			strcpy(TmapInfo[texture_count].filename, short_name);
			texture_count++;
			Assert(texture_count < MAX_TEXTURES);
			NumTextures = texture_count;
		}
		Effects[clip_num].dest_bm_num = i;

		if (dest_vclip == -1)
			Error("Desctuction vclip missing on line %d", linenum);
		if (dest_size == -1)
			Error("Desctuction vclip missing on line %d", linenum);

		Effects[clip_num].dest_vclip = dest_vclip;
		Effects[clip_num].dest_size = dest_size;

		Effects[clip_num].dest_eclip = dest_eclip;
	}
	else {
		Effects[clip_num].dest_bm_num = -1;
		Effects[clip_num].dest_eclip = -1;
	}

	if (crit_flag)
		Effects[clip_num].flags |= EF_CRITICAL;
}


void bm_read_gauges()
{
	bitmap_index bitmap;
	int i, num_abm_frames;

	if (!abm_flag) {
		bitmap = bm_load_sub(arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	}
	else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		ab_load(arg, bm, &num_abm_frames);
		for (i = clip_count; i < clip_count + num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i - clip_count];
		}
		clip_count += num_abm_frames;
	}
}

void bm_read_wclip()
{
	bitmap_index bitmap;
	Assert(clip_num < MAX_WALL_ANIMS);

	WallAnims[clip_num].flags = 0;

	if (wall_explodes)	WallAnims[clip_num].flags |= WCF_EXPLODES;
	if (wall_blastable)	WallAnims[clip_num].flags |= WCF_BLASTABLE;
	if (wall_hidden)		WallAnims[clip_num].flags |= WCF_HIDDEN;
	if (tmap1_flag)		WallAnims[clip_num].flags |= WCF_TMAP1;

	if (!abm_flag) {
		bitmap = bm_load_sub(arg);
		if ((WallAnims[clip_num].num_frames > -1) && (clip_count == 0))
			Error("Wall Clip %d is already used!", clip_num);
		WallAnims[clip_num].play_time = fl2f(time);
		WallAnims[clip_num].num_frames = frames;
		//WallAnims[clip_num].frame_time = fl2f(time)/frames;
		Assert(clip_count < frames);
		WallAnims[clip_num].frames[clip_count++] = texture_count;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;
		Textures[texture_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_flags);
		set_texture_name(arg);
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num + 1;
	}
	else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		int nframes;
		if ((WallAnims[clip_num].num_frames > -1))
			Error("AB_Wall clip %d is already used!", clip_num);
		abm_flag = 0;
		ab_load(arg, bm, &nframes);
		WallAnims[clip_num].num_frames = nframes;
		//printf("WC");
		WallAnims[clip_num].play_time = fl2f(time);
		//WallAnims[clip_num].frame_time = fl2f(time)/nframes;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;

		WallAnims[clip_num].close_sound = wall_close_sound;
		strcpy(WallAnims[clip_num].filename, arg);
		REMOVE_DOTS(WallAnims[clip_num].filename);

		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num + 1;

		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);

		for (clip_count = 0; clip_count < WallAnims[clip_num].num_frames; clip_count++) {
			//printf("%d", clip_count);
			Textures[texture_count] = bm[clip_count];
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
			WallAnims[clip_num].frames[clip_count] = texture_count;
			REMOVE_DOTS(arg);
			sprintf(TmapInfo[texture_count].filename, "%s#%d", arg, clip_count);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			NumTextures = texture_count;
		}
	}
}

void bm_read_vclip()
{
	bitmap_index bi;
	Assert(clip_num < VCLIP_MAXNUM);

	if (!abm_flag) {
		if ((Vclip[clip_num].num_frames > -1) && (clip_count == 0))
			Error("Vclip %d is already used!", clip_num);
		bi = bm_load_sub(arg);
		Vclip[clip_num].play_time = fl2f(time);
		Vclip[clip_num].num_frames = frames;
		Vclip[clip_num].frame_time = fl2f(time) / frames;
		Vclip[clip_num].light_value = fl2f(vlighting);
		Vclip[clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bi.index].bm_flags);
		Assert(clip_count < frames);
		Vclip[clip_num].frames[clip_count++] = bi;
		if (rod_flag) {
			rod_flag = 0;
			Vclip[clip_num].flags |= VF_ROD;
		}

	}
	else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		if ((Vclip[clip_num].num_frames > -1))
			Error("AB_Vclip %d is already used!", clip_num);
		ab_load(arg, bm, &Vclip[clip_num].num_frames);

		if (rod_flag) {
			//int i;
			rod_flag = 0;
			Vclip[clip_num].flags |= VF_ROD;
		}
		//printf("VC");
		Vclip[clip_num].play_time = fl2f(time);
		Vclip[clip_num].frame_time = fl2f(time) / Vclip[clip_num].num_frames;
		Vclip[clip_num].light_value = fl2f(vlighting);
		Vclip[clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);

		for (clip_count = 0; clip_count < Vclip[clip_num].num_frames; clip_count++) {
			//printf("%d", clip_count);
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
			Vclip[clip_num].frames[clip_count] = bm[clip_count];
		}
	}
}

// ------------------------------------------------------------------------------
void get4fix(fix* fixp)
{
	char* curtext;
	int	i;

	for (i = 0; i < NDL; i++) {
		curtext = strtok(NULL, space);
		fixp[i] = fl2f(atof(curtext));
	}
}

// ------------------------------------------------------------------------------
void get4byte(int8_t* bytep)
{
	char* curtext;
	int	i;

	for (i = 0; i < NDL; i++) {
		curtext = strtok(NULL, space);
		bytep[i] = atoi(curtext);
	}
}

// ------------------------------------------------------------------------------
//	Convert field of view from an angle in 0..360 to cosine.
void adjust_field_of_view(fix* fovp)
{
	int		i;
	fixang	tt;
	float		ff;
	fix		temp;

	for (i = 0; i < NDL; i++) {
		ff = -f2fl(fovp[i]);
		if (ff > 179) {
			mprintf((1, "Warning: Bogus field of view (%7.3f).  Must be in 0..179.\n", ff));
			ff = 179;
		}
		ff = ff / 360;
		tt = fl2f(ff);
		fix_sincos(tt, &temp, &fovp[i]);
	}
}

void clear_to_end_of_line(void)
{
	arg = strtok(NULL, space);
	while (arg != NULL)
		arg = strtok(NULL, space);
}

void bm_read_sound()
{
	int sound_num;
	int alt_sound_num;

	sound_num = get_int();
	alt_sound_num = get_int();

	if (sound_num >= MAX_SOUNDS)
		Error("Too many sound files.\n");

	if (sound_num >= num_sounds)
		num_sounds = sound_num + 1;

	arg = strtok(NULL, space);

	Sounds[sound_num] = ds_load(arg);

	if (alt_sound_num == 0)
		AltSounds[sound_num] = sound_num;
	else if (alt_sound_num < 0)
		AltSounds[sound_num] = 255;
	else
		AltSounds[sound_num] = alt_sound_num;

	if (Sounds[sound_num] == 255)
		Error("Can't load soundfile <%s>", arg);
}

// ------------------------------------------------------------------------------
void bm_read_robot_ai()
{
	char* robotnum_text;
	int			robotnum;
	robot_info* robptr;

	robotnum_text = strtok(NULL, space);
	robotnum = atoi(robotnum_text);
	Assert(robotnum < MAX_ROBOT_TYPES);
	robptr = &Robot_info[robotnum];

	Assert(robotnum == Num_robot_ais);		//make sure valid number

#ifdef SHAREWARE
	if (Registered_only) {
		Num_robot_ais++;
		clear_to_end_of_line();
		return;
	}
#endif

	Num_robot_ais++;

	get4fix(robptr->field_of_view);
	get4fix(robptr->firing_wait);
	get4byte(robptr->rapidfire_count);
	get4fix(robptr->turn_time);
	get4fix(robptr->fire_power);
	get4fix(robptr->shield);
	get4fix(robptr->max_speed);
	get4fix(robptr->circle_distance);
	get4byte(robptr->evade_speed);

	robptr->always_0xabcd = 0xabcd;

	adjust_field_of_view(robptr->field_of_view);

}

//	----------------------------------------------------------------------------------------------
//this will load a bitmap for a polygon models.  it puts the bitmap into
//the array ObjBitmaps[], and also deals with animating bitmaps
//returns a pointer to the bitmap
grs_bitmap* load_polymodel_bitmap(char* name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

	//	Assert( N_ObjBitmaps == N_ObjBitmapPtrs );

	if (name[0] == '%') //an animating bitmap!
	{
		int eclip_num;

		eclip_num = atoi(name + 1);

		if (Effects[eclip_num].changing_object_texture == -1) //first time referenced
		{
			Effects[eclip_num].changing_object_texture = N_ObjBitmaps;
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
			N_ObjBitmaps++;
		}
		else
		{
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = Effects[eclip_num].changing_object_texture;
		}
		return NULL;
	}
	else
	{
		ObjBitmaps[N_ObjBitmaps] = bm_load_sub(name);
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
		N_ObjBitmaps++;
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps - 1].index];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
void bm_read_robot()
{
	char* model_name[MAX_MODEL_VARIANTS];
	int			n_models, i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char* equal_ptr;
	int 			exp1_vclip_num = -1;
	int			exp1_sound_num = -1;
	int 			exp2_vclip_num = -1;
	int			exp2_sound_num = -1;
	fix			lighting = F1_0 / 2;		// Default
	fix			strength = F1_0 * 10;		// Default strength
	fix			mass = f1_0 * 4;
	fix			drag = f1_0 / 2;
	short 		weapon_type = 0;
	int			g, s;
	char			name[ROBOT_NAME_LENGTH];
	int			contains_count = 0, contains_id = 0, contains_prob = 0, contains_type = 0;
	int			score_value = 1000;
	int			cloak_type = 0;		//	Default = this robot does not cloak
	int			attack_type = 0;		//	Default = this robot attacks by firing (1=lunge)
	int			boss_flag = 0;				//	Default = robot is not a boss.
	int			see_sound = ROBOT_SEE_SOUND_DEFAULT;
	int			attack_sound = ROBOT_ATTACK_SOUND_DEFAULT;
	int			claw_sound = ROBOT_CLAW_SOUND_DEFAULT;

	Assert(N_robot_types < MAX_ROBOT_TYPES);

#ifdef SHAREWARE
	if (Registered_only)
	{
		Robot_info[N_robot_types].model_num = -1;
		N_robot_types++;
		Num_total_object_types++;
		clear_to_end_of_line();
		return;
	}
#endif

	model_name[0] = strtok(NULL, space);
	first_bitmap_num[0] = N_ObjBitmapPtrs;
	n_models = 1;

	// Process bitmaps 
	bm_flag = BM_ROBOT;
	arg = strtok(NULL, space);
	while (arg != NULL)
	{
		equal_ptr = strchr(arg, '=');
		if (equal_ptr)
		{
			*equal_ptr = '\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!_stricmp(arg, "exp1_vclip"))
			{
				exp1_vclip_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "exp2_vclip"))
			{
				exp2_vclip_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "exp1_sound"))
			{
				exp1_sound_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "exp2_sound"))
			{
				exp2_sound_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "lighting")) {
				lighting = fl2f(atof(equal_ptr));
				if ((lighting < 0) || (lighting > F1_0))
				{
					mprintf((1, "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting)));
					Error("In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			}
			else if (!_stricmp(arg, "weapon_type"))
			{
				weapon_type = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "strength"))
			{
				strength = i2f(atoi(equal_ptr));
			}
			else if (!_stricmp(arg, "mass"))
			{
				mass = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "drag"))
			{
				drag = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "contains_id"))
			{
				contains_id = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "contains_type"))
			{
				contains_type = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "contains_count"))
			{
				contains_count = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "contains_prob"))
			{
				contains_prob = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "cloak_type"))
			{
				cloak_type = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "attack_type"))
			{
				attack_type = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "boss"))
			{
				boss_flag = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "score_value"))
			{
				score_value = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "see_sound"))
			{
				see_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "attack_sound"))
			{
				attack_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "claw_sound"))
			{
				claw_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "name"))
			{
				Assert(strlen(equal_ptr) < ROBOT_NAME_LENGTH);	//	Oops, name too long.
				strcpy(name, &equal_ptr[1]);
				name[strlen(name) - 1] = 0;
			}
			else if (!_stricmp(arg, "simple_model"))
			{
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
			}
			else
			{
				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));
			}
		}
		else // Must be a texture specification...
		{
			load_polymodel_bitmap(arg);
		}
		arg = strtok(NULL, space);
	}

	//clear out anim info
	for (g = 0; g < MAX_GUNS + 1; g++)
		for (s = 0; s < N_ANIM_STATES; s++)
			Robot_info[N_robot_types].anim_states[g][s].n_joints = 0;	//inialize to zero

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i = 0; i < n_models; i++)
	{
		int n_textures;
		int model_num, last_model_num = 0;

		n_textures = first_bitmap_num[i + 1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i], n_textures, first_bitmap_num[i], (i == 0) ? &Robot_info[N_robot_types] : NULL);

		if (i == 0)
			Robot_info[N_robot_types].model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num + 1;

		last_model_num = model_num;
	}

	ObjType[Num_total_object_types] = OL_ROBOT;
	ObjId[Num_total_object_types] = N_robot_types;

	Robot_info[N_robot_types].exp1_vclip_num = exp1_vclip_num;
	Robot_info[N_robot_types].exp2_vclip_num = exp2_vclip_num;
	Robot_info[N_robot_types].exp1_sound_num = exp1_sound_num;
	Robot_info[N_robot_types].exp2_sound_num = exp2_sound_num;
	Robot_info[N_robot_types].lighting = lighting;
	Robot_info[N_robot_types].weapon_type = weapon_type;
	Robot_info[N_robot_types].strength = strength;
	Robot_info[N_robot_types].mass = mass;
	Robot_info[N_robot_types].drag = drag;
	Robot_info[N_robot_types].cloak_type = cloak_type;
	Robot_info[N_robot_types].attack_type = attack_type;
	Robot_info[N_robot_types].boss_flag = boss_flag;

	Robot_info[N_robot_types].contains_id = contains_id;
	Robot_info[N_robot_types].contains_count = contains_count;
	Robot_info[N_robot_types].contains_prob = contains_prob;
	Robot_info[N_robot_types].score_value = score_value;
	Robot_info[N_robot_types].see_sound = see_sound;
	Robot_info[N_robot_types].attack_sound = attack_sound;
	Robot_info[N_robot_types].claw_sound = claw_sound;

	if (contains_type)
		Robot_info[N_robot_types].contains_type = OBJ_ROBOT;
	else
		Robot_info[N_robot_types].contains_type = OBJ_POWERUP;

	strcpy(Robot_names[N_robot_types], name);

	N_robot_types++;
	Num_total_object_types++;
}

//read a polygon object of some sort
void bm_read_object()
{
	char* model_name, * model_name_dead = NULL;
	int first_bitmap_num, first_bitmap_num_dead, n_normal_bitmaps;
	char* equal_ptr;
	short model_num;
	short explosion_vclip_num = -1;
	short explosion_sound_num = SOUND_ROBOT_DESTROYED;
	fix	lighting = F1_0 / 2;		// Default
	int type = -1;
	fix strength = 0;

	model_name = strtok(NULL, space);

	// Process bitmaps 
	bm_flag = BM_NONE;
	arg = strtok(NULL, space);
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg != NULL)
	{
		equal_ptr = strchr(arg, '=');

		if (equal_ptr)
		{
			*equal_ptr = '\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!_stricmp(arg, "type"))
			{
				if (!_stricmp(equal_ptr, "controlcen"))
					type = OL_CONTROL_CENTER;
				else if (!_stricmp(equal_ptr, "clutter"))
					type = OL_CLUTTER;
				else if (!_stricmp(equal_ptr, "exit"))
					type = OL_EXIT;
			}
			else if (!_stricmp(arg, "exp_vclip"))
			{
				explosion_vclip_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "dead_pof"))
			{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead = N_ObjBitmapPtrs;
			}
			else if (!_stricmp(arg, "exp_sound"))
			{
				explosion_sound_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "lighting")) {
				lighting = fl2f(atof(equal_ptr));
				if ((lighting < 0) || (lighting > F1_0))
				{
					mprintf((1, "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting)));
					Error("In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			}
			else if (!_stricmp(arg, "strength"))
			{
				strength = fl2f(atof(equal_ptr));
			}
			else
			{
				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));
			}
		}
		else // Must be a texture specification...
		{
			load_polymodel_bitmap(arg);
		}
		arg = strtok(NULL, space);
	}

	if (model_name_dead)
		n_normal_bitmaps = first_bitmap_num_dead - first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs - first_bitmap_num;

	model_num = load_polygon_model(model_name, n_normal_bitmaps, first_bitmap_num, NULL);

	if (type == OL_CONTROL_CENTER)
		N_controlcen_guns = read_model_guns(model_name, controlcen_gun_points, controlcen_gun_dirs, NULL);

	if (model_name_dead)
		Dead_modelnums[model_num] = load_polygon_model(model_name_dead, N_ObjBitmapPtrs - first_bitmap_num_dead, first_bitmap_num_dead, NULL);
	else
		Dead_modelnums[model_num] = -1;

	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n", linenum);

	ObjType[Num_total_object_types] = type;
	ObjId[Num_total_object_types] = model_num;
	ObjStrength[Num_total_object_types] = strength;

	//printf( "Object type %d is a control center\n", Num_total_object_types );
	Num_total_object_types++;

	if (type == OL_EXIT)
	{
		exit_modelnum = model_num;
		destroyed_exit_modelnum = Dead_modelnums[model_num];
	}

}

void bm_read_player_ship()
{
	char* model_name_dying = NULL;
	char* model_name[MAX_MODEL_VARIANTS];
	int	n_models = 0, i;
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	char* equal_ptr;
	robot_info ri;
	int last_multi_bitmap_num = -1;

	// Process bitmaps 
	bm_flag = BM_NONE;

	arg = strtok(NULL, space);

	Player_ship->mass = Player_ship->drag = 0;	//stupid defaults
	Player_ship->expl_vclip_num = -1;

	while (arg != NULL)
	{
		equal_ptr = strchr(arg, '=');

		if (equal_ptr)
		{

			*equal_ptr = '\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!_stricmp(arg, "model"))
			{
				Assert(n_models == 0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models = 1;
			}
			else if (!_stricmp(arg, "simple_model"))
			{
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;

				if (First_multi_bitmap_num != -1 && last_multi_bitmap_num == -1)
					last_multi_bitmap_num = N_ObjBitmapPtrs;
			}
			else if (!_stricmp(arg, "mass"))
				Player_ship->mass = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "drag"))
				Player_ship->drag = fl2f(atof(equal_ptr));
			//			else if (!_stricmp( arg, "low_thrust" ))
			//				Player_ship->low_thrust = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "max_thrust"))
				Player_ship->max_thrust = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "reverse_thrust"))
				Player_ship->reverse_thrust = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "brakes"))
				Player_ship->brakes = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "wiggle"))
				Player_ship->wiggle = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "max_rotthrust"))
				Player_ship->max_rotthrust = fl2f(atof(equal_ptr));
			else if (!_stricmp(arg, "dying_pof"))
				model_name_dying = equal_ptr;
			else if (!_stricmp(arg, "expl_vclip_num"))
				Player_ship->expl_vclip_num = atoi(equal_ptr);
			else
			{
				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));
			}
		}
		else if (!_stricmp(arg, "multi_textures"))
		{

			First_multi_bitmap_num = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...

			load_polymodel_bitmap(arg);

		arg = strtok(NULL, space);
	}

	Assert(model_name != NULL);

	if (First_multi_bitmap_num != -1 && last_multi_bitmap_num == -1)
		last_multi_bitmap_num = N_ObjBitmapPtrs;

	if (First_multi_bitmap_num == -1)
		first_bitmap_num[n_models] = N_ObjBitmapPtrs;

#ifndef NETWORK //ISB hack
#define MAX_NUM_NET_PLAYERS 8
#endif
	Assert(last_multi_bitmap_num - First_multi_bitmap_num == (MAX_NUM_NET_PLAYERS - 1) * 2);

	for (i = 0; i < n_models; i++)
	{
		int n_textures;
		int model_num, last_model_num = 0;

		n_textures = first_bitmap_num[i + 1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i], n_textures, first_bitmap_num[i], (i == 0) ? &ri : NULL);

		if (i == 0)
			Player_ship->model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num + 1;

		last_model_num = model_num;
	}

	if (model_name_dying)
	{
		Assert(n_models);
		Dying_modelnums[Player_ship->model_num] = load_polygon_model(model_name_dying, first_bitmap_num[1] - first_bitmap_num[0], first_bitmap_num[0], NULL);
	}

	Assert(ri.n_guns == N_PLAYER_GUNS);

	//calc player gun positions

	{
		polymodel* pm;
		robot_info* r;
		vms_vector pnt;
		int mn;				//submodel number
		int gun_num;

		r = &ri;
		pm = &Polygon_models[Player_ship->model_num];

		for (gun_num = 0; gun_num < r->n_guns; gun_num++)
		{

			pnt = r->gun_points[gun_num];
			mn = r->gun_submodels[gun_num];

			//instance up the tree for this gun
			while (mn != 0)
			{
				vm_vec_add2(&pnt, &pm->submodel_offsets[mn]);
				mn = pm->submodel_parents[mn];
			}

			Player_ship->gun_points[gun_num] = pnt;
		}
	}
}

void bm_read_some_file()
{
	switch (bm_flag)
	{
	case BM_COCKPIT:
	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(arg);
		Assert(Num_cockpits < N_COCKPIT_BITMAPS);
		cockpit_bitmap[Num_cockpits++] = bitmap;

		//bm_flag = BM_NONE;
	}
	break;
	case BM_GAUGES:
		bm_read_gauges();
		break;
	case BM_WEAPON:
		bm_read_weapon(0);
		break;
	case BM_VCLIP:
		bm_read_vclip();
		break;
	case BM_ECLIP:
		bm_read_eclip();
		break;
	case BM_TEXTURES:
	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(arg);
		Assert(tmap_count < MAX_TEXTURES);
		TmapList[tmap_count++] = texture_count;
		Textures[texture_count] = bitmap;
		set_texture_name(arg);
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
	}
	break;
	case BM_WCLIP:
		bm_read_wclip();
		break;
	default:
		break;
	}
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
void bm_read_weapon(int unused_flag)
{
	int	i, n;
	int	n_models = 0;
	char* equal_ptr;
	char* pof_file_inner = NULL;
	char* model_name[MAX_MODEL_VARIANTS];
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	int	lighted;					//flag for whether is a texture is lighted

	Assert(N_weapon_types < MAX_WEAPON_TYPES);

	n = N_weapon_types;
	N_weapon_types++;

	if (unused_flag)
	{
		clear_to_end_of_line();
		return;
	}

#ifdef SHAREWARE
	if (Registered_only)
	{
		clear_to_end_of_line();
		return;
	}
#endif

	// Initialize weapon array
	Weapon_info[n].render_type = WEAPON_RENDER_NONE;		// 0=laser, 1=blob, 2=object
	Weapon_info[n].bitmap.index = 0;
	Weapon_info[n].model_num = -1;
	Weapon_info[n].model_num_inner = -1;
	Weapon_info[n].blob_size = 0x1000;									// size of blob
	Weapon_info[n].flash_vclip = -1;
	Weapon_info[n].flash_sound = SOUND_LASER_FIRED;
	Weapon_info[n].flash_size = 0;
	Weapon_info[n].robot_hit_vclip = -1;
	Weapon_info[n].robot_hit_sound = -1;
	Weapon_info[n].wall_hit_vclip = -1;
	Weapon_info[n].wall_hit_sound = -1;
	Weapon_info[n].impact_size = 0;
	for (i = 0; i < NDL; i++) {
		Weapon_info[n].strength[i] = F1_0;
		Weapon_info[n].speed[i] = F1_0 * 10;
	}
	Weapon_info[n].mass = F1_0;
	Weapon_info[n].thrust = 0;
	Weapon_info[n].drag = 0;
	Weapon_info[n].persistent = 0;

	Weapon_info[n].energy_usage = 0;					//	How much fuel is consumed to fire this weapon.
	Weapon_info[n].ammo_usage = 0;					//	How many units of ammunition it uses.
	Weapon_info[n].fire_wait = F1_0 / 4;				//	Time until this weapon can be fired again.
	Weapon_info[n].fire_count = 1;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	Weapon_info[n].damage_radius = 0;				//	Radius of damage for missiles, not lasers.  Does damage to objects within this radius of hit point.
//--01/19/95, mk--	Weapon_info[n].damage_force = 0;					//	Force (movement) due to explosion
	Weapon_info[n].destroyable = 1;					//	Weapons default to destroyable
	Weapon_info[n].matter = 0;							//	Weapons default to not being constructed of matter (they are energy!)
	Weapon_info[n].bounce = 0;							//	Weapons default to not bouncing off walls

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0 * 10;

	Weapon_info[n].picture.index = 0;
	Weapon_info[n].homing_flag = 0;

	// Process arguments
	arg = strtok(NULL, space);

	lighted = 1;			//assume first texture is lighted

	while (arg != NULL)
	{
		equal_ptr = strchr(arg, '=');
		if (equal_ptr)
		{
			*equal_ptr = '\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!_stricmp(arg, "laser_bmp"))
			{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_LASER;

			}
			else if (!_stricmp(arg, "blob_bmp"))
			{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_BLOB;

			}
			else if (!_stricmp(arg, "weapon_vclip"))
			{
				// Set vclip to play for this weapon.
				Weapon_info[n].bitmap.index = 0;
				Weapon_info[n].render_type = WEAPON_RENDER_VCLIP;
				Weapon_info[n].weapon_vclip = atoi(equal_ptr);

			}
			else if (!_stricmp(arg, "none_bmp"))
			{
				Weapon_info[n].bitmap = bm_load_sub(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_NONE;

			}
			else if (!_stricmp(arg, "weapon_pof"))
			{
				// Load pof file
				Assert(n_models == 0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models = 1;
			}
			else if (!_stricmp(arg, "simple_model"))
			{
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
			}
			else if (!_stricmp(arg, "weapon_pof_inner"))
			{
				// Load pof file
				pof_file_inner = equal_ptr;
			}
			else if (!_stricmp(arg, "strength"))
			{
				for (i = 0; i < NDL - 1; i++)
				{
					Weapon_info[n].strength[i] = i2f(atoi(equal_ptr));
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].strength[i] = i2f(atoi(equal_ptr));
			}
			else if (!_stricmp(arg, "mass"))
			{
				Weapon_info[n].mass = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "drag"))
			{
				Weapon_info[n].drag = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "thrust"))
			{
				Weapon_info[n].thrust = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "matter"))
			{
				Weapon_info[n].matter = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "bounce"))
			{
				Weapon_info[n].bounce = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "speed"))
			{
				for (i = 0; i < NDL - 1; i++)
				{
					Weapon_info[n].speed[i] = i2f(atoi(equal_ptr));
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].speed[i] = i2f(atoi(equal_ptr));
			}
			else if (!_stricmp(arg, "flash_vclip"))
			{
				Weapon_info[n].flash_vclip = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "flash_sound"))
			{
				Weapon_info[n].flash_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "flash_size"))
			{
				Weapon_info[n].flash_size = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "blob_size"))
			{
				Weapon_info[n].blob_size = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "robot_hit_vclip"))
			{
				Weapon_info[n].robot_hit_vclip = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "robot_hit_sound"))
			{
				Weapon_info[n].robot_hit_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "wall_hit_vclip"))
			{
				Weapon_info[n].wall_hit_vclip = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "wall_hit_sound"))
			{
				Weapon_info[n].wall_hit_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "impact_size"))
			{
				Weapon_info[n].impact_size = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "lighted"))
			{
				lighted = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "lw_ratio"))
			{
				Weapon_info[n].po_len_to_width_ratio = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "lightcast"))
			{
				Weapon_info[n].light = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "persistent"))
			{
				Weapon_info[n].persistent = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "energy_usage"))
			{
				Weapon_info[n].energy_usage = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "ammo_usage"))
			{
				Weapon_info[n].ammo_usage = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "fire_wait"))
			{
				Weapon_info[n].fire_wait = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "fire_count"))
			{
				Weapon_info[n].fire_count = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "damage_radius"))
			{
				Weapon_info[n].damage_radius = fl2f(atof(equal_ptr));
				//--01/19/95, mk--			} else if (!_stricmp(arg, "damage_force" )) {
				//--01/19/95, mk--				Weapon_info[n].damage_force = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "lifetime")) {
				Weapon_info[n].lifetime = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "destroyable"))
			{
				Weapon_info[n].destroyable = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "picture"))
			{
				Weapon_info[n].picture = bm_load_sub(equal_ptr);
			}
			else if (!_stricmp(arg, "homing"))
			{
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			}
			else
			{
				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));
			}
		}
		else // Must be a texture specification...
		{
			grs_bitmap* bm;

			bm = load_polymodel_bitmap(arg);
			if (bm) //[ISB] load_polymodel_bitmap returns NULL on a EClip.
			{
				if (!lighted)
					bm->bm_flags |= BM_FLAG_NO_LIGHTING;
			}

			lighted = 1;			//default for next bitmap is lighted
		}
		arg = strtok(NULL, space);
	}

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i = 0; i < n_models; i++)
	{
		int n_textures;
		int model_num, last_model_num = 0;

		n_textures = first_bitmap_num[i + 1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i], n_textures, first_bitmap_num[i], NULL);

		if (i == 0)
		{
			Weapon_info[n].render_type = WEAPON_RENDER_POLYMODEL;
			Weapon_info[n].model_num = model_num;
		}
		else
			Polygon_models[last_model_num].simpler_model = model_num + 1;

		last_model_num = model_num;
	}

	if (pof_file_inner)
	{
		Assert(n_models);
		Weapon_info[n].model_num_inner = load_polygon_model(pof_file_inner, first_bitmap_num[1] - first_bitmap_num[0], first_bitmap_num[0], NULL);
	}

	if ((Weapon_info[n].ammo_usage == 0) && (Weapon_info[n].energy_usage == 0))
		mprintf((1, "Warning: Weapon %i has ammo and energy usage of 0.\n", n));

	// -- render type of none is now legal --	Assert( Weapon_info[n].render_type != WEAPON_RENDER_NONE );
}





// ------------------------------------------------------------------------------
#define DEFAULT_POWERUP_SIZE i2f(3)

void bm_read_powerup(int unused_flag)
{
	int n;
	char* equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag)
	{
		clear_to_end_of_line();
		return;
	}

	// Initialize powerup array
	Powerup_info[n].light = F1_0 / 3;		//	Default lighting value.
	Powerup_info[n].vclip_num = -1;
	Powerup_info[n].hit_sound = -1;
	Powerup_info[n].size = DEFAULT_POWERUP_SIZE;
	Powerup_names[n][0] = 0;

	// Process arguments
	arg = strtok(NULL, space);

	while (arg != NULL) {
		equal_ptr = strchr(arg, '=');
		if (equal_ptr) {
			*equal_ptr = '\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!_stricmp(arg, "vclip_num")) {
				Powerup_info[n].vclip_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "light")) {
				Powerup_info[n].light = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "hit_sound")) {
				Powerup_info[n].hit_sound = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "name")) {
				Assert(strlen(equal_ptr) < POWERUP_NAME_LENGTH);	//	Oops, name too long.
				strcpy(Powerup_names[n], &equal_ptr[1]);
				Powerup_names[n][strlen(Powerup_names[n]) - 1] = 0;
			}
			else if (!_stricmp(arg, "size")) {
				Powerup_info[n].size = fl2f(atof(equal_ptr));
			}
			else {
				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));
			}
		}
		else {			// Must be a texture specification...
			mprintf((1, "Invalid argument, %s in bitmaps.tbl\n", arg));
		}
		arg = strtok(NULL, space);
	}

	ObjType[Num_total_object_types] = OL_POWERUP;
	ObjId[Num_total_object_types] = n;
	//printf( "Object type %d is a powerup\n", Num_total_object_types );
	Num_total_object_types++;

}

void bm_read_hostage()
{
	int n;
	char* equal_ptr;

	Assert(N_hostage_types < MAX_HOSTAGE_TYPES);

	n = N_hostage_types;
	N_hostage_types++;

	// Process arguments
	arg = strtok(NULL, space);

	while (arg != NULL)
	{
		equal_ptr = strchr(arg, '=');
		if (equal_ptr)
		{
			*equal_ptr = '\0';
			equal_ptr++;

			if (!_stricmp(arg, "vclip_num"))

				Hostage_vclip_num[n] = atoi(equal_ptr);

			else

				mprintf((1, "Invalid parameter, %s=%s in bitmaps.tbl\n", arg, equal_ptr));

		}
		else
			mprintf((1, "Invalid argument, %s in bitmaps.tbl at line %d\n", arg, linenum));

		arg = strtok(NULL, space);
	}

	ObjType[Num_total_object_types] = OL_HOSTAGE;
	ObjId[Num_total_object_types] = n;
	//printf( "Object type %d is a hostage\n", Num_total_object_types );
	Num_total_object_types++;

}


void bm_read_hostage_face()
{
	char* abm_name, * equal_ptr;
	int clip_num = -1, sound_num = -1;
	fix time;

	abm_name = strtok(NULL, space);

	arg = strtok(NULL, space);
	while (arg != NULL) {
		equal_ptr = strchr(arg, '=');
		if (equal_ptr) {
			*equal_ptr = '\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!_stricmp(arg, "clip_num")) {
				clip_num = atoi(equal_ptr);
			}
			else if (!_stricmp(arg, "time")) {
				time = fl2f(atof(equal_ptr));
			}
			else if (!_stricmp(arg, "sound_num")) {
				sound_num = atoi(equal_ptr);
			}
		}

		arg = strtok(NULL, space);
	}

#ifndef SHAREWARE

	Assert(clip_num >= 0 && clip_num < MAX_HOSTAGES);

	ab_load(abm_name, Hostage_face_clip[clip_num].frames, &Hostage_face_clip[clip_num].num_frames);

	Assert(Hostage_face_clip[clip_num].num_frames < MAX_BITMAPS_PER_BRUSH);

	Hostage_face_clip[clip_num].play_time = time;
	Hostage_face_clip[clip_num].sound_num = sound_num;
	Hostage_face_clip[clip_num].frame_time = time / Hostage_face_clip[clip_num].num_frames;

#endif
}

void bm_write_all(FILE* fp)
{
	int i;

	fwrite(&NumTextures, sizeof(int), 1, fp);
	for (i = 0; i < MAX_TEXTURES; i++)
		file_write_short(fp, Textures[i].index);
	write_tmap_info(fp);

	fwrite(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	fwrite(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	file_write_int(fp, Num_vclips);
	write_vclip_info(fp);

	file_write_int(fp, Num_effects);
	write_effect_info(fp);

	file_write_int(fp, Num_wall_anims);
	write_wallanim_info(fp);

	file_write_int(fp, N_robot_types);
	write_robot_info(fp);

	file_write_int(fp, N_robot_joints);
	write_robot_joints_info(fp);

	file_write_int(fp, N_weapon_types);
	write_weapon_info(fp);

	file_write_int(fp, N_powerup_types);
	write_powerup_info(fp);

	file_write_int(fp, N_polygon_models);
	write_polygon_models(fp);

	for (i = 0; i < N_polygon_models; i++)
	{
		fwrite(Polygon_models[i].model_data, sizeof(uint8_t), Polygon_models[i].model_data_size, fp);
	}

	for (i = 0; i < MAX_GAUGE_BMS; i++)
		file_write_short(fp, Gauges[i].index);

	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		file_write_int(fp, Dying_modelnums[i]);
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		file_write_int(fp, Dead_modelnums[i]);

	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		file_write_short(fp, ObjBitmaps[i].index);
	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		file_write_short(fp, ObjBitmapPtrs[i]);

	write_player_ship(fp);

	file_write_int(fp, Num_cockpits);
	for (i = 0; i < N_COCKPIT_BITMAPS; i++)
		file_write_short(fp, cockpit_bitmap[i].index);

	fwrite(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	fwrite(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	file_write_int(fp, Num_total_object_types);
	for (i = 0; i < MAX_OBJTYPE; i++)
		file_write_byte(fp, ObjType[i]);
	for (i = 0; i < MAX_OBJTYPE; i++)
		file_write_byte(fp, ObjId[i]);
	for (i = 0; i < MAX_OBJTYPE; i++)
		file_write_int(fp, ObjStrength[i]);

	file_write_int(fp, First_multi_bitmap_num);

	file_write_int(fp, N_controlcen_guns);
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
	{
		file_write_int(fp, controlcen_gun_points[i].x);
		file_write_int(fp, controlcen_gun_points[i].y);
		file_write_int(fp, controlcen_gun_points[i].z);
	}
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
	{
		file_write_int(fp, controlcen_gun_dirs[i].x);
		file_write_int(fp, controlcen_gun_dirs[i].y);
		file_write_int(fp, controlcen_gun_dirs[i].z);
	}
	file_write_int(fp, exit_modelnum);
	file_write_int(fp, destroyed_exit_modelnum);
}

#endif
