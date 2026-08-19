#include "agent_registry.h"

agent *agent_registry::create(const agent_config &config)
{
	if (config.name.empty() || _agents.contains(config.name))
		return nullptr;

	auto created = std::make_unique<agent>(config);
	created->init();
	agent *ptr = created.get();
	_agents[config.name] = std::move(created);
	return ptr;
}

agent *agent_registry::find(const std::string &name) const
{
	auto it = _agents.find(name);
	return it == _agents.end() ? nullptr : it->second.get();
}

void agent_registry::remove(const std::string &name)
{
	if (auto it = _agents.find(name); it != _agents.end())
	{
		it->second->end();
		_agents.erase(it);
	}
}

std::vector<agent *> agent_registry::all() const
{
	std::vector<agent *> out;
	out.reserve(_agents.size());
	for (const auto &[_, a] : _agents)
		out.push_back(a.get());
	return out;
}