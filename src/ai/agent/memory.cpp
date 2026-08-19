#include "memory.h"

void memory::add(const std::string &role, const std::string &content)
{
	_history.push_back({role, content});
}

const std::vector<memory::entry> &memory::history() const
{
	return _history;
}

void memory::clear()
{
	_history.clear();
}