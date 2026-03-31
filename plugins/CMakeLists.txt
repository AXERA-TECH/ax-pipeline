cmake_minimum_required(VERSION 3.16)

# Common code shared by plugins.
add_subdirectory(common)

# Scan plugin subdirectories (each plugin provides its own CMakeLists.txt).
file(GLOB _plugin_cmakelists RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
foreach(_cmakelists IN LISTS _plugin_cmakelists)
    get_filename_component(_dir "${_cmakelists}" DIRECTORY)
    if(_dir STREQUAL "common")
        continue()
    endif()
    add_subdirectory("${_dir}")
endforeach()

get_property(_plugin_targets GLOBAL PROPERTY AXP_PLUGIN_TARGETS)
if(_plugin_targets)
    add_custom_target(ax_pipeline_plugins ALL DEPENDS ${_plugin_targets})
endif()

