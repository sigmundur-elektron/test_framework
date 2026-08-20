#pragma once
#include "../../ai/agent/agent_config.h"
#include "../../ai/agent/memory.h"
#include <string>
#include <vector>

namespace features
{
/// One persisted console log line.
struct log_record
{
	std::string tag;	  // e.g. "info", "agent", "error"
	std::string message;  // the log text
	std::string ts;		  // ISO-8601 timestamp (empty = set by repo)
};

/// One persisted agent: its runtime config plus its full memory history.
struct agent_record
{
	agent_config config;
	std::vector<memory::entry> history;
};

/// Repository abstraction — the ONLY seam through which services read/write
/// durable state. Implemented by postgres_repository (real DB) and
/// in_memory_repository (fallback / no DB available).
///
/// Contract: methods return false and populate 'error' on failure; on a
/// healthy connection they persist immediately.
struct i_repository
{
	virtual ~i_repository() = default;

	/// Establish the backing store and ensure schema exists.
	virtual bool connect(std::string &error) = 0;
	/// Flush and release the backing store.
	virtual void disconnect() = 0;
	/// True when a real durable backend is active (false for in-memory).
	virtual bool is_durable() const = 0;

	// --- Agents (config + memory history) ---
	virtual bool upsert_agent(const agent_record &record, std::string &error) = 0;
	virtual bool remove_agent(const std::string &name, std::string &error) = 0;
	virtual bool load_agents(std::vector<agent_record> &out, std::string &error) = 0;

	// --- Console logs ---
	virtual bool append_log(const log_record &record, std::string &error) = 0;
	virtual bool load_logs(std::vector<log_record> &out, std::string &error) = 0;
};
} // namespace features
