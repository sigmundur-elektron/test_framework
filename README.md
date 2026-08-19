# test_framework

A C++23 desktop application and experimentation sandbox that combines an
OpenGL/ImGui graphical front end with a small **AI agent runtime** and a
DocTest-based unit test suite.

## Overview

The project is a single CMake-built executable (`test`) that can run in two
modes, selected by the `testing` flag in `src/main.cpp`:

- **Application mode** — launches a GLFW window with a GLAD-loaded OpenGL
  context and a Dear ImGui UI (`app` → `window`), driven by an input manager.
- **Testing mode** — runs the DocTest suite covering the math utilities, the
  tool layer, and the AI agent subsystem.

## Architecture

The codebase is organized into three conceptual layers:

| Layer | Responsibility | Location |
|-------|----------------|----------|
| **Agent** | Reasoning, planning, task decomposition, memory | `src/ai/agent/` |
| **Tool / Workflow** | Domain operations, validation, permissions, orchestration (MCP) | `src/ai/tools/` |
| **Backends** | Application APIs (database, GitHub, project) | `src/ai/backends/` |

### AI subsystem highlights
- `agent` — a configurable reasoning unit (config + planner + memory + tools).
  Multiple agents can coexist at runtime via an `agent_registry` singleton.
- `itool` — a uniform tool interface (`name`, `schema`, `execute`) returning
  `std::expected<tool_result, std::string>`. Tools validate arguments and
  check permissions before calling backends.
- **A2A (agent-to-agent)** — `agent_call_tool` lets one agent delegate a goal
  to a peer agent.
- **Persistence** — `ai_persistence` saves/loads agents and their memory to
  JSON (via Glaze).

### GUI / rendering
- `app` orchestrates the lifecycle (`init` → `update` loop → `end`).
- `window` wraps GLFW + GLAD + ImGui for rendering.
- `input_manager` handles key mapping and input processing.

## Tech stack

- **Language:** C++23 (with C11 for GLAD)
- **Build:** CMake (>= 3.20), Ninja generator, MSVC toolchain
- **Testing:** DocTest

### Vendored dependencies (`external/`)
- **GLFW** — windowing and input
- **GLAD** — OpenGL function loader
- **Dear ImGui** — immediate-mode GUI
- **Glaze** — header-only JSON / serialization
- **DocTest** — unit testing
- **profiler** — single-header, zero-dependency scoped profiler (`external/profiler/profiler.h`)

## Profiling startup

The application is instrumented with a lightweight, header-only scoped
profiler (`external/profiler/profiler.h`). It requires no external build
dependency — it is compiled in by default and can be stripped entirely by
defining `PROFILING_ENABLED=0`.

### How it works
- `app::run` opens a profiling session around `app::init`
  (`PROFILE_BEGIN_SESSION` / `PROFILE_END_SESSION`).
- Startup phases are wrapped with `PROFILE_FUNCTION()` / `PROFILE_SCOPE(name)`
  in `app::init` and `window::init` (GLFW init, window creation, input setup,
  GLAD load, and ImGui setup).
- On session end the profiler:
  1. prints a **timer summary** to the console (per-phase milliseconds and
     percentage of total startup), and
  2. writes a **Chrome Trace Event** file, `startup_profile.json`, next to the
     working directory.

### Viewing the flamegraph
Open `startup_profile.json` in any of:
- `chrome://tracing` (Chrome/Edge),
- [Perfetto UI](https://ui.perfetto.dev), or
- [speedscope](https://www.speedscope.app) (flamegraph view).

### Findings — why startup is slow
The instrumentation isolates the startup cost to `window::init`; the input key
mapping, GLAD load and ImGui setup are negligible by comparison. A representative
run (x64 Debug) measured a total `init` of **~453 ms**, broken down as:

| Phase | Duration | Share |
|-------|----------|-------|
| `glfwCreateWindow` | ~364 ms | ~80% |
| `set_input` | ~54 ms | ~12% |
| `glfwInit` | ~29 ms | ~6% |
| `imgui_setup` | ~3.5 ms | <1% |
| `gladLoadGLLoader` | ~0.4 ms | <1% |
| `input.set_key_mapping` | ~0.1 ms | <1% |

**`glfwCreateWindow` dominates** — creating the OS window and the OpenGL 3.3
core context (driver/context creation) is by far the single largest cost. The
secondary cost is `set_input`, which enumerates joysticks (a loop over every
`GLFW_JOYSTICK_LAST` slot calling `glfwJoystickPresent`) and registers input
devices.

`glfwSwapInterval(1)` (vsync) is enabled during init; the first buffer swap can
also stall until the driver is ready. These costs are driver-bound rather than
CPU-algorithmic, so the practical levers are: deferring window creation until
needed, trimming the joystick enumeration in `set_input`, lazy-loading panels,
or hiding context creation behind a splash. Re-run the profiler after any change
and compare `startup_profile.json` to quantify the impact.


## Building