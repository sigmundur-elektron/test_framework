#include "read_file_tool.h"

std::string read_file_tool::name() const
{
	return "read_file";
}

std::string read_file_tool::schema() const
{
	return R"({
  "name": "read_file",
  "description": "Read a file from the project by path.",
  "parameters": {
	"type": "object",
	"properties": { "path": { "type": "string" } },
	"required": ["path"]
  }
})";
}

std::expected<tool_result, std::string> read_file_tool::execute(const std::string &json_args,
																const permissions &perms)
{
	// NOTE: minimal extraction; swap for a real JSON parser (e.g. nlohmann/json).
	const std::string key = "\"path\"";
	auto pos = json_args.find(key);
	if (pos == std::string::npos)
		return std::unexpected("missing required argument: path");

	auto start = json_args.find('"', json_args.find(':', pos) + 1);
	auto end = json_args.find('"', start + 1);
	if (start == std::string::npos || end == std::string::npos)
		return std::unexpected("invalid 'path' argument");

	std::string path = json_args.substr(start + 1, end - start - 1);

	if (!perms.allowed(permissions::scope::read_project))
		return std::unexpected("permission denied: read_project");

	tool_result result;
	result.output = _project.read_file(path);
	result.success = true;
	return result;
}