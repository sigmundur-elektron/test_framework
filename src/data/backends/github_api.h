#pragma once
#include <string>
#include <vector>

/// GitHub Backend API (stub).
struct github_api
{
	std::vector<std::string> list_issues() const;
	std::string get_issue(int number) const;
	void create_issue(const std::string &title, const std::string &body);
};