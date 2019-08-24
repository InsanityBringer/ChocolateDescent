#include "misc/types.h"
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"

#ifdef USE_SDL

#include "SDL_joystick.h"

#define MAX_BUTTONS 20

char joy_installed = 0;
char joy_present = 0;

typedef struct Button_info 
{
	uint8_t		ignore;
	uint8_t		state;
	uint8_t		last_state;
	fix		timedown; //[ISB] okay fuck all of this seriously. no ticks. 
	uint8_t		downcount;
	uint8_t		upcount;
} Button_info;

typedef struct Joy_info 
{
	uint8_t			present_mask;
	uint8_t			slow_read;
	int			max_timer;
	int			read_count;
	uint8_t			last_value;
	Button_info	buttons[MAX_BUTTONS];
	int			axis_min[4];
	int			axis_center[4];
	int			axis_max[4];
	fix			CurTime;
} Joy_info;

Joy_info joystick;

extern int joy_bogus_reading;
extern int joy_retries;

void joy_get_cal_vals(int* axis_min, int* axis_center, int* axis_max)
{
	int i;

	for (i = 0; i < 4; i++) 
	{
		axis_min[i] = joystick.axis_min[i];
		axis_center[i] = joystick.axis_center[i];
		axis_max[i] = joystick.axis_max[i];
	}
}

void joy_set_cal_vals(int* axis_min, int* axis_center, int* axis_max)
{
	int i;

	for (i = 0; i < 4; i++) {
		joystick.axis_min[i] = axis_min[i];
		joystick.axis_center[i] = axis_center[i];
		joystick.axis_max[i] = axis_max[i];
	}
}

//[ISB] useless stubs
//[ISB] TODO clean up all this useless shit
uint8_t joy_get_present_mask()
{
	return joystick.present_mask;
}

void joy_set_timer_rate(int max_value) 
{
	joystick.max_timer = max_value;
}

int joy_get_timer_rate() 
{
	return joystick.max_timer;
}

void joy_flush() 
{
	int i;

	if (!joy_installed) return;

	for (i = 0; i < MAX_BUTTONS; i++) {
		joystick.buttons[i].ignore = 0;
		joystick.buttons[i].state = 0;
		joystick.buttons[i].timedown = 0;
		joystick.buttons[i].downcount = 0;
		joystick.buttons[i].upcount = 0;
	}
}

void I_JoyHandler(int buttons, dbool down)
{
	uint8_t value;
	int i, state;
	Button_info* button;

	//	joystick.max_timer = ticks_this_time;
	joystick.CurTime = timer_get_fixed_secondsX();

	value = buttons;

	for (i = 0; i < MAX_BUTTONS; i++) 
	{
		button = &joystick.buttons[i];
		if (!button->ignore) 
		{
			if (i < 5)
				state = (value >> i) & 1;
			else if (i == (value + 4))
				state = 1;
			else
				state = 0;

			if (button->last_state == state) 
			{
				if (state) button->timedown = timer_get_fixed_secondsX(); //[ISB] I hate DOS joysticks AAAA. fixed point seconds timer. 
			}
			else 
			{
				if (state) 
				{
					button->downcount += state;
					button->state = 1;
				}
				else 
				{
					button->upcount += button->state;
					button->state = 0;
				}
				button->last_state = state;
			}
		}
	}
}


uint8_t joy_read_raw_buttons() 
{
	//Warning("joy_read_raw_buttons: STUB\n");
	return 0;
}

void joy_set_slow_reading(int flag)
{
	//Warning("joy_set_slow_reading: STUB\n");
}

uint8_t joystick_read_raw_axis(uint8_t mask, int* axis)
{
	//Warning("joystick_read_raw_axis: STUB\n");
	return 0;
}

int joy_init()
{
	Warning("joy_init: STUB\n");
	return 0;
}

void joy_close()
{
	if (!joy_installed) return;
	joy_installed = 0;
}

//[ISB] calibration functions aren't relevant
void joy_set_ul()
{
	//Warning("joy_set_ul: STUB\n");
}

void joy_set_lr()
{
	//Warning("joy_set_lr: STUB\n");
}

void joy_set_cen()
{
	//Warning("joy_set_cen: STUB\n");
}

void joy_set_cen_fake(int channel)
{
	//Warning("joy_set_cen_fake: STUB\n");
}

int joy_get_scaled_reading(int raw, int axn)
{
	//Warning("joy_get_scaled_reading: STUB\n");
	return 0;
}

int last_reading[4] = { 0, 0, 0, 0 };

void joy_get_pos(int* x, int* y)
{
	//Warning("joy_get_pos: STUB\n");
}

uint8_t joy_read_stick(uint8_t masks, int* axis)
{
	//Warning("joy_read_stick: STUB\n");
	return 0;
}

int joy_get_btns()
{
	//Warning("joy_get_btns: STUB\n");
	return 0;
}

void joy_get_btn_down_cnt(int* btn0, int* btn1)
{
	//Warning("joy_get_btn_down_cnt: STUB\n");
}

int joy_get_button_state(int btn)
{
	//Warning("joy_get_button_state: STUB\n");
	return 0;
}

int joy_get_button_up_cnt(int btn)
{
	//Warning("joy_get_button_up_cnt: STUB\n");
	return 0;
}

int joy_get_button_down_cnt(int btn)
{
	//Warning("joy_get_button_down_cnt: STUB\n");
	return 0;
}


fix joy_get_button_down_time(int btn)
{
	//Warning("joy_get_button_down_time: STUB\n");
	return 0;
}

void joy_get_btn_up_cnt(int* btn0, int* btn1)
{
	//Warning("joy_get_btn_up_cnt: STUB\n");
}

void joy_set_btn_values(int btn, int state, fix timedown, int downcount, int upcount)
{
	//Warning("joy_set_btn_values: STUB\n");
}

void joy_poll()
{
	//Warning("joy_poll: STUB\n");
}

#else

void I_JoyHandler(int buttons, dbool down) { }

int joy_init() { return 0; }
void joy_close() { }
void joy_set_ul() { }
void joy_set_lr() { }
void joy_set_cen() { }
void joy_get_pos(int* x, int* y) { }
int joy_get_btns() { return 0; }
int joy_get_button_up_cnt(int btn) { return 0; }
int joy_get_button_down_cnt(int btn) { return 0; }
fix joy_get_button_down_time(int btn) { return 0; }

uint8_t joy_read_raw_buttons() { return 0; }
uint8_t joystick_read_raw_axis(uint8_t mask, int* axis) { return 0; }
void joy_flush() { }
uint8_t joy_get_present_mask() { return 0; }
void joy_set_timer_rate(int max_value) { }
int joy_get_timer_rate() { return 0; }

int joy_get_button_state(int btn) { return 0; }
void joy_set_cen_fake(int channel) { }
uint8_t joy_read_stick(uint8_t masks, int* axis) { return 0; }
void joy_get_cal_vals(int* axis_min, int* axis_center, int* axis_max) { }
void joy_set_cal_vals(int* axis_min, int* axis_center, int* axis_max) { }
void joy_set_btn_values(int btn, int state, fix timedown, int downcount, int upcount) { }
int joy_get_scaled_reading(int raw, int axn) { return 0; }
void joy_set_slow_reading(int flag) { }

char joy_installed;
char joy_present;

#endif
