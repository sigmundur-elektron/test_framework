#include "agent_panel.h"
#include "../ai/agent/agent_registry.h"
#include "../ai/persistence/ai_persistence.h"
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
	if (ImGui::Button("Add agent"))
	{
		agent_config cfg;
		cfg.name = new_name;
		cfg.model_endpoint = new_endpoint;
		registry.create(cfg); // no-op if name taken
	}

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

			// Dispatch a goal to this agent.
			static char goal[256] = "";
			ImGui::InputText("goal", goal, sizeof(goal));
			if (ImGui::Button("Run"))
				a->handle(goal);
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
				registry.remove(cfg.name);
		}

		ImGui::PopID();
	}

	// --- Persistence ---
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

	ImGui::End();
}