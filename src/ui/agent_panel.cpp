#include "agent_panel.h"
#include "../ai/agent/agent_registry.h"
#include "../include/imgui.h"
#include <cstring>

void show_agent_panel(bool *p_open)
{
	if (!ImGui::Begin("Agents", p_open))
	{
		ImGui::End();
		return;
	}

	auto &registry = agent_registry::get_instance();

	// --- Create a new agent ---
	static char new_name[64] = "planner";
	static char new_endpoint[128] = "http://localhost:0/a2a";
	ImGui::SeparatorText("Create agent");
	ImGui::InputText("name", new_name, sizeof(new_name));
	ImGui::InputText("endpoint", new_endpoint, sizeof(new_endpoint));
	static std::string create_status;
	if (ImGui::Button("Add agent"))
	{
		agent_config cfg;
		cfg.name = new_name;
		cfg.model_endpoint = new_endpoint;
		if (registry.create(cfg)) // nullptr if name empty or taken
			create_status = "Created '" + cfg.name + "'.";
		else
			create_status = "Not created (name empty or already exists).";
	}
	if (!create_status.empty())
		ImGui::TextUnformatted(create_status.c_str());

	ImGui::SeparatorText("Live agents");

	// --- Edit existing agents + wire A2A ---
	for (agent *a : registry.all())
	{
		agent_config &cfg = a->mutable_config();
		ImGui::PushID(cfg.name.c_str());

		if (ImGui::CollapsingHeader(cfg.name.c_str()))
		{
			ImGui::Checkbox("enabled", &cfg.enabled);

			// A2A wiring: list every other agent as a toggleable peer.
			ImGui::TextUnformatted("A2A peers:");
			for (agent *other : registry.all())
			{
				if (other == a)
					continue;
				const std::string &peer = other->config().name;
				bool linked = std::find(cfg.peer_agents.begin(),
										cfg.peer_agents.end(), peer) != cfg.peer_agents.end();
				if (ImGui::Checkbox(peer.c_str(), &linked))
				{
					if (linked)
						cfg.peer_agents.push_back(peer);
					else
						std::erase(cfg.peer_agents, peer);
				}
			}

			// Goal dispatch lives in the Planner window now.
			if (ImGui::Button("Remove"))
				registry.remove(cfg.name);
		}

		ImGui::PopID();
	}

	ImGui::End();
}