#pragma once
#include <doctest/doctest.h>
#include "../src/data/agent/agent.h"
#include "../src/data/agent/agent_registry.h"
#include "../src/features/agent_template.h"
#include <cstdio>

TEST_CASE("[template] save an agent as a template and read it back")
{
	auto &registry = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "tmpl_source";
	cfg.model_endpoint = "http://localhost:7000/a2a";
	cfg.peer_agents = {"reviewer"};
	cfg.grants = {permissions::scope::read_project, permissions::scope::write_project};
	REQUIRE(registry.create(cfg) != nullptr);

	auto &svc = features::template_service::instance();
	const std::string path = "agent_templates_test.json";
	std::remove(path.c_str());

	std::string error;
	agent *a = registry.find("tmpl_source");
	REQUIRE(a != nullptr);
	REQUIRE(svc.save_agent_as_template(path, *a, "backend-template", error));
	CHECK(error.empty());

	features::agent_template_book book;
	REQUIRE(svc.load_templates(path, book, error));
	REQUIRE(book.templates.size() == 1);
	const features::agent_template &t = book.templates[0];
	CHECK(t.label == "backend-template");
	CHECK(t.endpoint == "http://localhost:7000/a2a");
	REQUIRE(t.peers.size() == 1);
	CHECK(t.peers[0] == "reviewer");
	REQUIRE(t.permissions.size() == 2);

	registry.remove("tmpl_source");
	std::remove(path.c_str());
}

TEST_CASE("[template] create a new agent from a template")
{
	auto &registry = agent_registry::get_instance();
	auto &svc = features::template_service::instance();

	features::agent_template tmpl;
	tmpl.label = "researcher-template";
	tmpl.endpoint = "http://localhost:8000/a2a";
	tmpl.peers = {"writer"};
	tmpl.permissions = {"read_project"};

	std::string error;
	agent *created = svc.create_from_template("from_tmpl", tmpl, error);
	REQUIRE(created != nullptr);
	CHECK(error.empty());

	const agent_config &cfg = created->config();
	CHECK(cfg.name == "from_tmpl");
	CHECK(cfg.model_endpoint == "http://localhost:8000/a2a");
	REQUIRE(cfg.peer_agents.size() == 1);
	CHECK(cfg.peer_agents[0] == "writer");
	REQUIRE(cfg.grants.size() == 1);
	CHECK(cfg.grants[0] == permissions::scope::read_project);

	registry.remove("from_tmpl");
}

TEST_CASE("[template] create_from_template rejects a duplicate name")
{
	auto &registry = agent_registry::get_instance();
	auto &svc = features::template_service::instance();

	agent_config cfg;
	cfg.name = "dup_agent";
	cfg.model_endpoint = "http://localhost:1/a2a";
	REQUIRE(registry.create(cfg) != nullptr);

	features::agent_template tmpl;
	tmpl.label = "x";
	tmpl.endpoint = "http://localhost:2/a2a";

	std::string error;
	agent *created = svc.create_from_template("dup_agent", tmpl, error);
	CHECK(created == nullptr);
	CHECK_FALSE(error.empty());

	registry.remove("dup_agent");
}
