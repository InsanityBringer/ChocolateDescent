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

static char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform/posixstub.h"

#include "2d/gr.h"
#include "ui/ui.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "platform/timer.h"
#include "3d/3d.h"
#include "bm.h"
#include "inferno.h"
#include "misc/error.h"
//#include "cflib.h"
#include "cfile/cfile.h"
//#include "div0.h" //[ISB] one worry
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "mem/mem.h"
#include "textures.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap/texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "switch.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "2d/palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#ifdef NETWORK
#include "ipx.h"
#endif
#include "newdemo.h"
//#include "victor.h"
#include "network.h"
#include "modem.h"
#include "gamefont.h"
#include "kconfig.h"
#ifdef ARCADE
#include "arcade.h"
#include "coindev.h"
#endif
#include "platform/mouse.h"
#include "platform/joy.h"
//#include "dpmi.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
//#include "iglasses.h"
#include "songs.h"
#include "cfile/cfile.h"
#ifdef USE_CDROM
#include "cdrom.h"
#endif
#include "gameseq.h"

#ifdef EDITOR
#include "editor\editor.h"
#include "editor\kdefs.h"
#endif

#include "vers_id.h"
#include "2d/i_gr.h"

extern int Game_simuleyes_flag;

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM;
char desc_id_exit_num = 0;

int Function_mode = FMODE_MENU;		//game or editor?
int Screen_mode = -1;					//game screen or editor screen?

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running = 0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
//[ISB] ugh
extern int bm_init_use_tbl();
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present

#ifndef NDEBUG
void do_heap_check()
{
	/*
	int heap_status;

	heap_status = _heapset(0xFF);
	switch (heap_status)
	{
	case _HEAPBADBEGIN:
		mprintf((1, "ERROR - heap is damaged\n"));
		Int3();
		break;
	case _HEAPBADNODE:
		mprintf((1, "ERROR - bad node in heap\n"));
		Int3();
		break;
	}*/
}
#endif

int registered_copy = 0;
char name_copy[sizeof(DESC_ID_STR)];

void check_id_checksum_and_date()
{
	const char name[] = DESC_ID_STR;
	char time_str[] = DESC_DEAD_TIME;
	int i, found;
	unsigned long* checksum, test_checksum;
	time_t current_time, saved_time;

	saved_time = (time_t)strtol(&(time_str[strlen(time_str) - 10]), NULL, 16);
	if (saved_time == (time_t)0)
		return;

	strcpy(name_copy, name);
	registered_copy = 1;

	current_time = time(NULL);
	if (current_time >= saved_time)
		desc_id_exit_num = 1;

	test_checksum = 0;
	for (i = 0; i < (int)strlen(name); i++) 
	{
		found = 0;
		test_checksum += name[i];
		if (((test_checksum / 2) * 2) != test_checksum)
			found = 1;
		test_checksum = test_checksum >> 1;
		if (found)
			test_checksum |= 0x80000000;
	}
	checksum = (unsigned long*) & (desc_id_checksum_str[0]);
	if (test_checksum != *checksum)
		desc_id_exit_num = 2;

	printf("%s %s\n", TXT_REGISTRATION, name);
}

int is_3dbios_installed()
{
	Error("is_3dbios_installed: STUB\n");
	return 0;
}


int init_graphics()
{
	int result;

	result = gr_check_mode(SM_320x200C);
#ifdef EDITOR
	if (result == 0)
		result = gr_check_mode(SM_800x600V);
#endif

	switch (result) {
	case  0:		//Mode set OK
#ifdef EDITOR
		Inferno_is_800x600_available = 1;
#endif
		break;
	case  1:		//No VGA adapter installed
		printf("%s\n", TXT_REQUIRES_VGA);
		return 1;
	case 10:		//Error allocating selector for A0000h
		printf("%s\n", TXT_ERROR_SELECTOR);
		return 1;
	case 11:		//Not a valid mode support by gr.lib
		printf("%s\n", TXT_ERROR_GRAPHICS);
		return 1;
#ifdef EDITOR
	case  3:		//Monitor doesn't support that VESA mode.
	case  4:		//Video card doesn't support that VESA mode.

		printf("Your VESA driver or video hardware doesn't support 800x600 256-color mode.\n");
		break;
	case  5:		//No VESA driver found.
		printf("No VESA driver detected.\n");
		break;
	case  2:		//Program doesn't support this VESA granularity
	case  6:		//Bad Status after VESA call/
	case  7:		//Not enough DOS memory to call VESA functions.
	case  8:		//Error using DPMI.
	case  9:		//Error setting logical line width.
	default:
		printf("Error %d using 800x600 256-color VESA mode.\n", result);
		break;
#endif
	}

	return 0;
}

extern fix fixed_frametime;

// Returns 1 if ok, 0 if failed...
int init_gameport()
{
	Error("init_gameport: STUB\n");
	return 0;
}

void check_dos_version()
{
	Error("check_dos_version: STUB\n");
}

void change_to_dir(char* cmd_line)
{
	Error("change_to_dir: STUB\n");
}

void dos_check_file_handles(int num_required)
{
	Error("dos_check_file_handles: STUB\n");
}

#define NEEDED_DOS_MEMORY   			( 300*1024)		// 300 K
#define NEEDED_LINEAR_MEMORY 			(7680*1024)		// 7.5 MB
#define LOW_PHYSICAL_MEMORY_CUTOFF	(5*1024*1024)	// 5.0 MB
#define NEEDED_PHYSICAL_MEMORY		(2000*1024)		// 2000 KB

extern int piggy_low_memory;

void mem_int_to_string(int number, char* dest)
{
	int i, l, c;
	char buffer[20], * p;

	sprintf(buffer, "%d", number);

	l = strlen(buffer);
	if (l <= 3) {
		// Don't bother with less than 3 digits
		sprintf(dest, "%d", number);
		return;
	}

	c = 0;
	p = dest;
	for (i = l - 1; i >= 0; i--) {
		if (c == 3) {
			*p++ = ',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	_strrev(dest);
}

void check_memory()
{
	//it's fine don't worry about it
}


int Inferno_verbose = 0;

//NO_STACK_SIZE_CHECK uint32_t * stack, *stack_ptr;
//NO_STACK_SIZE_CHECK int stack_size, unused_stack_space;
//NO_STACK_SIZE_CHECK int sil;
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK int main(int argc,char **argv)
//NO_STACK_SIZE_CHECK {
//NO_STACK_SIZE_CHECK 	uint32_t ret_value;
//NO_STACK_SIZE_CHECK 	
//NO_STACK_SIZE_CHECK 	unused_stack_space = 0;
//NO_STACK_SIZE_CHECK 	stack = &ret_value;
//NO_STACK_SIZE_CHECK 	stack_size = stackavail()/4;
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK 	for ( sil=0; sil<stack_size; sil++ )	{
//NO_STACK_SIZE_CHECK 		stack--;
//NO_STACK_SIZE_CHECK 		*stack = 0xface0123;
//NO_STACK_SIZE_CHECK 	}
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK 	ret_value = descent_main( argc, argv );		// Rename main to be descent_main
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK 	for ( sil=0; sil<stack_size; sil++ )	{
//NO_STACK_SIZE_CHECK 		if ( *stack == 0xface0123 )	
//NO_STACK_SIZE_CHECK 			unused_stack_space++;
//NO_STACK_SIZE_CHECK 		stack++;
//NO_STACK_SIZE_CHECK 	}
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK 	mprintf(( 0, "Program used %d/%d stack space\n", (stack_size - unused_stack_space)*4, stack_size*4 ));
//NO_STACK_SIZE_CHECK 	key_getch();
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK 	return ret_value;
//NO_STACK_SIZE_CHECK }

extern int digi_timer_rate;

int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(uint8_t*);

extern int Game_vfx_flag;
extern int Game_victor_flag;
extern int Game_vio_flag;
extern int Game_3dmax_flag;
extern int VR_low_res;

#ifdef USE_CD
char destsat_cdpath[128] = "";
int find_descent_cd();
#endif

extern int Config_vr_type;
extern int Config_vr_tracking;

//[ISB] Okay, the trouble is that SDL redefines main. I don't want to include SDL here. Solution is to rip off doom
//and add a separate main function
int D_DescentMain(int argc, const char** argv)
{
	int t;
	uint8_t title_pal[768];

	error_init(NULL);

	setbuf(stdout, NULL);	// unbuffered output via printf

	InitArgs(argc, argv);

	int initStatus = I_Init();
	if (initStatus)
	{
		Error("Error initalizing graphics library, code %d\n", initStatus);
		return 1;
	}

	if (FindArg("-verbose"))
		Inferno_verbose = 1;

	//change_to_dir(argv[0]);

	//[ISB] cut DPMI

	/*if (!cfile_init("DESCENT.HOG")) //[ISB] changed, don't actually init hogfile for obscure vanilla compatibility reasons. 
	{
		Error("Could not find required file <%s>", "DESCENT.HOG");
	}*/

	load_text();

	//	set_exit_message("\n\n%s", TXT_THANKS);

	printf("\nDESCENT   %s %s %s\n", VERSION_NAME, __DATE__, __TIME__);
	printf("%s\n%s\n", TXT_COPYRIGHT, TXT_TRADEMARK);

	check_id_checksum_and_date();

	if (FindArg("-?") || FindArg("-help") || FindArg("?")) {

		printf("%s\n", TXT_COMMAND_LINE_0);

		printf("  -SimulEyes     %s\n",
			"Enables StereoGraphics SimulEyes VR stereo display");

		printf("  -Iglasses      %s\n", TXT_IGLASSES);
		printf("  -VioTrack <n>  %s n\n", TXT_VIOTRACK);
		printf("  -3dmaxLo       %s\n", TXT_KASAN);
		printf("                 %s\n", TXT_KASAN_2);
		printf("  -3dmaxHi       %s\n", TXT_3DMAX);
		printf("%s\n", TXT_COMMAND_LINE_1);
		printf("%s\n", TXT_COMMAND_LINE_2);
		printf("%s\n", TXT_COMMAND_LINE_3);
		printf("%s\n", TXT_COMMAND_LINE_4);
		printf("%s\n", TXT_COMMAND_LINE_5);
		printf("%s\n", TXT_COMMAND_LINE_6);
		printf("%s\n", TXT_COMMAND_LINE_7);
		printf("%s\n", TXT_COMMAND_LINE_8);
		printf("%s\n", TXT_COMMAND_LINE_9);
		printf("%s\n", TXT_COMMAND_LINE_10);
		printf("%s\n", TXT_COMMAND_LINE_11);
		printf("%s\n", TXT_COMMAND_LINE_12);
		printf("%s\n", TXT_COMMAND_LINE_13);
		printf("%s\n", TXT_COMMAND_LINE_14);
		printf("%s\n", TXT_COMMAND_LINE_15);
		printf("%s\n", TXT_COMMAND_LINE_16);
		printf("%s\n", TXT_COMMAND_LINE_17);
		printf("%s\n", TXT_COMMAND_LINE_18);
		printf("  -DynamicSockets %s\n", TXT_SOCKET);
		printf("  -NoFileCheck    %s\n", TXT_NOFILECHECK);
		printf("  -GamePort       %s\n", "Use Colorado Spectrum's Notebook Gameport");
		printf("  -NoDoubleBuffer %s\n", "Use only one page of video memory");
		printf("  -LCDBios        %s\n", "Enables LCDBIOS for using LCD shutter glasses");
		printf("  -JoyNice        %s\n", "Joystick poller allows interrupts to occur");
		set_exit_message("");
		return(0);
	}

	printf("\n%s\n", TXT_HELP);

#ifdef PASSWORD
	if ((t = FindArg("-pswd")) != 0) {
		int	n;
		int8_t* pp = Side_to_verts;
		int ch;
		for (n = 0; n < 6; n++)
			for (ch = 0; ch < strlen(Args[t + 1]); ch++)
				* pp++ ^= Args[t + 1][ch];
	}
	else
		Error("Invalid processor");		//missing password
#endif

	if (FindArg("-autodemo"))
		Auto_demo = 1;

#ifndef RELEASE
	if (FindArg("-noscreens"))
		Skip_briefing_screens = 1;
#endif

	//[ISB] Allow the user to configure the FPS limit, if desired
	int limitParam = FindArg("-fpslimit");
	if (limitParam && limitParam < (Num_args - 1))
	{
		FPSLimit = atoi(Args[limitParam + 1]);
		if (FPSLimit < 4) FPSLimit = 4; if (FPSLimit > 150) FPSLimit = 150;
	}
	if (Inferno_verbose) printf("Setting FPS Limit %d\n", FPSLimit);

	Lighting_on = 1;

	strcpy(Menu_pcx_name, "menu.pcx");	//	Used to be menu2.pcx.

	if (init_graphics()) return 1;

#ifdef EDITOR
	if (!Inferno_is_800x600_available) {
		printf("The editor will not be available...\n");
		Function_mode = FMODE_MENU;
	}
#endif

#ifndef NDEBUG
	minit();
	mopen(0, 9, 1, 78, 15, "Debug Spew");
	mopen(1, 2, 1, 78, 5, "Errors & Serious Warnings");
#endif

	if (Inferno_verbose) printf("%s", TXT_VERBOSE_1);
	ReadConfigFile();
	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_2);

	timer_init();
	joy_set_timer_rate(digi_timer_rate);	 	// Tell joystick how fast timer is going

	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_3);
	key_init();
	if (!FindArg("-nomouse")) {
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_4);
		mouse_init(0); //[ISB] cut cyberman
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_5);
	}
	if (!FindArg("-nojoystick")) 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_6);
		joy_init();
		//[ISB] cut j
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_10);
	}
	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_11);
	//div0_init(DM_ERROR); //[ISB] uh i should figure out what to do with this...

	//------------ Init sound ---------------
	if (!FindArg("-nosound")) //[ISB] no sound yet
	{
		if (digi_init()) 
		{
			printf("\nFailed to start sound.\n");
		}
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_SOUND_DISABLED);
	}

#ifdef NETWORK
	if (!FindArg("-nonetwork")) {
		int socket = 0, showaddress = 0;
		int ipx_error;
		if (Inferno_verbose) printf("\n%s ", TXT_INITIALIZING_NETWORK);
		if ((t = FindArg("-socket")))
			socket = atoi(Args[t + 1]);
		if (FindArg("-showaddress")) showaddress = 1;
		if ((ipx_error = ipx_init(IPX_DEFAULT_SOCKET + socket, showaddress)) == 0) {
			if (Inferno_verbose) printf("%s %d.\n", TXT_IPX_CHANNEL, socket);
			Network_active = 1;
		}
		else {
			switch (ipx_error) {
			case 3: 	if (Inferno_verbose) printf("%s\n", TXT_NO_NETWORK); break;
			case -2: if (Inferno_verbose) printf("%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET + socket); break;
			case -4: if (Inferno_verbose) printf("%s\n", TXT_MEMORY_IPX); break;
			default:
				if (Inferno_verbose) printf("%s %d", TXT_ERROR_IPX, ipx_error);
			}
			if (Inferno_verbose) printf("%s\n", TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		ipx_read_user_file("descent.usr");
		ipx_read_network_file("descent.net");
		if (FindArg("-dynamicsockets"))
			Network_allow_socket_changes = 1;
		else
			Network_allow_socket_changes = 0;
	}
	else {
		if (Inferno_verbose) printf("%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}

	if (!FindArg("-noserial"))
	{
		serial_active = 1;
	}
	else
	{
		serial_active = 0;
	}
#endif

	//[ISB] kill a ridiculous amount of VR stuff. it'd be cool to try to get some sort of crude VR working but...
	{
		int screen_mode = SM_320x200C;
		int screen_width = 320;
		int screen_height = 200;
		int vr_mode = VR_NONE;
		int screen_compatible = 1;
		int use_double_buffer = 0;

		if (FindArg("-320x240")) 
		{
			if (Inferno_verbose) printf("Using 320x240 ModeX...\n");
			screen_mode = SM_320x240U;
			screen_width = 320;
			screen_height = 240;
			screen_compatible = 0;
			use_double_buffer = 1;
		}

		if (FindArg("-320x400")) 
		{
			if (Inferno_verbose) printf("Using 320x400 ModeX...\n");
			screen_mode = SM_320x400U;
			screen_width = 320;
			screen_height = 400;
			screen_compatible = 0;
			use_double_buffer = 1;
		}

		if (!Game_simuleyes_flag && FindArg("-640x400"))
		{
			if (Inferno_verbose) printf("Using 640x400 VESA...\n");
			screen_mode = SM_640x400V;
			screen_width = 640;
			screen_height = 400;
			screen_compatible = 0;
			use_double_buffer = 1;
		}

		if (!Game_simuleyes_flag && FindArg("-640x480")) 
		{
			if (Inferno_verbose) printf("Using 640x480 VESA...\n");
			screen_mode = SM_640x480V;
			screen_width = 640;
			screen_height = 480;
			screen_compatible = 0;
			use_double_buffer = 1;
		}
		if (FindArg("-320x100")) 
		{
			if (Inferno_verbose) printf("Using 320x100 VGA...\n");
			screen_mode = 19;
			screen_width = 320;
			screen_height = 100;
			screen_compatible = 0;
		}

		if (FindArg("-nodoublebuffer")) 
		{
			if (Inferno_verbose) printf("Double-buffering disabled...\n");
			use_double_buffer = 0;
		}

		game_init_render_buffers(screen_mode, screen_width, screen_height, use_double_buffer, vr_mode, screen_compatible);
	}

	VR_switch_eyes = 0;

#ifdef NETWORK
	//	i = FindArg( "-rinvul" );
	//	if (i > 0) {
	//		int mins = atoi(Args[i+1]);
	//		if (mins > 314)
	//			mins = 314;
	// 	control_invul_time = mins/5;
	//	}
	control_invul_time = 0;
#endif

	if (Inferno_verbose) printf("\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if ((t = gr_init(SM_ORIGINAL)) != 0)
		Error(TXT_CANT_INIT_GFX, t);
	// Load the palette stuff. Returns non-zero if error.
	mprintf((0, "Going into graphics mode..."));
	gr_set_mode(SM_320x200C);
	mprintf((0, "\nInitializing palette system..."));
	gr_use_palette_table("PALETTE.256");
	mprintf((0, "\nInitializing font system..."));
	gamefont_init();	// must load after palette data loaded.
	songs_play_song(SONG_TITLE, 1);

#ifndef RELEASE
	if (!FindArg("-notitles"))
#endif
	{	//NOTE LINK TO ABOVE!
		show_title_screen("iplogo1.pcx", 1);
		show_title_screen("logo.pcx", 1);
	}

	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		strcpy(filename, "descent.pcx");

		if ((pcx_error = pcx_read_bitmap(filename, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal)) == PCX_ERROR_NONE) {
			vfx_set_palette_sub(title_pal);
			gr_palette_clear();
			//gr_bitmap( 0, 0, &title_bm );
			gr_palette_fade_in(title_pal, 32, 0);
			//free(title_bm.bm_data);
		}
		else {
			gr_close();
			Error("Couldn't load pcx file '%s', PCX load error: %s\n", filename, pcx_errormsg(pcx_error));
		}
	}

#ifdef EDITOR
	if (!FindArg("-nobm"))
		bm_init_use_tbl();
	else
		bm_init();
#else
	bm_init();
#endif

	if (FindArg("-norun"))
		return(0);

	mprintf((0, "\nInitializing 3d system..."));
	g3_init();
	mprintf((0, "\nInitializing texture caching system..."));
	texmerge_init(10);		// 10 cache bitmaps
	mprintf((0, "\nRunning game...\n"));
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';
	if (!Auto_demo) {
		key_flush();
		RegisterPlayer();		//get player's name
	}

	gr_palette_fade_out(title_pal, 32, 0);

	//check for special stamped version
	if (registered_copy) {
		nm_messagebox("EVALUATION COPY", 1, "Continue",
			"This special evaluation copy\n"
			"of DESCENT has been issued to:\n\n"
			"%s\n"
			"\n\n    NOT FOR DISTRIBUTION",
			name_copy);

		gr_palette_fade_out(gr_palette, 32, 0);
	}

	//kconfig_load_all();

	Game_mode = GM_GAME_OVER;

	if (Auto_demo) {
		newdemo_start_playback("DESCENT.DEM");
		if (Newdemo_state == ND_STATE_PLAYBACK)
			Function_mode = FMODE_GAME;
	}

	build_mission_list(0);		// This also loads mission 0.

	while (Function_mode != FMODE_EXIT)
	{
		switch (Function_mode) 
		{
		case FMODE_MENU:
			if (Auto_demo) 
			{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)
					Error("No demo files were found for autodemo mode!");
			}
			else 
			{
				check_joystick_calibration();
				DoMenu();
#ifdef EDITOR
				if (Function_mode == FMODE_EDITOR) {
					create_new_mine();
					SetPlayerFromCurseg();
				}
#endif
			}
			break;
		case FMODE_GAME:
#ifdef EDITOR
			keyd_editor_mode = 0;
#endif
			game();
			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE, 1);
#ifdef EDITOR
			else if (Function_mode == FMODE_EDITOR) //[ISB] If you do menu->game->editor cursegp won't be valid. Fix this. 
			{
				if (!Cursegp)
					init_editor_data_for_mine();
			}
#endif
			break;
#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
			//_harderr((void*)descent_critical_error_handler);		// Reinstall game error handler
			if (Function_mode == FMODE_GAME) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
#endif
		default:
			Error("Invalid function mode %d", Function_mode);
		}
	}

	WriteConfigFile();

#ifndef ROCKWELL_CODE
#ifndef RELEASE
	if (!FindArg("-notitles"))
#endif
		//NOTE LINK TO ABOVE!!
#ifndef EDITOR
		show_order_form();
#endif
#endif

#ifndef NDEBUG
	if (FindArg("-showmeminfo"))
		//		show_mem_info = 1;		// Make memory statistics show
#endif

		I_Shutdown();
		return(0);		//presumably successful exit
}


void check_joystick_calibration()
{
	/*
	int x1, y1, x2, y2, c;
	fix t1;

	if ((Config_control_type != CONTROL_JOYSTICK) &&
		(Config_control_type != CONTROL_FLIGHTSTICK_PRO) &&
		(Config_control_type != CONTROL_THRUSTMASTER_FCS) &&
		(Config_control_type != CONTROL_GRAVIS_GAMEPAD)
		) return;

	joy_get_pos(&x1, &y1);

	t1 = timer_get_fixed_seconds();
	while (timer_get_fixed_seconds() < t1 + F1_0 / 100)
		;

	joy_get_pos(&x2, &y2);

	// If joystick hasn't moved...
	
	if ((abs(x2 - x1) < 30) && (abs(y2 - y1) < 30)) 
	{
		if ((abs(x1) > 30) || (abs(x2) > 30) || (abs(y1) > 30) || (abs(y2) > 30)) {
			c = nm_messagebox(NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN);
			if (c == 0) {
				joydefs_calibrate();
			}
		}
	}
	*/
}

void show_order_form()
{
	int pcx_error;
	uint8_t title_pal[768];
	char	exit_screen[16];

	gr_set_current_canvas(NULL);
	gr_palette_clear();

	key_flush();

#ifdef SHAREWARE
	strcpy(exit_screen, "order01.pcx");
#else
#ifdef DEST_SAT
	strcpy(exit_screen, "order01.pcx");
#else
	strcpy(exit_screen, "warning.pcx");
#endif
#endif
	if ((pcx_error = pcx_read_bitmap(exit_screen, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal)) == PCX_ERROR_NONE) 
	{
		vfx_set_palette_sub(title_pal);
		gr_palette_fade_in(title_pal, 32, 0);
		{
			int done = 0;
			fix time_out_value = timer_get_approx_seconds() + i2f(60 * 5);
			while (!done) 
			{
				I_MarkStart();
				I_DoEvents();
				if (timer_get_approx_seconds() > time_out_value) done = 1;
				if (key_inkey()) done = 1;
				I_DrawCurrentCanvas(0);
				I_MarkEnd(US_70FPS);
			}
		}
		gr_palette_fade_out(title_pal, 32, 0);
	}
	key_flush();
}
