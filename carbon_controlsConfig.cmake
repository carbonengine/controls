# Copyright © 2026 CCP ehf.

include(CMakeFindDependencyMacro)

# ${CMAKE_CURRENT_LIST_DIR}/project_name.cmake is generated automatically by cmake as part of the install step
include(${CMAKE_CURRENT_LIST_DIR}/carbon_controls.cmake)

find_dependency(carbon-blue CONFIG NO_CMAKE_PATH REQUIRED)
find_dependency(carbon-blueexposure CONFIG NO_CMAKE_PATH REQUIRED)
find_dependency(carbon-core CONFIG NO_CMAKE_PATH REQUIRED)
find_dependency(Python3 CONFIG NO_CMAKE_PATH REQUIRED)
