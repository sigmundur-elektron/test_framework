#include "agent_export.h"
#include "agent_service.h"
#include "../data/agent/agent.h"
#include "../data/tools/tool_registry.h"
#include <ctime>
#include <unordered_set>

namespace features
{

std::string permission_scope_id(permissions::scope s)
{
	switch (s)
	{
	case permissions::scope::read_project:
		return "read_project";
	case permissions::scope::write_project:
		return "write_project";
	case permissions::scope::read_github:
		return "read_github";
	case permissions::scope::write_github:
		return "write_github";
	case permissions::scope::read_database:
		return "read_database";
	case permissions::scope::write_database:
		return "write_database";
	}
	return "unknown";
}

static std::string utc_timestamp()
{
	std::time_t now = std::time(nullptr);
	std::tm tm_utc{};
#if defined(_WIN32)
	gmtime_s(&tm_utc, &now);
#else
	gmtime_r(&now, &tm_utc);
#endif
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
	return buf;
}

export_document build_export_document()
{
	export_document doc;
	doc.exported_at = utc_timestamp();

	// Shared tool catalog (referenced by agents). Every local tool the
	// registry knows about is exported so the consumer can validate calls.
	std::vector<std::string> all_tool_ids;
	for (itool *tool : tool_registry::get_instance().all())
	{
		if (!tool)
			continue;
		export_tool_binding binding;
		binding.id = tool->name();
		binding.name = tool->name();
		binding.kind = "local";
		binding.schema = glz::raw_json{tool->schema()};
		doc.tools.push_back(std::move(binding));
		all_tool_ids.push_back(tool->name());
	}

	std::unordered_set<std::string> permission_ids;

	for (agent *a : agent_service::instance().all())
	{
		if (!a)
			continue;
		const agent_config &cfg = a->config();

		export_agent ea;
		ea.id = cfg.name;
		ea.name = cfg.name;
		ea.endpoint = cfg.model_endpoint;
		ea.enabled = cfg.enabled;
		ea.peers = cfg.peer_agents;
		// Every agent may reference the shared tool catalog.
		ea.tools = all_tool_ids;

		for (permissions::scope grant : cfg.grants)
		{
			std::string id = permission_scope_id(grant);
			ea.permissions.push_back(id);
			permission_ids.insert(id);
		}

		for (const memory::entry &e : a->get_memory().history())
			ea.memory.push_back({e.role, e.content});

		// No persisted authored plans in v1; left empty by design.
		doc.agents.push_back(std::move(ea));
	}

	// Build the permission catalog from every id referenced by an agent so the
	// consumer can enforce authority independently.
	for (const std::string &id : permission_ids)
	{
		export_permission perm;
		perm.id = id;
		perm.description = "Grant '" + id + "' from agent_config.";
		doc.permissions.push_back(std::move(perm));
	}

	return doc;
}

std::string export_document_to_json(const export_document &doc, std::string &error)
{
	std::string out;
	auto ec = glz::write<glz::opts{.prettify = true}>(doc, out);
	if (ec)
	{
		error = "failed to serialize export document";
		return {};
	}
	return out;
}

} // namespace features
