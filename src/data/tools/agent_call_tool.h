#pragma once
#include "itool.h"

/// A2A tool: lets one agent invoke another by name via the agent_registry.
/// Args JSON: { "agent": "<name>", "goal": "<text>" }
struct agent_call_tool : public itool
{
	std::string name() const override;
	std::string schema() const override;
	std::expected<tool_result, std::string> execute(const std::string &json_args) override;
};