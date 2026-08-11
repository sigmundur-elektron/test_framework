#pragma once
#include "../include/imgui.h"
#include "../utils/inputs/multiplatform_input.h"

class window
{
  public:
	int init();
	void render();
	void update();
	void resize();
	void terminate();
	bool should_exit() { return m_should_exit; }

  private:
	void set_input();
	void set_demo_mode();
	void set_wireframe_mode();
	std::unordered_map<input_key, input_device_state> get_gamepad_state(int joystick_id);
	multiplatform_input _input{};
	GLFWwindow *m_window = 0;
	ImGuiID m_docking_id;
	bool m_should_exit = false;
};