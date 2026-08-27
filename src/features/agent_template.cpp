#include "agent_template.h"
#include "agent_export.h" // permission_scope_id
#include "agent_service.h"
#include "../data/agent/agent.h"
#include "../data/agent/agent_registry.h"
#include <glaze/glaze.hpp>
#include <fstream>

namespace features
{

// Declared in agent_export.cpp; reused here to keep one mapping source.
std::string permission_scope_id(permissions::scope s);

// Reverse of permission_scope_id: id string -> scope. Returns false if unknown.
static bool permission_scope_from_id(const std::string &id, permissions::scope &out)
{
	using scope = permissions::scope;
	static const std::pair<const char *, scope> table[] = {
		{"read_project", scope::read_project},
		{"write_project", scope::write_project},
		{"read_github", scope::read_github},
		{"write_github", scope::write_github},
		{"read_database", scope::read_database},
		{"write_database", scope::write_database},
	};
	for (const auto &[name, value] : table)
		if (id == name)
		{
			out = value;
			return true;
		}
	return false;
}

template_service &template_service::instance()
{
	static template_service s;
	return s;
}

std::string template_service::default_path() { return "agent_templates.json"; }

agent_template template_service::from_agent(const agent &a, const std::string &label)
{
	const agent_config &cfg = a.config();
	agent_template tmpl;
	tmpl.label = label.empty() ? cfg.name : label;
	tmpl.endpoint = cfg.model_endpoint;
	tmpl.peers = cfg.peer_agents;
	for (permissions::scope grant : cfg.grants)
		tmpl.permissions.push_back(permission_scope_id(grant));
	return tmpl;
}

bool template_service::load_templates(const std::string &path,
									  agent_template_book &book, std::string &error)
{
	std::ifstream in(path, std::ios::binary);
	if (!in)
	{
		// Missing file is not an error — it is simply an empty book.
		book = agent_template_book{};
		return true;
	}
	std::string content((std::istreambuf_iterator<char>(in)),
						std::istreambuf_iterator<char>());
	auto ec = glz::read_json(book, content);
	if (ec)
	{
		error = "failed to parse templates in '" + path + "'";
		return false;
	}
	return true;
}

bool template_service::save_template(const std::string &path,
									 const agent_template &tmpl, std::string &error)
{
	agent_template_book book;
	if (!load_templates(path, book, error))
		return false;

	// Replace a template with the same label, otherwise append.
	bool replaced = false;
	for (agent_template &existing : book.templates)
	{
		if (existing.label == tmpl.label)
		{
			existing = tmpl;
			replaced = true;
			break;
		}
	}
	if (!replaced)
		book.templates.push_back(tmpl);

	std::string json;
	auto ec = glz::write<glz::opts{.prettify = true}>(book, json);
	if (ec)
	{
		error = "failed to serialize templates";
		return false;
	}

	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	if (!out)
	{
		error = "failed to open '" + path + "' for writing";
		return false;
	}
	out << json;
	return true;
}

bool template_service::save_agent_as_template(const std::string &path, const agent &a,
											  const std::string &label,
											  std::string &error)
{
	return save_template(path, from_agent(a, label), error);
}

agent *template_service::create_from_template(const std::string &name,
											  const agent_template &tmpl,
											  std::string &error)
{
	if (name.empty())
	{
		error = "agent name is empty";
		return nullptr;
	}

	agent *a = agent_service::instance().create(name, tmpl.endpoint);
	if (!a)
	{
		error = "agent name already exists or could not be created";
		return nullptr;
	}

	// Apply reusable configuration from the template.
	agent_config &cfg = a->mutable_config();
	cfg.peer_agents = tmpl.peers;
	cfg.grants.clear();
	for (const std::string &id : tmpl.permissions)
	{
		permissions::scope scope;
		if (permission_scope_from_id(id, scope))
			cfg.grants.push_back(scope);
	}

	// Persist the enriched config through the service (peer link path persists).
	agent_service::instance().set_enabled(name, cfg.enabled);
	return a;
}

} // namespace features
