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

#include <stdlib.h>

#include "2d/gr.h"
#include "gamefont.h"

const char* Gamefont_filenames[] = { "font1-1.fnt",			// Font 0
											"font2-1.fnt",			// Font 1
											"font2-2.fnt",			// Font 2
											"font2-3.fnt",			// Font 3
											"font3-1.fnt",			// Font 4
};

grs_font* Gamefonts[MAX_FONTS];

int Gamefont_installed = 0;

void gamefont_init()
{
	int i;

	if (Gamefont_installed) return;
	Gamefont_installed = 1;

	for (i = 0; i < MAX_FONTS; i++)
		Gamefonts[i] = gr_init_font(Gamefont_filenames[i]);

	atexit(gamefont_close);
}

void gamefont_close(void)
{
	int i;

	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (i = 0; i < MAX_FONTS; i++)
	{
		gr_close_font(Gamefonts[i]);
		Gamefonts[i] = NULL;
	}
}

