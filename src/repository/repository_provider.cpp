#include "repository_provider.h"
#include "in_memory_repository.h"
#ifdef TF_POSTGRES_ENABLED
#include "postgres_repository.h"
#endif

namespace features
{
repository_provider &repository_provider::instance()
{
	static repository_provider p;
	return p;
}

bool repository_provider::start(std::string &status)
{
#ifdef TF_POSTGRES_ENABLED
	auto pg = std::make_unique<postgres_repository>();
	std::string error;
	if (pg->connect(error))
	{
		_repo = std::move(pg);
		status = "connected to PostgreSQL";
		return true;
	}

	// DB unavailable — degrade gracefully so the app still runs.
	auto mem = std::make_unique<in_memory_repository>();
	std::string ignore;
	mem->connect(ignore);
	_repo = std::move(mem);
	status = "PostgreSQL unavailable (" + error + "); using in-memory store";
	return false;
#else
	// Postgres backend not compiled in — use the in-memory store.
	auto mem = std::make_unique<in_memory_repository>();
	std::string ignore;
	mem->connect(ignore);
	_repo = std::move(mem);
	status = "PostgreSQL backend not built; using in-memory store";
	return false;
#endif
}

void repository_provider::stop()
{
	if (_repo)
	{
		_repo->disconnect();
		_repo.reset();
	}
}

i_repository &repository_provider::repo()
{
	if (!_repo)
	{
		// Safety net: if start() was never called, provide a fallback so
		// callers never dereference null.
		_repo = std::make_unique<in_memory_repository>();
		std::string ignore;
		_repo->connect(ignore);
	}
	return *_repo;
}
} // namespace features
