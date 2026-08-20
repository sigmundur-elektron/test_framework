#include "agent_service.h"
#include "../ai/agent/agent_registry.h"
#include "repository/i_repository.h"
#include "repository/repository_provider.h"
#include <algorithm>

namespace features
{
agent_service &agent_service::instance()
{
	static agent_service s;
	return s;
}

static agent_registry &registry() { return agent_registry::get_instance(); }

void agent_service::persist(agent *a)
{
	if (!a)
		return;
	agent_record rec;
	rec.config = a->config();
	rec.history = a->get_memory().history();
	std::string error;
	repository_provider::instance().repo().upsert_agent(rec, error);
}

void agent_service::load_from_repository()
{
	std::vector<agent_record> records;
	std::string error;
	if (!repository_provider::instance().repo().load_agents(records, error))
		return;

	for (auto &rec : records)
	{
		registry().remove(rec.config.name); // replace if present
		if (agent *a = registry().create(rec.config))
			a->mutable_memory().load(std::move(rec.history));
	}
}

std::vector<agent *> agent_service::all() const { return registry().all(); }

agent *agent_service::create(const std::string &name, const std::string &endpoint)
{
	agent_config cfg;
	cfg.name = name;
	cfg.model_endpoint = endpoint;
	agent *a = registry().create(cfg); // nullptr if empty/taken
	if (a)
		persist(a);
	return a;
}

void agent_service::remove(const std::string &name)
{
	registry().remove(name);
	std::string error;
	repository_provider::instance().repo().remove_agent(name, error);
}

void agent_service::set_enabled(const std::string &name, bool enabled)
{
	agent *a = registry().find(name);
	if (!a)
		return;
	a->mutable_config().enabled = enabled;
	persist(a);
}

void agent_service::set_peer_linked(const std::string &name, const std::string &peer,
									bool linked)
{
	agent *a = registry().find(name);
	if (!a)
		return;
	auto &peers = a->mutable_config().peer_agents;
	if (linked)
	{
		if (std::find(peers.begin(), peers.end(), peer) == peers.end())
			peers.push_back(peer);
	}
	else
	{
		std::erase(peers, peer);
	}
	persist(a);
}

std::vector<plan_step> agent_service::preview(const std::string &name,
											  const std::string &goal) const
{
	agent *a = registry().find(name);
	return a ? a->preview(goal) : std::vector<plan_step>{};
}

std::string agent_service::run(const std::string &name, const std::string &goal)
{
	agent *a = registry().find(name);
	if (!a)
		return "[unknown agent '" + name + "']";
	std::string transcript = a->handle(goal);
	persist(a); // memory changed — persist the new history
	return transcript;
}
} // namespace features
