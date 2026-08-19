# Copilot Instructions

## Project Guidelines
- User prefers upgrading the project to the latest C++ language standard that remains compatible and buildable.


graph TD
    A["Agent Layer<br/>reasoning · planning · task decomposition · memory"] -->|agent-specific tools| B["Tool / Workflow Layer<br/>domain ops · validation · permissions · orchestration · MCP"]
    B -->|application APIs| C["Database"]
    B -->|application APIs| D["GitHub"]
    B -->|application APIs| E["Project"]


src/ai/
├─ agent/
│  ├─ agent.h / agent.cpp            // top-level orchestrator (lifecycle)
│  ├─ planner.h                      // task decomposition / reasoning
│  └─ memory.h                       // context + short/long-term memory
├─ tools/
│  ├─ itool.h                        // tool interface (execute + schema)
│  ├─ tool_registry.h                // singleton, like input_manager
│  ├─ permissions.h                  // permission checks
│  └─ mcp/
│     └─ mcp_client.h                // MCP tool transport
└─ backends/
   ├─ database_api.h                 // normal app API
   ├─ github_api.h
   └─ project_api.h