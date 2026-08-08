cmake_minimum_required(VERSION 3.20)
project(RemotingTest_Rendering_Wayland LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_executable(RemotingTest_Rendering_Wayland
    "GuiMain.cpp"
    "Main.cpp"
)
target_include_directories(RemotingTest_Rendering_Wayland PRIVATE
    "../Import"
    "../WGac"
    "../WGacShared"
)
target_link_libraries(RemotingTest_Rendering_Wayland PRIVATE
    WGac
    WGacShared
)
