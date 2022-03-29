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
#include <stdarg.h>
#include <string.h>
#include "fix/fix.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "ui.h"
#include "platform/key.h"

const char* KeyDesc[256] = { \
"","{Esc}","{1}","{2}","{3}","{4}","{5}","{6}","{7}","{8}","{9}","{0}","{-}",           \
"{=}","{Backspace}","{Tab}","{Q}","{W}","{E}","{R}","{T}","{Y}","{U}","{I}","{O}",      \
"{P}","{[}","{]}","{Enter}","{LeftCtrl}","{A}","{S}","{D}","{F}",        \
"{G}","{H}","{J}","{K}","{L}","{;}","{'}","{`}",        \
"{RightShift}","{\\}","{Z}","{X}","{C}","{V}","{B}","{N}","{M}","{,}",      \
"{.}","{/}","{LeftShift}","{Pad*}","{LeftAlt}","{Spacebar}",      \
"{CapsLock}","{F1}","{F2}","{F3}","{F4}","{F5}","{F6}","{F7}","{F8}","{F9}",        \
"{F10}","{NumLock}","{ScrollLock}","{Pad7}","{Pad8}","{Pad9}","{Pad-}",   \
"{Pad4}","{Pad5}","{Pad6}","{Pad+}","{Pad1}","{Pad2}","{Pad3}","{Pad0}", \
"{Pad.}","","","","{F11}","{F12}","","","","","","","","","",         \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","",           \
"{PadEnter}","{RightCtrl}","","","","","","","","","","","","","", \
"","","","","","","","","","","{Pad/}","","","{RightAlt}","",      \
"","","","","","","","","","","","","","{Home}","{Up}","{PageUp}",     \
"","{Left}","","{Right}","","{End}","{Down}","{PageDown}","{Insert}",       \
"{Delete}","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","" };

void GetKeyDescription(char* text, size_t len, int keypress)
{
	char Ctrl[10];
	char Alt[10];
	char Shift[10];

	if (keypress & KEY_CTRLED)
		strncpy(Ctrl, "{Ctrl}", 9);
	else
		strncpy(Ctrl, "", 9);

	if (keypress & KEY_ALTED)
		strncpy(Alt, "{Alt}", 9);
	else
		strncpy(Alt, "", 9);

	if (keypress & KEY_SHIFTED)
		strncpy(Shift, "{Shift}", 9);
	else
		strncpy(Shift, "", 9);

	Ctrl[9] = Alt[9] = Shift[9] = '\0';

	snprintf(text, len, "%s%s%s%s", Ctrl, Alt, Shift, KeyDesc[keypress & 255]);
}


int DecodeKeyText(const char* text)
{
	int i, code = 0;

	if (strstr(text, "{Ctrl}"))
		code |= KEY_CTRLED;
	if (strstr(text, "{Alt}"))
		code |= KEY_ALTED;
	if (strstr(text, "{Shift}"))
		code |= KEY_SHIFTED;

	for (i = 0; i < 256; i++)
	{
		if (strlen(KeyDesc[i]) > 0 && strstr(text, KeyDesc[i]))
		{
			code |= i;
			return code;
		}
	}
	return -1;
}

int GetKeyCode(char* text)
{
	UI_WINDOW* wnd;
	UI_GADGET_BUTTON* DoneButton;
	char temp_text[100];

	text = text;

	wnd = ui_open_window(200, 200, 400, 200, WIN_DIALOG);

	DoneButton = ui_add_gadget_button(wnd, 170, 165, 60, 25, "Ok", NULL);

	ui_gadget_calc_keys(wnd);

	//key_flush();

	wnd->keyboard_focus_gadget = (UI_GADGET*)DoneButton;

	while (1)
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		if (last_keypress > 0)
		{
			GetKeyDescription(temp_text, 100, last_keypress);
			ui_wprintf_at(wnd, 10, 100, "%s     ", temp_text);
		}

		if (DoneButton->pressed)
			break;
	}

	ui_close_window(wnd);

	return 0;
}
