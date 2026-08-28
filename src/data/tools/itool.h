#pragma once
#include "permissions.h"
#include <expected>
#include <string>

/// Result of a tool invocation.
struct tool_result
{
	bool success{};
	std::string output;
};

/// Tool interface.
//
/// Uniform contract the agent uses to invoke any capability.
/// Implementations validate args + check permissions, then call backend APIs.
struct itool
{
	virtual ~itool() = default;

	/// Unique tool name (used by the agent/registry).
	virtual std::string name() const = 0;

	/// JSON schema describing arguments (also used to expose via MCP).
	virtual std::string schema() const = 0;

	/// Validate -> check permissions -> call backend. Error string on failure.
	///
	/// `perms` is the *caller's* resolved grant set, supplied by the agent that
	/// is dispatching this step. A tool gates itself against it; it never
	/// consults an ambient policy of its own, so it cannot be reached with more
	/// authority than the invoking agent holds.
	virtual std::expected<tool_result, std::string> execute(const std::string &json_args,
															const permissions &perms) = 0;
};