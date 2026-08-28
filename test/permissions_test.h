#pragma once
#include "../src/data/agent/agent.h"
#include "../src/data/agent/agent_config.h"
#include "../src/data/agent/planner.h"
#include "../src/data/tools/permissions.h"
#include <doctest/doctest.h>
#include <fstream>
#include <type_traits>
#include <vector>

// --- A12 / R7 / R8: freeze the wire-visible shape of the scope enum. ---------
//
// The enumerator names and their order are load-bearing in three independent
// tables that are NOT compiled against each other: permission_scope_id
// (features/agent_export.cpp), permission_scope_from_id
// (features/agent_template.cpp) and the Glaze enumerate list
// (data/persistence/ai_setup.h). A rename, a reorder, an insertion or a removal
// silently changes what previously exported documents mean. Pinning every
// ordinal turns all four of those mistakes into build failures.
//
// LIMITATION, stated rather than papered over: appending a SEVENTH enumerator
// after write_database is NOT caught here. C++ has no portable way to assert an
// enum's cardinality without reflection, and a fake assertion would be worse
// than a documented gap. Insertion, reorder, rename and removal are covered.
static_assert(static_cast<int>(permissions::scope::read_project) == 0,
			  "permissions::scope::read_project must stay first (R7)");
static_assert(static_cast<int>(permissions::scope::write_project) == 1,
			  "permissions::scope::write_project must stay second (R7)");
static_assert(static_cast<int>(permissions::scope::read_github) == 2,
			  "permissions::scope::read_github must stay third (R7)");
static_assert(static_cast<int>(permissions::scope::write_github) == 3,
			  "permissions::scope::write_github must stay fourth (R7)");
static_assert(static_cast<int>(permissions::scope::read_database) == 4,
			  "permissions::scope::read_database must stay fifth (R7)");
static_assert(static_cast<int>(permissions::scope::write_database) == 5,
			  "permissions::scope::write_database must stay last (R7)");

static_assert(std::is_same_v<decltype(agent_config::grants),
							 std::vector<permissions::scope>>,
			  "agent_config::grants must stay std::vector<permissions::scope> (R8)");

// --- A1 / R1, R2, R3 --------------------------------------------------------
TEST_CASE("[permissions] explicit grant is honoured")
{
	const permissions perms{{permissions::scope::write_database}};

	CHECK(perms.allowed(permissions::scope::write_database));

	CHECK_FALSE(perms.allowed(permissions::scope::read_project));
	CHECK_FALSE(perms.allowed(permissions::scope::write_project));
	CHECK_FALSE(perms.allowed(permissions::scope::read_github));
	CHECK_FALSE(perms.allowed(permissions::scope::write_github));
	CHECK_FALSE(perms.allowed(permissions::scope::read_database));
}

// --- A2 / R4: deny by default ----------------------------------------------
TEST_CASE("[permissions] default construction denies all")
{
	const permissions perms;

	CHECK_FALSE(perms.allowed(permissions::scope::read_project));
	CHECK_FALSE(perms.allowed(permissions::scope::write_project));
	CHECK_FALSE(perms.allowed(permissions::scope::read_github));
	CHECK_FALSE(perms.allowed(permissions::scope::write_github));
	CHECK_FALSE(perms.allowed(permissions::scope::read_database));
	CHECK_FALSE(perms.allowed(permissions::scope::write_database));
}

// --- A4 / R5, R6: the caller's grants gate the tool -------------------------
TEST_CASE("[permissions] ungranted tool call is denied")
{
	// Fixture content deliberately cannot appear in a denial message, so
	// "contents absent" is a sound proxy for "the file was not read".
	const std::string path = "permissions_test_denied.tmp";
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		out << "hello agent";
	}

	agent_config cfg;
	cfg.name = "ungranted_agent";
	cfg.grants = {permissions::scope::read_database}; // NOT read_project

	agent a{cfg};
	a.init(); // registers "read_file" into the process-wide tool_registry

	const plan_step step{"read_file", R"({ "path": "permissions_test_denied.tmp" })"};
	const tool_result result = a.execute_step(step);

	CHECK_FALSE(result.success);
	// Names the denied scope: distinguishes this from the "unknown tool: ..."
	// path in agent::execute_step, which names no scope.
	CHECK(result.output.find("read_project") != std::string::npos);
	CHECK(result.output.find("unknown tool") == std::string::npos);
	// The backend was not reached.
	CHECK(result.output.find("hello agent") == std::string::npos);

	std::remove(path.c_str());
}

// --- A5 / R5: a granted agent still gets through ----------------------------
TEST_CASE("[permissions] granted tool call succeeds")
{
	const std::string path = "permissions_test_granted.tmp";
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		out << "hello agent";
	}

	agent_config cfg;
	cfg.name = "granted_agent";
	cfg.grants = {permissions::scope::read_project};

	agent a{cfg};
	a.init();

	const plan_step step{"read_file", R"({ "path": "permissions_test_granted.tmp" })"};
	const tool_result result = a.execute_step(step);

	CHECK(result.success);
	CHECK(result.output.find("hello agent") != std::string::npos);

	std::remove(path.c_str());
}
