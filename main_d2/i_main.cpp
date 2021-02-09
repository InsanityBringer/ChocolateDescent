
#ifdef USE_SDL

#include <stdio.h>

#include "inferno.h"
#include "platform/platform.h"

#include "SDL.h" //[ISB] required for main replacement macro

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
