#pragma once

#include "hid.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

class Win32HidDevice
{
public:
	static std::vector<std::unique_ptr<Win32HidDevice>> create_devices(HWND window);

	Win32HidDevice(HANDLE rawinput_device);

	std::string get_name() const;
	std::string get_device_name() const;
	std::string get_key_name(int id) const;
	bool get_keycode(int keycode) const;
	float get_axis(int index) const;
	std::vector<int> get_axis_ids() const;
	int get_hat(int index) const;
	int get_button_count() const;
	int get_hat_count() const;

	void update(RAWINPUT* raw_input);

	enum KnownIds
	{
		joystick_x,
		joystick_y,
		joystick_z,
		joystick_rx,
		joystick_ry,
		joystick_rz,
		joystick_slider,
		joystick_dial,
		joystick_wheel,
		joystick_hat
	};

private:
	HANDLE open_device();
	void get_preparse_data();

	void find_names(HANDLE device);
	void find_button_names(HANDLE device);
	void find_value_names(HANDLE device);

	void update(void* report, int report_size);
	void update_buttons(void* report, int report_size);
	void update_values(void* report, int report_size);

	static std::string utf16_to_utf8(const std::wstring& str);

	Hid hid;
	HANDLE rawinput_device;

	std::string product_name;
	std::string manufacturer_name;
	std::string serial_number;

	std::vector<bool> buttons;
	std::vector<float> axis_values;
	std::vector<int> hat_values;

	std::vector<int> axis_ids;
	std::vector<int> hat_ids;

	std::vector<std::string> button_names;
	std::vector<std::string> axis_names;
	std::vector<std::string> hat_names;

	std::map<Hid::USAGE, int> usage_to_button_index;
	std::map<Hid::USAGE, int> usage_to_axis_index;
	std::map<Hid::USAGE, int> usage_to_hat_index;

	std::vector<uint8_t> preparse_data;
};
