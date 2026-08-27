#include "log_service.h"
#include "../repository/i_repository.h"
#include "../repository/repository_provider.h"

namespace features
{
log_service &log_service::instance()
{
	static log_service s;
	return s;
}

void log_service::append(const std::string &tag, const std::string &message)
{
	log_record rec;
	rec.tag = tag;
	rec.message = message;
	std::string ignore;
	repository_provider::instance().repo().append_log(rec, ignore);
}
} // namespace features
