#pragma once
#include "../singleton.h"
#include "input_key.h"
#include <functional>
#include <map>
#include <unordered_map>

enum class intput_device_type
{
	keyboard,
	mouse,
	gamepad
};

/// Input Device State
///
/// default value set to -69 for error checking
/// usualy it's 0 for default, 0.1 for release, 1.0f for press and repeat for keyboard
/// for game pad it depends if you're pressing it hard or softly
struct input_device_state
{
	float value{-69.f};
};

using input_device_state_callback_func = std::function<std::unordered_map<input_key, input_device_state>(int)>;

/// Input Device
///
/// Used to organize inputs, also to register gamepads and so on
struct input_device
{
	intput_device_type m_type;
	int m_index;
	std::unordered_map<input_key, input_device_state> m_current_state;
	input_device_state_callback_func m_state_func;
};