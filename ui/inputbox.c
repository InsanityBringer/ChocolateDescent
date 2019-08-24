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
/*
 * $Source: f:/miner/source/ui/rcs/inputbox.c $
 * $Revision: 1.6 $
 * $Author: john $
 * $Date: 1994/11/18 23:07:30 $
 *
 * Routines for doing inputbox gadgets.
 *
 * $Log: inputbox.c $
 * Revision 1.6  1994/11/18  23:07:30  john
 * Changed a bunch of shorts to ints.
 *
 * Revision 1.5  1994/07/22  17:02:17  john
 * fixed bug with inputbox not sucking up its keypresses.
 *
 * Revision 1.4  1993/12/07  12:30:55  john
 * new version.
 *
 * Revision 1.3  1993/10/26  13:45:59  john
 * *** empty log message ***
 *
 * Revision 1.2  1993/10/05  17:30:50  john
 * *** empty log message ***
 *
 * Revision 1.1  1993/09/20  10:35:12  john
 * Initial revision
 *
 *
 */

#include <stdlib.h>
#include <string.h>

#include "mem/mem.h"
#include "fix/fix.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "ui.h"
#include "bios/key.h"

// insert character c into string s at position p.
void strcins(char* s, int p, char c)
{
	int n;
	for (n = strlen(s) - p; n >= 0; n--)
		* (s + p + n + 1) = *(s + p + n);   // Move everything over
	*(s + p) = c;         // then insert the character
}

// delete n character from string s starting at position p

void strndel(char* s, int p, int n)
{
	for (; (*(s + p) = *(s + p + n)) != '\0'; s++)
		* (s + p + n) = '\0';    // Delete and zero fill
}

void ui_draw_inputbox(UI_GADGET_INPUTBOX* inputbox)
{
	int w, h, aw;

	if ((inputbox->status == 1) || (inputbox->position != inputbox->oldposition))
	{
		ui_mouse_hide();
		gr_set_current_canvas(inputbox->canvas);

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET*)inputbox)
		{
			if (inputbox->first_time)
				gr_set_fontcolor(CBLACK, CRED);
			else
				gr_set_fontcolor(CRED, CBLACK);
		}
		else
			gr_set_fontcolor(CWHITE, CBLACK);

		inputbox->status = 0;

		gr_string(2, 2, inputbox->text);
		gr_get_string_size(inputbox->text, &w, &h, &aw);

		gr_setcolor(CBLACK);
		gr_rect(2 + w, 0, inputbox->width - 1, inputbox->height - 1);

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET*)inputbox && !inputbox->first_time)
		{
			gr_setcolor(CRED);
			Vline(2, inputbox->height - 3, 2 + w + 1);
			Vline(2, inputbox->height - 3, 2 + w + 2);
		}

		ui_mouse_show();
	}
}

UI_GADGET_INPUTBOX* ui_add_gadget_inputbox(UI_WINDOW* wnd, short x, short y, short length, short slength, char* text)
{
	int h, w, aw, f;
	UI_GADGET_INPUTBOX* inputbox;

	gr_get_string_size(NULL, &w, &h, &aw);

	inputbox = (UI_GADGET_INPUTBOX*)ui_gadget_add(wnd, 6, x, y, x + aw * slength - 1, y + h - 1 + 4);

	f = 0;

	inputbox->text = malloc(length + 1);
	strncpy_s(inputbox->text, inputbox->textLen, text, length);
	inputbox->position = (short)strlen(inputbox->text); 
	inputbox->oldposition = inputbox->position;
	inputbox->width = aw * slength;
	inputbox->height = h + 4;
	inputbox->length = length;
	inputbox->slength = slength;
	inputbox->pressed = 0;
	inputbox->first_time = 1;

	gr_set_current_canvas(inputbox->canvas);
	gr_setcolor(CBLACK);
	gr_rect(0, 0, inputbox->width - 1, inputbox->height - 1);

	return inputbox;

}


void ui_inputbox_do(UI_GADGET_INPUTBOX* inputbox, int keypress)
{
	unsigned char ascii;
	inputbox->oldposition = inputbox->position;

	inputbox->pressed = 0;

	if (CurWindow->keyboard_focus_gadget == (UI_GADGET*)inputbox)
	{
		switch (keypress)
		{
		case 0:
			break;
		case (KEY_LEFT):
		case (KEY_BACKSP):
			if (inputbox->position > 0)
				inputbox->position--;
			inputbox->text[inputbox->position] = 0;
			inputbox->status = 1;
			if (inputbox->first_time) inputbox->first_time = 0;
			break;
		case (KEY_ENTER):
			inputbox->pressed = 1;
			inputbox->status = 1;
			if (inputbox->first_time) inputbox->first_time = 0;
			break;
		default:
			ascii = key_to_ascii(keypress);
			if ((ascii < 255) && (inputbox->position < inputbox->length - 2))
			{
				if (inputbox->first_time) {
					inputbox->first_time = 0;
					inputbox->position = 0;
				}
				inputbox->text[inputbox->position++] = ascii;
				inputbox->text[inputbox->position] = 0;
			}
			inputbox->status = 1;
			break;
		}
	}
	else {
		inputbox->first_time = 1;
	}

	last_keypress = 0;

	ui_draw_inputbox(inputbox);

}
