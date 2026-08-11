
#pragma once
#include <string>

enum class input_key
{
	unknown,
	key_a,
	key_b,
	key_c,
	key_d,
	key_e,
	key_f,
	key_g,
	key_h,
	key_i,
	key_j,
	key_k,
	key_l,
	key_m,
	key_n,
	key_o,
	key_p,
	key_q,
	key_r,
	key_s,
	key_t,
	key_u,
	key_v,
	key_w,
	key_x,
	key_y,
	key_z,
	key_enter,
	space,
	gamepad_l_thumb_x,
	gamepad_l_thumb_y,
	gamepad_r_thumb_x,
	gamepad_r_thumb_y,
	gamepad_r_trigger,
	gamepad_l_trigger,
	gamepad_y,
	gamepad_x,
	gamepad_b,
	gamepad_a,
	gamepad_start,
	gamepad_select,
	gamepad_bumper_r,
	gamepad_bumper_l,
	gamepad_l3,
	gamepad_r3,
	gamepad_dpad_up,
	gamepad_dpad_right,
	gamepad_dpad_left,
	gamepad_dpad_down,
	mouse_pos_x,
	mouse_pos_y,
	mouse_move_x,
	mouse_move_y,
	mouse_right,
	mouse_left,
	mouse_middle
};

enum class input_source
{
	keyboard,
	mouse,
	gamepad,
	unknown
};

struct input_action
{
	std::string action_name{""};
	float scale{1.f};
};

input_source get_input_source_from_key(input_key key);