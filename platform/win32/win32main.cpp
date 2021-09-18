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
#include <stdlib.h>
#include <wchar.h>
#include <vector>

int D_DescentMain(int argc, const char** argv);

//There is absolutely, positively a function that does this already but MS's docs are organized in such a way
//I couldn't find it. sorry.
//I also apologize for taking a wide string and shrinking it into an ansi string.
char* ANSIfyString(PWSTR str)
{
	size_t i, len = lstrlenW(str);
	char* newstr = (char*)malloc((len + 1) * sizeof(char));

	for (i = 0; i < len; i++)
	{
		newstr[i] = str[i];
	}
	newstr[len] = '\0';

	return newstr;
}

//Placeholder for the executable name, contents not important.
char placeholder[16] = "placeholder";

void ParseArgs(std::vector<char*> &argv, char* pCmdLine)
{
	int argc;
	int i;

	argc = 0;
	i = 0;

	argv.push_back(placeholder);

	while (pCmdLine[i])
	{
		if (pCmdLine[i] == ' ') i++;
		else 
		{
			argv.push_back(&pCmdLine[i]);

			while (pCmdLine[i] != ' ' && pCmdLine[i] != 0) 
			{
				i++;
			}
			if (pCmdLine[i] != 0)
				pCmdLine[i++] = 0;
		}
	}

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	std::vector<char*> argvhack;
	char* ansihack = ANSIfyString(pCmdLine);
	ParseArgs(argvhack, ansihack);
	return D_DescentMain(argvhack.size(), const_cast<const char**>(argvhack.data()));
}

#endif
