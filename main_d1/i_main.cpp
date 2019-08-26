
#ifdef USE_SDL

#include <stdio.h>

#include "inferno.h"
#include "2d/i_gr.h"

#include "SDL.h" //[ISB] required for main replacement macro

int main(int argc, const char** argv)
{
	return D_DescentMain(argc, argv);
}

#else

#include <Windows.h>

int D_DescentMain(int argc, const char** argv);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	return D_DescentMain(0, nullptr);
}

#endif
