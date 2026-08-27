#include "planner_panel.h"
#include "../data/agent/agent.h"
#include "../features/agent_service.h"
#include "../include/imgui.h"
#include <string>

void show_planner_panel(bool *p_open)
{
	if (!ImGui::Begin("Planner", p_open))
	{
		ImGui::End();
		return;
	}

	auto &service = features::agent_service::instance();
	std::vector<agent *> agents = service.all();

	if (agents.empty())
	{
		ImGui::TextUnformatted("No agents. Create one in the Agents panel.");
		ImGui::End();
		return;
	}

	// --- Pick which agent to plan/run with ---
	static int selected = 0;
	if (selected >= static_cast<int>(agents.size()))
		selected = 0;

	if (ImGui::BeginCombo("agent", agents[selected]->config().name.c_str()))
	{
		for (int i = 0; i < static_cast<int>(agents.size()); ++i)
		{
			const bool is_selected = (i == selected);
			if (ImGui::Selectable(agents[i]->config().name.c_str(), is_selected))
				selected = i;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	agent *active = agents[selected];
	const std::string active_name = active->config().name;

	// --- Goal input ---
	static char goal[256] = "";
	ImGui::InputTextMultiline("goal", goal, sizeof(goal),
							  ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3));

	// --- Plan preview (non-mutating) ---
	ImGui::SeparatorText("Plan preview");
	const std::vector<plan_step> steps = service.preview(active_name, goal);
	if (steps.empty())
	{
		ImGui::TextDisabled("(planner produced no steps yet)");
	}
	else
	{
		if (ImGui::BeginTable("plan", 3,
							  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 24.0f);
			ImGui::TableSetupColumn("tool");
			ImGui::TableSetupColumn("args");
			ImGui::TableHeadersRow();
			for (int i = 0; i < static_cast<int>(steps.size()); ++i)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%d", i + 1);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(steps[i].tool_name.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(steps[i].json_args.c_str());
			}
			ImGui::EndTable();
		}
	}

	// --- Dispatch ---
	ImGui::SeparatorText("Run");
	static std::string transcript;
	if (ImGui::Button("Run goal"))
		transcript = service.run(active_name, goal);
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		goal[0] = '\0';
		transcript.clear();
	}

	if (!transcript.empty())
	{
		ImGui::SeparatorText("Result");
		ImGui::TextWrapped("%s", transcript.c_str());
	}

	ImGui::End();
}
