#pragma once
#include <string>
#include <vector>

/// A single step in a plan: which tool to run with which args.
struct plan_step
{
	std::string tool_name;
	std::string json_args;
};

/// Task decomposition / reasoning. Turns a goal into ordered tool steps.
struct planner
{
	std::vector<plan_step> plan(const std::string &goal) const;
};