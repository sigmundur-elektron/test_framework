#pragma once
#include "../data/tools/permissions.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

/// Portable, self-describing export of an agent setup (schema "tf.agent-export").
///
/// Frozen as v1 under tracker task T-007; see docs/plans/export-format.md.
/// This is a one-way JSON snapshot (Glaze) intended to be consumed by an
/// external "open code" agent. Backend endpoints (e.g. database) are NOT part
/// of the MVP export.
namespace features
{

/// Where the export came from.
struct export_source
{
	std::string app{"test_framework"};
	std::string app_version{"0.1.0"};
};

/// One agent as it appears in the export.
struct export_agent
{
	std::string id;								 // unique id (== name)
	std::string name;							 // display name
	std::string endpoint;						 // from agent_config.model_endpoint
	bool enabled{true};
	std::vector<std::string> permissions;		 // permission ids (from grants)
	std::vector<std::string> peers;				 // A2A links (agent ids)
	std::vector<std::string> tools;				 // tool ids it may use
	std::vector<std::string> plans;				 // plan ids owned by this agent
	std::vector<std::vector<std::string>> memory; // [role, content] pairs
};

/// A tool the consumer may call, derived from itool name + schema.
struct export_tool_binding
{
	std::string id;						  // tool id (== name)
	std::string name;					  // tool name
	std::string kind{"local"};			  // local | mcp | a2a
	glz::raw_json schema{"{}"};			  // argument schema from itool.schema()
	std::vector<std::string> requires_permissions;
};

/// A permission the consumer can enforce independently of the app.
struct export_permission
{
	std::string id;
	std::string description;
};

/// A single step of an exported plan (mirrors plan_step).
struct export_plan_step
{
	int index{0};
	std::string tool;			// ToolBinding id
	glz::raw_json args{"{}"};	// step arguments
};

/// An authored plan, referenced by agents.
struct export_plan
{
	std::string id;
	std::string goal;
	std::vector<export_plan_step> steps;
};

/// Root export document.
struct export_document
{
	std::string schema{"tf.agent-export"};
	int version{1};
	std::string exported_at;
	export_source source;
	std::vector<export_agent> agents;
	std::vector<export_tool_binding> tools;
	std::vector<export_permission> permissions;
	std::vector<export_plan> plans;
};

/// Builds an export_document from the live agent_service + tool_registry state.
export_document build_export_document();

/// Serializes an export_document to prettified JSON. Empty + 'error' on failure.
std::string export_document_to_json(const export_document &doc, std::string &error);

/// Maps a permission scope to its stable string id (matches the Glaze enum).
std::string permission_scope_id(permissions::scope s);

} // namespace features
