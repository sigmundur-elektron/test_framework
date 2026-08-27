#pragma once
#include "../tools/tool_registry.h"
#include "agent_config.h"
#include "memory.h"
#include "planner.h"
#include <string>

/// Agent — one configurable reasoning unit. Multiple can coexist at runtime.
struct agent
{
  public:
	explicit agent(agent_config config);

	void init();
	std::string handle(const std::string &user_goal);
	void end();

	/// Non-mutating plan preview for the planner UI (does not touch memory).
	std::vector<plan_step> preview(const std::string &user_goal) const
	{
		return _planner.plan(user_goal);
	}

	const agent_config &config() const { return _config; }
	agent_config &mutable_config() { return _config; } // GUI edits this live

	const memory &get_memory() const { return _memory; }
	memory &mutable_memory() { return _memory; } // used for save/load

  private:
	agent_config _config;
	tool_registry &_tools;
	planner _planner;
	memory _memory;

	tool_result execute_step(const plan_step &step);
};