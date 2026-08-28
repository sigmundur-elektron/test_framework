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

	/// Dispatch a single planned step against this agent's grants.
	///
	/// Public because it is the only observable seam for permission
	/// enforcement: the alternative route, handle(), dispatches steps from
	/// planner::plan, which is still a stub returning {} (T-005). Treat this as
	/// a test seam; production flow should go through handle().
	tool_result execute_step(const plan_step &step);

  private:
	agent_config _config;
	tool_registry &_tools;
	planner _planner;
	memory _memory;
};