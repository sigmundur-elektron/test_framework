#include "permissions.h"
#include <algorithm>

bool permissions::allowed(scope s) const
{
	// Flat allow-list, deny by default. No wildcards, no hierarchy, no
	// deny-entries: a scope is permitted only if it was explicitly granted.
	return std::find(_granted.begin(), _granted.end(), s) != _granted.end();
}