cmake_minimum_required(VERSION 3.20)

project(test VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependency paths
set(GLFW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw")
set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(GLAD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/glad")
set(DOCTEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/doctest")
set(GLAZE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/glaze")
set(PROFILER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/profiler")
set(LIBPQXX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/libpqxx")

# Validate vendored dependencies
if(NOT EXISTS "${GLFW_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Missing vendored dependency: external/glfw")
endif()
if(NOT EXISTS "${IMGUI_PATH}/imgui.cpp")
    message(FATAL_ERROR "Missing vendored dependency: external/imgui")
endif()
if(NOT EXISTS "${PROFILER_PATH}/profiler.h")
    message(FATAL_ERROR "Missing vendored dependency: external/profiler")
endif()
if(NOT EXISTS "${GLAD_PATH}/src/glad.c")
    message(FATAL_ERROR "Missing vendored dependency: external/glad")
endif()
if(NOT EXISTS "${DOCTEST_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Missing vendored dependency: external/doctest")
endif()
if(NOT EXISTS "${GLAZE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Missing vendored dependency: external/glaze")
endif()
if(NOT EXISTS "${LIBPQXX_PATH}/CMakeLists.txt")
    message(WARNING "Vendored external/libpqxx not found; PostgreSQL backend disabled")
endif()

# Build GLFW as static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
add_subdirectory("${GLFW_PATH}")

# Doctest
add_subdirectory("${DOCTEST_PATH}")

# Glaze (JSON / serialization) - vendored, header-only. Defines glaze::glaze.
set(glaze_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
add_subdirectory("${GLAZE_PATH}")

# libpqxx (PostgreSQL C++ client) - vendored. Only enabled when the PostgreSQL
# client library (libpq) is available on the system. When it is not found the
# app still builds and runs using the in-memory repository fallback.
option(TF_ENABLE_POSTGRES "Build the PostgreSQL repository backend (requires libpq)" ON)
set(TF_POSTGRES_ENABLED OFF)
if(TF_ENABLE_POSTGRES AND EXISTS "${LIBPQXX_PATH}/CMakeLists.txt")
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)
        set(PQXX_LIBRARIES pqxx CACHE STRING "" FORCE)
        add_subdirectory("${LIBPQXX_PATH}")
        set(TF_POSTGRES_ENABLED ON)
        message(STATUS "PostgreSQL backend enabled (libpqxx)")
    else()
        message(STATUS "PostgreSQL client (libpq) not found; using in-memory repository fallback")
    endif()
else()
    message(STATUS "PostgreSQL backend disabled; using in-memory repository fallback")
endif()

# GLAD static lib
add_library(glad STATIC
    "${GLAD_PATH}/src/glad.c"
)
target_include_directories(glad PUBLIC
    "${GLAD_PATH}/include"
)

# ImGui static lib
add_library(imgui STATIC
    "${IMGUI_PATH}/imgui.cpp"
    "${IMGUI_PATH}/imgui_demo.cpp"
    "${IMGUI_PATH}/imgui_draw.cpp"
    "${IMGUI_PATH}/imgui_tables.cpp"
    "${IMGUI_PATH}/imgui_widgets.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp"
)

target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD=1)

target_include_directories(imgui PUBLIC
    "${IMGUI_PATH}"
    "${IMGUI_PATH}/backends"
    "${GLFW_PATH}/include"
    "${GLAD_PATH}/include"
)

target_link_libraries(imgui PUBLIC glfw glad)

# App sources (includes src/main.cpp)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# The PostgreSQL repository implementation is only compiled when the backend
# is enabled; otherwise the in-memory fallback is used.
if(NOT TF_POSTGRES_ENABLED)
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*/repository/postgres_repository\\.cpp$")
endif()

add_executable(test ${APP_SOURCES})

target_include_directories(test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GLFW_PATH}/include"
    "${GLAD_PATH}/include"
    "${IMGUI_PATH}"
    "${IMGUI_PATH}/backends"
    "${DOCTEST_PATH}"
    "${PROFILER_PATH}"
)

target_link_libraries(test PRIVATE
    glfw
    imgui
    glad
    doctest::doctest
    glaze::glaze
)

# PostgreSQL backend (optional): link pqxx and define the enabling macro.
if(TF_POSTGRES_ENABLED)
    target_link_libraries(test PRIVATE pqxx)
    target_compile_definitions(test PRIVATE TF_POSTGRES_ENABLED=1)
endif()

# Glaze requires a conformant preprocessor on MSVC
if(MSVC)
    # /Zc:preprocessor - conformant preprocessor required by Glaze
    # /EHsc           - enable C++ exception unwind semantics (required by DocTest)
    target_compile_options(test PRIVATE /Zc:preprocessor /EHsc)
endif()

if(WIN32)
    target_link_libraries(test PRIVATE opengl32)
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(test PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(test PRIVATE OpenGL::GL dl pthread)
endif()

