#include <stdio.h>

#include "sdl.h"
#include "inferno.h"
#include "fix.h"
#include "vecmat.h"

#include "i_gr.h"

int main(int argc, char** argv)
{
	int ret;
	long long heh;
	heh = 0;
	fixmulaccum(&heh, 4875642, 45864);
	fixmulaccum(&heh, 12, 2);
	vms_vector vec; vec.x = 65536; vec.y = 65536; vec.z = 65536;
	int mag = 0;
	//printf("Prepare for Hell. Oh yeah btw %d %d %lld %d\n", quad_sqrt(4717438475534370619), fixdivquadlong(19480112136, 34), heh, mag);
	//quad_sqrt(-1994724344, 5);
	ret = D_DescentMain(argc, argv);
	return ret;
	//return 0;
}