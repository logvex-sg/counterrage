cmake_minimum_required(VERSION 3.20)
project(logvex_cs2_practice VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(LVX_BUILD_GUI "Build the ImGui/GLFW desktop trainer" ON)
option(LVX_BUILD_RADAR "Build the web radar server" ON)
option(LVX_BUILD_TESTS "Build the test suite" ON)
option(LVX_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

find_package(Threads REQUIRED)

# ---- third-party: GLM (header only) -----------------------------------------
find_package(glm QUIET)
if(NOT TARGET glm::glm)
  find_path(GLM_INCLUDE_DIR glm/glm.hpp)
  if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found (apt: libglm-dev)")
  endif()
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}")
endif()

# ---- third-party: nlohmann/json ---------------------------------------------
find_package(nlohmann_json QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
  find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
  if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann/json not found (apt: nlohmann-json3-dev)")
  endif()
  add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
  set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}")
endif()

# ---- core library ------------------------------------------------------------
add_library(lvx_core STATIC
  src/core/log.cpp
  src/core/types.cpp
  src/core/math.cpp
  src/core/weapon.cpp
  src/core/world.cpp
  src/core/ballistics.cpp
  src/core/simulation.cpp
  src/core/features.cpp
  src/core/config.cpp
  src/core/diagnostics.cpp
  src/core/radar_protocol.cpp
  src/core/radar_server.cpp
  src/core/scenarios.cpp
)
target_include_directories(lvx_core PUBLIC include)
target_link_libraries(lvx_core PUBLIC glm::glm nlohmann_json::nlohmann_json Threads::Threads)
target_compile_options(lvx_core PRIVATE -Wall -Wextra -Wpedantic)

if(LVX_WARNINGS_AS_ERRORS)
  target_compile_options(lvx_core PRIVATE -Werror)
endif()

# ---- GUI (ImGui + GLFW + OpenGL3) -------------------------------------------
if(LVX_BUILD_GUI)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 QUIET)
  if(NOT TARGET glfw)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    add_library(glfw INTERFACE IMPORTED)
    set_target_properties(glfw PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${GLFW_LINK_LIBRARIES}")
    target_link_directories(glfw INTERFACE ${GLFW_LIBRARY_DIRS})
  endif()

  set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
  add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
  # vendored third-party code is excluded from our warning policy
  target_compile_options(imgui PRIVATE -w)

  add_library(lvx_gui STATIC
    src/gui/app.cpp
    src/gui/panels.cpp
    src/gui/scene_view.cpp
  )
  target_include_directories(lvx_gui PUBLIC include)
  target_link_libraries(lvx_gui PUBLIC lvx_core imgui glfw OpenGL::GL)
  target_compile_options(lvx_gui PRIVATE -Wall -Wextra -Wpedantic)

  add_executable(logvex-practice src/main.cpp)
  target_link_libraries(logvex-practice PRIVATE lvx_core lvx_gui)
endif()

# ---- web radar server --------------------------------------------------------
if(LVX_BUILD_RADAR)
  add_executable(logvex-radar src/radar/server_main.cpp)
  target_link_libraries(logvex-radar PRIVATE lvx_core)
  target_compile_options(logvex-radar PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- tests -------------------------------------------------------------------
if(LVX_BUILD_TESTS)
  enable_testing()
  add_executable(lvx_tests
    tests/test_main.cpp
    tests/test_math.cpp
    tests/test_weapon.cpp
    tests/test_ballistics.cpp
    tests/test_simulation.cpp
    tests/test_features.cpp
    tests/test_config.cpp
    tests/test_radar_protocol.cpp
  )
  target_link_libraries(lvx_tests PRIVATE lvx_core)
  target_compile_options(lvx_tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME lvx_tests COMMAND lvx_tests)
endif()

# ---- headless CLI target (no GUI deps) --------------------------------------
add_executable(logvex-cli src/cli_main.cpp)
target_link_libraries(logvex-cli PRIVATE lvx_core)
target_compile_options(logvex-cli PRIVATE -Wall -Wextra -Wpedantic)
