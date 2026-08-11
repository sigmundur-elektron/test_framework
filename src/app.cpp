#include "app.h"

app::app()
	: input(input_manager::get_instance())
{
}

void app::run()
{
	init();

	while (!main_window.should_exit())
	{
		update();
	}
	end();
}

void app::init()
{
	std::print("Init..");
	input.set_key_mapping();
	main_window.init();
}

void app::update()
{
	main_window.render();
	input.process_input();
}

void app::end()
{
	std::print("Terminating..");
	main_window.terminate();
}