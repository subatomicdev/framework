cmake_minimum_required (VERSION 3.8)

include_directories("../../vcpkg/packages/asio_x64-linux/include")
include_directories("../../vcpkg/packages/poco_x64-linux/include")
include_directories("../../framework")

LINK_DIRECTORIES("../../vcpkg/packages/poco_x64-linux/lib")

# Add source to this project's executable.
add_executable (prototype "Prototype.cpp")

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")

target_link_libraries(prototype -lPocoFoundation -lpthread)
target_link_libraries(prototype frameworklib)
