#include "ai_persistence.h"
#include "../agent/agent_registry.h"
#include "ai_setup.h"
#include <glaze/glaze.hpp>

bool ai_persistence::save(const std::string &path, std::string &error)
{
	auto &registry = agent_registry::get_instance();

	ai_setup setup;
	for (agent *a : registry.all())
	{
		agent_snapshot snap;
		snap.config = a->config();
		snap.history = a->get_memory().history(); // copy
		setup.agents.push_back(std::move(snap));
	}

	// prettify so the saved file is human-readable/editable.
	auto ec = glz::write_file_json<glz::opts{.prettify = true}>(
		setup, path, std::string{});
	if (ec)
	{
		error = "failed to write '" + path + "'";
		return false;
	}
	return true;
}

bool ai_persistence::load(const std::string &path, std::string &error)
{
	ai_setup setup{};
	auto ec = glz::read_file_json(setup, path, std::string{});
	if (ec)
	{
		error = glz::format_error(ec, std::string{});
		return false;
	}

	auto &registry = agent_registry::get_instance();

	// Recreate agents from the snapshot.
	for (auto &snap : setup.agents)
	{
		registry.remove(snap.config.name); // replace if it already exists
		if (agent *a = registry.create(snap.config))
			a->mutable_memory().load(std::move(snap.history));
	}
	return true;
}