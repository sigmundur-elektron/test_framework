#pragma once
#include <string>

namespace features
{
/// Import/export of the full agent setup as JSON files. The JSON path is now
/// used ONLY for portable export/import; the repository remains the durable
/// source of truth. The UI talks to this service, never to ai_persistence.
class setup_service
{
  public:
	static setup_service &instance();

	/// Export the current runtime setup to a JSON file.
	/// Returns true on success; 'status' receives a user-facing message.
	bool export_json(const std::string &path, std::string &status);

	/// Import a setup from a JSON file into the runtime registry AND persist
	/// it into the repository. Returns true on success.
	bool import_json(const std::string &path, std::string &status);

  private:
	setup_service() = default;
};
} // namespace features
