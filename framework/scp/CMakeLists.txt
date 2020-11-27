# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

include_directories("../../vcpkg/packages/dcmtk_x64-linux/include")

LINK_DIRECTORIES("../../vcpkg/packages/dcmtk_x64-linux/lib")


# Add source to this project's executable.
add_executable (scpserver "scp.cpp")

target_link_libraries(scpserver -ldcmnet -ldcmdata -loflog -lofstd -ldcmtls -lpthread -licuuc)