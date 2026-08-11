#pragma once
#include "../include/imgui.h"
#include "singleton.h"
#include <string>
#include <vector>

/// Console for typing commands
///
/// can be used for logging, chatting, or calling commands
struct console : public singleton<console>
{
	console(token){};
	void clear();
	void add_tagged_line(const char *_tag, std::string _msg, ...);
	int add_line();
	void render(const char *title, bool *p_open = NULL);
  private:
	bool is_empty()
	{
		is_should_refocus = true;
		return m_is_empty;
	}
	ImGuiTextBuffer m_buffer;
	ImGuiTextFilter m_filter; // Keep scrolling if already at the bottom.
	bool m_is_empty = true;
	bool m_auto_scroll = true;
	bool is_should_refocus = true;
	ImGuiID m_dockspace_id = 0;
	std::vector<std::string> text_buffer;
};