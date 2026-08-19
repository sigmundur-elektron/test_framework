#pragma once
#include <doctest/doctest.h>
#include "../src/ai/agent/agent_registry.h"
#include "../src/ai/tools/agent_call_tool.h"
#include "../src/ai/tools/tool_registry.h"
#include <memory>

TEST_CASE("[ai] agent_registry creates and finds agents at runtime")
{
	auto &agents = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "worker_a";
	REQUIRE(agents.create(cfg) != nullptr);
	CHECK(agents.find("worker_a") != nullptr);

	// Duplicate name rejected.
	CHECK(agents.create(cfg) == nullptr);

	agents.remove("worker_a");
	CHECK(agents.find("worker_a") == nullptr);
}

TEST_CASE("[ai] A2A tool delegates to a peer agent")
{
	auto &agents = agent_registry::get_instance();
	agent_config peer;
	peer.name = "peer_agent";
	agents.create(peer);

	agent_call_tool tool;
	auto ok = tool.execute(R"({ "agent": "peer_agent", "goal": "hello" })");
	REQUIRE(ok.has_value());
	CHECK(ok->success);

	auto missing = tool.execute(R"({ "agent": "does_not_exist", "goal": "x" })");
	CHECK_FALSE(missing.has_value());

	agents.remove("peer_agent");
}