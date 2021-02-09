/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

//Note: SDL is included in this file as windows-specific things like SEH are still applicable to the SDL backend. 
#ifdef USE_SDL

#include <stdio.h>

#ifndef BUILD_DESCENT2
#include "main_d1/inferno.h"
#else
#include "main_d2/inferno.h"
#endif
#include "platform/platform.h"

#include "SDL.h" 

int main(int argc, char** argv) //[ISB] oops, must be called with c linkage...
{
	return D_DescentMain(argc, (const char**)argv);
}

#else

#include <Windows.h>

int D_DescentMain(int argc, const char** argv);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	return D_DescentMain(0, nullptr);
}

#endif
