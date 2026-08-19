#include "agent.h"
#include "../tools/read_file_tool.h"
#include <memory>

agent::agent(agent_config config)
	: _config(std::move(config)), _tools(tool_registry::get_instance())
{
}

void agent::init()
{
	_tools.register_tool(std::make_unique<read_file_tool>());
}

std::string agent::handle(const std::string &user_goal)
{
	if (!_config.enabled)
		return "[agent '" + _config.name + "' disabled]";

	_memory.add("user", user_goal);

	std::string transcript;
	for (const auto &step : _planner.plan(user_goal))
	{
		const tool_result result = execute_step(step);
		_memory.add("tool", result.output);
		transcript += result.output;
	}

	_memory.add("assistant", transcript);
	return transcript;
}

void agent::end()
{
	_memory.clear();
}

tool_result agent::execute_step(const plan_step &step)
{
	itool *tool = _tools.find(step.tool_name);
	if (!tool)
		return {false, "unknown tool: " + step.tool_name};

	auto result = tool->execute(step.json_args);
	if (!result)
		return {false, result.error()};

	return *result;
}