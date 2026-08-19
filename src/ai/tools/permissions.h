#pragma once
#include <string>

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

	bool allowed(scope s) const;
};