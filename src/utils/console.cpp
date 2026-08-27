#include "console.h"
#include "../features/log_service.h"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <vector>

static char s_keyboard_state[124] = "";
static bool toggle_size = false;

void console::add_tagged_line(const char *_tag, std::string _msg, ...)
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	text_buffer.emplace_back(std::string{_tag + _msg.insert(0, " : ") + "\n"}.c_str());
	// Persist through the features layer (never touch the repository directly).
	features::log_service::instance().append(_tag ? _tag : "",
											 text_buffer.back());
}

void console::render(const char *title, bool *p_open)
{
	ImGuiWindowFlags window_flags = ImGuiDockNodeFlags_AutoHideTabBar;
	if (!ImGui::Begin(title, p_open, window_flags))
	{
		ImGui::End();
		return;
	}
	ImGui::SameLine();
	m_filter.Draw("Filter", 200.0f);
	if (ImGui::GetFocusID() == ImGui::GetItemID())
		is_should_refocus = false;
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
		clear();
	ImGui::SetItemTooltip("Clear the scrolling text box");
	ImGui::Separator();

	static int max_height_in_lines = 8;
	// ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
	// ImGui::DragInt("Max Height (in Lines)", &max_height_in_lines, 0.2f);
	static ImGuiChildFlags child_flags = ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY;

	ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 10);
	if (ImGui::BeginChild("scrolling", ImVec2(0, 0), child_flags, ImGuiWindowFlags_HorizontalScrollbar))
	{
		std::string my_filter = m_filter.InputBuf;
		std::vector<std::string> filtered;
		std::copy_if(text_buffer.begin(), text_buffer.end(), std::back_inserter(filtered),
					 [my_filter](std::string i) {
						 return i.find(my_filter) != std::string::npos;
					 });

		for (int i = 0; i < filtered.size(); i++)
		{
			ImGui::Text(filtered[i].c_str());
		}
	}

	if (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	if (is_should_refocus)
	{
		ImGui::SetKeyboardFocusHere(); // Refocus the input field
		is_should_refocus = false;	   // Reset the flag after refocusing
	}
	ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 15);
	ImGui::InputText("##", s_keyboard_state, sizeof(s_keyboard_state));
	if (s_keyboard_state[0] != '\0')
		m_is_empty = false;
	ImGui::SetItemDefaultFocus();
	ImGui::End();
}

void console::clear()
{
	text_buffer.clear();
	m_filter.Clear();
}

int console::add_line()
{
	if (is_empty())
		return 0;
	std::string msg = s_keyboard_state;
	text_buffer.emplace_back(msg);
	s_keyboard_state[0] = 0;
	m_is_empty = true;
	is_should_refocus = true;
	return 0;
}
