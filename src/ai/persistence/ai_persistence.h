#pragma once
#include <string>

/// Serializes/deserializes the entire agent_registry to/from a JSON file.
/// Returns true on success. On failure, 'error' is populated.
struct ai_persistence
{
	static bool save(const std::string &path, std::string &error);
	static bool load(const std::string &path, std::string &error);
};