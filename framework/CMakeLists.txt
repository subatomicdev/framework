# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("framework")

# Include sub-projects.
add_subdirectory("framework")
# add_subdirectory("prototype")
add_subdirectory("scp")

