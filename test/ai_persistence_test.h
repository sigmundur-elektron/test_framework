#pragma once
#include <doctest/doctest.h>
#include "../src/data/agent/agent_registry.h"
#include "../src/data/persistence/ai_persistence.h"
#include <cstdio>

TEST_CASE("[ai] setup saves and loads agents + memory")
{
	auto &registry = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "persist_agent";
	cfg.model_endpoint = "http://localhost:1234/a2a";
	cfg.peer_agents = {"other"};

	agent *a = registry.create(cfg);
	REQUIRE(a != nullptr);
	a->mutable_memory().add("user", "remember this");

	const std::string path = "ai_setup_test.json";
	std::string err;
	REQUIRE(ai_persistence::save(path, err));

	// Wipe and reload.
	registry.remove("persist_agent");
	REQUIRE(registry.find("persist_agent") == nullptr);

	REQUIRE(ai_persistence::load(path, err));

	agent *restored = registry.find("persist_agent");
	REQUIRE(restored != nullptr);
	CHECK(restored->config().model_endpoint == "http://localhost:1234/a2a");
	REQUIRE(restored->get_memory().history().size() == 1);
	CHECK(restored->get_memory().history()[0].content == "remember this");

	registry.remove("persist_agent");
	std::remove(path.c_str());
}