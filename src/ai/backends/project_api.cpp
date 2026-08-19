#include "project_api.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

std::vector<std::string> project_api::list_files() const
{
	std::vector<std::string> files;
	for (const auto &entry : fs::recursive_directory_iterator(fs::current_path()))
	{
		if (entry.is_regular_file())
			files.push_back(entry.path().string());
	}
	return files;
}

std::string project_api::read_file(const std::string &path) const
{
	std::ifstream in(path, std::ios::binary);
	if (!in)
		return {};

	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

void project_api::write_file(const std::string &path, const std::string &content)
{
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	out << content;
}