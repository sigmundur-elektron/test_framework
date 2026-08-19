#pragma once
#include "../../utils/singleton.h"
#include "itool.h"
#include <memory>
#include <unordered_map>
#include <vector>

/// Tool Registry
///
/// Central catalog of tools available to the agent.
/// MCP-sourced tools register here alongside local tools.
struct tool_registry : public singleton<tool_registry>
{
  public:
	tool_registry(token){};

	void register_tool(std::unique_ptr<itool> tool);
	itool *find(const std::string &name) const;
	std::vector<itool *> all() const;

  private:
	std::unordered_map<std::string, std::unique_ptr<itool>> _tools;
};