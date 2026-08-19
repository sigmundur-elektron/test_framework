#pragma once
#include "../../utils/singleton.h"
#include "agent.h"
#include "agent_config.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// Agent Registry
///
/// Central catalog of live agents. Created/edited/removed at runtime from the GUI.
struct agent_registry : public singleton<agent_registry>
{
  public:
	agent_registry(token){};

	agent *create(const agent_config &config); // returns nullptr if name taken
	agent *find(const std::string &name) const;
	void remove(const std::string &name);
	std::vector<agent *> all() const;

  private:
	std::unordered_map<std::string, std::unique_ptr<agent>> _agents;
};