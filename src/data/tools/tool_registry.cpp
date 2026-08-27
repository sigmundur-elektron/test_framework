#include "tool_registry.h"

void tool_registry::register_tool(std::unique_ptr<itool> tool)
{
	if (!tool)
		return;
	_tools[tool->name()] = std::move(tool);
}

itool *tool_registry::find(const std::string &name) const
{
	auto it = _tools.find(name);
	return it == _tools.end() ? nullptr : it->second.get();
}

std::vector<itool *> tool_registry::all() const
{
	std::vector<itool *> out;
	out.reserve(_tools.size());
	for (const auto &[_, tool] : _tools)
		out.push_back(tool.get());
	return out;
}