
include_directories("../../vcpkg/packages/asio_x64-windows-static/include")
include_directories("../../vcpkg/packages/poco_x64-windows-static/include")

add_library(frameworklib STATIC   "../../framework/framework/ctpl_threadpool.hpp" "../../framework/framework/IntervalTimer.hpp" "../../framework/framework/Logger.hpp" "../../framework/framework/Pipeline.hpp" "../../framework/framework/PipelineStage.hpp" "../../framework/framework/ScopedTimer.hpp" "../../framework/framework/TcpServer.hpp" "../../framework/framework/IntervalTimer.cpp" "../../framework/framework/IntervalTimer.cpp" "../../framework/framework/Pipeline.cpp" "../../framework/framework/PipelineStage.cpp" "../../framework/framework/ScopedTimer.cpp" "../../framework/framework/TcpServer.cpp")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

