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
#include <string.h>
#include <ctype.h>

#include "misc/types.h"
#include "game.h"
#include "digi.h"
#include "kconfig.h"
#include "2d/palette.h"
#include "platform/joy.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "misc/error.h"

 //#include "sos.h"//These sos headers are part of a commercial library, and aren't included-KRB
 //#include "sosm.h"

static const char* digi_dev_str = "DigiDeviceID";
static const char* digi_port_str = "DigiPort";
static const char* digi_irq_str = "DigiIrq";
static const char* digi_dma_str = "DigiDma";
static const char* digi_volume_str = "DigiVolume";
static const char* midi_volume_str = "MidiVolume";
static const char* midi_dev_str = "MidiDeviceID";
static const char* midi_port_str = "MidiPort";
static const char* detail_level_str = "DetailLevel";
static const char* gamma_level_str = "GammaLevel";
static const char* stereo_rev_str = "StereoReverse";
static const char* joystick_min_str = "JoystickMin";
static const char* joystick_max_str = "JoystickMax";
static const char* joystick_cen_str = "JoystickCen";
static const char* last_player_str = "LastPlayer";
static const char* last_mission_str = "LastMission";
static const char* config_vr_type_str = "VR_type";
static const char* config_vr_tracking_str = "VR_tracking";


char config_last_player[CALLSIGN_LEN + 1] = "";
char config_last_mission[MISSION_NAME_LEN + 1] = "";

int Config_digi_type = 0;
int Config_midi_type = 0;

int Config_vr_type = 0;
int Config_vr_tracking = 0;

extern int8_t	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);

int ReadConfigFile()
{
	FILE* infile;
	char line[80], * token, * value, * ptr;
	uint8_t gamma;
	int joy_axis_min[4];
	int joy_axis_center[4];
	int joy_axis_max[4];
	int i;

	strcpy(config_last_player, "");

	joy_axis_min[0] = joy_axis_min[1] = joy_axis_min[2] = joy_axis_min[3] = 0;
	joy_axis_max[0] = joy_axis_max[1] = joy_axis_max[2] = joy_axis_max[3] = 0;
	joy_axis_center[0] = joy_axis_center[1] = joy_axis_center[2] = joy_axis_center[3] = 0;
	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);

	digi_driver_board = 0;
	digi_driver_port = 0;
	digi_driver_irq = 0;
	digi_driver_dma = 0;

	digi_midi_type = 0;
	digi_midi_port = 0;

	Config_digi_volume = 4;
	Config_midi_volume = 4;
	Config_control_type = 0;
	Config_channels_reversed = 0;

	infile = fopen("descent.cfg", "rt");
	if (infile == NULL) 
	{
		return 1;
	}
	while (!feof(infile))
	{
		memset(line, 0, 80);
		fgets(line, 80, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') 
		{
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!strcmp(token, digi_dev_str))
				digi_driver_board = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_port_str))
				digi_driver_port = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_irq_str))
				digi_driver_irq = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma_str))
				digi_driver_dma = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_volume_str))
				Config_digi_volume = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, midi_dev_str))
				digi_midi_type = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_port_str))
				digi_midi_port = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_volume_str))
				Config_midi_volume = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, stereo_rev_str))
				Config_channels_reversed = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, gamma_level_str)) 
			{
				gamma = (uint8_t)strtol(value, NULL, 10);
				gr_palette_set_gamma(gamma);
			}
			else if (!strcmp(token, detail_level_str)) 
			{
				Detail_level = strtol(value, NULL, 10);
				if (Detail_level == NUM_DETAIL_LEVELS - 1)
				{
					int count, dummy, oc, od, wd, wrd, da, sc;

					count = sscanf(value, "%d,%d,%d,%d,%d,%d,%d\n", &dummy, &oc, &od, &wd, &wrd, &da, &sc);

					if (count == 7) 
					{
						Object_complexity = oc;
						Object_detail = od;
						Wall_detail = wd;
						Wall_render_depth = wrd;
						Debris_amount = da;
						SoundChannels = sc;
						set_custom_detail_vars();
					}
				}
			}
			/*else if (!strcmp(token, joystick_min_str)) 
			{ //[ISB] cut calibration stuff since it's not usable for chocolate. 
				sscanf(value, "%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3]);
			}
			else if (!strcmp(token, joystick_max_str)) 
			{
				sscanf(value, "%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3]);
			}
			else if (!strcmp(token, joystick_cen_str)) 
			{
				sscanf(value, "%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3]);
			}*/
			else if (!strcmp(token, last_player_str)) 
			{
				char* p;
				memset(config_last_player, '\0', CALLSIGN_LEN + 1 * sizeof(char));
				strncpy(config_last_player, value, CALLSIGN_LEN);
				p = strchr(config_last_player, '\n');
				if (p)* p = 0;
			}
			else if (!strcmp(token, last_mission_str)) 
			{
				char* p;
				memset(config_last_mission, '\0', MISSION_NAME_LEN + 1 * sizeof(char));
				strncpy(config_last_mission, value, MISSION_NAME_LEN);
				p = strchr(config_last_mission, '\n');
				if (p)* p = 0;
			}
			else if (!strcmp(token, config_vr_type_str))
			{
				Config_vr_type = strtol(value, NULL, 10);
			}
			else if (!strcmp(token, config_vr_tracking_str))
			{
				Config_vr_tracking = strtol(value, NULL, 10);
			}
		}
	}

	fclose(infile);

	i = FindArg("-volume");

	if (i > 0) {
		i = atoi(Args[i + 1]);
		if (i < 0) i = 0;
		if (i > 100) i = 100;
		Config_digi_volume = (i * 8) / 100;
		Config_midi_volume = (i * 8) / 100;
	}

	if (Config_digi_volume > 8) Config_digi_volume = 8;

	if (Config_midi_volume > 8) Config_midi_volume = 8;

	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
	digi_set_volume((Config_digi_volume * 32768) / 8, (Config_midi_volume * 128) / 8);
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

	return 0;
}

int WriteConfigFile()
{
	FILE* infile;
	char str[256];
	int joy_axis_min[4];
	int joy_axis_center[4];
	int joy_axis_max[4];
	uint8_t gamma = gr_palette_get_gamma();

	joy_get_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);

	infile = fopen("descent.cfg", "wt");
	if (infile == NULL) 
	{
		return 1;
	}
	sprintf(str, "%s=0x%x\n", digi_dev_str, Config_digi_type);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", digi_port_str, digi_driver_port);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_irq_str, digi_driver_irq);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_dma_str, digi_driver_dma);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_volume_str, Config_digi_volume);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", midi_dev_str, Config_midi_type);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", midi_port_str, digi_midi_port);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", midi_volume_str, Config_midi_volume);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", stereo_rev_str, Config_channels_reversed);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", gamma_level_str, gamma);
	fputs(str, infile);
	if (Detail_level == NUM_DETAIL_LEVELS - 1)
		sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", detail_level_str, Detail_level,
			Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels);
	else
		sprintf(str, "%s=%d\n", detail_level_str, Detail_level);
	fputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d\n", joystick_min_str, joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3]);
	fputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d\n", joystick_cen_str, joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3]);
	fputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d\n", joystick_max_str, joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3]);
	fputs(str, infile);
	sprintf(str, "%s=%s\n", last_player_str, Players[Player_num].callsign);
	fputs(str, infile);
	sprintf(str, "%s=%s\n", last_mission_str, config_last_mission);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", config_vr_type_str, Config_vr_type);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", config_vr_tracking_str, Config_vr_tracking);
	fputs(str, infile);
	fclose(infile);
	return 0;
}
