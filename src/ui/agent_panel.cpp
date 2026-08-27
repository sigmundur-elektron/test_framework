#include "agent_panel.h"
#include "../data/agent/agent.h"
#include "../features/agent_service.h"
#include "../features/agent_template.h"
#include "../include/imgui.h"
#include <algorithm>
#include <string>
#include <vector>

void show_agent_panel(bool *p_open)
{
	if (!ImGui::Begin("Agents", p_open))
	{
		ImGui::End();
		return;
	}

	auto &service = features::agent_service::instance();
	auto &templates = features::template_service::instance();
	const std::string template_path = features::template_service::default_path();

	// --- Create a new agent ---
	static char new_name[64] = "planner";
	static char new_endpoint[128] = "http://localhost:0/a2a";
	ImGui::SeparatorText("Create agent");
	ImGui::InputText("name", new_name, sizeof(new_name));
	ImGui::InputText("endpoint", new_endpoint, sizeof(new_endpoint));
	static std::string create_status;
	if (ImGui::Button("Add agent"))
	{
		if (service.create(new_name, new_endpoint)) // nullptr if empty/taken
			create_status = std::string("Created '") + new_name + "'.";
		else
			create_status = "Not created (name empty or already exists).";
	}

	// Create from a saved template (reuses endpoint/grants/peers).
	{
		features::agent_template_book book;
		std::string load_err;
		templates.load_templates(template_path, book, load_err);
		if (!book.templates.empty())
		{
			static int selected_template = 0;
			if (selected_template >= (int)book.templates.size())
				selected_template = 0;
			const char *preview = book.templates[selected_template].label.c_str();
			if (ImGui::BeginCombo("template", preview))
			{
				for (int i = 0; i < (int)book.templates.size(); ++i)
				{
					bool is_selected = (selected_template == i);
					if (ImGui::Selectable(book.templates[i].label.c_str(), is_selected))
						selected_template = i;
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button("Create from template"))
			{
				std::string err;
				if (templates.create_from_template(new_name,
												   book.templates[selected_template], err))
					create_status = std::string("Created '") + new_name + "' from template.";
				else
					create_status = "Not created: " + err;
			}
		}
	}

	if (!create_status.empty())
		ImGui::TextUnformatted(create_status.c_str());

	ImGui::SeparatorText("Live agents");

	// --- Edit existing agents + wire A2A ---
	for (agent *a : service.all())
	{
		const agent_config &cfg = a->config();
		ImGui::PushID(cfg.name.c_str());

		if (ImGui::CollapsingHeader(cfg.name.c_str()))
		{
			bool enabled = cfg.enabled;
			if (ImGui::Checkbox("enabled", &enabled))
				service.set_enabled(cfg.name, enabled);

			// A2A wiring: list every other agent as a toggleable peer.
			ImGui::TextUnformatted("A2A peers:");
			for (agent *other : service.all())
			{
				if (other == a)
					continue;
				const std::string &peer = other->config().name;
				bool linked = std::find(cfg.peer_agents.begin(),
										cfg.peer_agents.end(), peer) != cfg.peer_agents.end();
				if (ImGui::Checkbox(peer.c_str(), &linked))
					service.set_peer_linked(cfg.name, peer, linked);
			}

			// Save this agent's reusable config as a template.
			static char template_label[64] = "";
			ImGui::InputText("template label", template_label, sizeof(template_label));
			ImGui::SameLine();
			static std::string template_status;
			if (ImGui::Button("Save as template"))
			{
				std::string err;
				const std::string label =
					template_label[0] ? template_label : cfg.name;
				if (templates.save_agent_as_template(template_path, *a, label, err))
					template_status = "Saved template '" + label + "'.";
				else
					template_status = "Save failed: " + err;
			}
			if (!template_status.empty())
				ImGui::TextUnformatted(template_status.c_str());

			// Goal dispatch lives in the Planner window now.
			if (ImGui::Button("Remove"))
			{
				service.remove(cfg.name);
				ImGui::PopID();
				break; // 'a' is now dangling; rebuild list next frame
			}
		}

		ImGui::PopID();
	}

	// --- Advanced: export the whole setup (de-emphasized) ---
	if (ImGui::CollapsingHeader("Advanced: export setup"))
	{
		static char export_path[128] = "agent_export.json";
		ImGui::InputText("file", export_path, sizeof(export_path));
		static std::string export_status;
		if (ImGui::SmallButton("Export setup"))
		{
			std::string error;
			if (service.export_setup(export_path, error))
				export_status = std::string("Exported to '") + export_path + "'.";
			else
				export_status = "Export failed: " + error;
		}
		if (!export_status.empty())
			ImGui::TextUnformatted(export_status.c_str());
	}

	ImGui::End();
}