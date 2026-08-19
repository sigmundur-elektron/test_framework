#pragma once
#include "../agent/agent_config.h"
#include "../agent/memory.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

/// Full snapshot of one agent: its runtime config + its memory history.
struct agent_snapshot
{
	agent_config config;
	std::vector<memory::entry> history;
};

/// Complete AI setup: every agent, its configuration and memories.
/// This is the root object that gets written to / read from disk.
struct ai_setup
{
	int version{1};
	std::vector<agent_snapshot> agents;
};

/// Register the permissions enum so it serializes as readable strings
/// instead of raw integers.
template <>
struct glz::meta<permissions::scope>
{
	using enum permissions::scope;
	static constexpr auto value = enumerate(
		read_project, write_project,
		read_github, write_github,
		read_database, write_database);
};