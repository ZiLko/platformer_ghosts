cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(PlatformerGhost VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED
    src/Main.cpp
    src/Player/Player.cpp
    src/Recorder/Recorder.cpp
    src/RecordsManager/RecordsManager.cpp
    src/UI/RecordsLayer.cpp
    src/UI/ManagerLayer.cpp
    src/UI/ListLayer.cpp
    src/UI/GhostUI.cpp
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.11.2
)

target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)