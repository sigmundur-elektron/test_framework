#include "github_api.h"

// TODO: wire to a real HTTP/GitHub client.
std::vector<std::string> github_api::list_issues() const
{
	return {};
}

std::string github_api::get_issue(int /*number*/) const
{
	return {};
}

void github_api::create_issue(const std::string & /*title*/, const std::string & /*body*/)
{
}