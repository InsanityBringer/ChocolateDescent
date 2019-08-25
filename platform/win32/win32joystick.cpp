/*
**  Win32 HID devices
**  Copyright (c) 2016 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#include "win32joystick.h"
#undef min
#undef max
#include <algorithm>
#include <memory>
#include <stdexcept>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC		((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE	((USHORT) 0x02)
#endif

#ifndef HID_USAGE_GENERIC_JOYSTICK
#define HID_USAGE_GENERIC_JOYSTICK	((USHORT) 0x04)
#endif

#ifndef HID_USAGE_GENERIC_GAMEPAD
#define HID_USAGE_GENERIC_GAMEPAD	((USHORT) 0x05)
#endif

#ifndef RIDEV_INPUTSINK
#define RIDEV_INPUTSINK	(0x100)
#endif

std::vector<std::unique_ptr<Win32HidDevice>> Win32HidDevice::create_devices(HWND window)
{
	std::vector<std::unique_ptr<Win32HidDevice>> devices;

	// Generate raw input for all input device types
	RAWINPUTDEVICE rid[3];
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage = HID_USAGE_GENERIC_JOYSTICK;
	rid[0].dwFlags = 0;
	rid[0].hwndTarget = window;
	rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[1].usUsage = HID_USAGE_GENERIC_GAMEPAD;
	rid[1].dwFlags = 0;
	rid[1].hwndTarget = window;
	rid[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[2].usUsage = HID_USAGE_GENERIC_MOUSE;
	rid[2].dwFlags = 0;
	rid[2].hwndTarget = window;
	RegisterRawInputDevices(rid, 3, sizeof(RAWINPUTDEVICE));

	// Search for all joysticks and gamepads
	UINT num_devices = 0;
	UINT result = GetRawInputDeviceList(0, &num_devices, sizeof(RAWINPUTDEVICELIST));
	if (result != (UINT)-1 && num_devices > 0)
	{
		std::vector<RAWINPUTDEVICELIST> device_list(num_devices);
		result = GetRawInputDeviceList(&device_list[0], &num_devices, sizeof(RAWINPUTDEVICELIST));
		if (result == (UINT)-1)
			return devices;

		for (size_t i = 0; i < device_list.size(); i++)
		{
			RID_DEVICE_INFO device_info;
			UINT device_info_size = sizeof(RID_DEVICE_INFO);
			device_info.cbSize = device_info_size;
			result = GetRawInputDeviceInfo(device_list[i].hDevice, RIDI_DEVICEINFO, &device_info, &device_info_size);
			if (result == (UINT)-1)
				return devices;

			if (device_info.dwType == RIM_TYPEHID)
			{
				if (device_info.hid.usUsagePage == HID_USAGE_PAGE_GENERIC && (device_info.hid.usUsage == HID_USAGE_GENERIC_JOYSTICK || device_info.hid.usUsage == HID_USAGE_GENERIC_GAMEPAD))
				{
					try
					{
						devices.push_back(std::make_unique<Win32HidDevice>(device_list[i].hDevice));
					}
					catch (const std::exception&)
					{
						// Hmm log this maybe?
					}
				}
			}
		}
	}

	return devices;
}

Win32HidDevice::Win32HidDevice(HANDLE rawinput_device) : rawinput_device(rawinput_device)
{
	get_preparse_data();
	HANDLE device = open_device();
	try
	{
		find_names(device);
		find_button_names(device);
		find_value_names(device);
		CloseHandle(device);
	}
	catch (...)
	{
		CloseHandle(device);
		throw;
	}
}

void Win32HidDevice::update(RAWINPUT* raw_input)
{
	if (raw_input->header.hDevice == rawinput_device)
	{
		for (DWORD i = 0; i < raw_input->data.hid.dwCount; i++)
		{
			BYTE* raw_data = const_cast<BYTE*>(raw_input->data.hid.bRawData);
			DWORD offset = i * raw_input->data.hid.dwSizeHid;

			void* report = raw_data + offset;
			int report_size = raw_input->data.hid.dwSizeHid;
			update(report, report_size);
		}
	}
}

std::string Win32HidDevice::get_name() const
{
	return product_name;
}

std::string Win32HidDevice::get_device_name() const
{
	UINT name_size = 0;
	UINT result = GetRawInputDeviceInfo(rawinput_device, RIDI_DEVICENAME, 0, &name_size);
	if (result == (UINT)-1 || name_size == 0)
		throw std::runtime_error("GetRawInputDeviceInfo failed");

	std::unique_ptr<WCHAR[]> name_buffer(new WCHAR[name_size]);
	result = GetRawInputDeviceInfo(rawinput_device, RIDI_DEVICENAME, name_buffer.get(), &name_size);
	if (result == (UINT)-1)
		throw std::runtime_error("GetRawInputDeviceInfo failed");

	return utf16_to_utf8(name_buffer.get());
}

std::string Win32HidDevice::get_key_name(int id) const
{
	if (id >= 0 && id < (int)button_names.size())
		return button_names[id];
	else
		return std::string();
}

bool Win32HidDevice::get_keycode(int id) const
{
	if (id >= 0 && id < (int)button_names.size())
		return buttons[id];
	else
		return false;
}

float Win32HidDevice::get_axis(int id) const
{
	if (id >= 0 && id < (int)axis_names.size())
		return axis_values[id];
	else
		return 0.0f;
}

std::vector<int> Win32HidDevice::get_axis_ids() const
{
	return axis_ids;
}

int Win32HidDevice::get_hat(int id) const
{
	if (id >= 0 && id < (int)hat_names.size())
		return hat_values[id];
	else
		return -1;
}

int Win32HidDevice::get_button_count() const
{
	return buttons.size();
}

int Win32HidDevice::get_hat_count() const
{
	return hat_names.size();
}

HANDLE Win32HidDevice::open_device()
{
	UINT name_size = 0;
	UINT result = GetRawInputDeviceInfo(rawinput_device, RIDI_DEVICENAME, 0, &name_size);
	if (result == (UINT)-1 || name_size == 0)
		throw std::runtime_error("GetRawInputDeviceInfo failed");

	std::unique_ptr<TCHAR[]> name_buffer(new TCHAR[name_size + 1]);
	name_buffer.get()[name_size] = 0;
	result = GetRawInputDeviceInfo(rawinput_device, RIDI_DEVICENAME, name_buffer.get(), &name_size);
	if (result == (UINT)-1)
		throw std::runtime_error("GetRawInputDeviceInfo failed");

	//  Windows XP fix: The raw device path in its native form (\??\...). When you have the form \\?\ that is a crutch MS invented to make long path names available on Win32 when NT arrived despite the limitation of the Win32 subsystem to the \?? object directory
	if (name_size > 2)
	{
		TCHAR* ptr = name_buffer.get();
		if ((ptr[0] == '\\') && (ptr[1] == '?'))
			ptr[1] = '\\';
	}

	HANDLE device_handle = CreateFile(name_buffer.get(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
	if (device_handle == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Unable to open input device");

	return device_handle;
}

void Win32HidDevice::get_preparse_data()
{
	UINT preparse_data_size = 0;
	UINT result = GetRawInputDeviceInfo(rawinput_device, RIDI_PREPARSEDDATA, 0, &preparse_data_size);
	if (result == (UINT)-1)
		throw std::runtime_error("GetRawInputDeviceInfo failed");

	preparse_data.resize(preparse_data_size);
	result = GetRawInputDeviceInfo(rawinput_device, RIDI_PREPARSEDDATA, preparse_data.data(), &preparse_data_size);
	if (result == (UINT)-1)
	{
		preparse_data.clear();
		throw std::runtime_error("GetRawInputDeviceInfo failed");
	}
}

void Win32HidDevice::find_names(HANDLE device)
{
	const int max_name_length = 1024;
	WCHAR name[max_name_length];

	if (hid.GetProductString(device, name, max_name_length * sizeof(WCHAR)))
		product_name = utf16_to_utf8(name);

	if (hid.GetManufacturerString(device, name, max_name_length * sizeof(WCHAR)))
		manufacturer_name = utf16_to_utf8(name);

	if (hid.GetSerialNumberString(device, name, max_name_length * sizeof(WCHAR)))
		serial_number = utf16_to_utf8(name);
}

void Win32HidDevice::find_button_names(HANDLE device)
{
	Hid::CAPS caps;
	hid.GetCaps(preparse_data.data(), &caps);

	std::vector<Hid::BUTTON_CAPS> button_caps(caps.NumberInputButtonCaps);
	if (!button_caps.empty())
	{
		USHORT num_button_caps = (USHORT)button_caps.size();
		hid.GetButtonCaps(Hid::HidP_Input, &button_caps[0], &num_button_caps, preparse_data.data());
		for (size_t collection = 0; collection < button_caps.size(); collection++)
		{
			for (Hid::USAGE usage = button_caps[collection].Range.UsageMin; usage <= button_caps[collection].Range.UsageMax; usage++)
			{
				std::string name;

				if (button_caps[collection].IsStringRange)
				{
					const int max_name_length = 1024;
					WCHAR buffer[max_name_length];

					int offset = usage - button_caps[collection].Range.UsageMin;
					int string_index = button_caps[collection].Range.StringMin + offset;
					if (hid.GetIndexedString(device, string_index, buffer, max_name_length * sizeof(WCHAR)))
						name = utf16_to_utf8(buffer);
				}

				if (name.empty())
					name = "Button " + std::to_string(buttons.size() + 1);

				button_names.push_back(name);
				usage_to_button_index[usage] = buttons.size();
				buttons.push_back(false);
			}
		}
	}
}

void Win32HidDevice::find_value_names(HANDLE device)
{
	// Place all standard axis and hats at the beginning of the list
	axis_values.resize(9);
	axis_names.push_back("X");
	axis_names.push_back("Y");
	axis_names.push_back("Z");
	axis_names.push_back("Rx");
	axis_names.push_back("Ry");
	axis_names.push_back("Rz");
	axis_names.push_back("Slider");
	axis_names.push_back("Dial");
	axis_names.push_back("Wheel");
	for (int i = 0; i < 9; i++)
		usage_to_axis_index[0x30 + i] = i;
	int next_axis_index = 9;

	hat_values.push_back(-1);
	hat_names.push_back("Hat");
	usage_to_hat_index[0x39] = 0;
	int next_hat_index = 2;

	Hid::CAPS caps;
	hid.GetCaps(preparse_data.data(), &caps);

	std::vector<Hid::VALUE_CAPS> value_caps(caps.NumberInputValueCaps);
	if (!value_caps.empty())
	{
		USHORT num_value_caps = (USHORT)value_caps.size();
		hid.GetValueCaps(Hid::HidP_Input, &value_caps[0], &num_value_caps, preparse_data.data());
		for (size_t collection = 0; collection < value_caps.size(); collection++)
		{
			for (Hid::USAGE usage = value_caps[collection].Range.UsageMin; usage <= value_caps[collection].Range.UsageMax; usage++)
			{
				if (usage < 0x30 || usage > 0x39)
				{
					if (value_caps[collection].LogicalMin == 0 && value_caps[collection].LogicalMax == 3 && value_caps[collection].HasNull) // Four direction hat
					{
						hat_names.push_back("Hat" + std::to_string(next_hat_index++));
						usage_to_hat_index[usage] = hat_values.size();
						hat_ids.push_back(hat_values.size());
						hat_values.push_back(-1);
					}
					else if (value_caps[collection].LogicalMin == 0 && value_caps[collection].LogicalMax == 7 && value_caps[collection].HasNull) // Eight direction hat
					{
						hat_names.push_back("Hat" + std::to_string(next_hat_index++));
						usage_to_hat_index[usage] = hat_values.size();
						hat_ids.push_back(hat_values.size());
						hat_values.push_back(-1);
					}
					else
					{
						axis_names.push_back("Axis" + std::to_string(next_axis_index++));
						usage_to_axis_index[usage] = axis_values.size();
						axis_ids.push_back(axis_values.size());
						axis_values.push_back(0.0f);
					}
				}
				else
				{
					switch (usage)
					{
					case 0x30: axis_ids.push_back(joystick_x); break;
					case 0x31: axis_ids.push_back(joystick_y); break;
					case 0x32: axis_ids.push_back(joystick_z); break;
					case 0x33: axis_ids.push_back(joystick_rx); break;
					case 0x34: axis_ids.push_back(joystick_ry); break;
					case 0x35: axis_ids.push_back(joystick_rz); break;
					case 0x36: axis_ids.push_back(joystick_slider); break;
					case 0x37: axis_ids.push_back(joystick_dial); break;
					case 0x38: axis_ids.push_back(joystick_wheel); break;
					case 0x39: hat_ids.push_back(joystick_hat); break;
					}
				}
			}
		}
	}
}

void Win32HidDevice::update(void* report, int report_size)
{
	update_buttons(report, report_size);
	update_values(report, report_size);
}

void Win32HidDevice::update_buttons(void* report, int report_size)
{
	for (size_t i = 0; i < buttons.size(); i++)
		buttons[i] = false;

	Hid::CAPS caps;
	hid.GetCaps(preparse_data.data(), &caps);

	std::vector<Hid::BUTTON_CAPS> button_caps(caps.NumberInputButtonCaps);
	if (!button_caps.empty())
	{
		USHORT num_button_caps = (USHORT)button_caps.size();
		hid.GetButtonCaps(Hid::HidP_Input, &button_caps[0], &num_button_caps, preparse_data.data());
		for (size_t collection = 0; collection < button_caps.size(); collection++)
		{
			ULONG array_length = button_caps[collection].Range.UsageMax - button_caps[collection].Range.UsageMin + 1;

			std::vector<Hid::USAGE> usages(array_length);
			std::vector<bool> button_values(array_length);

			ULONG usage_length = array_length;
			hid.GetUsages(Hid::HidP_Input, button_caps[collection].UsagePage, button_caps[collection].LinkCollection, &usages[0], &usage_length, preparse_data.data(), report, report_size);
			usage_length = std::min(usage_length, array_length);

			for (size_t i = 0; i < usage_length; i++)
			{
				std::map<Hid::USAGE, int>::iterator it = usage_to_button_index.find(usages[i]);
				if (it != usage_to_button_index.end())
					buttons[it->second] = true;
			}
		}
	}

}

void Win32HidDevice::update_values(void* report, int report_size)
{
	for (size_t i = 0; i < axis_values.size(); i++)
		axis_values[i] = 0.0f;
	for (size_t i = 0; i < hat_values.size(); i++)
		hat_values[i] = -1;

	Hid::CAPS caps;
	hid.GetCaps(preparse_data.data(), &caps);

	std::vector<Hid::VALUE_CAPS> value_caps(caps.NumberInputValueCaps);
	if (!value_caps.empty())
	{
		USHORT num_value_caps = (USHORT)value_caps.size();
		hid.GetValueCaps(Hid::HidP_Input, &value_caps[0], &num_value_caps, preparse_data.data());
		for (size_t collection = 0; collection < value_caps.size(); collection++)
		{
			ULONG value = 0;
			for (Hid::USAGE usage = value_caps[collection].Range.UsageMin; usage <= value_caps[collection].Range.UsageMax; usage++)
			{
				hid.GetUsageValue(Hid::HidP_Input, value_caps[collection].UsagePage, 0, usage, &value, preparse_data.data(), report, report_size);

				if (value_caps[collection].LogicalMin == 0 && value_caps[collection].LogicalMax == 3 && value_caps[collection].HasNull) // Four direction hat
				{
					std::map<Hid::USAGE, int>::iterator it = usage_to_hat_index.find(usage);
					if (it != usage_to_hat_index.end())
						hat_values[it->second] = (value == 8) ? -1 : (value * 360 / 4);
				}
				else if (value_caps[collection].LogicalMin == 0 && value_caps[collection].LogicalMax == 7 && value_caps[collection].HasNull) // Eight direction hat
				{
					std::map<Hid::USAGE, int>::iterator it = usage_to_hat_index.find(usage);
					if (it != usage_to_hat_index.end())
						hat_values[it->second] = (value == 8) ? -1 : (value * 360 / 8);
				}
				else
				{
					std::map<Hid::USAGE, int>::iterator it = usage_to_axis_index.find(usage);
					if (it != usage_to_axis_index.end())
						axis_values[it->second] =
						std::min(std::max((value - value_caps[collection].LogicalMin) / (float)(value_caps[collection].LogicalMax - value_caps[collection].LogicalMin) * 2.0f - 1.0f, -1.0f), 1.0f);
				}
			}
		}
	}
}

std::string Win32HidDevice::utf16_to_utf8(const std::wstring& str)
{
	if (str.empty()) return {};
	int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), nullptr, 0, nullptr, nullptr);
	if (length <= 0) return {};
	std::string sutf8(length + 1, 0);
	int result = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), &sutf8[0], sutf8.length(), nullptr, nullptr);
	if (result < length) return  {};
	return sutf8;
}
