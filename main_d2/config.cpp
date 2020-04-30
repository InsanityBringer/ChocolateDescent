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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "winapp.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "misc/types.h"
#include "game.h"
#include "menu.h"
#include "movie.h"
#include "digi.h"
#include "kconfig.h"
#include "2d/palette.h"
#include "platform/joy.h"
#include "songs.h"
#include "misc/args.h"
#include "player.h"
#include "mission.h"
#include "platform/mono.h"
//#include "pa_enabl.h"

uint8_t Config_digi_volume = 8;
uint8_t Config_midi_volume = 8;
uint8_t Config_redbook_volume = 8;
uint8_t Config_control_type = 0;
uint8_t Config_channels_reversed = 0;
uint8_t Config_joystick_sensitivity = 8;

static const char *digi_dev8_str = "DigiDeviceID8";
static const char *digi_dev16_str = "DigiDeviceID16";
static const char *digi_port_str = "DigiPort";
static const char *digi_irq_str = "DigiIrq";
static const char *digi_dma8_str = "DigiDma8";
static const char *digi_dma16_str = "DigiDma16";
static const char *digi_volume_str = "DigiVolume";
static const char *midi_volume_str = "MidiVolume";
static const char *redbook_enabled_str = "RedbookEnabled";
static const char *redbook_volume_str = "RedbookVolume";
static const char *midi_dev_str = "MidiDeviceID";
static const char *midi_port_str = "MidiPort";
static const char *detail_level_str = "DetailLevel";
static const char *gamma_level_str = "GammaLevel";
static const char *stereo_rev_str = "StereoReverse";
static const char *joystick_min_str = "JoystickMin";
static const char *joystick_max_str = "JoystickMax";
static const char *joystick_cen_str = "JoystickCen";
static const char *last_player_str = "LastPlayer";
static const char *last_mission_str = "LastMission";
static const char *config_vr_type_str = "VR_type";
static const char *config_vr_resolution_str = "VR_resolution";
static const char *config_vr_tracking_str = "VR_tracking";
static const char *movie_hires_str = "MovieHires";

#define _CRYSTAL_LAKE_8_ST		0xe201
#define _CRYSTAL_LAKE_16_ST	0xe202
#define _AWE32_8_ST				0xe208
#define _AWE32_16_ST				0xe209

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";

int Config_digi_type = 0;
int Config_digi_dma = 0;
int Config_midi_type = 0;

#ifdef WINDOWS
int	 DOSJoySaveMin[4];
int	 DOSJoySaveCen[4];
int	 DOSJoySaveMax[4];

char win95_current_joyname[256];
#endif



int Config_vr_type = 0;
int Config_vr_resolution = 0;
int Config_vr_tracking = 0;

int digi_driver_board_16;
int digi_driver_dma_16;

extern int8_t	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);


#define CL_MC0 0xF8F
#define CL_MC1 0xF8D

void CrystalLakeWriteMCP( uint16_t mc_addr, uint8_t mc_data )
{
	/*_disable();
	outp( CL_MC0, 0xE2 );				// Write password
	outp( mc_addr, mc_data );		// Write data
	_enable();*/
	//Warning("CrystalLakeWriteMCP: STUB\n");
	//fprintf(stderr, "CrystalLakeWriteMCP: STUB\n");
}

uint8_t CrystalLakeReadMCP(uint16_t mc_addr )
{
	uint8_t value = 0;
	/*_disable();
	outp( CL_MC0, 0xE2 );		// Write password
	value = inp( mc_addr );		// Read data
	_enable();*/
	//fprintf(stderr, "CrystalLakeReadMCP: STUB\n");
	return value;
}

void CrystalLakeSetSB()
{
	/*uint8_t tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp &= 0x7F;
	CrystalLakeWriteMCP( CL_MC1, tmp );*/
}

void CrystalLakeSetWSS()
{
	/*uint8_t tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp |= 0x80;
	CrystalLakeWriteMCP( CL_MC1, tmp );*/
}

//MovieHires might be changed by -nohighres, so save a "real" copy of it
int SaveMovieHires;
int save_redbook_enabled;

#ifdef WINDOWS
void CheckMovieAttributes()
{
		HKEY hKey;
		DWORD len, type, val;
		long lres;
  
		lres = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Parallax\\Descent II\\1.1\\INSTALL",
							0, KEY_READ, &hKey);
		if (lres == ERROR_SUCCESS) {
			len = sizeof(val);
			lres = RegQueryValueEx(hKey, "HIRES", NULL, &type, &val, &len);
			if (lres == ERROR_SUCCESS) {
				MovieHires = val;
				logentry("HIRES=%d\n", val);
			}
			RegCloseKey(hKey);
		}
}
#endif

int ReadConfigFile()
{
	FILE *infile;
	char line[80], *token, *value, *ptr;
	uint8_t gamma;
	int joy_axis_min[7];
	int joy_axis_center[7];
	int joy_axis_max[7];
	int i;

	strcpy( config_last_player, "" );

	joy_axis_min[0] = joy_axis_min[1] = joy_axis_min[2] = joy_axis_min[3] = 0;
	joy_axis_max[0] = joy_axis_max[1] = joy_axis_max[2] = joy_axis_max[3] = 0;
	joy_axis_center[0] = joy_axis_center[1] = joy_axis_center[2] = joy_axis_center[3] = 0;

#ifdef WINDOWS
	memset(&joy_axis_min[0], 0, sizeof(int)*7);
	memset(&joy_axis_max[0], 0, sizeof(int)*7);
	memset(&joy_axis_center[0], 0, sizeof(int)*7);
//@@	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#else
	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#endif

	digi_driver_board = 0;
	digi_driver_port = 0;
	digi_driver_irq = 0;
	digi_driver_dma = 0;

	digi_midi_type = 0;
	digi_midi_port = 0;

	Config_digi_volume = 8;
	Config_midi_volume = 8;
	Config_redbook_volume = 8;
	Config_control_type = 0;
	Config_channels_reversed = 0;

	//set these here in case no cfg file
	SaveMovieHires = MovieHires;
	save_redbook_enabled = Redbook_enabled;

	infile = fopen("descent.cfg", "rt");
	if (infile == NULL) {
		WIN(CheckMovieAttributes());
		return 1;
	}
	while (!feof(infile)) {
		memset(line, 0, 80);
		fgets(line, 80, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (value[strlen(value)-1] == '\n')
				value[strlen(value)-1] = 0;
			if (!strcmp(token, digi_dev8_str))
				digi_driver_board = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_dev16_str))
				digi_driver_board_16 = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_port_str))
				digi_driver_port = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_irq_str))
				digi_driver_irq = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma8_str))
				digi_driver_dma = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma16_str))
				digi_driver_dma_16 = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_volume_str))
				Config_digi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, midi_dev_str))
				digi_midi_type = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_port_str))
				digi_midi_port = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_volume_str))
				Config_midi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, redbook_enabled_str))
				Redbook_enabled = save_redbook_enabled = strtol(value, NULL, 10);
			else if (!strcmp(token, redbook_volume_str))
				Config_redbook_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, stereo_rev_str))
				Config_channels_reversed = strtol(value, NULL, 10);
			else if (!strcmp(token, gamma_level_str)) {
				gamma = strtol(value, NULL, 10);
				gr_palette_set_gamma( gamma );
			}
			else if (!strcmp(token, detail_level_str)) {
				Detail_level = strtol(value, NULL, 10);
				if (Detail_level == NUM_DETAIL_LEVELS-1) {
					int count,dummy,oc,od,wd,wrd,da,sc;

					count = sscanf (value, "%d,%d,%d,%d,%d,%d,%d\n",&dummy,&oc,&od,&wd,&wrd,&da,&sc);

					if (count == 7) {
						Object_complexity = oc;
						Object_detail = od;
						Wall_detail = wd;
						Wall_render_depth = wrd;
						Debris_amount = da;
						SoundChannels = sc;
						set_custom_detail_vars();
					}
				  #ifdef PA_3DFX_VOODOO   // Set to highest detail because you can't change em	
					   Object_complexity=Object_detail=Wall_detail=
						Wall_render_depth=Debris_amount=SoundChannels = NUM_DETAIL_LEVELS-1;
						Detail_level=NUM_DETAIL_LEVELS-1;
						set_custom_detail_vars();
					#endif
				}
			}
			else if (!strcmp(token, joystick_min_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3] );
			} 
			else if (!strcmp(token, joystick_max_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3] );
			}
			else if (!strcmp(token, joystick_cen_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3] );
			}
			else if (!strcmp(token, last_player_str))	{
				char * p;
				strncpy( config_last_player, value, CALLSIGN_LEN );
				p = strchr( config_last_player, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, last_mission_str))	{
				char * p;
				strncpy( config_last_mission, value, MISSION_NAME_LEN );
				p = strchr( config_last_mission, '\n');
				if ( p ) *p = 0;
			} else if (!strcmp(token, config_vr_type_str)) {
				Config_vr_type = strtol(value, NULL, 10);
			} else if (!strcmp(token, config_vr_resolution_str)) {
				Config_vr_resolution = strtol(value, NULL, 10);
			} else if (!strcmp(token, config_vr_tracking_str)) {
				Config_vr_tracking = strtol(value, NULL, 10);
			} else if (!strcmp(token, movie_hires_str)) {
				SaveMovieHires = MovieHires = strtol(value, NULL, 10);
			}
		}
	}

	fclose(infile);

#ifdef WINDOWS
	for (i=0;i<4;i++)
	{
	 DOSJoySaveMin[i]=joy_axis_min[i];
	 DOSJoySaveCen[i]=joy_axis_center[i];
	 DOSJoySaveMax[i]=joy_axis_max[i];
   	}
#else
	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#endif

	i = FindArg( "-volume" );
	
	if ( i > 0 )	{
		i = atoi( Args[i+1] );
		if ( i < 0 ) i = 0;
		if ( i > 100 ) i = 100;
		Config_digi_volume = (i*8)/100;
		Config_midi_volume = (i*8)/100;
		Config_redbook_volume = (i*8)/100;
	}

	if ( Config_digi_volume > 8 ) Config_digi_volume = 8;
	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;
	if ( Config_redbook_volume > 8 ) Config_redbook_volume = 8;

	digi_set_volume( (Config_digi_volume*32768)/8, (Config_midi_volume*128)/8 );
/*
	printf( "DigiDeviceID: 0x%x\n", digi_driver_board );
	printf( "DigiPort: 0x%x\n", digi_driver_port		);
	printf( "DigiIrq: 0x%x\n",  digi_driver_irq		);
	printf( "DigiDma: 0x%x\n",	digi_driver_dma	);
	printf( "MidiDeviceID: 0x%x\n", digi_midi_type	);
	printf( "MidiPort: 0x%x\n", digi_midi_port		);
  	key_getch();
*/

	Config_midi_type = digi_midi_type;
	Config_digi_type = digi_driver_board;
	Config_digi_dma = digi_driver_dma;

#ifndef WINDOWS
	if (digi_driver_board_16 > 0 && !FindArg("-no16bit")/* && digi_driver_board_16 != _GUS_16_ST*/) //[ISB] todo
	{
		digi_driver_board = digi_driver_board_16;
		digi_driver_dma = digi_driver_dma_16;
	}
#endif

	// HACK!!! 
	//Hack to make some cards look like others, such as
	//the Crytal Lake look like Microsoft Sound System
#ifndef WINDOWS //[ISB] todo
	/*
	if ( digi_driver_board == _CRYSTAL_LAKE_8_ST )	{
		uint8_t tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_8_ST;
	} else if ( digi_driver_board == _CRYSTAL_LAKE_16_ST )	{
		uint8_t tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_16_ST;
	} else if ( digi_driver_board == _AWE32_8_ST )	{
		digi_driver_board = _SB16_8_ST;
	} else if ( digi_driver_board == _AWE32_16_ST )	{
		digi_driver_board = _SB16_16_ST;
	} else
		digi_driver_board		= digi_driver_board;*/


#else
	infile = fopen("descentw.cfg", "rt");
	if (infile) {
		while (!feof(infile)) {
			memset(line, 0, 80);
			fgets(line, 80, infile);
			ptr = &(line[0]);
			while (isspace(*ptr))
				ptr++;
			if (*ptr != '\0') {
				token = strtok(ptr, "=");
				value = strtok(NULL, "=");
				if (value[strlen(value)-1] == '\n')
					value[strlen(value)-1] = 0;
				if (!strcmp(token, joystick_min_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3], &joy_axis_min[4], &joy_axis_min[5], &joy_axis_min[6] );
				} 
				else if (!strcmp(token, joystick_max_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3], &joy_axis_max[4], &joy_axis_max[5], &joy_axis_max[6] );
				}
				else if (!strcmp(token, joystick_cen_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3], &joy_axis_center[4], &joy_axis_center[5], &joy_axis_center[6] );
				}
			}
		}
		fclose(infile);
	}
#endif

	return 0;
}

int WriteConfigFile()
{
	FILE *infile;
   int i;
	char str[256];
	int joy_axis_min[7];
	int joy_axis_center[7];
	int joy_axis_max[7];
	uint8_t gamma = gr_palette_get_gamma();
	
	joy_get_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);

#ifdef WINDOWS
	for (i=0;i<4;i++)
   {
	 joy_axis_min[i]=DOSJoySaveMin[i];
	 joy_axis_center[i]=DOSJoySaveCen[i];
	 joy_axis_max[i]=DOSJoySaveMax[i];
   }
#endif

	infile = fopen("descent.cfg", "wt");
	if (infile == NULL) {
		return 1;
	}
	sprintf (str, "%s=0x%x\n", digi_dev8_str, Config_digi_type);
	fputs(str, infile);
	sprintf (str, "%s=0x%x\n", digi_dev16_str, digi_driver_board_16);
	fputs(str, infile);
	sprintf (str, "%s=0x%x\n", digi_port_str, digi_driver_port);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", digi_irq_str, digi_driver_irq);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", digi_dma8_str, Config_digi_dma);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", digi_dma16_str, digi_driver_dma_16);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", digi_volume_str, Config_digi_volume);
	fputs(str, infile);
	sprintf (str, "%s=0x%x\n", midi_dev_str, Config_midi_type);
	fputs(str, infile);
	sprintf (str, "%s=0x%x\n", midi_port_str, digi_midi_port);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", midi_volume_str, Config_midi_volume);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", redbook_enabled_str, FindArg("-noredbook")?save_redbook_enabled:Redbook_enabled);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", redbook_volume_str, Config_redbook_volume);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", stereo_rev_str, Config_channels_reversed);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", gamma_level_str, gamma);
	fputs(str, infile);
	if (Detail_level == NUM_DETAIL_LEVELS-1)
		sprintf (str, "%s=%d,%d,%d,%d,%d,%d,%d\n", detail_level_str, Detail_level,
				Object_complexity,Object_detail,Wall_detail,Wall_render_depth,Debris_amount,SoundChannels);
	else
		sprintf (str, "%s=%d\n", detail_level_str, Detail_level);
	fputs(str, infile);

	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_min_str, joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3] );
	fputs(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_cen_str, joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3] );
	fputs(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_max_str, joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3] );
	fputs(str, infile);

	sprintf (str, "%s=%s\n", last_player_str, Players[Player_num].callsign );
	fputs(str, infile);
	sprintf (str, "%s=%s\n", last_mission_str, config_last_mission );
	fputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_type_str, Config_vr_type );
	fputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_resolution_str, Config_vr_resolution );
	fputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_tracking_str, Config_vr_tracking );
	fputs(str, infile);
	sprintf (str, "%s=%d\n", movie_hires_str, SaveMovieHires );
	fputs(str, infile);

	fclose(infile);

#ifdef WINDOWS
{
//	Save Windows Config File
	char joyname[256];
						

	joy_get_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
	
	infile = fopen("descentw.cfg", "wt");
	if (infile == NULL) return 1;

	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_min_str, 
			joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3],
			joy_axis_min[4], joy_axis_min[5], joy_axis_min[6]);
	fputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_cen_str, 
			joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3],
			joy_axis_center[4], joy_axis_center[5], joy_axis_center[6]);
	fputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_max_str, 
			joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3],
			joy_axis_max[4], joy_axis_max[5], joy_axis_max[6]);
	fputs(str, infile);

	fclose(infile);
}
	CheckMovieAttributes();
#endif

	return 0;
}		
