#pragma once
#include "i_repository.h"
#include <memory>

namespace features
{
/// PostgreSQL-backed repository (libpqxx). Connects on startup, ensures the
/// schema exists, and persists agents + console logs durably.
///
/// Connection string resolution (first non-empty wins):
///   1. explicit ctor argument
///   2. environment variable TF_PG_CONN
///   3. built-in default (localhost / test_framework db)
class postgres_repository : public i_repository
{
  public:
	explicit postgres_repository(std::string conn_string = {});
	~postgres_repository() override;

	bool connect(std::string &error) override;
	void disconnect() override;
	bool is_durable() const override;

	bool upsert_agent(const agent_record &record, std::string &error) override;
	bool remove_agent(const std::string &name, std::string &error) override;
	bool load_agents(std::vector<agent_record> &out, std::string &error) override;

	bool append_log(const log_record &record, std::string &error) override;
	bool load_logs(std::vector<log_record> &out, std::string &error) override;

  private:
	struct impl;			   // pimpl hides libpqxx from callers
	std::unique_ptr<impl> _p;  // owns the connection
	std::string _conn_string;
	bool _connected{false};

	static std::string resolve_conn_string(const std::string &explicit_conn);
};
} // namespace features
