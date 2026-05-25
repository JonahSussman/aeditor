cmake_minimum_required(VERSION 3.16)
project(aeditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVLC REQUIRED IMPORTED_TARGET libvlc)

include(FetchContent)

FetchContent_Declare(
  imgui
  URL "https://github.com/ocornut/imgui/archive/v1.89.9.zip"
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(BUILD_SHARED_LIBS ON)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" ON)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui demo" OFF)

FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG 2.6.x
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui-sfml)

# --- Application ---

add_executable(aeditor
  src/aeditor.cpp
  src/services/locator.cpp
  src/services/service_vlc.cpp
  src/services/service_script.cpp
  src/services/service_tools.cpp
)

target_include_directories(aeditor PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(aeditor PRIVATE
  ImGui-SFML::ImGui-SFML
  sfml-graphics
  sfml-window
  sfml-system
  PkgConfig::LIBVLC
  OpenGL::GL
  Threads::Threads
)

# --- Copy runtime assets to build directory ---

add_custom_command(TARGET aeditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/assets/iosevka.ttf
    $<TARGET_FILE_DIR:aeditor>/iosevka.ttf
)

add_custom_command(TARGET aeditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:aeditor>/data
)

add_custom_command(TARGET aeditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/assets/data/sample.csv
    $<TARGET_FILE_DIR:aeditor>/data/sample.csv
)

add_custom_command(TARGET aeditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/scripts
    $<TARGET_FILE_DIR:aeditor>/scripts
  COMMAND chmod +x $<TARGET_FILE_DIR:aeditor>/scripts/*.sh
)
