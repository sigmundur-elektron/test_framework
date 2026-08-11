#pragma once
#include "utils/inputs/input_key.h"
#include "ui/window.h"
#include "utils/console.h"
#include "utils/inputs/input_manager.h"
#include <iostream>
#include <print>

struct app
{
public:
	app();
	void run();
private:
	window main_window;
	input_manager& input;
	void init();
	void update();
	void end();
};