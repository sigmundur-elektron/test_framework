#include "setup_service.h"
#include "../ai/agent/agent_registry.h"
#include "../ai/persistence/ai_persistence.h"
#include "agent_service.h"
#include "repository/i_repository.h"
#include "repository/repository_provider.h"

namespace features
{
setup_service &setup_service::instance()
{
	static setup_service s;
	return s;
}

bool setup_service::export_json(const std::string &path, std::string &status)
{
	std::string err;
	if (ai_persistence::save(path, err))
	{
		status = "Exported to '" + path + "'.";
		return true;
	}
	status = "Export failed: " + err;
	return false;
}

bool setup_service::import_json(const std::string &path, std::string &status)
{
	std::string err;
	if (!ai_persistence::load(path, err))
	{
		status = "Import failed: " + err;
		return false;
	}

	// ai_persistence::load populated the runtime registry; mirror everything
	// into the durable repository so the imported setup persists.
	auto &repo = repository_provider::instance().repo();
	for (agent *a : agent_registry::get_instance().all())
	{
		agent_record rec;
		rec.config = a->config();
		rec.history = a->get_memory().history();
		std::string ignore;
		repo.upsert_agent(rec, ignore);
	}

	status = "Imported from '" + path + "'.";
	return true;
}
} // namespace features
