/* $Id: mveplay.c,v 1.11.2.1 2003/06/06 22:12:55 btb Exp $ */
#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifndef __MSDOS__
//[ISB] kill audio for now, it's a problem
//#define AUDIO
#endif
//#define DEBUG

#include <string.h>
#include <errno.h>
#include <time.h>
//#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>

#include "mvelib.h"
#include "mve_audio.h"

#include "decoders.h"

#include "libmve.h"
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/palette.h"
#include "platform/timer.h"
#include "misc/types.h"
#include "platform/i_sound.h"
#include "platform/mono.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define MVE_OPCODE_ENDOFSTREAM          0x00
#define MVE_OPCODE_ENDOFCHUNK           0x01
#define MVE_OPCODE_CREATETIMER          0x02
#define MVE_OPCODE_INITAUDIOBUFFERS     0x03
#define MVE_OPCODE_STARTSTOPAUDIO       0x04
#define MVE_OPCODE_INITVIDEOBUFFERS     0x05

#define MVE_OPCODE_DISPLAYVIDEO         0x07
#define MVE_OPCODE_AUDIOFRAMEDATA       0x08
#define MVE_OPCODE_AUDIOFRAMESILENCE    0x09
#define MVE_OPCODE_INITVIDEOMODE        0x0A

#define MVE_OPCODE_SETPALETTE           0x0C
#define MVE_OPCODE_SETPALETTECOMPRESSED 0x0D

#define MVE_OPCODE_SETDECODINGMAP       0x0F

#define MVE_OPCODE_VIDEODATA            0x11

#define MVE_AUDIO_FLAGS_STEREO     1
#define MVE_AUDIO_FLAGS_16BIT      2
#define MVE_AUDIO_FLAGS_COMPRESSED 4

int g_spdFactorNum=0;
static int g_spdFactorDenom=10;
static int g_frameUpdated = 0;

#ifdef STANDALONE
static int playing = 1;
int g_sdlVidFlags = SDL_ANYFORMAT | SDL_DOUBLEBUF;
int g_loop = 0;

void initializeMovie(MVESTREAM *mve);
void playMovie(MVESTREAM *mve);
void shutdownMovie(MVESTREAM *mve);
#endif

static short get_short(unsigned char *data)
{
	short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static unsigned short get_ushort(unsigned char *data)
{
	unsigned short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static int get_int(unsigned char *data)
{
	int value;
	value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	return value;
}

static unsigned int unhandled_chunks[32*256];

static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	unhandled_chunks[major<<8|minor]++;
	//fprintf(stderr, "unknown chunk type %02x/%02x\n", major, minor);
	return 1;
}


/*************************
 * general handlers
 *************************/
static int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	return 0;
}

/*************************
 * timer handlers
 *************************/
//[ISB] I should fix the definition but that was annoying
struct timeval_s {
	long    tv_sec;
	long    tv_usec;
};

/*
 * timer variables
 */
static int timer_created = 0;
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval_s timer_expire = {0, 0};

static uint64_t nextTimerTick;

 //[ISB] todo: just gotta gut all this timer stuff and replace it with the I_ platform stuff
#if 0
#if !HAVE_STRUCT_TIMESPEC
struct timespec
{
	long int tv_sec;            /* Seconds.  */
	long int tv_nsec;           /* Nanoseconds.  */
};
#endif 
#endif

#if defined(HAVE_DECL_NANOSLEEP) && !HAVE_DECL_NANOSLEEP
int nanosleep(struct timespec *ts, void *rem);
#endif

#ifdef __WIN32
#include <sys/timeb.h>

int gettimeofday(struct timeval_s *tv, void *tz)
{
	static int counter = 0;
	struct timeb tm;

	counter++; /* to avoid collisions */
	ftime(&tm);
	tv->tv_sec  = tm.time;
	tv->tv_usec = (tm.millitm * 1000) + counter;

	return 0;
}

int nanosleep(struct timespec *ts, void *rem)
{
	_sleep(ts->tv_sec * 1000 + ts->tv_nsec / 1000000);

	return 0;
}
#endif

static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	long long temp;

	if (timer_created)
		return 1;
	else
		timer_created = 1;

	micro_frame_delay = get_int(data) * (int)get_short(data+4);
	//printf("micro_frame_delay: %d\n", micro_frame_delay);
	if (g_spdFactorNum != 0)
	{
		temp = micro_frame_delay;
		temp *= g_spdFactorNum;
		temp /= g_spdFactorDenom;
		micro_frame_delay = (int)temp;
	}

	return 1;
}

//[ISB] TODO: move to timer.cpp perhaps
#include <chrono>
static uint64_t GetClockTimeUS()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

static void timer_stop(void)
{
	timer_expire.tv_sec = 0;
	timer_expire.tv_usec = 0;
	timer_started = 0;
}

static void timer_start(void)
{
	//[ISB] TODO
	/*int nsec=0;
	gettimeofday(&timer_expire, NULL);
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}*/
	nextTimerTick = GetClockTimeUS() + micro_frame_delay;
	timer_started=1;
}

static void do_timer_wait(void)
{
	/*
	int nsec=0;
	struct timespec ts;
	struct timeval_s tv;
	if (! timer_started)
		return;

	gettimeofday(&tv, NULL);
	if (tv.tv_sec > timer_expire.tv_sec)
		goto end;
	else if (tv.tv_sec == timer_expire.tv_sec  &&  tv.tv_usec >= timer_expire.tv_usec)
		goto end;

	ts.tv_sec = timer_expire.tv_sec - tv.tv_sec;
	ts.tv_nsec = 1000 * (timer_expire.tv_usec - tv.tv_usec);
	if (ts.tv_nsec < 0)
	{
		ts.tv_nsec += 1000000000UL;
		--ts.tv_sec;
	}
#ifdef __CYGWIN__
	usleep(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
	if (nanosleep(&ts, NULL) == -1  &&  errno == EINTR)
		exit(1);
#endif

 end:
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}*/
	//[ISB] godawful hack
	//I_Delay(micro_frame_delay / 1000); //[ISB] should do polling like the above code, perhaps (actually it isn't polling what am I saying...)
	//[ISB] okay this burns the CPU more but might give more precise results? We'll see...
	uint64_t startTick = GetClockTimeUS();
	uint64_t numTicks = nextTimerTick - startTick;
	if (numTicks > 2000) //[ISB] again inspired by dpJudas, with 2000 US number from GZDoom
		I_DelayUS(numTicks - 2000);
	while (GetClockTimeUS() < nextTimerTick);
	nextTimerTick += micro_frame_delay;
}

/*************************
 * audio handlers
 *************************/

#define TOTAL_AUDIO_BUFFERS 64

static int audiobuf_created = 0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
static int mve_audio_compressed=0;
static int mve_audio_enabled = 1;
static int mve_audio_paused = 0;

static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	int flags;
	int sample_rate;
	int desired_buffer;

	int stereo;
	int bitsize;
	int compressed;

	int format;

	if (!mve_audio_enabled)
		return 1;

	flags = get_ushort(data + 2);
	sample_rate = get_ushort(data + 4);
	desired_buffer = get_int(data + 6);

	//mprintf((0, "flags: %d, sample rate: %d, desired_buffer: %d\n", flags, sample_rate, desired_buffer));

	stereo = (flags & MVE_AUDIO_FLAGS_STEREO) ? 1 : 0;
	bitsize = (flags & MVE_AUDIO_FLAGS_16BIT) ? 1 : 0;

	if (minor > 0) 
	{
		compressed = flags & MVE_AUDIO_FLAGS_COMPRESSED ? 1 : 0;
	} else 
	{
		compressed = 0;
	}

	mve_audio_compressed = compressed;

	if (bitsize == 1)
	{
		format = MVESND_S16LSB;
	} 
	else 
	{
		format = MVESND_U8;
	}

	I_InitMovieAudio(format, sample_rate, stereo);
	mve_audio_canplay = 1;

	//fprintf(stderr, "creating audio buffers:\n");
	//fprintf(stderr, "sample rate = %d, stereo = %d, bitsize = %d, compressed = %d\n",
	//		sample_rate, stereo, bitsize ? 16 : 8, compressed);

	//mve_audio_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
	//mve_audio_spec->freq = sample_rate;
	//mve_audio_spec->format = format;
	//mve_audio_spec->channels = (stereo) ? 2 : 1;
	//mve_audio_spec->samples = 4096;
	//mve_audio_spec->callback = mve_audio_callback;
	//mve_audio_spec->userdata = NULL;
	/*if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0)
	{
		fprintf(stderr, "   success\n");
		mve_audio_canplay = 1;
	}
	else
	{
		fprintf(stderr, "   failure : %s\n", SDL_GetError());
		mve_audio_canplay = 0;
	}*/

	//memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
	//memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));

	return 1;
}

static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#ifdef AUDIO
	if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
	{
		SDL_PauseAudio(0);
		mve_audio_playing = 1;
	}
#endif
	return 1;
}

//[ISB] this handler is gutting the present buffeirng code and instead making it the property of the sound library. 
static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	static const int selected_chan=1;
	int chan;
	int nsamp;
	short* buf = NULL;
	if (mve_audio_canplay)
	{
		//if (mve_audio_playing)
		//	SDL_LockAudio();

		chan = get_ushort(data + 2);
		nsamp = get_ushort(data + 4);
		if (chan & selected_chan)
		{
			/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
			if (major == MVE_OPCODE_AUDIOFRAMEDATA) 
			{
				if (mve_audio_compressed)
				{
					nsamp += 4;

					//mve_audio_buflens[mve_audio_buftail] = nsamp;
					//mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
					//mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
					buf = (short*)malloc(nsamp+4);
					mveaudio_uncompress(buf, data, -1);
				} 
				else 
				{
					nsamp -= 8;
					data += 8;

					//mve_audio_buflens[mve_audio_buftail] = nsamp;
					//mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);
					//memcpy(mve_audio_buffers[mve_audio_buftail], data, nsamp);
					buf = (short*)malloc(nsamp+8);
					mveaudio_uncompress(buf, data, -1);
				}
			} 
			else 
			{
				//mve_audio_buflens[mve_audio_buftail] = nsamp;
				//mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp);

				buf = (short*)malloc(nsamp+4);
				mveaudio_uncompress(buf, data, -1);

				memset(buf, 0, nsamp); /* XXX */
			}

			I_QueueMovieAudioBuffer(nsamp, buf);

			//if (++mve_audio_buftail == TOTAL_AUDIO_BUFFERS)
			//	mve_audio_buftail = 0;

			//if (mve_audio_buftail == mve_audio_bufhead)
			//	fprintf(stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
		}

		//if (mve_audio_playing)
		//	SDL_UnlockAudio();
	}

	if (buf) free(buf);

	return 1;
}

/*************************
 * video handlers
 *************************/
static int videobuf_created = 0;
static int video_initialized = 0;
int g_width, g_height;
void *g_vBuffers = NULL, *g_vBackBuf1, *g_vBackBuf2;
void* hackBuf1 = NULL, * hackBuf2 = NULL;

#ifdef STANDALONE
static SDL_Surface *g_screen;
#else
static int g_destX, g_destY;
#endif
static int g_screenWidth, g_screenHeight;
static unsigned char g_palette[768];
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;
static int g_truecolor;

static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short w, h;
	short count, truecolor;

	if (videobuf_created)
		return 1;
	else
		videobuf_created = 1;

	w = get_short(data);
	h = get_short(data+2);

	if (minor > 0) 
	{
		count = get_short(data+4);
	} 
	else 
	{
		count = 1;
	}

	if (minor > 1) 
	{
		truecolor = get_short(data+6);
	}
	else
	{
		truecolor = 0;
	}

	g_width = w << 3;
	g_height = h << 3;

	if (hackBuf1 == NULL && hackBuf2 == NULL)
	{
		/* TODO: * 4 causes crashes on some files */
		g_vBackBuf1 = g_vBuffers = (uint8_t*)malloc(g_width * g_height * 8);
		if (truecolor)
		{
			g_vBackBuf2 = (unsigned short*)g_vBackBuf1 + (g_width * g_height);
		}
		else
		{
			g_vBackBuf2 = (unsigned char*)g_vBackBuf1 + (g_width * g_height);
		}

		memset(g_vBackBuf1, 0, g_width * g_height * 4);
	}

#ifdef DEBUG
	fprintf(stderr, "DEBUG: w,h=%d,%d count=%d, tc=%d\n", w, h, count, truecolor);
#endif

	g_truecolor = truecolor;

	return 1;
}

//[ISB] godawful hack
static mve_cb_ShowFrame* ShowFrameCallback = NULL;
static mve_cb_SetPalette* SetPaletteCallback = NULL;

static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#ifdef STANDALONE
	ConvertAndDraw();

	SDL_Flip(g_screen);

	do_sdl_events();
#else
	if (ShowFrameCallback == NULL)
	{
		grs_bitmap* bitmap;

		bitmap = gr_create_bitmap_raw(g_width, g_height, (uint8_t*)g_vBackBuf1);

		if (g_destX == -1) // center it
			g_destX = (g_screenWidth - g_width) >> 1;
		if (g_destY == -1) // center it
			g_destY = (g_screenHeight - g_height) >> 1;

		gr_palette_load(g_palette);

		gr_bitmap(g_destX, g_destY, bitmap);

		gr_free_sub_bitmap(bitmap);
	}
	else
	{
		if (g_destX == -1) // center it
			g_destX = (g_screenWidth - g_width) >> 1;
		if (g_destY == -1) // center it
			g_destY = (g_screenHeight - g_height) >> 1;
		(*ShowFrameCallback)((uint8_t*)g_vBackBuf1, g_width, g_height, 0, 0, g_width, g_height, g_destX, g_destY);
	}
#endif
	g_frameUpdated = 1;

	return 1;
}

static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short width, height;

	if (video_initialized)
		return 1;
	else
		video_initialized = 1;

	width = get_short(data);
	height = get_short(data+2);
#ifdef STANDALONE
	g_screen = SDL_SetVideoMode(width, height, 16, g_sdlVidFlags);
#endif
	g_screenWidth = width;
	g_screenHeight = height;
	memset(g_palette, 0, 765);
	// 255 needs to default to white, for subtitles, etc
	memset(g_palette + 765, 63, 3);

	return 1;
}

static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short start, count;
	start = get_short(data);
	count = get_short(data+2);
	if (SetPaletteCallback == NULL)
		memcpy(g_palette + 3 * start, data + 4, 3 * count);
	else
	{
		//[ISB] offset is a dumb hack to ensure the palette callback works without modification
		(*SetPaletteCallback)(data + 4 - (start * 3), start, count);
	}

	return 1;
}

static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap = data;
	g_nMapLength = len;
	return 1;
}

static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short nFrameHot, nFrameCold;
	short nXoffset, nYoffset;
	short nXsize, nYsize;
	unsigned short nFlags;
	unsigned char *temp;

	nFrameHot  = get_short(data);
	nFrameCold = get_short(data+2);
	nXoffset   = get_short(data+4);
	nYoffset   = get_short(data+6);
	nXsize     = get_short(data+8);
	nYsize     = get_short(data+10);
	nFlags     = get_ushort(data+12);

	if (nFlags & 1)
	{
		temp = (unsigned char *)g_vBackBuf1;
		g_vBackBuf1 = g_vBackBuf2;
		g_vBackBuf2 = temp;
	}

	/* convert the frame */
	if (g_truecolor) {
		decodeFrame16((unsigned char *)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	} else {
		decodeFrame8((uint8_t*)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	}

	return 1;
}

static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap=NULL;
	return 1;
}

static MVESTREAM *mve = NULL;

int MVE_rmPrepMovie(int filehandle, int x, int y, int track)
{
	int i;

	if (mve) 
	{
		mve_reset(mve);
		return 0;
	}
	//[ISB] make sure it doesn't try to play sound if the previous movie had sound but this one doesn't
	mve_audio_canplay = 0;

	mve = mve_open_filehandle(filehandle);

	if (!mve)
		return 1;

	g_destX = x;
	g_destY = y;

	for (i = 0; i < 32; i++)
		mve_set_handler(mve, i, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_ENDOFSTREAM,          end_movie_handler);
	mve_set_handler(mve, MVE_OPCODE_ENDOFCHUNK,           end_chunk_handler);
	mve_set_handler(mve, MVE_OPCODE_CREATETIMER,          create_timer_handler);
	mve_set_handler(mve, MVE_OPCODE_INITAUDIOBUFFERS,     create_audiobuf_handler);
	mve_set_handler(mve, MVE_OPCODE_STARTSTOPAUDIO,       play_audio_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOBUFFERS,     create_videobuf_handler);

	mve_set_handler(mve, MVE_OPCODE_DISPLAYVIDEO,         display_video_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMEDATA,       audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMESILENCE,    audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOMODE,        init_video_handler);

	mve_set_handler(mve, MVE_OPCODE_SETPALETTE,           video_palette_handler);
	mve_set_handler(mve, MVE_OPCODE_SETPALETTECOMPRESSED, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_SETDECODINGMAP,       video_codemap_handler);

	mve_set_handler(mve, MVE_OPCODE_VIDEODATA,            video_data_handler);

	return 0;
}

int MVE_rmStepMovie()
{
	static int init_timer=0;
	int cont=1;

	if (!timer_started)
		timer_start();

	while (cont && !g_frameUpdated) // make a "step" be a frame, not a chunk...
		cont = mve_play_next_chunk(mve);

	if (micro_frame_delay  && !init_timer)
	{
		timer_start();
		init_timer = 1;
	}

	if (mve_audio_paused)
	{
		mve_audio_paused = 0;
		I_UnPauseMovieAudio();
	}

	if (g_frameUpdated)
		do_timer_wait();
	g_frameUpdated = 0;

	if (cont)
		return 0;
	else
		return MVE_ERR_EOF;
}

void MVE_rmEndMovie()
{
#ifdef AUDIO
	int i;
#endif

	timer_stop();
	timer_created = 0;

#ifdef AUDIO
	if (mve_audio_canplay) {
		// only close audio if we opened it
		SDL_CloseAudio();
		mve_audio_canplay = 0;
	}
	for (i = 0; i < TOTAL_AUDIO_BUFFERS; i++)
		if (mve_audio_buffers[i] != NULL)
			d_free(mve_audio_buffers[i]);
	memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
	memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));
	mve_audio_curbuf_curpos=0;
	mve_audio_bufhead=0;
	mve_audio_buftail=0;
	mve_audio_playing=0;
	mve_audio_canplay=0;
	mve_audio_compressed=0;
	if (mve_audio_spec)
		d_free(mve_audio_spec);
	mve_audio_spec=NULL;
	audiobuf_created = 0;
#endif

	if (g_vBuffers != NULL)
		free(g_vBuffers);
	g_vBuffers = NULL;
	g_pCurMap=NULL;
	g_nMapLength=0;
	videobuf_created = 0;
	video_initialized = 0;

	mve_close_filehandle(mve);
	mve = NULL;

	if (mve_audio_canplay)
		I_DestroyMovieAudio();
}


void MVE_rmHoldMovie()
{
	timer_started = 0;
	I_PauseMovieAudio();
	mve_audio_paused = 1;
}


void MVE_sndInit(int x)
{
#ifdef AUDIO
	if (x == -1)
		mve_audio_enabled = 0;
	else
		mve_audio_enabled = 1;
#endif
}

//[ISB] whoops i made assumptions again. I guess D2X really tweaked how all this crap worked
void MVE_memVID(void* first, void* second, size_t len)
{
	hackBuf1 = first;
	hackBuf2 = second;

	if (g_vBackBuf1 != NULL && g_vBackBuf2 != NULL)
	{
		g_vBackBuf1 = first;
		g_vBackBuf2 = second;
	}
}

void MVE_sfCallbacks(mve_cb_ShowFrame *func)
{
	ShowFrameCallback = func;
}

void MVE_palCallbacks(mve_cb_SetPalette* func)
{
	SetPaletteCallback = func;
}

void MVE_ReleaseMem()
{
	hackBuf1 = hackBuf2 = NULL;
}
