#pragma once
#include <doctest/doctest.h>
#include "../src/data/agent/agent.h"
#include "../src/data/agent/agent_registry.h"
#include "../src/data/agent/planner.h"
#include "../src/data/agent/memory.h"
#include "../src/data/tools/permissions.h"
#include "../src/data/backends/github_api.h"
#include "../src/features/agent_export.h"
#include <glaze/glaze.hpp>

// These tests deliberately FAIL until the guarded MVP features are implemented.
// They act as living TODO markers so unfinished work stays visible in the test
// run. See docs/notes/open-questions.md (Q1, Q7, Q8, Q9) and the tracker.
//
// When you implement a feature, update/relax the matching assertion here.

// --- Q1 / T-005: planner is a stub (returns {}). MVP needs real plans. ---
TEST_CASE("[mvp-gap][planner] planner produces multi-step plans" * doctest::may_fail())
{
	planner p;
	auto steps = p.plan("Summarize the repository and open a PR");
	CHECK_MESSAGE(!steps.empty(),
				  "planner::plan is still a stub (Q1/T-005) — returns no steps");
}

// --- Q8 / T-011: permissions are not wired to agent_config (always allow). ---
TEST_CASE("[mvp-gap][permissions] denied scope is actually denied" * doctest::may_fail())
{
	permissions perms;
	// Once policy is wired, an ungranted write scope should be denied.
	CHECK_MESSAGE(!perms.allowed(permissions::scope::write_database),
				  "permissions::allowed is a stub (Q8/T-011) — always returns true");
}

// --- Q7 / T-017: mcp_client transport is scaffolding only. ---
// mcp_client's translation unit currently has no usable header declaration, so
// this gap is tracked as a documented expectation rather than a compiled probe.
TEST_CASE("[mvp-gap][mcp] mcp_client transport is implemented" * doctest::may_fail())
{
	FAIL_CHECK("mcp_client::connect / register_discovered_tools are empty stubs "
			   "(Q7/T-017)");
}

// --- Q9 / T-003: github backend is a stub on the MVP critical path. ---
TEST_CASE("[mvp-gap][backend] github_api returns real data" * doctest::may_fail())
{
	github_api gh;
	CHECK_MESSAGE(!gh.list_issues().empty(),
				  "github_api is a stub (Q9/T-003) — returns no issues");
}

// --- Q4 (CLOSED): memory serialization round-trips via the export document. ---
// This test PASSES and closes Q4: memory survives a Glaze JSON round-trip inside
// the versioned tf.agent-export document.
TEST_CASE("[memory] memory round-trips through the versioned export document")
{
	auto &registry = agent_registry::get_instance();

	agent_config cfg;
	cfg.name = "mem_roundtrip";
	cfg.model_endpoint = "http://localhost:3/a2a";
	agent *a = registry.create(cfg);
	REQUIRE(a != nullptr);
	a->mutable_memory().add("user", "remember this");
	a->mutable_memory().add("assistant", "remembered");

	features::export_document doc = features::build_export_document();
	std::string error;
	std::string json = features::export_document_to_json(doc, error);
	REQUIRE(error.empty());

	features::export_document parsed{};
	auto ec = glz::read_json(parsed, json);
	REQUIRE_FALSE(static_cast<bool>(ec));
	CHECK(parsed.version == doc.version); // memory versioned by document version

	const features::export_agent *found = nullptr;
	for (const auto &ea : parsed.agents)
		if (ea.id == "mem_roundtrip")
			found = &ea;
	REQUIRE(found != nullptr);
	REQUIRE(found->memory.size() == 2);
	CHECK(found->memory[0][0] == "user");
	CHECK(found->memory[0][1] == "remember this");
	CHECK(found->memory[1][0] == "assistant");
	CHECK(found->memory[1][1] == "remembered");

	registry.remove("mem_roundtrip");
}
