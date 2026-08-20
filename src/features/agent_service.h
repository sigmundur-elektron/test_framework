#pragma once
#include "../ai/agent/agent_config.h"
#include "../ai/agent/planner.h"
#include <string>
#include <vector>

struct agent; // fwd

namespace features
{
/// Application logic for managing agents. The UI talks ONLY to this service;
/// it never touches agent_registry or the repository directly.
///
/// Keeps the in-RAM agent_registry (runtime catalog) in sync with the durable
/// repository (source of truth).
class agent_service
{
  public:
	static agent_service &instance();

	/// Load persisted agents from the repository into the runtime registry.
	/// Call once during app startup.
	void load_from_repository();

	// --- Queries ---
	std::vector<agent *> all() const;

	// --- Commands (each persists through the repository) ---
	/// Returns nullptr if the name is empty or already taken.
	agent *create(const std::string &name, const std::string &endpoint);
	void remove(const std::string &name);
	void set_enabled(const std::string &name, bool enabled);
	void set_peer_linked(const std::string &name, const std::string &peer, bool linked);

	// --- Planner / execution ---
	std::vector<plan_step> preview(const std::string &name, const std::string &goal) const;
	std::string run(const std::string &name, const std::string &goal);

  private:
	agent_service() = default;
	void persist(agent *a); // upsert one agent into the repository
};
} // namespace features
