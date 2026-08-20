#pragma once
#include "i_repository.h"
#include <memory>
#include <string>

namespace features
{
/// Owns the active repository instance for the application's lifetime.
/// On start() it attempts a durable PostgreSQL connection and falls back to a
/// non-durable in-memory repository if the DB is unavailable, so the app keeps
/// running. Constructed/destroyed alongside the app (see app::init/end).
class repository_provider
{
  public:
	/// Global accessor. The instance must be started once during app init.
	static repository_provider &instance();

	/// Connect the durable backend (or fall back). 'status' receives a
	/// human-readable summary suitable for logging. Returns true if a durable
	/// backend is active, false if it fell back to in-memory.
	bool start(std::string &status);

	/// Tear down the active backend (called on app shutdown).
	void stop();

	/// The active repository. Never null after start().
	i_repository &repo();

  private:
	repository_provider() = default;
	std::unique_ptr<i_repository> _repo;
};
} // namespace features
