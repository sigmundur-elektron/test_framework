#pragma once
#include "input_devices.h"
#include "input_key.h"
#include <functional>
#include <unordered_map>
#include <vector>

/// Input Manager
///
/// The main controller for handeling all inputs.
/// registering all devices or removing them.
/// mapping all keybindings, and setting all callbacks.
struct input_manager : public singleton<input_manager>
{
  public:
	using action_callback_func = std::function<bool(input_source, int, float)>;
	struct action_callback
	{
		std::string ref;
		action_callback_func func;
	};

	input_manager(token){};
	void process_input();
	void register_device(const input_device &device);
	void remove_device(intput_device_type source, int input_index);
	void register_action_callback(const std::string &action_name, const action_callback &callback);
	void remove_action_callback(const std::string &action_name, const std::string &callback_ref);
	void map_input_to_action(input_key key, const input_action &action);
	void unmap_input_from_action(input_key key, const std::string &action);
	void set_key_mapping();


  private:
	struct action_event
	{
		std::string action_name;
		input_source source;
		int source_index;
		float value;
	};

	std::vector<action_event> generate_action_event(int device_index, input_key key, float new_val);
	void propagate_action_event(action_event event);
	std::unordered_map<input_key, std::vector<input_action>> _input_action_mapping{};
	std::unordered_map<std::string, std::vector<action_callback>> _action_callback{};
	std::vector<input_device> _devices;
};