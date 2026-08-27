#include "setup_panel.h"
#include "../features/setup_service.h"
#include "../include/imgui.h"

void show_setup_controls()
{
	static char setup_path[128] = "ai_setup.json";
	static std::string status;

	ImGui::SeparatorText("Setup");
	ImGui::InputText("file", setup_path, sizeof(setup_path));
	if (ImGui::Button("Export setup"))
		features::setup_service::instance().export_json(setup_path, status);
	ImGui::SameLine();
	if (ImGui::Button("Import setup"))
		features::setup_service::instance().import_json(setup_path, status);
	if (!status.empty())
		ImGui::TextUnformatted(status.c_str());
}
