/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <stdio.h>

#ifndef BUILD_DESCENT2
#include "main_d1/inferno.h"
#else
#include "main_d2/inferno.h"
#endif
#include "platform/platform.h"

#include "SDL.h" //[ISB] required for main replacement macro

int main(int argc, char** argv) //[ISB] oops, must be called with c linkage...
{
	return D_DescentMain(argc, (const char**)argv);
}
