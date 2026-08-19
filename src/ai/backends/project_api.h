#pragma once
#include <string>
#include <vector>

/// Project Backend API
///
/// Plain application API for project/workspace operations.
/// Contains NO AI awareness. Reusable and independently testable.
struct project_api
{
	std::vector<std::string> list_files() const;
	std::string read_file(const std::string &path) const;
	void write_file(const std::string &path, const std::string &content);
};