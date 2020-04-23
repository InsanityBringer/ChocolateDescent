/*
	Hi this code isn't the sole property of PARALLAX SOFTWARE CORPORATION
	and is instead released under the MIT license.
*/

#include <chrono>
#include <thread>
#include "platform/timer.h"
#include "misc/error.h"
#include "fix/fix.h"

#ifdef WIN32
#include <Windows.h>
#include <timeapi.h>

#pragma comment(lib, "Winmm.lib")
#endif

static uint64_t baseTick;

static uint64_t markTick = 0;

static uint64_t GetClockTimeMS()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void timer_init()
{
	baseTick = GetClockTimeMS();
#ifdef WIN32
	//[ISB] Thie code is provided by dpJudas, and ensures the clock accuracy is increased on Windows. 
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(tc)) == TIMERR_NOERROR)
		timeBeginPeriod(tc.wPeriodMin);
	else
		timeBeginPeriod(1);
#endif
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

uint32_t I_GetMS()
{
	return GetClockTimeMS() - baseTick;
}

uint64_t I_GetUS()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

void I_Delay(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void I_DelayUS(uint64_t us)
{
	std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void I_MarkStart()
{
	markTick = I_GetUS();
}

void I_MarkEnd(uint64_t numUS)
{
	uint64_t diff = (markTick + numUS) - I_GetUS();
	if (diff > 2000) //[ISB] Sleep only if there's sufficient time to do so, since the scheduler isn't precise enough
		I_DelayUS(diff - 2000);
	while (I_GetUS() < markTick + numUS);
}
