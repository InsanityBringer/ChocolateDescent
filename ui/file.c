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
 * $Source: f:/miner/source/ui/rcs/file.c $
 * $Revision: 1.6 $
 * $Author: john $
 * $Date: 1994/06/09 12:18:29 $
 *
 * File dialog box stuff.
 *
 * $Log: file.c $
 * Revision 1.6  1994/06/09  12:18:29  john
 * Took out keyboard flushes.
 *
 * Revision 1.5  1994/04/27  18:30:49  john
 * Fixed bug with enter a directory without a slash on
 * the end not working.
 *  .
 *
 * Revision 1.4  1994/04/22  11:09:47  john
 * Speed up directory loading by only searching for *. instead of *.*
 *
 * Revision 1.3  1993/12/07  12:30:18  john
 * new version.
 *
 * Revision 1.2  1993/10/26  13:46:22  john
 * *** empty log message ***
 *
 * Revision 1.1  1993/10/06  11:10:23  john
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
//#include <dos.h>
#include <ctype.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "key.h"

#include "ui.h"
#include "mono.h"
#include "timer.h"

#include "mem.h"

#include "error.h"

//#define TICKER (*(volatile int *)0x46C)

char filename_list[300][13];
char directory_list[100][13];

static char* Message[] = {
	"Disk is write protected",
	"Unknown unit",
	"Drive not ready",
	"Unknown command",
	"CRC error in data",
	"Bad drive-request stuct length",
	"Seek error",
	"Unknown media type",
	"Sector not found",
	"Printer out of paper",
	"Write fault",
	"Read fault",
	"General Failure" };

static int error_mode = 0;

/*int __far critical_error_handler(unsigned deverr, unsigned errcode, unsigned far* devhdr)
{
	int x;

	devhdr = devhdr; deverr = deverr;

	if (error_mode == 1) return _HARDERR_FAIL;

	x = MessageBox(-2, -2, 2, Message[errcode], "Retry", "Fail");

	switch (x)
	{
	case 1: return _HARDERR_RETRY;
	case 2: return _HARDERR_FAIL;
	default: return _HARDERR_FAIL;
	}
}

void InstallErrorHandler()
{
	_harderr((void*)critical_error_handler);
	//Above line modified by KRB, added (void *) cast
	//for the compiler.
}*/

void file_sort(int n, char list[][13])
{
	int i, j, incr;
	char t[14];

	incr = n / 2;
	while (incr > 0)
	{
		for (i = incr; i < n; i++)
		{
			j = i - incr;
			while (j >= 0)
			{
				if (strncmp(list[j], list[j + incr], 12) > 0)
				{
					memcpy(t, list[j], 13);
					memcpy(list[j], list[j + incr], 13);
					memcpy(list[j + incr], t, 13);
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}


int SingleDrive()
{
	Warning("SingleDrive: STUB\n");
	return 0;
}

void SetFloppy(int d)
{
	Warning("SetFloppy: STUB\n");
}


void file_capitalize(char* s)
{
	while (*s++ = toupper(*s));
}


// Changes to a drive if valid.. 1=A, 2=B, etc
// If flag, then changes to it.
// Returns 0 if not-valid, 1 if valid.
int file_chdrive(int DriveNum, int flag)
{
	Warning("file_chdrive: STUB\n");
	return 0;
}


// Changes to directory in dir.  Even drive is changed.
// Returns 1 if failed.
//  0 = Changed ok.
//  1 = Invalid disk drive.
//  2 = Invalid directory.

int file_chdir(char* dir)
{
	Warning("file_chdir: STUB\n");
	return 0;
}


int file_getdirlist(int MaxNum, char list[][13])
{
	Warning("file_getdirlist: STUB\n");
	return 0;
}

int file_getfilelist(int MaxNum, char list[][13], char* filespec)
{
	Warning("file_getfilelist: STUB\n");
	return 0;
}

static int FirstTime = 1;
static char CurDir[128];

int ui_get_filename(char* filename, char* Filespec, char* message)
{
	FILE* TempFile;
	int NumFiles, NumDirs, i;
	char InputText[100];
	char Spaces[35];
	char ErrorMessage[100];
	UI_WINDOW* wnd;
	UI_GADGET_BUTTON* Button1, * Button2, * HelpButton;
	UI_GADGET_LISTBOX* ListBox1;
	UI_GADGET_LISTBOX* ListBox2;
	UI_GADGET_INPUTBOX* UserFile;
	int new_listboxes;

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char fulldir[_MAX_DIR + _MAX_DRIVE];
	char fullfname[_MAX_FNAME + _MAX_EXT];


	char OrgDir[128];

	_getcwd(OrgDir, 128);

	if (FirstTime)
		_getcwd(CurDir, 128);
	FirstTime = 0;

	file_chdir(CurDir);

	//MessageBox( -2,-2, 1,"DEBUG:0", "Ok" );
	for (i = 0; i < 35; i++)
		Spaces[i] = ' ';
	Spaces[34] = 0;

	NumFiles = file_getfilelist(300, filename_list, Filespec);

	NumDirs = file_getdirlist(100, directory_list);

	wnd = ui_open_window(200, 100, 400, 370, WIN_DIALOG);

	ui_wprintf_at(wnd, 10, 5, message);

	_splitpath_s(filename, drive, 3, dir, 256, fname, 256, ext, 256);

	sprintf_s(InputText, 100, "%s%s", fname, ext);

	ui_wprintf_at(wnd, 20, 32, "N&ame");
	UserFile = ui_add_gadget_inputbox(wnd, 60, 30, 40, 40, InputText);

	ui_wprintf_at(wnd, 20, 86, "&Files");
	ui_wprintf_at(wnd, 210, 86, "&Dirs");

	ListBox1 = ui_add_gadget_listbox(wnd, 20, 110, 125, 200, NumFiles, filename_list, 13);
	ListBox2 = ui_add_gadget_listbox(wnd, 210, 110, 100, 200, NumDirs, directory_list, 13);

	Button1 = ui_add_gadget_button(wnd, 20, 330, 60, 25, "Ok", NULL);
	Button2 = ui_add_gadget_button(wnd, 100, 330, 60, 25, "Cancel", NULL);
	HelpButton = ui_add_gadget_button(wnd, 180, 330, 60, 25, "Help", NULL);

	wnd->keyboard_focus_gadget = (UI_GADGET*)UserFile;

	Button1->hotkey = KEY_CTRLED + KEY_ENTER;
	Button2->hotkey = KEY_ESC;
	HelpButton->hotkey = KEY_F1;
	ListBox1->hotkey = KEY_ALTED + KEY_F;
	ListBox2->hotkey = KEY_ALTED + KEY_D;
	UserFile->hotkey = KEY_ALTED + KEY_A;

	ui_gadget_calc_keys(wnd);

	ui_wprintf_at(wnd, 20, 60, "%s", Spaces);
	ui_wprintf_at(wnd, 20, 60, "%s", CurDir);

	new_listboxes = 0;

	while (1)
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		if (Button2->pressed)
		{
			file_chdir(OrgDir);
			ui_close_window(wnd);
			return 0;
		}

		if (HelpButton->pressed)
			MessageBox(-1, -1, 1, "Sorry, no help is available!", "Ok");

		if (ListBox1->moved || new_listboxes)
		{
			if (ListBox1->current_item >= 0)
			{
				strcpy_s(UserFile->text, UserFile->textLen, filename_list[ListBox1->current_item]);
				UserFile->position = (short)strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status = 1;
				UserFile->first_time = 1;
			}
		}

		if (ListBox2->moved || new_listboxes)
		{
			if (ListBox2->current_item >= 0)
			{
				if (strrchr(directory_list[ListBox2->current_item], ':'))
					sprintf_s(UserFile->text, UserFile->textLen, "%s%s", directory_list[ListBox2->current_item], Filespec);
				else
					sprintf_s(UserFile->text, UserFile->textLen, "%s\\%s", directory_list[ListBox2->current_item], Filespec);
				UserFile->position = (short)strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status = 1;
				UserFile->first_time = 1;
			}
		}
		new_listboxes = 0;

		if (Button1->pressed || UserFile->pressed || (ListBox1->selected_item > -1) || (ListBox2->selected_item > -1))
		{
			ui_mouse_hide();

			if (ListBox2->selected_item > -1) {
				if (strrchr(directory_list[ListBox2->selected_item], ':'))
					sprintf_s(UserFile->text, UserFile->textLen, "%s%s", directory_list[ListBox2->selected_item], Filespec);
				else
					sprintf_s(UserFile->text, UserFile->textLen, "%s\\%s", directory_list[ListBox2->selected_item], Filespec);
			}

			error_mode = 1; // Critical error handler automatically fails.

			//TempFile = fopen(UserFile->text, "r");
			errno_t err = fopen_s(&TempFile, UserFile->text, "r");
			if (TempFile)
			{
				// Looks like a valid filename that already exists!
				fclose(TempFile);
				break;
			}

			// File doesn't exist, but can we create it?
			//TempFile = fopen(UserFile->text, "w");
			err = fopen_s(&TempFile, UserFile->text, "w");
			if (TempFile)
			{
				// Looks like a valid filename!
				fclose(TempFile);
				remove(UserFile->text);
				break;
			}

			_splitpath_s(UserFile->text, drive, 3, dir, 256, fname, 256, ext, 256);
			sprintf_s(fullfname, 512, "%s%s", fname, ext);

			//mprintf( 0, "----------------------------\n" );
			//mprintf( 0, "Full text: '%s'\n", UserFile->text );
			//mprintf( 0, "Drive: '%s'\n", drive );
			//mprintf( 0, "Dir: '%s'\n", dir );
			//mprintf( 0, "Filename: '%s'\n", fname );
			//mprintf( 0, "Extension: '%s'\n", ext );
			//mprintf( 0, "Full dir: '%s'\n", fulldir );
			//mprintf( 0, "Full fname: '%s'\n", fname );

			if (strrchr(fullfname, '?') || strrchr(fullfname, '*'))
			{
				sprintf_s(fulldir, 259, "%s%s.", drive, dir);
			}
			else 
			{
				sprintf_s(fullfname, 512, "%s", Filespec);
				sprintf_s(fulldir, 259, "%s", UserFile->text);
			}

			//mprintf( 0, "----------------------------\n" );
			//mprintf( 0, "Full dir: '%s'\n", fulldir );
			//mprintf( 0, "Full fname: '%s'\n", fullfname );

			if (file_chdir(fulldir) == 0)
			{
				NumFiles = file_getfilelist(300, filename_list, fullfname);

				strcpy_s(UserFile->text, UserFile->textLen, fullfname);
				UserFile->position = (short)strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status = 1;
				UserFile->first_time = 1;

				NumDirs = file_getdirlist(100, directory_list);

				ui_listbox_change(wnd, ListBox1, NumFiles, filename_list, 13);
				ui_listbox_change(wnd, ListBox2, NumDirs, directory_list, 13);
				new_listboxes = 0;

				_getcwd(CurDir, 35);
				ui_wprintf_at(wnd, 20, 60, "%s", Spaces);
				ui_wprintf_at(wnd, 20, 60, "%s", CurDir);

				//i = TICKER;
				//while (TICKER < i + 2);
				i = I_GetTicks();
				while (I_GetTicks() < i + 2);
				//[ISB] this code is going to be hell to get ported..
			}
			else 
			{
				sprintf_s(ErrorMessage, 100, "Error changing to directory '%s'", fulldir);
				MessageBox(-2, -2, 1, ErrorMessage, "Ok");
				UserFile->first_time = 1;
			}

			error_mode = 0;

			ui_mouse_show();

		}
	}

	//key_flush();

	_splitpath_s(UserFile->text, drive, 3, dir, 256, fname, 256, ext, 256);
	sprintf_s(fulldir, 259, "%s%s.", drive, dir);
	sprintf_s(fullfname, 512, "%s%s", fname, ext);

	if (strlen(fulldir) > 1)
		file_chdir(fulldir);

	_getcwd(CurDir, 35);

	if (strlen(CurDir) > 0)
	{
		if (CurDir[strlen(CurDir) - 1] == '\\')
			CurDir[strlen(CurDir) - 1] = 0;
	}

	sprintf_s(filename, strlen(filename), "%s\\%s", CurDir, fullfname);
	//MessageBox( -2, -2, 1, filename, "Ok" );

	file_chdir(OrgDir);

	ui_close_window(wnd);

	return 1;
}



int ui_get_file(char* filename, int filenamelen, char* Filespec)
{
	int x, i, NumFiles;
	char* text[200];

	NumFiles = file_getfilelist(200, filename_list, Filespec);

	for (i = 0; i < NumFiles; i++)
	{
		//[ISB] TODO: Long file names?
		MALLOC( text[i], char, 15 );//Another compile hack -KRB undone by [ISB]
		//text[i] = (char*)malloc(15 * sizeof(char));
		strcpy_s(text[i], 15, filename_list[i]);
	}

	x = MenuX(-1, -1, NumFiles, text);

	if (x > 0)
		strcpy_s(filename, strlen(filenamelen), filename_list[x - 1]);

	for (i = 0; i < NumFiles; i++)
	{
		free(text[i]);
	}

	if (x > 0)
		return 1;
	else
		return 0;

}
