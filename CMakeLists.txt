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

# Validate vendored dependencies
if(NOT EXISTS "${GLFW_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Missing vendored dependency: external/glfw")
endif()
if(NOT EXISTS "${IMGUI_PATH}/imgui.cpp")
    message(FATAL_ERROR "Missing vendored dependency: external/imgui")
endif()
if(NOT EXISTS "${GLAD_PATH}/src/glad.c")
    message(FATAL_ERROR "Missing vendored dependency: external/glad")
endif()
if(NOT EXISTS "${DOCTEST_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Missing vendored dependency: external/doctest")
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

add_executable(test ${APP_SOURCES})

target_include_directories(test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GLFW_PATH}/include"
    "${GLAD_PATH}/include"
    "${IMGUI_PATH}"
    "${IMGUI_PATH}/backends"
    "${DOCTEST_PATH}"
)

target_link_libraries(test PRIVATE
    glfw
    imgui
    glad
    doctest::doctest
)

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
