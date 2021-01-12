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
#include <string.h>

#include "platform/mono.h"
#include "platform/key.h"
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
//#include "victor.h"
#include "render.h"
#include "2d/palette.h"
#include "newmenu.h"
#include "misc/args.h" //[ISB] STOP PUTTING ARA ARA IN THE SOURCE AND FIX THE PROBLEM
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

int joydefs_calibrate_flag = 0;

#ifdef MACINTOSH
ubyte joydefs_calibrating = 0;		// stupid hack all because of silly mouse cursor emulation
#endif

void joy_delay()
{
	stop_time();
	//	timer_delay(.25);
	I_Delay(250);				// changed by allender because	1) more portable //[ISB] k
							//								2) was totally broken on PC
	joy_flush();
	start_time();
}


int joycal_message(char* title, char* text)
{
	int i;
	newmenu_item	m[2];
	MAC(joydefs_calibrating = 1;)
		m[0].type = NM_TYPE_TEXT; m[0].text = text;
	m[1].type = NM_TYPE_MENU; m[1].text = TXT_OK;
	i = newmenu_do(title, NULL, 2, m, NULL);
	MAC(joydefs_calibrating = 0;)
		if (i < 0)
			return 1;
	return 0;
}

extern int WriteConfigFile();

void joydefs_calibrate(void)
{
	nm_messagebox(NULL, 1, TXT_OK, "Joystick calibration not\npossible in chocolate.");
}


//char *control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick", "Flightstick Pro", "Thrustmaster FCS", "Gravis Gamepad", "Mouse", "Cyberman" };

const char* control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick (1-2)", "Gamepad", "Joystick w/ throttle", "-", "Mouse", "-", "-" };
int choco_menu_remap[CONTROL_MAX_TYPES] = { 0, 1, 2, 3, 5, 0, 0, 0 }; //Remaps the new options to the old input ID
int choco_id_to_menu_remap[CONTROL_MAX_TYPES] = { 0, 1, 2, 3, 0, 4, 0, 0 }; //Remaps an old ID to the new menu option


#define CONTROL_MAX_TYPES_DOS	(CONTROL_MAX_TYPES-1)	//last item is windows only

void joydef_menuset_1(int nitems, newmenu_item* items, int* last_key, int citem)
{
	int i;
	int oc_type = Config_control_type;

	nitems = nitems;
	last_key = last_key;
	citem = citem;

	for (i = 0; i < 5; i++)
		if (items[i].value) Config_control_type = choco_menu_remap[i];

	/*if ((oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS))
	{
		nm_messagebox(TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS);
	}*/

	if (oc_type != Config_control_type)
	{
		/*switch (Config_control_type)
		{
			//		case	CONTROL_NONE:
		case	CONTROL_JOYSTICK:
		case	CONTROL_FLIGHTSTICK_PRO:
		case	CONTROL_THRUSTMASTER_FCS:
			//		case	CONTROL_GRAVIS_GAMEPAD:
			//		case	CONTROL_MOUSE:
			//		case	CONTROL_CYBERMAN:
			joydefs_calibrate_flag = 1;
		}*/
		kc_set_controls();
	}

}

extern uint8_t kc_use_external_control;
extern uint8_t kc_enable_external_control;
extern uint8_t* kc_external_name;

//NOTE: DOS VERSION
void joydefs_config()
{
	int i, old_masks, masks, nitems;
	newmenu_item m[14];
	int i1 = 11;
	char xtext[128];

	do 
	{
		nitems = 10;
		//[ISB] TODO: These const_casts are disgusting, and are merely a way to work around compiler warnings in gcc. 
		//It works for now since the code down the line is adjusted to expect const strings, but this should be improved.
		m[0].type = NM_TYPE_RADIO; m[0].text = const_cast<char*>(control_text[0]); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = const_cast<char*>(control_text[1]); m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO; m[2].text = const_cast<char*>(control_text[2]); m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO; m[3].text = const_cast<char*>(control_text[3]); m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_RADIO; m[4].text = const_cast<char*>(control_text[5]); m[4].value = 0; m[4].group = 0;

		m[5].type = NM_TYPE_MENU;		m[5].text = TXT_CUST_ABOVE;
		m[6].type = NM_TYPE_TEXT;		m[6].text = const_cast<char*>("");
		m[7].type = NM_TYPE_SLIDER;		m[7].text = TXT_JOYS_SENSITIVITY; m[7].value = Config_joystick_sensitivity; m[7].min_value = 0; m[7].max_value = 8;
		m[8].type = NM_TYPE_TEXT;		m[8].text = const_cast<char*>("");
		m[9].type = NM_TYPE_MENU;		m[9].text = TXT_CUST_KEYBOARD;

		m[choco_id_to_menu_remap[Config_control_type]].value = 1;

		i1 = newmenu_do1(NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1);
		Config_joystick_sensitivity = m[7].value;

		switch (i1) 
		{
		case 5: 
		{
			/*old_masks = 0;
			for (i = 0; i < 4; i++) 
			{
				if (kconfig_is_axes_used(i))
					old_masks |= (1 << i);
			}*/
			if (Config_control_type == 0)
				kconfig(0, TXT_KEYBOARD);
			else if (Config_control_type < 5)
				kconfig(1, const_cast<char*>(control_text[Config_control_type]));
			else
				kconfig(2, const_cast<char*>(control_text[Config_control_type]));

			/*masks = 0;
			for (i = 0; i < 4; i++)
			{
				if (kconfig_is_axes_used(i))
					masks |= (1 << i);
			}

			switch (Config_control_type) 
			{
				case	CONTROL_JOYSTICK:
				case	CONTROL_FLIGHTSTICK_PRO:
				case	CONTROL_THRUSTMASTER_FCS:
				{
					for (i = 0; i < 4; i++) 
					{
						if ((masks & (1 << i)) && (!(old_masks & (1 << i))))
							joydefs_calibrate_flag = 1;
					}
				}
			break;
			}*/
		}
				break;
		case 9:
			kconfig(0, TXT_KEYBOARD);
			break;
		}

		if (kc_use_external_control) 
		{
			kc_enable_external_control = m[12].value;
		}

	} while (i1 > -1);

	/*switch (Config_control_type) 
	{
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
		if (joydefs_calibrate_flag)
			joydefs_calibrate();
		break;
	}*/
}



