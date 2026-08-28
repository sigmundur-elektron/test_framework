#pragma once
#include "../backends/project_api.h"
#include "itool.h"
#include "permissions.h"

/// Reads a file from the project. Demonstrates validate -> permission -> backend.
struct read_file_tool : public itool
{
	std::string name() const override;
	std::string schema() const override;
	std::expected<tool_result, std::string> execute(const std::string &json_args,
													const permissions &perms) override;

  private:
	project_api _project;
};