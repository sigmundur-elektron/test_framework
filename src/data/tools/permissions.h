#pragma once
#include <string>
#include <vector>

/// Permission checks live in the tool layer so the agent cannot bypass them.
struct permissions
{
	enum class scope
	{
		read_project,
		write_project,
		read_github,
		write_github,
		read_database,
		write_database
	};

	/// Grants nothing. Denial is the default so that a tool reached without a
	/// resolved grant set fails closed rather than open.
	permissions() = default;

	/// Grants exactly the listed scopes. Typically built from
	/// agent_config::grants at the point of tool dispatch.
	explicit permissions(std::vector<scope> granted)
	  : _granted(std::move(granted))
	{
	}

	bool allowed(scope s) const;

  private:
	std::vector<scope> _granted;
};