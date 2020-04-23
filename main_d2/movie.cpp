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

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "platform/posixstub.h"

//#include "pa_enabl.h"                   //$$POLY_ACC
#include "2d/i_gr.h"
#include "inferno.h"
#include "text.h"
#include "misc/args.h"
#include "mem/mem.h"
#include "misc/byteswap.h"
#include "cfile/cfile.h"
#include "gamefont.h"
#include "2d/gr.h"
#include "2d/palette.h"
#include "config.h"
#include "mve/libmve.h"
//#include "mvegfx.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "digi.h"
#include "songs.h"
#include "platform/timer.h"
#include "platform/joy.h"
#include "platform/key.h"
#include "movie.h"
#include "screens.h"
#include "newmenu.h"
//#include "vga.h"
#include "menu.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

extern char* Args[100];                                 // Arguments from args.c for
char* FirstVid, * SecondVid;
char* RoboBuffer[50];
char RobBufCount = 0, PlayingBuf = 0, RobBufLimit = 0;

unsigned RobSX = 75, RobSY = 50, RobDX = 100, RobDY = 100;
int RoboFile = 0, RoboFilePos = 0, MVEPaletteCalls = 0;

//      Function Prototypes
int RunMovie(char* filename, int highres_flag, int allow_abort, int dx, int dy);
extern void do_briefing_screens(char*, int);

// Subtitle data
typedef struct {
	short first_frame, last_frame;
	char* msg;
} subtitle;


// #define BUFFER_MOVIE 

#define MAX_SUBTITLES 500
subtitle Subtitles[MAX_SUBTITLES];
int Num_subtitles;

// ----------------------------------------------------------------------
void* MPlayAlloc(unsigned size)
{
	return malloc(size);
}

void MPlayFree(void* p)
{
	free(p);
}

extern uint16_t hSOSDigiDriver;

int HiResRoboMovie = 0;

//-----------------------------------------------------------------------

unsigned FileRead(int handle, void* buf, unsigned count)
{
	unsigned numread;
	numread = _read(handle, buf, count);
	return (numread == count);
}

#define VID_PLAY 0
#define VID_PAUSE 1

int Vid_State;

extern int Digi_initialized;
extern int digi_timer_rate;

//int MovieHires = 0;		//default for now is lores
int MovieHires = 1; //[ISB] hack, with the lack of a setup program this won't be set to hires. 

#define MOVIE_VOLUME_SCALE  (32767)		//32767 is MAX

//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
int PlayMovie(const char* filename, int must_have)
{
	char name[FILENAME_LEN], * p;
	int c, ret;
	int save_sample_rate;

#if defined(POLY_ACC)
	PA_DFX(pa_set_write_mode(1));
	PA_DFX(pa_set_frontbuffer_current());
#endif

	if (!Digi_initialized)
		return MOVIE_NOT_PLAYED;

#ifndef RELEASE
	if (FindArg("-nomovies"))
		return MOVIE_NOT_PLAYED;
#endif

	strcpy(name, filename);

	if ((p = strchr(name, '.')) == NULL)		//add extension, if missing
		strcat(name, ".mve");

	//check for escape already pressed & abort if so
	while ((c = key_inkey()) != 0)
		if (c == KEY_ESC)
			return MOVIE_ABORTED;

	// Stop all digital sounds currently playing.
	digi_stop_all();

	// Stop all songs
	songs_stop_all();

	save_sample_rate = digi_sample_rate;
	digi_sample_rate = SAMPLE_RATE_22K;		//always 22K for movies
	digi_reset(); digi_reset();

	// Start sound //[ISB] TODO
	/*
	if (hSOSDigiDriver < 0xffff) 
	{
		MVE_SOS_sndInit(hSOSDigiDriver);
		MVE_sndVolume((Config_digi_volume * MOVIE_VOLUME_SCALE) / 8);
	}
	else
		MVE_SOS_sndInit(-1);*/ 

	//MVE_rmFastMode(MVE_RM_NORMAL);

	ret = RunMovie(name, MovieHires, must_have, -1, -1);

	//@@	if (ret == MOVIE_NOT_PLAYED) {		//couldn't find movie. try other version
	//@@		name[strlen(name)-5] = MovieHires?'l':'h';				//change name
	//@@		ret = RunMovie(name,!MovieHires,allow_abort,-1,-1);	//try again
	//@@	}

	gr_palette_clear();		//clear out palette in case movie aborted

	digi_sample_rate = save_sample_rate;		//restore rate for game
	digi_reset(); digi_reset();

	Screen_mode = -1;		//force screen reset

#if defined(POLY_ACC)
	PA_DFX(pa_set_write_mode(0));
#endif

	return ret;
}

//uint8_t localPal[768];

void MovieShowFrame(uint8_t* buf, uint32_t bufw, uint32_t bufh, uint32_t sx, uint32_t sy, uint32_t w, uint32_t h, uint32_t dstx, uint32_t dsty)
{
	grs_bitmap source_bm;

	//mprintf((0,"MovieShowFrame %d,%d  %d,%d  %d,%d  %d,%d\n",bufw,bufh,sx,sy,w,h,dstx,dsty));

	Assert(bufw == w && bufh == h);

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = bufw;
	source_bm.bm_h = bufh;
	source_bm.bm_type = BM_LINEAR;
	source_bm.bm_flags = 0;
	source_bm.bm_data = buf;

	gr_palette_load(gr_palette);
	gr_bm_ubitblt(bufw, bufh, dstx, dsty, sx, sy, &source_bm, &grd_curcanv->cv_bitmap);
}

//our routine to set the pallete, called from the movie code
void MovieSetPalette(unsigned char* p, unsigned start, unsigned count)
{
	//memcpy(localPal, gr_palette, 768); //[ISB] test hack
	if (count == 0)
		return;

	//mprintf((0,"SetPalette p=%x, start=%d, count=%d\n",p,start,count));

	//Color 0 should be black, and we get color 255
	Assert(start >= 1 && start + count - 1 <= 254);

	//Set color 0 to be black
	gr_palette[0] = gr_palette[1] = gr_palette[2] = 0;

	//Set color 255 to be our subtitle color
	gr_palette[765] = gr_palette[766] = gr_palette[767] = 50;

	//movie libs palette into our array  
	memcpy(gr_palette + start * 3, p + start * 3, count * 3);

	//finally set the palette in the hardware
	gr_palette_load(gr_palette);

	//MVE_SetPalette(p, start, count);
}

typedef struct bkg
{
	short x, y, w, h;			// The location of the menu.
	grs_bitmap* bmp;			// The background under the menu.
} bkg;

bkg movie_bg = { 0,0,0,0,NULL };

#define BOX_BORDER (MenuHires?40:20)

void show_pause_message(char* msg)
{
	int w, h, aw;
	int x, y;

	gr_set_current_canvas(NULL);
	gr_set_curfont(SMALL_FONT);

	gr_get_string_size(msg, &w, &h, &aw);

	x = (grd_curscreen->sc_w - w) / 2;
	y = (grd_curscreen->sc_h - h) / 2;

	if (movie_bg.bmp)
	{
		gr_free_bitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}

	// Save the background of the display
	movie_bg.x = x; movie_bg.y = y; movie_bg.w = w; movie_bg.h = h;

	movie_bg.bmp = gr_create_bitmap(w + BOX_BORDER, h + BOX_BORDER);

	gr_bm_ubitblt(w + BOX_BORDER, h + BOX_BORDER, 0, 0, x - BOX_BORDER / 2, y - BOX_BORDER / 2, &(grd_curcanv->cv_bitmap), movie_bg.bmp);

	gr_setcolor(0);
	gr_rect(x - BOX_BORDER / 2, y - BOX_BORDER / 2, x + w + BOX_BORDER / 2 - 1, y + h + BOX_BORDER / 2 - 1);

	gr_set_fontcolor(255, -1);

	gr_ustring(0x8000, y, msg);
}

void clear_pause_message()
{
	if (movie_bg.bmp)
	{
		gr_bitmap(movie_bg.x - BOX_BORDER / 2, movie_bg.y - BOX_BORDER / 2, movie_bg.bmp);

		gr_free_bitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}
}

int open_movie_file(const char* filename, int must_have);
void draw_subtitles(int frame_num);

//returns status.  see movie.h
int RunMovie(char* filename, int hires_flag, int must_have, int dx, int dy)
{
	int filehndl;
	int result, aborted = 0;
	int track = 0;
	int frame_num;

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = open_movie_file(filename, must_have);

	if (filehndl == -1) {
#ifndef EDITOR
		if (must_have)
		{
			_strupr(filename);
			Error("Cannot open movie file <%s>", filename);
		}
		else
			return MOVIE_NOT_PLAYED;
#else
		return MOVIE_NOT_PLAYED;
#endif
	}

	//MVE_memCallbacks(MPlayAlloc, MPlayFree);
	//MVE_ioCallbacks(FileRead);

#if defined(POLY_ACC)
	Assert(hires_flag);

	pa_flush();
#ifdef PA_3DFX_VOODOO
	pa_begin_lfb();
	MVE_sfSVGA(640, 480, 2048, 0, pa_get_buffer_address(0), 0, 0, NULL, 1);
	pa_end_lfb();
#else
	MVE_sfSVGA(640, 480, 1280, 0, pa_get_buffer_address(0), 0, 0, NULL, 1);
#endif

	pa_clear_buffer(0, 0);
#else
	if (hires_flag)
	{
		gr_set_mode(SM_640x480V);
		/*if (!MVE_gfxMode(MVE_GFX_VESA_CURRENT)) 
		{
			Int3();
			close(filehndl);                           // Close Movie File
			return MOVIE_NOT_PLAYED;
		}*/
	}
	else
	{
		gr_set_mode(SM_320x200C);
		/*if (!MVE_gfxMode(MVE_GFX_VGA_CURRENT)) //[ISB] todo
		{
			Int3();
			close(filehndl);                           // Close Movie File
			return MOVIE_NOT_PLAYED;
		}*/
	}
#endif

	Vid_State = VID_PLAY;                           // Set to PLAY

	if (MVE_rmPrepMovie(filehndl, dx, dy, track)) 
	{
		Int3();
		return MOVIE_NOT_PLAYED;
	}

	//[ISB] I think these are handled by mvelib. I hope. 
	//[ISB] why do i make fucking dumb assumptions all the time. Argh.
#if !defined(POLY_ACC)
	MVE_sfCallbacks((mve_cb_ShowFrame*)MovieShowFrame);
	MVE_palCallbacks((mve_cb_SetPalette*)MovieSetPalette);
#endif

	frame_num = 0;

	FontHires = hires_flag;

	while ((result = MVE_rmStepMovie()) == 0) 
	{
		int key;
		I_DoEvents(); //Timed from rmStepMovie

		draw_subtitles(frame_num);

		key = key_inkey();

		// If ESCAPE pressed, then quit movie.
		if (key == KEY_ESC) 
		{
			result = aborted = 1;
			break;
		}

		// If PAUSE pressed, then pause movie
		if (key == KEY_PAUSE) 
		{
			MVE_rmHoldMovie();
			show_pause_message(TXT_PAUSE);
			while (!key_inkey())
			{
				I_MarkStart();
				I_DoEvents();
				I_DrawCurrentCanvas(0);
				I_MarkEnd(MovieHires ? US_60FPS : US_70FPS);
			}
			clear_pause_message();
		}
		I_DrawCurrentCanvas(0);
		frame_num++;
	}

	Assert(aborted || result == MVE_ERR_EOF);		///movie should be over

	MVE_rmEndMovie();
	//MVE_ReleaseMem();

	_close(filehndl);                           // Close Movie File

	//MVE_gfxReset();

	// Restore old graphic state

	Screen_mode = -1;		//force reset of screen mode

//@@   if (MenuHires) 
//@@		vga_set_mode(SM_640x480V);
//@@   else	
//@@		vga_set_mode(SM_320x200C);

	return (aborted ? MOVIE_ABORTED : MOVIE_PLAYED_FULL);
}


int InitMovieBriefing()
{
#if defined(POLY_ACC)
	Assert(MenuHires);
	pa_flush();

#ifdef PA_3DFX_VOODOO
	pa_begin_lfb();
	MVE_sfSVGA(640, 480, 2048, 0, pa_get_buffer_address(0), 0, 0, NULL, 1);
	pa_end_lfb();
#else
	MVE_sfSVGA(640, 480, 1280, 0, pa_get_buffer_address(0), 0, 0, NULL, 1);
#endif

	pa_clear_buffer(0, 0);
	return 1;
#endif

	if (MenuHires)
	{
		gr_set_mode(SM_640x480V);
		/*if (!MVE_gfxMode(MVE_GFX_VESA_CURRENT))
		{
			Int3();
			return MOVIE_NOT_PLAYED;
		}*/
	}
	else 
	{
		gr_set_mode(SM_320x200C);
		/*if (!MVE_gfxMode(MVE_GFX_VGA_CURRENT)) 
		{
			Int3();
			return MOVIE_NOT_PLAYED;
		}*/
	}
	gr_clear_canvas(0); //[ISB] hack
	return (1);
}

int FlipFlop = 0;

int MyShowFrame(void)
{
	grs_bitmap source_bm;

	int rw, rh, rdx, rdy;

	if (MenuHires)
	{
		rw = 320; rh = 200; rdx = 280; rdy = 200;
	}
	else
	{
		rw = 160; rh = 100; rdx = 140; rdy = 80;
	}

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = rw;
	source_bm.bm_h = rh;
	source_bm.bm_type = BM_LINEAR;
	source_bm.bm_flags = 0;
	if (FlipFlop)
	{
#ifdef BUFFER_MOVIE
		memcpy(RoboBuffer[RobBufCount++], SecondVid, rw * rh);
#endif
		source_bm.bm_data = (unsigned char*)SecondVid;
	}
	else
	{
		source_bm.bm_data = (unsigned char*)FirstVid;
#ifdef BUFFER_MOVIE
		memcpy(RoboBuffer[RobBufCount++], FirstVid, rw * rh);
#endif
	}

	gr_bm_ubitblt(rw, rh, rdx, rdy, 0, 0, &source_bm, &grd_curcanv->cv_bitmap);

	FlipFlop = 1 - FlipFlop;

	return 0;
}

#ifdef BUFFER_MOVIE
static fix RobBufTime = 0;
#endif

void ShowRobotBuffer()
{
	// shows a frame from the robot buffer

#ifndef BUFFER_MOVIE
	Int3(); // Get Jason...how'd we get here?
	return;
#else
	grs_bitmap source_bm;
	int rw, rh, rdx, rdy;

	if (timer_get_approx_seconds() < (RobBufTime + fixdiv(F1_0, i2f(15))))
		return;

	RobBufTime = timer_get_approx_seconds();

	if (MenuHires)
	{
		rw = 320; rh = 200; rdx = 280; rdy = 200;
	}
	else
	{
		rw = 160; rh = 100; rdx = 140; rdy = 80;
	}

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = rw;
	source_bm.bm_h = rh;
	source_bm.bm_type = BM_LINEAR;
	source_bm.bm_flags = 0;

	source_bm.bm_data = RoboBuffer[RobBufCount];

	gr_bm_ubitblt(rw, rh, rdx, rdy, 0, 0, &source_bm, &grd_curcanv->cv_bitmap);

	RobBufCount++;
	RobBufCount %= RobBufLimit;
#endif
}

int reset_movie_file(int handle); //[ISB] PROTOTYPES PLEASE

//returns 1 if frame updated ok
int RotateRobot()
{
	int err;

	if (!Digi_initialized) 			//we should fix this for full version
		return 0;
#ifdef BUFFER_MOVIE
	if (PlayingBuf)
	{
		ShowRobotBuffer();
		return (1);
	}
#endif

#if defined(POLY_ACC)
	PA_DFX(pa_set_write_mode(1));
#endif

	err = MVE_rmStepMovie();

#if defined(POLY_ACC)
	PA_DFX(pa_set_write_mode(0));
#endif


	if (err == MVE_ERR_EOF)		//end of movie, so reset
	{
#ifdef BUFFER_MOVIE
		PlayingBuf = 1;
		RobBufLimit = RobBufCount;
		RobBufCount = 0;
		//RobBufTime=timer_get_approx_seconds();
		return 1;
#else
		reset_movie_file(RoboFile);
#if defined(POLY_ACC)
		if (MVE_rmPrepMovie(RoboFile, 280, 200, 0)) 
		{
#else
		if (MVE_rmPrepMovie(RoboFile, -1, -1, 0)) 
		{
#endif
			Int3();
			return 0;
		}
#endif
		}
	else if (err) 
	{
		Int3();
		return 0;
	}
	return 0;
}

void FreeRoboBuffer(int n)
{
	// frees the 64k frame buffers, starting with n and then working down

#ifndef BUFFER_MOVIE
	n++;	//kill warning
	return;
#else
	int i;

	for (i = n; i >= 0; i--)
		free(RoboBuffer[i]);

#endif 
}


void DeInitRobotMovie()
{
	RobBufCount = 0; PlayingBuf = 0;

#if !defined(POLY_ACC)
	memset(FirstVid, 0, 64000);
	memset(SecondVid, 0, 64000);
	MyShowFrame();
#endif

	MVE_rmEndMovie();
	MVE_ReleaseMem();
	free(FirstVid);
	free(SecondVid);

	FreeRoboBuffer(49);

	//MVE_palCallbacks(MVE_SetPalette);
	MVE_palCallbacks(NULL);
	_close(RoboFile);                           // Close Movie File
}

void PaletteChecker(unsigned char* p, unsigned start, unsigned count)
{
	int i;

	for (i = 0; i < 256; i++)
		if (p[i] != 0)
			break;

	if (i >= 255 && (MVEPaletteCalls++) > 0)
		return;

	//MVE_SetPalette(p, start, count); //[ISB] I need to figure out if I should do something here
	//[ISB] for whatever reason, the implication of the above line is that the palette is set without actually informing the game's palette code
	//Replicate this by using a raw palette push call. 
	I_WritePalette(start, start + count - 1, p + (start*3));
}


int InitRobotMovie(const char* filename)
{
#ifdef BUFFER_MOVIE
	int i;
#endif
	FlipFlop = 0;

	RobBufCount = 0; PlayingBuf = 0; RobBufLimit = 0;

	if (FindArg("-nomovies"))
		return (0);

	mprintf((0, "RoboFile=%s\n", filename));

#ifdef BUFFER_MOVIE

	for (i = 0; i < 50; i++)
	{
		if (MenuHires)
			RoboBuffer[i] = malloc(65000L);
		else
			RoboBuffer[i] = malloc(17000L);

		if (RoboBuffer[i] == NULL)
		{
			mprintf((0, "ROBOERROR: Could't allocate frame %d!\n", i));
			if (i)
				FreeRoboBuffer(i - 1);
			return (NULL);
		}
	}
#endif

	if ((FirstVid = (char*)calloc(65000L, 1)) == NULL)
	{
		FreeRoboBuffer(49);
		return (0);
	}
	if ((SecondVid = (char*)calloc(65000L, 1)) == NULL)
	{
		free(FirstVid);
		FreeRoboBuffer(49);
		return (0);
	}

	//MVE_SOS_sndInit(-1);		//tell movies to play no sound for robots

	//MVE_memCallbacks(MPlayAlloc, MPlayFree);
	//MVE_ioCallbacks(FileRead);
	MVE_memVID(FirstVid, SecondVid, 65000);

	RoboFile = open_movie_file(filename, 1);

	if (RoboFile == -1) 
	{
		free(FirstVid);
		free(SecondVid);
		FreeRoboBuffer(49);
#ifdef RELEASE
		Error("Cannot open movie file <%s>", filename);
#else
		return MOVIE_NOT_PLAYED;
#endif
	}

	Vid_State = VID_PLAY;

#if !defined(POLY_ACC)
	MVE_sfCallbacks((mve_cb_ShowFrame*)MyShowFrame);
#endif

#if defined(POLY_ACC)
	if (MVE_rmPrepMovie(RoboFile, 280, 200, 0)) 
	{
#else
	if (MVE_rmPrepMovie(RoboFile, -1, -1, 0)) 
	{
#endif
		Int3();
		free(FirstVid);
		free(SecondVid);
		FreeRoboBuffer(49);
		return 0;
	}

#if !defined(POLY_ACC)
	MVE_palCallbacks(PaletteChecker);
#endif

	RoboFilePos = _lseek(RoboFile, 0L, SEEK_CUR);

	mprintf((0, "RoboFilePos=%d!\n", RoboFilePos));
	return (1);
}

/*
 *		Subtitle system code
 */

uint8_t* subtitle_raw_data;

//search for next field following whitespace 
uint8_t* next_field(uint8_t * p)
{
	while (*p && !isspace(*p))
		p++;

	if (!*p)
		return NULL;

	while (*p && isspace(*p))
		p++;

	if (!*p)
		return NULL;

	return p;
}

void change_filename_ext(char* dest, const char* src, const char* ext);
void decode_text_line(char* p);

int init_subtitles(const char* filename)
{
	CFILE* ifile;
	int size, read_count;
	uint8_t* p;
	int have_binary = 0;

	Num_subtitles = 0;

	if (!FindArg("-subtitles"))
		return 0;

	ifile = cfopen(filename, "rb");		//try text version

	if (!ifile) {								//no text version, try binary version
		char filename2[FILENAME_LEN];
		change_filename_ext(filename2, filename, ".TXB");
		ifile = cfopen(filename2, "rb");
		if (!ifile)
			return 0;
		have_binary = 1;
	}

	size = cfilelength(ifile);

	MALLOC(subtitle_raw_data, uint8_t, size + 1);

	read_count = cfread(subtitle_raw_data, 1, size, ifile);

	cfclose(ifile);

	subtitle_raw_data[size] = 0;

	if (read_count != size) 
	{
		free(subtitle_raw_data);
		return 0;
	}

	p = subtitle_raw_data;

	while (p && p < subtitle_raw_data + size) 
	{
		char* endp;

		endp = strchr((char*)p, '\n');
		if (endp) 
		{
			if (endp[-1] == '\r')
				endp[-1] = 0;		//handle 0d0a pair
			*endp = 0;			//string termintor
		}

		if (have_binary)
			decode_text_line((char*)p);

		if (*p != ';') 
		{
			Subtitles[Num_subtitles].first_frame = atoi((char*)p);
			p = next_field(p); if (!p) continue;
			Subtitles[Num_subtitles].last_frame = atoi((char*)p);
			p = next_field(p); if (!p) continue;
			Subtitles[Num_subtitles].msg = (char*)p;

			Assert(Num_subtitles == 0 || Subtitles[Num_subtitles].first_frame >= Subtitles[Num_subtitles - 1].first_frame);
			Assert(Subtitles[Num_subtitles].last_frame >= Subtitles[Num_subtitles].first_frame);

			Num_subtitles++;
		}

		p = (uint8_t*)(endp + 1);

	}

	return 1;

}

void close_subtitles()
{
	if (subtitle_raw_data)
		free(subtitle_raw_data);
	subtitle_raw_data = NULL;
	Num_subtitles = 0;
}

#define MAX_ACTIVE_SUBTITLES 3

//draw the subtitles for this frame
void draw_subtitles(int frame_num)
{
	static int active_subtitles[MAX_ACTIVE_SUBTITLES];
	static int num_active_subtitles, next_subtitle, line_spacing;
	int t, y;
	int must_erase = 0;

	if (frame_num == 0) 
	{
		num_active_subtitles = 0;
		next_subtitle = 0;
		gr_set_curfont(GAME_FONT);
		line_spacing = grd_curcanv->cv_font->ft_h + (grd_curcanv->cv_font->ft_h >> 2);
		gr_set_fontcolor(255, -1);
	}

	//get rid of any subtitles that have expired
	for (t = 0; t < num_active_subtitles;)
		if (frame_num > Subtitles[active_subtitles[t]].last_frame) 
		{
			int t2;
			for (t2 = t; t2 < num_active_subtitles - 1; t2++)
				active_subtitles[t2] = active_subtitles[t2 + 1];
			num_active_subtitles--;
			must_erase = 1;
		}
		else
			t++;

	//get any subtitles new for this frame 
	while (next_subtitle < Num_subtitles && frame_num >= Subtitles[next_subtitle].first_frame) 
	{
		if (num_active_subtitles >= MAX_ACTIVE_SUBTITLES)
			Error("Too many active subtitles!");
		active_subtitles[num_active_subtitles++] = next_subtitle;
		next_subtitle++;
	}

	//find y coordinate for first line of subtitles
	y = grd_curcanv->cv_bitmap.bm_h - ((line_spacing + 1) * MAX_ACTIVE_SUBTITLES + 2);

	//erase old subtitles if necessary
	if (must_erase)
	{
		gr_setcolor(0);
		gr_rect(0, y, grd_curcanv->cv_bitmap.bm_w - 1, grd_curcanv->cv_bitmap.bm_h - 1);
	}

	//now draw the current subtitles
	for (t = 0; t < num_active_subtitles; t++)
		if (active_subtitles[t] != -1) 
		{
			gr_string(0x8000, y, Subtitles[active_subtitles[t]].msg);
			y += line_spacing + 1;
		}
}

typedef struct 
{
	char name[FILENAME_LEN];
	int offset, len;
} ml_entry;

#define MLF_ON_CD		1

typedef struct 
{
	char		name[100];	//[FILENAME_LEN];
	int		n_movies;
	uint8_t		flags, pad[3];
	ml_entry	movies[];
} movielib;

#define MAX_MOVIES_PER_LIB		50		//determines size of malloc

movielib * init_new_movie_lib(const char* filename, FILE * fp)
{
	int nfiles, offset;
	int i, n;
	movielib* table;

	//read movie file header

	fread(&nfiles, 4, 1, fp);		//get number of files

	table = (movielib*)malloc(sizeof(*table) + sizeof(ml_entry) * nfiles);

	strcpy(table->name, filename);
	table->n_movies = nfiles;

	offset = 4 + 4 + nfiles * (13 + 4);	//id + nfiles + nfiles * (filename + size)

	for (i = 0; i < nfiles; i++) 
	{
		int len;

		n = fread(table->movies[i].name, 13, 1, fp);
		if (n != 1)
			break;		//end of file (probably)

		//n = fread(&len, 4, 1, fp);
		len = F_ReadInt(fp);
		//if (n != 1)
		//	Error("error reading movie library <%s>", filename);

		table->movies[i].len = len;
		table->movies[i].offset = offset;

		offset += table->movies[i].len;
	}
	fclose(fp);
	table->flags = 0;
	return table;
}

movielib* init_old_movie_lib(const char* filename, FILE * fp)
{
	int nfiles, size;
	int i;
	movielib* table, * table2;

	nfiles = 0;

	//allocate big table
	table = (movielib*)malloc(sizeof(*table) + sizeof(ml_entry) * MAX_MOVIES_PER_LIB);

	while (1) 
	{
		int len;

		i = fread(table->movies[nfiles].name, 13, 1, fp);
		if (i != 1)
			break;		//end of file (probably)

		i = fread(&len, 4, 1, fp);
		len = F_ReadInt(fp);
		//if (i != 1)
		//	Error("error reading movie library <%s>", filename);

		table->movies[nfiles].len = len;
		table->movies[nfiles].offset = ftell(fp);

		fseek(fp, len, SEEK_CUR);		//skip data

		nfiles++;
	}

	//allocate correct-sized table
	size = sizeof(*table) + sizeof(ml_entry) * nfiles;
	table2 = (movielib*)malloc(size);
	memcpy(table2, table, size);
	free(table);
	table = table2;

	strcpy(table->name, filename);

	table->n_movies = nfiles;

	fclose(fp);

	table->flags = 0;

	return table;
}

//find the specified movie library, and read in list of movies in it   
movielib* init_movie_lib(const char* filename)
{
	//note: this based on cfile_init_hogfile()

	char id[4];
	FILE* fp;

	fp = fopen(filename, "rb");
#ifndef _WINDOWS
	if (!fp)
	{
		char* filename2 = (char*)malloc(sizeof(char) * (strlen(filename) + 1));
		strcpy(filename2, filename);
		_strupr(filename2);
		fp = fopen(filename2, "rb");
		if (!fp)
		{
			strcpy(filename2, filename);
			_strupr(filename2);
			fp = fopen(filename2, "rb");
		}
		free(filename2);
	}
#endif
	if (fp == NULL)
		return NULL;

	fread(id, 4, 1, fp);
	if (!strncmp(id, "DMVL", 4))
		return init_new_movie_lib(filename, fp);
	else if (!strncmp(id, "DHF", 3)) 
	{
		fseek(fp, -1, SEEK_CUR);		//old file had 3 char id
		return init_old_movie_lib(filename, fp);
	}
	else 
	{
		fclose(fp);
		return NULL;
	}
}

#ifdef D2_OEM
const char* movielib_files[] = { "intro-l.mvl","other-l.mvl","robots-l.mvl","oem-l.mvl" };
#else
const char* movielib_files[] = { "intro-l.mvl","other-l.mvl","robots-l.mvl" };
#endif

#define N_BUILTIN_MOVIE_LIBS (sizeof(movielib_files)/sizeof(*movielib_files))
#define N_MOVIE_LIBS (N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB N_BUILTIN_MOVIE_LIBS
movielib* movie_libs[N_MOVIE_LIBS];

void close_movie(int i)
{
	if (movie_libs[i])
		free(movie_libs[i]);
}

void close_movies()
{
	int i;

	for (i = 0; i < N_MOVIE_LIBS; i++)
		close_movie(i);
}

#include "gamepal.h"

extern char CDROM_dir[];
extern int MenuHiresAvailable;

extern uint8_t last_palette_for_color_fonts[];

extern int force_rb_register;

//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int request_cd()
{
	return 0;
}

//do we have the robot movies available
int robot_movies = 0;	//0 means none, 1 means lowres, 2 means hires

void init_movie(const char* filename, int libnum, int is_robots, int required)
{
	int high_res;
	char tempBuffer[FILENAME_LEN]; //[ISB] I dunno

	strncpy(&tempBuffer[0], filename, FILENAME_LEN);

#ifndef RELEASE
	if (FindArg("-nomovies")) 
	{
		movie_libs[libnum] = NULL;
		return;
	}
#endif

	//for robots, load highres versions if highres menus set
	if (is_robots)
		high_res = MenuHiresAvailable;
	else
		high_res = MovieHires;

	if (high_res)
	{
		char* heh = strchr(tempBuffer, '.');//[-1] = 'h';
		heh--; *heh = 'h';
	}

try_again:;

	if ((movie_libs[libnum] = init_movie_lib(tempBuffer)) == NULL)
	{
		char name2[100];

		strcpy(name2, CDROM_dir);
		strcat(name2, tempBuffer);
		movie_libs[libnum] = init_movie_lib(name2);

		if (movie_libs[libnum] != NULL)
			movie_libs[libnum]->flags |= MLF_ON_CD;
		else {
			if (required) {
#if defined(RELEASE) && !defined(D2_OEM)		//allow no movies if not release
				_strupr(tempBuffer);
				Error("Cannot open movie file <%s>", tempBuffer);
#endif
			}
#if defined(D2_OEM)		//if couldn't get higres, try low
			if (is_robots == 1) {	//first try, try again with lowres
				strchr(tempBuffer, '.')[-1] = 'l';
				high_res = 0;
				is_robots++;
				goto try_again;
			}
			else if (is_robots == 2) {		//failed twice. bail with error
				_strupr(tempBuffer);
				Error("Cannot open movie file <%s>", filename);
			}
#endif
		}
	}

	if (is_robots && movie_libs[libnum] != NULL)
		robot_movies = high_res ? 2 : 1;
}

//find and initialize the movie libraries
void init_movies()
{
	int i;
	int is_robots;

	for (i = 0; i < N_BUILTIN_MOVIE_LIBS; i++) 
	{

		if (!_strnicmp(movielib_files[i], "robot", 5))
			is_robots = 1;
		else
			is_robots = 0;

		init_movie(movielib_files[i], i, is_robots, 1);
	}

	movie_libs[EXTRA_ROBOT_LIB] = NULL;

	atexit(close_movies);
}

void init_extra_robot_movie(char* filename)
{
	close_movie(EXTRA_ROBOT_LIB);
	init_movie(filename, EXTRA_ROBOT_LIB, 1, 0);
}


int movie_handle, movie_start;

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
int search_movie_lib(movielib * lib, const char* filename, int must_have)
{
	int i;
	int filehandle;

	if (lib == NULL)
		return -1;

	for (i = 0; i < lib->n_movies; i++)
		if (!_stricmp(filename, lib->movies[i].name)) {	//found the movie in a library 
			int from_cd;

			from_cd = (lib->flags & MLF_ON_CD);

			if (from_cd)
				songs_stop_redbook();		//ready to read from CD

			do {		//keep trying until we get the file handle
				movie_handle = filehandle = _open(lib->name, O_RDONLY + O_BINARY);

#ifndef _WINDOWS
				if (filehandle == -1)
				{
					//horrible hack for opening multiple filename cases.
					_strupr(lib->name);
					return search_movie_lib(lib, filename, must_have);
				}
#endif

				if (must_have && from_cd && filehandle == -1) {		//didn't get file!

					if (request_cd() == -1)		//ESC from requester
						break;						//bail from here. will get error later
				}

			} while (must_have && from_cd && filehandle == -1);

			if (filehandle != -1)
				_lseek(filehandle, (movie_start = lib->movies[i].offset), SEEK_SET);

			return filehandle;
		}

	return -1;
}

//returns file handle
int open_movie_file(const char* filename, int must_have)
{
	int filehandle, i;

	for (i = 0; i < N_MOVIE_LIBS; i++) {

		if ((filehandle = search_movie_lib(movie_libs[i], filename, must_have)) != -1)
			return filehandle;
	}

	return -1;		//couldn't find it
}

//sets the file position to the start of this already-open file
int reset_movie_file(int handle)
{
	//[ISB] TODO: I hate making hacks like this. This suggests there's an odd problem in mvelib that doesn't conform to the library's original functionality. I'll work it out later maybe...
	FlipFlop = 1 - FlipFlop;
	Assert(handle == movie_handle);

	_lseek(handle, movie_start, SEEK_SET);

	return 0;		//everything is cool
}


