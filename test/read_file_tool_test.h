#pragma once
#include <doctest/doctest.h>
#include "../src/data/tools/read_file_tool.h"
#include "../src/data/tools/tool_registry.h"
#include <fstream>
#include <memory>

TEST_CASE("[ai] read_file_tool exposes name and schema")
{
	read_file_tool tool;
	CHECK(tool.name() == "read_file");
	CHECK(tool.schema().find("\"read_file\"") != std::string::npos);
	CHECK(tool.schema().find("\"path\"") != std::string::npos);
}

TEST_CASE("[ai] read_file_tool fails when path arg is missing")
{
	read_file_tool tool;
	auto result = tool.execute("{}");
	CHECK_FALSE(result.has_value());
	CHECK(result.error() == "missing required argument: path");
}

TEST_CASE("[ai] read_file_tool reads an existing file")
{
	const std::string path = "read_file_tool_test.tmp";
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		out << "hello agent";
	}

	read_file_tool tool;
	auto result = tool.execute(R"({ "path": "read_file_tool_test.tmp" })");

	REQUIRE(result.has_value());
	CHECK(result->success);
	CHECK(result->output == "hello agent");

	std::remove(path.c_str());
}

TEST_CASE("[ai] tool_registry finds a registered tool")
{
	auto &registry = tool_registry::get_instance();
	registry.register_tool(std::make_unique<read_file_tool>());

	itool *found = registry.find("read_file");
	REQUIRE(found != nullptr);
	CHECK(found->name() == "read_file");

	CHECK(registry.find("does_not_exist") == nullptr);
}