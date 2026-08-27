#include "postgres_repository.h"
#include "../ai/persistence/ai_setup.h" // glz::meta<permissions::scope>
#include <cstdlib>
#include <glaze/glaze.hpp>
#include <pqxx/pqxx>

namespace features
{
struct postgres_repository::impl
{
	std::unique_ptr<pqxx::connection> conn;
};

postgres_repository::postgres_repository(std::string conn_string)
	: _p(std::make_unique<impl>()),
	  _conn_string(resolve_conn_string(conn_string))
{
}

postgres_repository::~postgres_repository() { disconnect(); }

std::string postgres_repository::resolve_conn_string(const std::string &explicit_conn)
{
	if (!explicit_conn.empty())
		return explicit_conn;
	if (const char *env = std::getenv("TF_PG_CONN"); env && *env)
		return env;
	return "host=localhost port=5432 dbname=test_framework "
		   "user=postgres password=postgres";
}

bool postgres_repository::is_durable() const { return _connected; }

bool postgres_repository::connect(std::string &error)
{
	try
	{
		_p->conn = std::make_unique<pqxx::connection>(_conn_string);
		if (!_p->conn->is_open())
		{
			error = "postgres connection is not open";
			return false;
		}

		pqxx::work tx{*_p->conn};
		tx.exec(
			"CREATE TABLE IF NOT EXISTS agents ("
			"  name TEXT PRIMARY KEY,"
			"  config JSONB NOT NULL,"
			"  history JSONB NOT NULL,"
			"  updated_at TIMESTAMPTZ NOT NULL DEFAULT now()"
			")");
		tx.exec(
			"CREATE TABLE IF NOT EXISTS console_logs ("
			"  id BIGSERIAL PRIMARY KEY,"
			"  tag TEXT NOT NULL,"
			"  message TEXT NOT NULL,"
			"  ts TIMESTAMPTZ NOT NULL DEFAULT now()"
			")");
		tx.commit();

		_connected = true;
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		_connected = false;
		return false;
	}
}

void postgres_repository::disconnect()
{
	_connected = false;
	if (_p)
		_p->conn.reset();
}

bool postgres_repository::upsert_agent(const agent_record &record, std::string &error)
{
	if (!_connected)
	{
		error = "not connected";
		return false;
	}
	try
	{
		std::string config_json;
		std::string history_json;
		if (auto ec = glz::write_json(record.config, config_json))
		{
			error = "serialize config failed";
			return false;
		}
		if (auto ec = glz::write_json(record.history, history_json))
		{
			error = "serialize history failed";
			return false;
		}

		pqxx::work tx{*_p->conn};
		tx.exec(
			pqxx::zview{
				"INSERT INTO agents (name, config, history, updated_at) "
				"VALUES ($1, $2::jsonb, $3::jsonb, now()) "
				"ON CONFLICT (name) DO UPDATE SET "
				"config = EXCLUDED.config, history = EXCLUDED.history, "
				"updated_at = now()"},
			pqxx::params{record.config.name, config_json, history_json});
		tx.commit();
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		return false;
	}
}

bool postgres_repository::remove_agent(const std::string &name, std::string &error)
{
	if (!_connected)
	{
		error = "not connected";
		return false;
	}
	try
	{
		pqxx::work tx{*_p->conn};
		tx.exec(pqxx::zview{"DELETE FROM agents WHERE name = $1"},
				pqxx::params{name});
		tx.commit();
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		return false;
	}
}

bool postgres_repository::load_agents(std::vector<agent_record> &out, std::string &error)
{
	if (!_connected)
	{
		error = "not connected";
		return false;
	}
	try
	{
		out.clear();
		pqxx::work tx{*_p->conn};
		pqxx::result rows = tx.exec("SELECT config, history FROM agents");
		tx.commit();

		for (const auto &row : rows)
		{
			agent_record rec;
			const std::string config_json = row[0].as<std::string>();
			const std::string history_json = row[1].as<std::string>();
			if (glz::read_json(rec.config, config_json))
				continue; // skip malformed rows rather than abort
			(void)glz::read_json(rec.history, history_json);
			out.push_back(std::move(rec));
		}
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		return false;
	}
}

bool postgres_repository::append_log(const log_record &record, std::string &error)
{
	if (!_connected)
	{
		error = "not connected";
		return false;
	}
	try
	{
		pqxx::work tx{*_p->conn};
		if (record.ts.empty())
		{
			tx.exec(pqxx::zview{"INSERT INTO console_logs (tag, message) "
								"VALUES ($1, $2)"},
					pqxx::params{record.tag, record.message});
		}
		else
		{
			tx.exec(pqxx::zview{"INSERT INTO console_logs (tag, message, ts) "
								"VALUES ($1, $2, $3::timestamptz)"},
					pqxx::params{record.tag, record.message, record.ts});
		}
		tx.commit();
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		return false;
	}
}

bool postgres_repository::load_logs(std::vector<log_record> &out, std::string &error)
{
	if (!_connected)
	{
		error = "not connected";
		return false;
	}
	try
	{
		out.clear();
		pqxx::work tx{*_p->conn};
		pqxx::result rows = tx.exec(
			"SELECT tag, message, ts::text FROM console_logs ORDER BY id");
		tx.commit();

		for (const auto &row : rows)
			out.push_back(log_record{row[0].as<std::string>(),
									 row[1].as<std::string>(),
									 row[2].as<std::string>()});
		return true;
	}
	catch (const std::exception &e)
	{
		error = e.what();
		return false;
	}
}
} // namespace features
