#pragma once
#include <string>
#include <vector>

/// Short/long-term context store for the agent.
struct memory
{
	struct entry
	{
		std::string role;	 // "user" | "assistant" | "tool"
		std::string content;
	};

	void add(const std::string &role, const std::string &content);
	const std::vector<entry> &history() const;
	void clear();

	// Serialization seams.
	std::vector<entry> &mutable_history() { return _history; }
	void load(std::vector<entry> history) { _history = std::move(history); }

  private:
	std::vector<entry> _history;
};