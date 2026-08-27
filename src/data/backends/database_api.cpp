#include "database_api.h"

// TODO: wire to a real DB driver.
std::vector<std::string> database_api::query(const std::string & /*sql*/) const
{
	return {};
}

void database_api::execute(const std::string & /*sql*/)
{
}