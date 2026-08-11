#include "input_manager.h"
#include "../console.h"
#include <vector>

input_source get_input_source_from_key(input_key key)
{
	switch (key)
	{
	case input_key::key_a:
	case input_key::key_b:
	case input_key::key_c:
	case input_key::key_d:
	case input_key::key_e:
	case input_key::key_f:
	case input_key::key_g:
	case input_key::key_h:
	case input_key::key_i:
	case input_key::key_j:
	case input_key::key_k:
	case input_key::key_l:
	case input_key::key_m:
	case input_key::key_n:
	case input_key::key_o:
	case input_key::key_p:
	case input_key::key_q:
	case input_key::key_r:
	case input_key::key_s:
	case input_key::key_t:
	case input_key::key_u:
	case input_key::key_v:
	case input_key::key_w:
	case input_key::key_x:
	case input_key::key_y:
	case input_key::key_z:
		return input_source::keyboard;
	case input_key::gamepad_l_thumb_x:
	case input_key::gamepad_l_thumb_y:
	case input_key::gamepad_r_thumb_x:
	case input_key::gamepad_r_thumb_y:
	case input_key::gamepad_r_trigger:
	case input_key::gamepad_l_trigger:
	case input_key::gamepad_y:
	case input_key::gamepad_x:
	case input_key::gamepad_b:
	case input_key::gamepad_a:
	case input_key::gamepad_start:
	case input_key::gamepad_select:
	case input_key::gamepad_bumper_r:
	case input_key::gamepad_bumper_l:
	case input_key::gamepad_l3:
	case input_key::gamepad_r3:
	case input_key::gamepad_dpad_up:
	case input_key::gamepad_dpad_right:
	case input_key::gamepad_dpad_left:
	case input_key::gamepad_dpad_down:
		return input_source::gamepad;
	case input_key::mouse_left:
	case input_key::mouse_right:
	case input_key::mouse_middle:
	case input_key::mouse_move_x:
	case input_key::mouse_move_y:
		return input_source::mouse;
	default:
		return input_source::unknown;
	}
}

void input_manager::register_action_callback(const std::string &action_name, const input_manager::action_callback &callback)
{
	auto &console = console::get_instance();
	console.add_tagged_line("input_manager", "Action registered of type: " + action_name);
	_action_callback[action_name].emplace_back(callback);
}

void input_manager::remove_action_callback(const std::string &actionName, const std::string &callback_ref)
{
	erase_if(_action_callback[actionName], [callback_ref](const action_callback &callback) {
		return callback.ref == callback_ref;
	});
}

/// Map input to action
/// 
/// @warning doesn't check for duplicates
void input_manager::map_input_to_action(input_key key, const input_action &action)
{
	// TODO: Check for duplicates
	_input_action_mapping[key].emplace_back(action);
}

void input_manager::unmap_input_from_action(input_key key, const std::string &action)
{
	std::erase_if(_input_action_mapping[key], [action](const input_action &input_action) {
		return input_action.action_name == action;
	});
}


/// process_input will get new device state and compare with old state; then generate action events
///
/// @warning can have conflicting mappings
void input_manager::process_input()
{
	std::vector<action_event> events{};
	for (auto &device : _devices)
	{
		auto new_state = device.m_state_func(device.m_index);

		// compare to old state for device
		for (auto &key_state : new_state)
		{
			if (device.m_current_state[key_state.first].value != key_state.second.value)
			{
				// TODO: Fix cases where conflicting mappings -- additive fashion?
				auto generated_events = generate_action_event(device.m_index, key_state.first, key_state.second.value);
				events.insert(events.end(), generated_events.begin(), generated_events.end());
				device.m_current_state[key_state.first].value = key_state.second.value;
			}
		}
	}
	for (auto &event : events)
	{
		propagate_action_event(event);
	}
}

std::vector<input_manager::action_event> input_manager::generate_action_event(int device_index, input_key key, float new_val)
{
	auto &actions = _input_action_mapping[key];

	std::vector<action_event> action_events{};

	input_source source = get_input_source_from_key(key);

	for (auto &action : actions)
	{
		action_events.emplace_back(action_event{
		  .action_name = action.action_name,
		  .source = source,
		  .source_index = device_index,
		  .value = new_val * action.scale});
	}

	return action_events;
}

void input_manager::propagate_action_event(action_event event)
{
	for (size_t i = _action_callback[event.action_name].size() - 1; i >= 0; i--)
	{
		auto &actionCallback = _action_callback[event.action_name][i];

		if (actionCallback.func(event.source, event.source_index, event.value))
			break;
	}
}

void input_manager::register_device(const input_device &device)
{
	auto &console = console::get_instance();
	console.add_tagged_line("input_manager", "Device registered of type: {}", static_cast<int>(device.m_type));
	_devices.emplace_back(device);
	console.add_tagged_line("input_manager", "Device #: {}", _devices.size());
}

void input_manager::remove_device(intput_device_type type, int inputIndex)
{
	auto &console = console::get_instance();
	erase_if(_devices, [type, inputIndex](const input_device &device) {
		return device.m_type == type && device.m_index == inputIndex;
	});
	console.add_tagged_line("input_manager", "Device unregistered of type: ", static_cast<int>(type));
	console.add_tagged_line("input_manager", "Device #: {}", _devices.size());
}

void input_manager::set_key_mapping()
{
	map_input_to_action(input_key::key_enter, input_action{.action_name = "enter"});
	register_action_callback("enter", input_manager::action_callback{.ref = "enter console", .func = [](input_source source, int source_index, float value) {
																		 if (value == 0.1f)
																		 {
																			 auto &console = console::get_instance();
																				 console.add_line();
																		 }
																		 return true;
																	 }});
}