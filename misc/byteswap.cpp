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

#include "misc/types.h"

//[ISB]: Frankly, swaps totally suck. This is a stopgap measure to enable
//some mac code to compile. In the insane off-chance someone actually
//attempts to build this on a big-endian machine, it would be more worthwhile
//to have functions in cfile that read a little endian number with 2/4 byte reads and shifts.

uint16_t swapshort(uint16_t s)
{
	//return ((s >> 8) & 0x00ff) | ((s << 8) & 0xff00);
	return s; 
}

uint32_t swapint(uint32_t i)
{
	/*uint16_t s1, s2;

	s1 = (i >> 16) & 0x0000ffff;
	s2 = i & 0x0000ffff;
	return ((swapshort(s2) << 16) | swapshort(s1));*/
	return i;
}
