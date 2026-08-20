#include "agent_panel.h"
#include "../ai/agent/agent.h"
#include "../features/agent_service.h"
#include "../include/imgui.h"
#include <algorithm>
#include <string>

void show_agent_panel(bool *p_open)
{
	if (!ImGui::Begin("Agents", p_open))
	{
		ImGui::End();
		return;
	}

	auto &service = features::agent_service::instance();

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

	ImGui::End();
}