#include <stdio.h>

#include "sdl.h"
#include "inferno.h"
#include "fix.h"
#include "vecmat.h"

#include "i_gr.h"

int main(int argc, char** argv)
{
	int ret;
	ret = D_DescentMain(argc, argv);
	return ret;
	//return 0;
}