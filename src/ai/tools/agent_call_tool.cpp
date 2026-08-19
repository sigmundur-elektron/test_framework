#include "agent_call_tool.h"
#include "../agent/agent_registry.h"

namespace
{
	// Minimal extractor; swap for a real JSON parser later.
	std::string extract(const std::string &json, const std::string &key)
	{
		auto pos = json.find("\"" + key + "\"");
		if (pos == std::string::npos)
			return {};
		auto start = json.find('"', json.find(':', pos) + 1);
		auto end = json.find('"', start + 1);
		if (start == std::string::npos || end == std::string::npos)
			return {};
		return json.substr(start + 1, end - start - 1);
	}
}

std::string agent_call_tool::name() const
{
	return "agent_call";
}

std::string agent_call_tool::schema() const
{
	return R"({
  "name": "agent_call",
  "description": "Delegate a goal to another agent (A2A).",
  "parameters": {
	"type": "object",
	"properties": {
	  "agent": { "type": "string" },
	  "goal":  { "type": "string" }
	},
	"required": ["agent", "goal"]
  }
})";
}

std::expected<tool_result, std::string> agent_call_tool::execute(const std::string &json_args)
{
	const std::string target = extract(json_args, "agent");
	const std::string goal = extract(json_args, "goal");
	if (target.empty())
		return std::unexpected("missing required argument: agent");

	agent *peer = agent_registry::get_instance().find(target);
	if (!peer)
		return std::unexpected("unknown agent: " + target);

	tool_result result;
	result.output = peer->handle(goal);
	result.success = true;
	return result;
}