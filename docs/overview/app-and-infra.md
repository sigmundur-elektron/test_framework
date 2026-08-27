---
last-updated: 2025-02-14
owner: copilot
status: active
---

# App & Infrastructure

Everything outside the three AI layers: the application service façade,
persistence, the UI, and the build/test/tooling infrastructure.

## Application logic — `src/features/`

### `agent_service` (`agent_service.h` / `agent_service.cpp`)
The **only** app-facing façade for agents. The UI talks solely to this service;
it never touches `agent_registry` or the repository directly. It keeps the
in-RAM `agent_registry` (runtime catalog) in sync with the durable repository
(source of truth).

- Singleton: `agent_service::instance()`.
- `load_from_repository()` — load persisted agents into the registry at startup.
- Queries: `all()`.
- Commands (each persists via the repository): `create(name, endpoint)`,
  `remove(name)`, `set_enabled(name, enabled)`,
  `set_peer_linked(name, peer, linked)`.
- Planner/execution: `preview(name, goal)`, `run(name, goal)`.
- Private `persist(agent*)` upserts one agent into the repository.

## Persistence — `src/repository/`

- `i_repository.h` — repository interface (durable source of truth for agents).
- In-memory implementation — default/fallback.
- `postgres_repository.cpp` — PostgreSQL implementation, **conditionally
  compiled** (only when `TF_POSTGRES_ENABLED`; excluded from the `GLOB` source
  list otherwise).
- `ai_persistence` — saves/loads agents and their memory to **JSON via Glaze**.

## UI / rendering — `src/ui/`

- `app` — lifecycle orchestrator (`init` → update loop → `end`).
- `window` (`window.cpp`) — wraps GLFW + GLAD + ImGui rendering.
- `agent_panel` (`agent_panel.cpp`) — ImGui panel for viewing/editing agents;
  calls `agent_service`.
- `input_manager` — key mapping and input processing (singleton pattern).

## Build & tooling

- **CMake** (>= 3.20), **Ninja** generator, **MSVC** toolchain.
- C++23 (`CMAKE_CXX_STANDARD 23`, extensions off); C11 for GLAD.
- MSVC flags: `/Zc:preprocessor` (Glaze), `/EHsc` (DocTest).
- Sources via `file(GLOB_RECURSE src/*.cpp CONFIGURE_DEPENDS)`; single
  executable target `test`.
- Optional Postgres: `TF_ENABLE_POSTGRES` + vendored `libpqxx`
  + `find_package(PostgreSQL)`; graceful in-memory fallback with a warning.
- Platform GL linking: Win32 `opengl32`; Apple frameworks; Unix `OpenGL::GL`.

## Testing

- **DocTest** suite; testing mode toggled by the `testing` flag in
  `src/main.cpp`, or at runtime via the `-t` / `--test` command-line flag
  (which also forwards remaining args to DocTest). Covers math utilities, the
  tool layer, the AI agent subsystem (see `test/agent_a2a_test.h`), and export
  (`test/agent_export_test.h`).

## Profiling

- Header-only scoped profiler (`external/profiler/profiler.h`), compiled in by
  default; strip with `PROFILING_ENABLED=0`.
- Startup instrumented in `app::run` / `app::init` / `window::init`; emits a
  console timer summary and a Chrome Trace `startup_profile.json`.
- Finding: `glfwCreateWindow` dominates init (~80%); see project
  [`README.md`](../../README.md).

## External dependencies (`external/`)

GLFW · GLAD · Dear ImGui · Glaze (JSON) · DocTest · profiler · libpqxx
(optional).

## MVP-relevant notes

- `agent_service` is the natural home for the **export** command (it already
  owns the authoritative agent set) — see
  [export-format](../plans/export-format.md). **Implemented** as
  `agent_service::export_setup` (T-008), backed by
  `src/features/agent_export.{h,cpp}`.
- `ai_persistence` (Glaze) is the serialization foundation to reuse for export.

See task IDs in [`../status/tracker.md`](../status/tracker.md).
