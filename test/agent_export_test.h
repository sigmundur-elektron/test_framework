#pragma once
#include <doctest/doctest.h>
#include "../src/data/agent/agent_registry.h"
#include "../src/features/agent_export.h"
#include "../src/features/agent_service.h"
#include <glaze/glaze.hpp>
#include <cstdio>

TEST_CASE("[export] builds a tf.agent-export document from live agents")
{
	auto &registry = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "export_agent";
	cfg.model_endpoint = "http://localhost:9000/a2a";
	cfg.peer_agents = {"peer_agent"};
	cfg.grants = {permissions::scope::read_project};

	agent *a = registry.create(cfg);
	REQUIRE(a != nullptr);
	a->mutable_memory().add("user", "hello export");

	features::export_document doc = features::build_export_document();

	CHECK(doc.schema == "tf.agent-export");
	CHECK(doc.version == 1);
	CHECK_FALSE(doc.exported_at.empty());

	// Find our agent in the document.
	const features::export_agent *found = nullptr;
	for (const auto &ea : doc.agents)
		if (ea.id == "export_agent")
			found = &ea;
	REQUIRE(found != nullptr);
	CHECK(found->endpoint == "http://localhost:9000/a2a");
	REQUIRE(found->peers.size() == 1);
	CHECK(found->peers[0] == "peer_agent");
	REQUIRE(found->permissions.size() == 1);
	CHECK(found->permissions[0] == "read_project");
	REQUIRE(found->memory.size() == 1);
	CHECK(found->memory[0][0] == "user");
	CHECK(found->memory[0][1] == "hello export");

	// Permission catalog contains the referenced grant.
	bool has_perm = false;
	for (const auto &p : doc.permissions)
		if (p.id == "read_project")
			has_perm = true;
	CHECK(has_perm);

	registry.remove("export_agent");
}

TEST_CASE("[export] document round-trips through Glaze JSON")
{
	auto &registry = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "roundtrip_agent";
	cfg.model_endpoint = "http://localhost:1/a2a";
	cfg.grants = {permissions::scope::read_project, permissions::scope::write_project};
	REQUIRE(registry.create(cfg) != nullptr);

	features::export_document doc = features::build_export_document();
	std::string error;
	std::string json = features::export_document_to_json(doc, error);
	REQUIRE(error.empty());
	CHECK_FALSE(json.empty());

	features::export_document parsed{};
	auto ec = glz::read_json(parsed, json);
	REQUIRE_FALSE(static_cast<bool>(ec));
	CHECK(parsed.schema == "tf.agent-export");
	CHECK(parsed.version == doc.version);
	CHECK(parsed.agents.size() == doc.agents.size());

	registry.remove("roundtrip_agent");
}

TEST_CASE("[export] agent_service writes an export file")
{
	auto &registry = agent_registry::get_instance();
	agent_config cfg;
	cfg.name = "file_agent";
	cfg.model_endpoint = "http://localhost:2/a2a";
	REQUIRE(registry.create(cfg) != nullptr);

	const std::string path = "agent_export_test.json";
	std::string error;
	REQUIRE(features::agent_service::instance().export_setup(path, error));
	CHECK(error.empty());

	// File exists and parses.
	features::export_document parsed{};
	auto ec = glz::read_file_json(parsed, path, std::string{});
	REQUIRE_FALSE(static_cast<bool>(ec));
	CHECK(parsed.schema == "tf.agent-export");

	registry.remove("file_agent");
	std::remove(path.c_str());
}
