#pragma once
#include <string>
#include <vector>

/// Database Backend API (stub).
struct database_api
{
	std::vector<std::string> query(const std::string &sql) const;
	void execute(const std::string &sql);
};