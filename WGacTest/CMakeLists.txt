cmake_minimum_required(VERSION 3.20)
project(WGacTest LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_executable(Test_HellWorld_Cpp
    "HelloWorlds/Cpp/Main.cpp"
)
target_include_directories(Test_HellWorld_Cpp PRIVATE
    "../Import"
    "../WGac"
    "../WGacShared"
)
target_link_libraries(Test_HellWorld_Cpp PRIVATE
    WGac
    WGacShared
)
