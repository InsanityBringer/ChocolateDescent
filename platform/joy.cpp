#include "misc/types.h"
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"

#define MAX_BUTTONS 20

char joy_present = 1;

struct JoystickState
{
	struct ButtonState
	{
		bool down;
		int upcount;
		int downcount;
		fix downtime;
	} buttons[MAX_BUTTONS];
	int axis_value[4];
	int axis_min[4];
	int axis_center[4];
	int axis_max[4];
	int presentmask;
} joystick;

int joy_init()
{
	for (int i = 0; i < 4; i++)
	{
		joystick.axis_min[i] = -127;
		joystick.axis_center[i] = 0;
		joystick.axis_max[i] = 127;
		joystick.axis_value[i] = 0; //[ISB] if no physical joystick present this should avoid issues
	}
	return 1;
}

void joy_close()
{
}

void joy_set_timer_rate(int max_value)
{
}

void joy_set_cen()
{
}

void joy_flush()
{
	for (int btn = 0; btn < MAX_BUTTONS; btn++)
	{
		joystick.buttons[btn].down = false;
		joystick.buttons[btn].upcount = 0;
		joystick.buttons[btn].downcount = 0;
		joystick.buttons[btn].downtime = 0;
	}
}

void joy_get_cal_vals(int* axis_min, int* axis_center, int* axis_max)
{
	for (int i = 0; i < 4; i++)
	{
		axis_min[i] = joystick.axis_min[i];
		axis_center[i] = joystick.axis_center[i];
		axis_max[i] = joystick.axis_max[i];
	}
}

void joy_set_cal_vals(int* axis_min, int* axis_center, int* axis_max)
{
	for (int i = 0; i < 4; i++)
	{
		//joystick.axis_min[i] = axis_min[i];
		//joystick.axis_center[i] = axis_center[i];
		//joystick.axis_max[i] = axis_max[i];
	}
}

void joy_get_pos(int* x, int* y)
{
	*x = joystick.axis_value[0];
	*y = joystick.axis_value[1];
}

int joy_get_scaled_reading(int raw, int axn)
{
	return raw;
}

uint8_t joystick_read_raw_axis(uint8_t mask, int* axis)
{
	// Note: mask is always set to JOY_ALL_AXIS
	axis[0] = joystick.axis_value[0];
	axis[1] = joystick.axis_value[1];
	axis[2] = joystick.axis_value[2];
	axis[3] = joystick.axis_value[3];
	return joy_get_present_mask();
}

uint8_t joy_get_present_mask()
{
	return joystick.presentmask;
}

int joy_get_btns()
{
	int buttons = 0;
	for (int btn = 0; btn < MAX_BUTTONS; btn++)
	{
		if (joystick.buttons[btn].down) buttons |= 1 << btn;
	}
	return buttons;
}

int joy_get_button_up_cnt(int btn)
{
	int count = joystick.buttons[btn].upcount;
	joystick.buttons[btn].upcount = 0;
	return count;
}

int joy_get_button_down_cnt(int btn)
{
	int count = joystick.buttons[btn].downcount;
	joystick.buttons[btn].downcount = 0;
	return count;
}

fix joy_get_button_down_time(int btn)
{
	fix downtime = joystick.buttons[btn].downtime;
	joystick.buttons[btn].downtime = 0;
	return downtime;
}

int joy_get_button_state(int btn)
{
	return joystick.buttons[btn].down;
}

void joy_set_btn_values(int btn, int state, fix timedown, int downcount, int upcount)
{
	joystick.buttons[btn].down = state;
	joystick.buttons[btn].downtime = timedown;
	joystick.buttons[btn].downcount = downcount;
	joystick.buttons[btn].upcount = upcount;
}

void JoystickInput(int buttons, int axes[4], int presentmask)
{
	static fix lasttime = 0;
	fix curtime = timer_get_fixed_seconds();
	if (lasttime == 0 || curtime < lasttime) lasttime = curtime;
	fix elapsedtime = curtime - lasttime;
	for (int btn = 0; btn < MAX_BUTTONS; btn++)
	{
		//[ISB] Increment the bit by one, so button 0 can actually be read
		if (buttons & (1 << (btn+1)))
		{
			if (!joystick.buttons[btn].down)
			{
				joystick.buttons[btn].down = true;
				joystick.buttons[btn].downcount++;
			}
			else
			{
				joystick.buttons[btn].downtime += elapsedtime;
			}
		}
		else
		{
			if (joystick.buttons[btn].down)
			{
				joystick.buttons[btn].down = false;
				joystick.buttons[btn].upcount++;
			}
		}
	}

	for (int i = 0; i < 4; i++)
	{
		joystick.axis_value[i] = axes[i];
	}

	joystick.presentmask = presentmask;
}
