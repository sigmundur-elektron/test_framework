#pragma once
#include "../data/tools/permissions.h"
#include <string>
#include <vector>

struct agent; // fwd

/// "Save as template": persist an agent's *reusable* configuration so a new
/// agent can be created from it later. A template intentionally omits the
/// agent's unique name and its memory — only the reusable shape is kept
/// (endpoint, permission grants, peer links). Serialized as versioned JSON via
/// Glaze, mirroring the rest of the persistence layer.
///
/// See docs/notes/decisions.md D-004 and open question Q13.
namespace features
{

/// One reusable agent template.
struct agent_template
{
	int version{1};						   // schema version for the template file
	std::string label;					   // human-facing template name
	std::string endpoint;				   // agent_config.model_endpoint
	std::vector<std::string> permissions;  // permission ids (from grants)
	std::vector<std::string> peers;		   // default A2A peer ids
};

/// Collection of templates as persisted on disk.
struct agent_template_book
{
	int version{1};
	std::vector<agent_template> templates;
};

/// Manages saving agents as templates and creating agents from them. The UI
/// talks to this service; it never touches persistence directly.
class template_service
{
  public:
	static template_service &instance();

	/// Default on-disk location for the template book.
	static std::string default_path();

	/// Build a template from a live agent (does NOT persist by itself).
	static agent_template from_agent(const agent &a, const std::string &label);

	/// Save 'tmpl' into the template book at 'path' (creating/appending).
	/// Returns false and populates 'error' on failure.
	bool save_template(const std::string &path, const agent_template &tmpl,
					   std::string &error);

	/// Convenience: build a template from a live agent and persist it.
	bool save_agent_as_template(const std::string &path, const agent &a,
								const std::string &label, std::string &error);

	/// Load all templates from 'path'. Missing file yields an empty book (no
	/// error). Returns false only on a genuine read/parse failure.
	bool load_templates(const std::string &path, agent_template_book &book,
						std::string &error);

	/// Create a new live agent named 'name' from 'tmpl' (endpoint, grants,
	/// peers), persisting it through the repository. Returns the new agent, or
	/// nullptr on failure (name empty/taken), with 'error' populated.
	agent *create_from_template(const std::string &name, const agent_template &tmpl,
								std::string &error);

  private:
	template_service() = default;
};
} // namespace features
