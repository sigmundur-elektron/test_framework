#include "app.h"
#include "features/agent_service.h"
#include "repository/repository_provider.h"
#include "profiler.h"

app::app()
	: input(input_manager::get_instance())
{
}

void app::run()
{
	PROFILE_BEGIN_SESSION("startup", "startup_profile.json");
	init();
	PROFILE_END_SESSION();

	while (!main_window.should_exit())
	{
		update();
	}
	end();
}

void app::init()
{
	std::print("Init..");
	{
		PROFILE_SCOPE("repository.start");
		std::string status;
		features::repository_provider::instance().start(status);
		std::println("DB: {}", status);
		features::agent_service::instance().load_from_repository();
	}
	{
		PROFILE_SCOPE("input.set_key_mapping");
		input.set_key_mapping();
	}
	{
		PROFILE_SCOPE("window.init");
		main_window.init();
	}
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
	features::repository_provider::instance().stop();
}