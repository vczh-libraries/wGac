cmake_minimum_required(VERSION 3.20)
project(wGac LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(WGacShared)
add_subdirectory(WGacTest)
add_subdirectory(WGacFullControlTest)
add_subdirectory(WGacCppTestRvm)
add_subdirectory(RemotingTest_Renderer_Wayland)
