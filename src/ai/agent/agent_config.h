#pragma once
#include "../tools/permissions.h"
#include <string>
#include <vector>

/// Runtime-editable agent configuration (edited live from the GUI).
struct agent_config
{
	std::string name;						 // unique id
	std::string model_endpoint;				 // LLM/A2A endpoint URL
	std::vector<std::string> peer_agents;	 // A2A: agents this one may call
	std::vector<permissions::scope> grants;	 // what backends it may touch
	bool enabled{true};
};