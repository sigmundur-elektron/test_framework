
#pragma once
#include "input_devices.h"
#include "input_key.h"
#include <GLFW/glfw3.h>
#include <unordered_map>

class multiplatform_input
{
  public:
	std::unordered_map<input_key, input_device_state> get_keyboard_state(int index) { return m_keyboard_state; }
	std::unordered_map<input_key, input_device_state> get_mouse_state(int index) { return m_mouse_state; }
	std::unordered_map<input_key, input_device_state> get_gamepad_state(const GLFWgamepadstate &state);

	void update_keyboard_state(int key, float value);
	void update_mouse_state(int button, float value);
	void get_mouse_pos(double *_x, double *_y);

  private:
	static input_key multiplatform_key_to_input_key(int key);
	static input_key multiplatform_mouse_button_to_input_key(int button);

  private:
	std::unordered_map<input_key, input_device_state> m_keyboard_state{};
	std::unordered_map<input_key, input_device_state> m_mouse_state{};
};