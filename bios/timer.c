/*
	Hi this code isn't the sole property of PARALLAX SOFTWARE CORPORATION
	and is instead released under the MIT license.
*/
/*
*	Code for SDL timer functions
*
*/

#include "timer.h"
#include "error.h"
#include "fix.h"

#include "SDL_timer.h"

int baseTick;

void timer_init()
{
	baseTick = SDL_GetTicks();
}

void timer_close()
{

}

void timer_set_rate(int count_val)
{
	Warning("timer_set_rate: STUB\n");
}

fix timer_get_fixed_seconds()
{
	fix time;
	//time = (SDL_GetTicks() << 16) / 1000;
	//[ISB] TODO HACK this is disgusting, but shifting ticks up 16 is a great way to cause an overflow
	//[ISB] tbf I could cast it to a 64-bit type before shifting IG... heh
	double hack;
	hack = ((double)SDL_GetTicks() - baseTick) / 1000.;
	time = fl2f(hack);
	return time;
}

fix timer_get_fixed_secondsX()
{
	//Not interrupt based, so just return normal timer
	return timer_get_fixed_seconds();
}

fix timer_get_approx_seconds()
{
	//I coooould change it to be accurate to 1/120th intervals.... but nah
	return timer_get_fixed_seconds();
}

int I_GetTicks()
{
	return ((SDL_GetTicks()-baseTick) * 18 / 1000);
}

void I_Delay(int ms)
{
	SDL_Delay(ms);
}