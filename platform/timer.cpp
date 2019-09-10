/*
	Hi this code isn't the sole property of PARALLAX SOFTWARE CORPORATION
	and is instead released under the MIT license.
*/

#include <chrono>
#include <thread>
#include "platform/timer.h"
#include "misc/error.h"
#include "fix/fix.h"

static uint64_t baseTick;

static uint64_t GetClockTimeMS()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void timer_init()
{
	baseTick = GetClockTimeMS();
}

fix timer_get_fixed_seconds()
{
	//time = (SDL_GetTicks() << 16) / 1000;
	//[ISB] TODO HACK this is disgusting, but shifting ticks up 16 is a great way to cause an overflow
	//[ISB] tbf I could cast it to a 64-bit type before shifting IG... heh
	double hack;
	hack = ((double)GetClockTimeMS() - baseTick) / 1000.;
	return fl2f(hack);
}

fix timer_get_approx_seconds()
{
	return timer_get_fixed_seconds();
}

uint32_t I_GetTicks()
{
	return (int)((GetClockTimeMS() - baseTick) * 18 / 1000);
}

void I_Delay(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
