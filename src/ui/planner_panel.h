#pragma once

/// Renders the planner window: pick an agent, enter a goal, preview the plan
/// steps, and dispatch it. Toggled from the sidebar like the console/agents.
/// Call from window::render(), like show_console(...).
void show_planner_panel(bool *p_open);
