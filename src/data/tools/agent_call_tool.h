#pragma once
#include "itool.h"

/// A2A tool: lets one agent invoke another by name via the agent_registry.
/// Args JSON: { "agent": "<name>", "goal": "<text>" }
struct agent_call_tool : public itool
{
	std::string name() const override;
	std::string schema() const override;

	/// Takes `perms` to satisfy the itool contract but does not consult it: no
	/// A2A scope exists in permissions::scope, and adding one would break the
	/// three tables keyed on that enum's names and order. Peer calls stay
	/// governed by agent_config::peer_agents. Gating A2A is a separate task.
	std::expected<tool_result, std::string> execute(const std::string &json_args,
													const permissions &perms) override;
};