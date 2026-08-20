#include "setup_panel.h"
#include "../ai/persistence/ai_persistence.h"
#include "../include/imgui.h"

void show_setup_controls()
{
	static char setup_path[128] = "ai_setup.json";
	static std::string status;

	ImGui::SeparatorText("Setup");
	ImGui::InputText("file", setup_path, sizeof(setup_path));
	if (ImGui::Button("Save setup"))
	{
		std::string err;
		status = ai_persistence::save(setup_path, err) ? "Saved." : ("Save failed: " + err);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load setup"))
	{
		std::string err;
		status = ai_persistence::load(setup_path, err) ? "Loaded." : ("Load failed: " + err);
	}
	if (!status.empty())
		ImGui::TextUnformatted(status.c_str());
}
