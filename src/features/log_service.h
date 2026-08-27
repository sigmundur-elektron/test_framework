#pragma once
#include <string>

namespace features
{
/// Persists console log lines to the repository. The console (UI/util layer)
/// forwards lines here; it never touches the repository directly.
class log_service
{
  public:
	static log_service &instance();

	/// Append one log line. Best-effort: failures are swallowed so logging
	/// never disrupts the UI.
	void append(const std::string &tag, const std::string &message);

  private:
	log_service() = default;
};
} // namespace features
