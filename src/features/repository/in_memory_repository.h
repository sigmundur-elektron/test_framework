#pragma once
#include "i_repository.h"
#include <unordered_map>

namespace features
{
/// Non-durable repository used when no PostgreSQL connection is available.
/// Keeps everything in RAM so the application still builds and runs.
class in_memory_repository : public i_repository
{
  public:
	bool connect(std::string & /*error*/) override { return true; }
	void disconnect() override {}
	bool is_durable() const override { return false; }

	bool upsert_agent(const agent_record &record, std::string & /*error*/) override
	{
		_agents[record.config.name] = record;
		return true;
	}

	bool remove_agent(const std::string &name, std::string & /*error*/) override
	{
		_agents.erase(name);
		return true;
	}

	bool load_agents(std::vector<agent_record> &out, std::string & /*error*/) override
	{
		out.clear();
		out.reserve(_agents.size());
		for (const auto &[_, rec] : _agents)
			out.push_back(rec);
		return true;
	}

	bool append_log(const log_record &record, std::string & /*error*/) override
	{
		_logs.push_back(record);
		return true;
	}

	bool load_logs(std::vector<log_record> &out, std::string & /*error*/) override
	{
		out = _logs;
		return true;
	}

  private:
	std::unordered_map<std::string, agent_record> _agents;
	std::vector<log_record> _logs;
};
} // namespace features
