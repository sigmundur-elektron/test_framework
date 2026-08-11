#include "gui.h"
#include "../utils/console.h"
#include <string>

ImGuiID set_docking_mode()
{
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	// ImGui::DockSpaceOverViewport(viewport); // enable dockspace on main window
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.0f); // disable dockspace overlay over the game window
	ImGui::Begin("DockSpace", NULL, window_flags);

	ImGui::PopStyleVar(3);
	ImGuiID dockspace_id = ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0, 0), dockspace_flags);

	static auto first_time = true;
	if (first_time)
	{
		first_time = false;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.75f, nullptr, &dockspace_id);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 1.0f, nullptr, &dockspace_id);

		auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.28f, nullptr, &dock_id_left);
		ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_None;
		dock_flags |= ImGuiDockNodeFlags_CentralNode;
		ImGui::DockBuilderDockWindow("Sidebar", dock_id_right);
		ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
		auto& console = console::get_instance();
		ImGui::DockBuilderFinish(dockspace_id);
	}
	return dockspace_id;
}

void set_render_stats()
{
	ImGuiIO &io = ImGui::GetIO();
	ImGui::Text("Application average %.3f ms/frame", 1000.0f / io.Framerate);
	ImGui::Text("(%.1f FPS)", io.Framerate);
}

void list_saved_states(bool &_show)
{
	static char loadName[64] = "saveName1";
	ImGui::InputTextMultiline(" ", loadName, sizeof(loadName), ImVec2(100, 25));
	ImGui::SameLine();
	if (ImGui::Button("load"))
	{
		_show = false;
	}
	ImGui::Separator();
	ImGui::Text("List of saved states");
	ImGui::Separator();
}


void show_console(bool *p_open)
{
	auto &console = console::get_instance();
	// Actually call in the regular Log helper (which will Begin() into the same window as we just did)
	console.render("Console", p_open);
}

static void help_marker(const char *desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}
